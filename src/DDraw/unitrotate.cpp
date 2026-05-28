#include "unitrotate.h"

#include "buildghost.h"
#include "dialog.h"
#include "iddrawsurface.h"
#include "hook/hook.h"
#include "taHPI.h"
#include "tafunctions.h"
#include "tamem.h"
#include "UnitDefExtensions.h"
#include "Profiler.h"

#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>

namespace
{
    // --- Hook addresses (from Ghidra analysis of TotalA.exe @ 0x400000) ---
    // _TestBuildSpot entry; 5-byte prologue (SUB ESP,0x14 + start of MOV).
    constexpr unsigned ADDR_TestBuildSpot_Entry     = 0x004197d0;
    // GUI_IssueMobileBuildOrder entry; 5-byte prologue.
    constexpr unsigned ADDR_IssueMobileBuild_Entry  = 0x00419670;
    // ORDERS_NewSubOrder2Unit: instruction right after CALL cmalloc_MM__(0x56).
    // Bytes are `ADD ESP, 4; TEST EAX, EAX` = exactly 5 bytes (3 + 2). EAX
    // at this point is the freshly-allocated OrderStruct*.
    constexpr unsigned ADDR_NewSubOrder_PostAlloc   = 0x0043adcb;
    // Order_MobileBuild: CALL UNITS_CreateUnit (5-byte E8 opcode).
    constexpr unsigned ADDR_MobileBuild_PreCreate   = 0x00403d5b;
    // Order_MobileBuild: CALL after the above (reads the just-returned EAX).
    constexpr unsigned ADDR_MobileBuild_PostCreate  = 0x00403d64;
    // Order_VTOL_MobileBuild: mirrors the above pair.
    constexpr unsigned ADDR_VtolBuild_PreCreate     = 0x0041409b;
    constexpr unsigned ADDR_VtolBuild_PostCreate    = 0x004140a4;

    // Order_MobileBuild / Order_VTOL_MobileBuild function entries. Install
    // footprint+yardmap rotation for the *entire* duration of the function so
    // every UNITINFO read inside sees rotated dims/yardmap, not just the inner
    // UNITS_CreateUnit window. The PreCreate hook above runs at the CALL
    // UNITS_CreateUnit ~0x33b bytes into the function — too late to cover:
    //   case 0  : `nFootPrintX/Z` read for the target_pos_x/z snap (case 0
    //             permanently rewrites target_pos with the snapped value, so an
    //             unrotated snap shifts the order half a cell).
    //   case 1  : `CanAttachUnitToPiece(pUVar14, …)` reads UNITINFO->yardmap
    //             and footprint to decide if the area is clear — using the
    //             unrotated yardmap on an asymmetric structure (factory door
    //             tiles) produces false "Waiting for target area to clear".
    //   case 1  : `ApplyYardmap(pUVar14, piVar19)` stamps with whatever yardmap
    //             is currently in UNITINFO — must be the rotated one.
    // The return thunk clears the swap on function exit so subsequent code
    // (TestBuildSpot preview, other builders' orders) sees clean state.
    constexpr unsigned ADDR_Order_MobileBuild_Entry      = 0x00403a20;
    constexpr unsigned ADDR_Order_VTOL_MobileBuild_Entry = 0x00413d80;

    // Yardmap-reading functions (all take UnitStruct* as first __stdcall arg).
    constexpr unsigned ADDR_Unit_UpdateYardmap          = 0x0047c790;
    constexpr unsigned ADDR_UNITS_RebuildFootPrint      = 0x0047cc30;
    constexpr unsigned ADDR_Unit_ClearMapTileOccupancy  = 0x0047d0e0;
    constexpr unsigned ADDR_UNITS_CanCloseOrOpenYard    = 0x0047d970;
    constexpr unsigned ADDR_YardOpen                    = 0x0047dac0;

    // FreeGameData entry — runs at game-end teardown right before
    // ReleaseUNITINFO frees every UNITINFO->p_YardMap. We hook it to
    // restore any active yardmap-pointer swap so TA's free() doesn't
    // try to release one of our `new[]`-allocated rotated yardmap
    // copies (which aren't in TA's CRT heap → null heap-entry, AV in
    // RtlSizeHeap+0x92 — see crash dump 2026-04-25).
    constexpr unsigned ADDR_FreeGameData                = 0x00491b60;

    // DrawBuildSpotQueue entry — 5-byte prologue (MOV EDX,[ESP+0xC] is 4 bytes,
    // SUB ESP,0x30 is 3 bytes; both fully displaced by the trampoline).
    // Engine function that draws the persistent build-order rectangle on the
    // main game view for one queued order. It reads UNITINFO->lFpBnd{X,Z}
    // directly, so those fields must reflect the ORDER's rotation (not the
    // current cursor's) for the duration of the call.
    constexpr unsigned ADDR_DrawBuildSpotQueue          = 0x00438c00;

    // DrawGameScreen, instruction immediately after `CALL DrawGUI` at 0x46a303
    // (DrawGUI is at 0x004ab170). DrawGUI repaints the active GUI panel (the
    // build menu sidebar) into the back buffer when the panel's dirty flag is
    // set, so anything we paint on those buttons earlier in the frame gets
    // overpainted whenever the panel is dirty (page change, hover redraw,
    // sub-menu, etc.). Hooking *after* this call guarantees the chevrons sit
    // on top of every panel repaint. Original instruction is a 6-byte
    // `MOV EDX, [0x00511de8]`; the inline-hook trampoline rounds 5 → 6 to
    // cover the full opcode.
    constexpr unsigned ADDR_DrawGameScreen_PostDrawGUI  = 0x0046a308;

    // Send_NewUnits_P09 entry — broadcasts packet 0x09 (unit-creation announcement
    // including bank/heading/pitch). Called from inside UNITS_CreateUnit AFTER
    // UNITS_AllocateUnit but BEFORE the function returns. We hook the entry to
    // overwrite the new unit's heading_angle with our rotation quarter-turn so the
    // outgoing packet carries the right value. __stdcall, 1 arg (pUnit) at [Esp+4].
    constexpr unsigned ADDR_SendNewUnitsP09             = 0x00456050;

    // UNITS_CreateFromNetwork entry — receive-side handler for packet 0x09. The
    // engine's stock implementation ignores the heading bytes in the packet
    // (sets heading=nBuildAngle for mobile, no-op for buildings), so remote
    // builders see all rotated buildings facing south. We install a return-thunk
    // hook that reads heading from the spawn record (offset 0x13) and applies
    // it to the just-created unit. __stdcall, args: ownerSlotFlags, pSpawnRecord.
    constexpr unsigned ADDR_UNITS_CreateFromNetwork     = 0x004861d0;

    // --- Global pointers ---
    constexpr unsigned TA_MAIN_PTR_ADDR             = 0x00511de8;

    // --- TAMainStruct offsets (verified via Ghidra /struct_fields on TAMainStruct) ---
    constexpr unsigned OFF_TA_UNITINFOArray_p       = 0x1439B;

    // --- UNITINFO offsets (0x249-byte struct; verified via Ghidra) ---
    constexpr unsigned OFF_UNITINFO_nFootPrintX     = 0x14A;
    constexpr unsigned OFF_UNITINFO_nFootPrintZ     = 0x14C;
    constexpr unsigned OFF_UNITINFO_p_YardMap       = 0x14E;
    // Footprint bounding box in world-space fixed-point (16.16). Pairs swap
    // for rotations 1 (E, 90°) and 3 (W, 270°) — see SwapUnitInfoFpBndXZ.
    constexpr unsigned OFF_UNITINFO_lFpBndLeft_X    = 0x15E;
    constexpr unsigned OFF_UNITINFO_lFpBndLeft_Z    = 0x166;
    constexpr unsigned OFF_UNITINFO_lFpBndRight_X   = 0x16A;
    constexpr unsigned OFF_UNITINFO_lFpBndRight_Z   = 0x172;
    constexpr unsigned OFF_UNITINFO_lFpBndWidth_X   = 0x176;
    constexpr unsigned OFF_UNITINFO_lFpBndWidth_Z   = 0x17E;
    constexpr unsigned OFF_UNITINFO_bmcode          = 0x22F;  // 0=building,1=mobile,2=feature
    constexpr unsigned UNITINFO_SIZE                = 0x249;

    constexpr unsigned OFF_UNIT_UNITINFO_p          = 0x92;

    // --- UnitStruct offsets (0x118-byte struct; verified via Ghidra) ---
    // Volume_Word is {bank, heading, pitch} at UnitStruct+0x64, so heading is at +0x66.
    constexpr unsigned OFF_UNIT_Volume_heading      = 0x66;
    constexpr unsigned OFF_UNIT_UnitINFOID          = 0xA6;

    // MODEL_PTRS array (Model3DONode** in TAMainStruct), indexed by UnitINFOID.
    constexpr unsigned OFF_TA_MODEL_PTRS            = 0x14377;

    // --- OrderStruct offsets ---
    constexpr unsigned OFF_ORDER_build_unitType     = 0x36;

    TAdynmemStruct* GetTA()
    {
        return *reinterpret_cast<TAdynmemStruct**>(TA_MAIN_PTR_ADDR);
    }

    BYTE* GetUnitInfoRaw(unsigned int idx)
    {
        TAdynmemStruct* ta = GetTA();
        if (!ta) return nullptr;
        BYTE* array = *reinterpret_cast<BYTE**>(reinterpret_cast<BYTE*>(ta) + OFF_TA_UNITINFOArray_p);
        if (!array) return nullptr;
        return array + idx * UNITINFO_SIZE;
    }

    // Forward decl so SwapUnitInfoFootprintWords can use SafeWriteProtected.
    BOOL SafeWriteProtected(void* dst, const void* src, SIZE_T size);

    // Swap two 16-bit fields. Uses SafeWriteProtected because UNITINFO pages
    // are read-only at runtime; plain WriteProcessMemory silently fails.
    void SwapUnitInfoFootprintWords(BYTE* unitInfo)
    {
        WORD x, z;
        memcpy(&x, unitInfo + OFF_UNITINFO_nFootPrintX, 2);
        memcpy(&z, unitInfo + OFF_UNITINFO_nFootPrintZ, 2);
        SafeWriteProtected(unitInfo + OFF_UNITINFO_nFootPrintX, &z, 2);
        SafeWriteProtected(unitInfo + OFF_UNITINFO_nFootPrintZ, &x, 2);
    }

    // Swap the lFpBnd{X,Z} pairs. Called around engine functions that compute
    // a screen rect from UNITINFO->lFpBnd* for a rotated order — DrawBuildSpotQueue
    // is the current user. Swap is its own inverse (just like the footprint
    // words above), so the same helper undoes the swap on function return.
    void SwapUnitInfoFpBndXZ(BYTE* unitInfo)
    {
        int tmp;
        memcpy(&tmp, unitInfo + OFF_UNITINFO_lFpBndLeft_X, 4);
        SafeWriteProtected(unitInfo + OFF_UNITINFO_lFpBndLeft_X,
                           unitInfo + OFF_UNITINFO_lFpBndLeft_Z, 4);
        SafeWriteProtected(unitInfo + OFF_UNITINFO_lFpBndLeft_Z, &tmp, 4);

        memcpy(&tmp, unitInfo + OFF_UNITINFO_lFpBndRight_X, 4);
        SafeWriteProtected(unitInfo + OFF_UNITINFO_lFpBndRight_X,
                           unitInfo + OFF_UNITINFO_lFpBndRight_Z, 4);
        SafeWriteProtected(unitInfo + OFF_UNITINFO_lFpBndRight_Z, &tmp, 4);

        memcpy(&tmp, unitInfo + OFF_UNITINFO_lFpBndWidth_X, 4);
        SafeWriteProtected(unitInfo + OFF_UNITINFO_lFpBndWidth_X,
                           unitInfo + OFF_UNITINFO_lFpBndWidth_Z, 4);
        SafeWriteProtected(unitInfo + OFF_UNITINFO_lFpBndWidth_Z, &tmp, 4);
    }

    BOOL SafeWriteProtected(void* dst, const void* src, SIZE_T size)
    {
        SIZE_T written = 0;
        if (WriteProcessMemory(GetCurrentProcess(), dst, src, size, &written) && written == size)
            return TRUE;
        DWORD oldProt = 0;
        if (!VirtualProtect(dst, size, PAGE_READWRITE, &oldProt)) return FALSE;
        memcpy(dst, src, size);
        DWORD dummy = 0;
        VirtualProtect(dst, size, oldProt, &dummy);
        return TRUE;
    }

    // Sentinel meaning "rotation is active but no footprint swap is needed" (0 or 180°).
    constexpr int ROT_ACTIVE_NO_SWAP = -2;
    constexpr int ROT_NONE            = -1;

    // Rotate a W×H yardmap by rotation*90° CW. Returns a newly-allocated array.
    // Output dimensions:
    //   rotation 1 (90° CW):  new W=H, new H=W
    //   rotation 2 (180°):    new W=W, new H=H
    //   rotation 3 (270° CW): new W=H, new H=W
    BYTE* RotateYardmap(const BYTE* src, unsigned W, unsigned H, int rotation)
    {
        if (!src || W == 0 || H == 0) return nullptr;
        rotation &= 3;
        if (rotation == 0) return nullptr;

        unsigned newW, newH;
        if (rotation == 2) { newW = W; newH = H; }
        else               { newW = H; newH = W; }

        BYTE* dst = new BYTE[newW * newH];

        for (unsigned nz = 0; nz < newH; ++nz)
        {
            for (unsigned nx = 0; nx < newW; ++nx)
            {
                unsigned ox, oz;
                // TA heading increases CCW when viewed from above (empirically:
                // hd=0x8000→south, 0xC000→east, 0x0000→north, 0x4000→west). So
                // "rot=1" in user-space (hd delta +0x4000) is actually 90° CCW
                // in world-space — the yardmap must rotate the same way so the
                // 'c' door tiles end up on the world-side where the aircraft
                // pad-piece will emerge after heading rotation.
                switch (rotation)
                {
                    case 1:  // 90° CCW (aircraft emerges at world-east)
                             // new[nz][nx] = old[nx][W-1-nz]
                        ox = W - 1 - nz;
                        oz = nx;
                        break;
                    case 2:  // 180°
                        ox = W - 1 - nx;
                        oz = H - 1 - nz;
                        break;
                    case 3:  // 90° CW = 270° CCW (aircraft emerges at world-west)
                             // new[nz][nx] = old[H-1-nx][nz]
                        oz = H - 1 - nx;
                        ox = nz;
                        break;
                    default:
                        ox = nx; oz = nz;
                        break;
                }
                dst[nz * newW + nx] = src[oz * W + ox];
            }
        }
        return dst;
    }
}

// ============================================================================
// Runtime-yardmap swap trampoline — wraps Unit_UpdateYardmap, etc. so
// UNITINFO.p_YardMap briefly points at the rotated copy for the duration of
// one call. Frame stack handles nesting (YardOpen → Unit_UpdateYardmap).
// ============================================================================

namespace
{
    struct YardmapFrame
    {
        BYTE* unitInfo;             // UNITINFO whose p_YardMap was swapped
        BYTE* originalYardmap;      // original pointer to restore
        DWORD realReturnAddr;       // real caller return address
    };
    constexpr int YARDMAP_FRAME_STACK_SIZE = 8;
    static YardmapFrame g_yardmapFrames[YARDMAP_FRAME_STACK_SIZE];
    static volatile int g_yardmapFrameTop = 0;
}

extern "C" DWORD __cdecl YardmapRestoreAndGetRet()
{
    if (g_yardmapFrameTop <= 0) return 0;
    int idx = --g_yardmapFrameTop;
    YardmapFrame& f = g_yardmapFrames[idx];
    if (f.unitInfo)
    {
        SafeWriteProtected(f.unitInfo + OFF_UNITINFO_p_YardMap,
                           &f.originalYardmap, sizeof(BYTE*));
    }
    return f.realReturnAddr;
}

__declspec(naked) static void YardmapReturnThunk()
{
    // Hooked function just RET'd here. ESP is at caller's clean state.
    // Preserve EAX (return value) while we pop our frame and jmp home.
    __asm
    {
        push eax
        call YardmapRestoreAndGetRet
        mov  ecx, eax                 // ecx = real ret addr
        pop  eax                      // restore original EAX
        jmp  ecx
    }
}

// ============================================================================
// Singleton
// ============================================================================

static CUnitRotate* g_instance = nullptr;

// Set by RegisterUnitDefKeys at early ddraw init; read by the later CUnitRotate
// constructor. Sentinel 0 means "not yet registered" (invalid as a real key idx
// because registerStringKey always returns a value with the STRING_KEY=3 tag in
// the top 2 bits, i.e. >= 0xC0000000).
static unsigned g_rotationsKeyIdx = 0;

CUnitRotate* CUnitRotate::GetInstance()
{
    return g_instance;
}

void CUnitRotate::RegisterUnitDefKeys()
{
    if (g_rotationsKeyIdx == 0)
    {
        // Default value "S": rotation 0 (= south, the engine's default
        // build heading 0x8000) is always allowed regardless of the FBI
        // string. Authors write e.g. "SENW" to allow all four facings,
        // "S" or omit-the-key to allow only south. See IsRotationAllowed.
        g_rotationsKeyIdx =
            UnitDefExtensions::GetInstance()->registerStringKey("Rotations", "S");
    }
}

// ============================================================================
// GUI_IssueMobileBuildOrder scoping — so we only tag orders that are created
// DIRECTLY as a result of the player clicking to place a building, not orders
// created by AI/campaign/squad commands.
// ============================================================================

// -1 when not in an issue; 0..3 when the user's click is being processed.
// Orders allocated while this is >= 0 get tagged with this rotation.
static volatile int g_issuingRotation = -1;

// Saved return address of GUI_IssueMobileBuildOrder's caller. The entry hook
// replaces the return address on the stack with IssueReturnThunk; when the
// hooked function RETs, control flows to the thunk which clears
// g_issuingRotation and jumps back to g_issueRealReturn.
static DWORD g_issueRealReturn = 0;

__declspec(naked) static void IssueReturnThunk()
{
    // Clear scoping flag. Restore control to the real caller.
    // GUI_IssueMobileBuildOrder is __stdcall, returns void, RET 4 — so by the
    // time we get here the stack is already at the caller's pre-call state
    // and register values don't matter (no return value convention to preserve).
    __asm
    {
        mov dword ptr [g_issuingRotation], -1
        jmp dword ptr [g_issueRealReturn]
    }
}

// ---- DrawBuildSpotQueue pre/post state ----
// Set by the entry router when a rotated order triggers a FpBnd swap; read by
// the return thunk to unswap. Single global is fine because DrawBuildSpotQueue
// doesn't recurse or call any other engine function that reads lFpBnd.
static DWORD g_drawBuildSpotQueueRealReturn = 0;
static BYTE* g_drawBuildSpotQueueSwappedUi  = nullptr;

static void DrawBuildSpotQueuePostSwap()
{
    if (g_drawBuildSpotQueueSwappedUi)
    {
        SwapUnitInfoFpBndXZ(g_drawBuildSpotQueueSwappedUi);
        g_drawBuildSpotQueueSwappedUi = nullptr;
    }
}

__declspec(naked) static void DrawBuildSpotQueueReturnThunk()
{
    // Engine's DrawBuildSpotQueue is __stdcall void with RET 0x10 — stack is
    // at caller's pre-call state and no return-value register needs preserving.
    // pushad / popad around our cleanup call keeps any incidental register
    // state intact regardless.
    __asm
    {
        pushad
        pushfd
        call DrawBuildSpotQueuePostSwap
        popfd
        popad
        jmp dword ptr [g_drawBuildSpotQueueRealReturn]
    }
}

// ---- UNITS_CreateFromNetwork pre/post state ----
// Save the spawn-record pointer at function entry; the return thunk reads it
// to copy heading_angle out of the network packet onto the just-created unit.
static DWORD g_createFromNetworkRealReturn = 0;
static BYTE* g_createFromNetworkSpawnRecord = nullptr;

// Receive-side: copy heading from the packet onto the freshly created unit.
// Only acts on buildings (bmcode==0); for mobile units the engine already
// sets heading=nBuildAngle, and applying jittered values from the packet
// would change long-standing behaviour for unrotated mobile spawns.
extern "C" void __cdecl ApplyHeadingFromSpawnRecord(BYTE* spawnRecord, BYTE* pUnit)
{
    if (!spawnRecord || !pUnit) return;
    WORD unitInfoId = *reinterpret_cast<WORD*>(pUnit + OFF_UNIT_UnitINFOID);
    if (unitInfoId == 0) return;

    BYTE* ui = GetUnitInfoRaw(unitInfoId);
    if (!ui) return;
    if (*(ui + OFF_UNITINFO_bmcode) != 0) return;  // mobile — leave alone

    WORD heading = *reinterpret_cast<WORD*>(spawnRecord + 0x13);
    *reinterpret_cast<WORD*>(pUnit + OFF_UNIT_Volume_heading) = heading;
}

__declspec(naked) static void CreateFromNetworkReturnThunk()
{
    // Function returns int* (pUnit) in EAX. pushad/popad preserves it across
    // our helper call, then we jmp to the real caller with EAX intact.
    __asm
    {
        pushad
        pushfd
        push eax                                ; arg2: pUnit (return value)
        push g_createFromNetworkSpawnRecord     ; arg1: spawn record
        call ApplyHeadingFromSpawnRecord
        add esp, 8
        popfd
        popad
        jmp dword ptr [g_createFromNetworkRealReturn]
    }
}

// ---- Order_MobileBuild / Order_VTOL_MobileBuild pre/post state ----
// One save slot is enough — Order_MobileBuild does not recurse into itself
// (and Order_VTOL_MobileBuild can't be on the stack while Order_MobileBuild
// is, or vice versa, because the per-builder dispatch picks one or the other).
static DWORD g_orderMobileBuildRealReturn = 0;
static bool  g_orderMobileBuildSwapActive = false;

static void OrderMobileBuildPostSwap()
{
    if (!g_orderMobileBuildSwapActive) return;
    g_orderMobileBuildSwapActive = false;
    if (CUnitRotate* self = CUnitRotate::GetInstance())
    {
        self->ClearRotation();
        self->ClearYardmapRotation();
    }
}

__declspec(naked) static void OrderMobileBuildReturnThunk()
{
    // Order_MobileBuild / Order_VTOL_MobileBuild are __stdcall returning int
    // in EAX. pushad/popad preserves EAX (and everything else) across the
    // cleanup call; then jump to the real caller with state intact.
    __asm
    {
        pushad
        pushfd
        call OrderMobileBuildPostSwap
        popfd
        popad
        jmp dword ptr [g_orderMobileBuildRealReturn]
    }
}

// ============================================================================
// Hook routers
// ============================================================================

// _TestBuildSpot entry — keep UNITINFO footprint swap in sync with the current
// build cursor state + user rotation. Called every frame the build cursor is up.
static int __stdcall TestBuildSpot_Entry_Proc(PInlineX86StackBuffer)
{
    CUnitRotate* self = CUnitRotate::GetInstance();
    if (!self) return 0;

    TAdynmemStruct* ta = GetTA();
    if (!ta)
    {
        self->ClearRotation();
        self->ClearYardmapRotation();
        return 0;
    }

    unsigned buildNum = static_cast<unsigned>(ta->BuildUnitID);
    int rot = self->GetRotation();

    if (buildNum == 0 || rot == 0)
    {
        self->ClearRotation();
        self->ClearYardmapRotation();
        return 0;
    }

    BYTE* ui = GetUnitInfoRaw(buildNum);
    if (!ui)
    {
        self->ClearRotation();
        self->ClearYardmapRotation();
        return 0;
    }
    BYTE bmcode = *(ui + OFF_UNITINFO_bmcode);
    if (bmcode != 0)  // only buildings
    {
        self->ClearRotation();
        self->ClearYardmapRotation();
        return 0;
    }

    // Don't rotate units whose FBI doesn't allow this rotation. Without
    // this, after rotating an ARMVP and switching to e.g. a solar (which
    // doesn't list any non-N rotation in its "Rotations=" key), m_rotation
    // would still apply and we'd swap footprint/yardmap on a unit that
    // shouldn't rotate.
    if (!self->IsRotationAllowed(buildNum, rot))
    {
        self->ClearRotation();
        self->ClearYardmapRotation();
        return 0;
    }

    self->ApplyRotationTo(buildNum);
    self->ApplyYardmapRotationTo(buildNum, rot);
    return 0;
}

// GUI_IssueMobileBuildOrder entry — capture rotation, install return trampoline.
static int __stdcall IssueMobileBuild_Entry_Proc(PInlineX86StackBuffer X86StrackBuffer)
{
    CUnitRotate* self = CUnitRotate::GetInstance();
    if (!self) return 0;

    // Save caller's real return address (top of stack at hook entry, before
    // the function's own prologue has run).
    DWORD* stackTop = reinterpret_cast<DWORD*>(X86StrackBuffer->Esp);
    g_issueRealReturn = *stackTop;
    *stackTop = reinterpret_cast<DWORD>(&IssueReturnThunk);

    // Scope: orders allocated during this function's execution get tagged
    // with the current rotation (including 0, so rotation-change-after-click
    // doesn't affect queued orders). Clamp to whatever the unit-being-built
    // actually allows — otherwise a leftover rotation from a previous
    // rotatable unit would tag a non-rotatable unit's order.
    TAdynmemStruct* ta = GetTA();
    int rot = self->GetRotation();
    if (ta && ta->BuildUnitID != 0 &&
        !self->IsRotationAllowed(static_cast<unsigned>(ta->BuildUnitID), rot))
    {
        rot = 0;
    }
    g_issuingRotation = rot;
    return 0;
}

// Send_NewUnits_P09 entry. Called from inside UNITS_CreateUnit just before the
// engine broadcasts packet 0x09 (unit creation). At this point the unit exists
// and has the random-jittered heading set by UNITS_AllocateUnit. Apply our
// rotation quarter-turn HERE so the outgoing packet carries the correct
// heading; PostCreate skips the heading-write to avoid double-rotation.
static int __stdcall SendNewUnitsP09_Entry_Proc(PInlineX86StackBuffer X86StrackBuffer)
{
    CUnitRotate* self = CUnitRotate::GetInstance();
    if (!self || self->m_pendingHeading < 0) return 0;  // not in our build path

    // __stdcall 1 arg — [Esp+0]=ret, [Esp+4]=pUnit.
    DWORD* stackTop = reinterpret_cast<DWORD*>(X86StrackBuffer->Esp);
    BYTE*  pUnit    = reinterpret_cast<BYTE*>(stackTop[1]);
    if (!pUnit) return 0;

    int rotation = self->m_pendingHeading & 3;
    WORD* pHeading = reinterpret_cast<WORD*>(pUnit + OFF_UNIT_Volume_heading);
    WORD  quarter  = static_cast<WORD>(rotation * 0x4000);
    *pHeading = static_cast<WORD>(*pHeading + quarter);
    return 0;
}

// UNITS_CreateFromNetwork entry. Stash the spawn record pointer and install
// the return thunk so we can copy the packet's heading onto the new unit
// after the engine finishes constructing it.
static int __stdcall CreateFromNetwork_Entry_Proc(PInlineX86StackBuffer X86StrackBuffer)
{
    DWORD* stackTop = reinterpret_cast<DWORD*>(X86StrackBuffer->Esp);
    g_createFromNetworkSpawnRecord = reinterpret_cast<BYTE*>(stackTop[2]);  // arg2
    g_createFromNetworkRealReturn  = stackTop[0];
    stackTop[0] = reinterpret_cast<DWORD>(&CreateFromNetworkReturnThunk);
    return 0;
}

// DrawBuildSpotQueue entry — per-order hook. Swap UNITINFO->lFpBnd{X,Z} pairs
// based on THIS order's stored rotation so the engine computes the rect with
// the rotated footprint. The return thunk restores the swap.
static int __stdcall DrawBuildSpotQueue_Entry_Proc_Inner(PInlineX86StackBuffer X86StrackBuffer);
static int __stdcall DrawBuildSpotQueue_Entry_Proc(PInlineX86StackBuffer X86StrackBuffer)
{
    PROFILE_SCOPE("Hook.DrawBuildSpotQueue");
    return DrawBuildSpotQueue_Entry_Proc_Inner(X86StrackBuffer);
}
static int __stdcall DrawBuildSpotQueue_Entry_Proc_Inner(PInlineX86StackBuffer X86StrackBuffer)
{
    CUnitRotate* self = CUnitRotate::GetInstance();
    if (!self) return 0;

    // __stdcall 4 args: [Esp+0]=ret, [Esp+4]=surf, [Esp+8]=cam, [Esp+0xC]=pOrder.
    DWORD* stackTop = reinterpret_cast<DWORD*>(X86StrackBuffer->Esp);
    BYTE*  order    = reinterpret_cast<BYTE*>(stackTop[3]);
    if (!order) return 0;

    int rotation = self->TakeOrderRotation(order);
    if ((rotation & 1) == 0) return 0;  // 0 or 2: no X/Z swap needed

    unsigned unitTypeIdx = *reinterpret_cast<unsigned*>(order + OFF_ORDER_build_unitType);
    if (unitTypeIdx == 0) return 0;

    // Defensive: ignore stale entries from a freed-and-reused order address —
    // if the unit type doesn't even allow this rotation, the entry can't be
    // ours. Prevents an AI build at a recycled OrderStruct pointer from
    // accidentally getting a rotated rect.
    if (!self->IsRotationAllowed(unitTypeIdx, rotation)) return 0;

    BYTE* ui = GetUnitInfoRaw(unitTypeIdx);
    if (!ui) return 0;

    // Apply swap now; arm the return thunk to undo it after the engine
    // finishes drawing this one order's rect.
    SwapUnitInfoFpBndXZ(ui);
    g_drawBuildSpotQueueSwappedUi = ui;
    g_drawBuildSpotQueueRealReturn = stackTop[0];
    stackTop[0] = reinterpret_cast<DWORD>(&DrawBuildSpotQueueReturnThunk);
    return 0;
}

// ORDERS_NewSubOrder2Unit: post-cmalloc hook. EAX = newly-allocated OrderStruct*.
// If we're currently inside GUI_IssueMobileBuildOrder, tag the order.
static int __stdcall NewSubOrder_PostAlloc_Proc(PInlineX86StackBuffer X86StrackBuffer)
{
    if (g_issuingRotation < 0) return 0;  // not scoped to a player build-issue

    CUnitRotate* self = CUnitRotate::GetInstance();
    if (!self) return 0;

    void* newOrder = reinterpret_cast<void*>(X86StrackBuffer->Eax);
    if (!newOrder) return 0;  // cmalloc failed

    self->TagOrderRotation(newOrder, g_issuingRotation);
    return 0;
}

// Order_MobileBuild / Order_VTOL_MobileBuild function entry. Install
// footprint+yardmap rotation on UNITINFO for the entire duration of the
// function. See ADDR_Order_MobileBuild_Entry comment block for the three
// pre-UNITS_CreateUnit reads this covers.
static int __stdcall OrderMobileBuild_Entry_Proc(PInlineX86StackBuffer X86StrackBuffer)
{
    CUnitRotate* self = CUnitRotate::GetInstance();
    if (!self) return 0;

    // __stdcall: [Esp]=ret, [Esp+4]=unit_ptr, [Esp+8]=order_ptr, [Esp+0xC]=flags
    DWORD* stackTop = reinterpret_cast<DWORD*>(X86StrackBuffer->Esp);
    BYTE* order = reinterpret_cast<BYTE*>(stackTop[2]);
    if (!order) return 0;

    // Clear any stale UNITINFO swap from _TestBuildSpot's cursor preview
    // BEFORE the engine reads UNITINFO this tick. Without this, an unrotated
    // builder order completing while the player has a rotated cursor preview
    // active on the same unit type ends up reading the cursor's swapped
    // dims / rotated yardmap from UNITINFO and bakes them into the new unit's
    // SizeFootX/SizeFootZ — intermittent staircase yardmap on the resulting
    // south-facing unit. _TestBuildSpot's next frame re-installs the cursor
    // swap so the preview reappears immediately. (This restores the spirit of
    // the ed71666 fix which lived in MobileBuild_PreCreate before the entry
    // hook took over the swap envelope.)
    self->ClearRotation();
    self->ClearYardmapRotation();

    // TakeOrderRotation is a peek — entry never erases. The map entry must
    // outlive every per-tick Order_MobileBuild call until the order
    // completes / is cancelled (DrawBuildSpotQueue still reads it).
    int rotation = self->TakeOrderRotation(order);
    if (rotation == 0) return 0;  // clean UNITINFO is what we want — done

    unsigned unitTypeIdx = *reinterpret_cast<unsigned*>(order + OFF_ORDER_build_unitType);
    if (unitTypeIdx == 0) return 0;

    BYTE* ui = GetUnitInfoRaw(unitTypeIdx);
    if (!ui) return 0;
    if (*(ui + OFF_UNITINFO_bmcode) != 0) return 0;  // mobile build target — no footprint to swap

    // Defensive: ignore stale entries from a freed-and-reused order address.
    if (!self->IsRotationAllowed(unitTypeIdx, rotation)) return 0;

    // Install footprint + yardmap swap. The (idx, rotation) overload drives
    // needSwap from the explicit order rotation without touching m_rotation
    // (and without printing "Build facing: …" — this hook fires per tick).
    self->ApplyRotationTo(unitTypeIdx, rotation);
    self->ApplyYardmapRotationTo(unitTypeIdx, rotation);

    // Arm the return thunk to clear the swap on function exit.
    g_orderMobileBuildSwapActive = true;
    g_orderMobileBuildRealReturn = stackTop[0];
    stackTop[0] = reinterpret_cast<DWORD>(&OrderMobileBuildReturnThunk);
    return 0;
}

// Just before UNITS_CreateUnit runs in Order_MobileBuild / Order_VTOL_MobileBuild.
// ESI = OrderStruct* at this site (confirmed by disassembly). The entry hook
// above already installed the footprint+yardmap swap; this hook only needs to
// stash m_pendingHeading so SendNewUnitsP09 (fired inside UNITS_CreateUnit) can
// apply the rotation to the outgoing creation packet.
static int __stdcall MobileBuild_PreCreate_Proc(PInlineX86StackBuffer X86StrackBuffer)
{
    CUnitRotate* self = CUnitRotate::GetInstance();
    if (!self) return 0;

    BYTE* order = reinterpret_cast<BYTE*>(X86StrackBuffer->Esi);
    if (!order) { self->m_pendingHeading = -1; return 0; }

    int rotation = self->TakeOrderRotation(order);
    if (rotation == 0) { self->m_pendingHeading = -1; return 0; }

    unsigned unitTypeIdx = *reinterpret_cast<unsigned*>(order + OFF_ORDER_build_unitType);
    if (unitTypeIdx == 0) { self->m_pendingHeading = -1; return 0; }

    BYTE* ui = GetUnitInfoRaw(unitTypeIdx);
    if (!ui) { self->m_pendingHeading = -1; return 0; }
    if (*(ui + OFF_UNITINFO_bmcode) != 0) { self->m_pendingHeading = -1; return 0; }

    if (!self->IsRotationAllowed(unitTypeIdx, rotation))
    {
        self->m_pendingHeading = -1;
        return 0;
    }

    self->m_pendingHeading = rotation;
    return 0;
}

// Just after UNITS_CreateUnit returned. The preceding instruction was `PUSH EAX`
// (the new unit pointer), so [ESP] at router entry holds newUnit. Mark the
// freshly-created unit as player-rotated so the runtime yardmap-reader hooks
// know to swap p_YardMap for it. The UNITINFO swap itself stays installed
// for the rest of Order_MobileBuild and is cleared by the entry return thunk.
static int __stdcall MobileBuild_PostCreate_Proc(PInlineX86StackBuffer X86StrackBuffer)
{
    CUnitRotate* self = CUnitRotate::GetInstance();
    if (!self || self->m_pendingHeading < 0) return 0;

    self->m_pendingHeading = -1;

    BYTE* newUnit = *reinterpret_cast<BYTE**>(X86StrackBuffer->Esp);
    if (newUnit)
    {
        // Heading was already applied by SendNewUnitsP09_Entry_Proc — which
        // fires INSIDE UNITS_CreateUnit, just before the engine broadcasts
        // packet 0x09 with bank/heading/pitch. Doing the rotate there means
        // the outgoing packet carries the rotated heading, and remote peers
        // (with our CreateFromNetwork hook) end up with matching state.
        //
        // Mark this unit as player-rotated so the runtime reader hook knows to
        // swap yardmap for it. (Without this, AI-placed structures with large
        // random buildangle get mistaken for rotated ones.)
        self->MarkUnitRotated(newUnit);
    }

    // Intentionally NOT clearing m_orderRotation here. The order is still in
    // the builder's queue throughout the actual construction, and engine
    // functions like DrawBuildSpotQueue keep iterating it to draw the build
    // rectangle — those callers consult TakeOrderRotation, so the entry must
    // outlive PostCreate. Stale entries (from completed/cancelled orders
    // whose addresses get reused) are handled by:
    //   (a) TagOrderRotation overwriting on any new player-issued build, and
    //   (b) DrawBuildSpotQueue / OrderMobileBuild_Entry / MobileBuild_PreCreate
    //       gating their effect behind IsRotationAllowed(unitType, rotation).

    // UNITINFO footprint/yardmap state is intentionally left installed — the
    // entry hook's return thunk (OrderMobileBuildReturnThunk) clears it after
    // the rest of Order_MobileBuild (Order_SetTargetUnit, ORDERS_NewSubOrder2Unit,
    // StartBuild_ResurrectReclaimHelp) finishes running with consistent state.

    return 0;
}

// ============================================================================
// Runtime yardmap readers — Unit_UpdateYardmap / UNITS_CanCloseOrOpenYard /
// YardOpen. Each takes UnitStruct* as the first __stdcall arg. If the unit
// has non-zero heading and a yardmap, swap UNITINFO.p_YardMap to the rotated
// copy for the duration of this call (trampoline restores on return).
// ============================================================================

static int __stdcall YardmapReader_Entry_Proc(PInlineX86StackBuffer X86StrackBuffer)
{
    PROFILE_SCOPE("Hook.YardmapReader");
    CUnitRotate* self = CUnitRotate::GetInstance();
    if (!self) return 0;

    // [Esp+0] = caller's return addr (we'll replace)
    // [Esp+4] = first arg = UnitStruct*
    DWORD* stackTop = reinterpret_cast<DWORD*>(X86StrackBuffer->Esp);
    BYTE* unit = *reinterpret_cast<BYTE**>(stackTop + 1);
    if (!unit) return 0;

    // Only swap yardmap for units WE marked as player-rotated. Units with
    // naturally-large random headings (e.g. CORSOLAR with buildangle=0x8000)
    // must NOT be treated as if we'd rotated them.
    if (!self->IsUnitRotated(unit)) return 0;

    // Read heading (unsigned word at offset 0x66). TA's default facing is
    // 0x8000 (NOT 0x0000), and PostCreate adds rotation*0x4000 to that default.
    // So the user-frame rotation is (heading - 0x8000) / 0x4000, with rounding
    // for jitter. This correctly identifies hd=0x0000 as rotation=2 (180°).
    WORD heading = *reinterpret_cast<WORD*>(unit + OFF_UNIT_Volume_heading);
    WORD deltaFromDefault = (WORD)(heading - 0x8000);
    int rotation = (((deltaFromDefault + 0x2000) & 0xFFFF) / 0x4000) & 3;
    if (rotation == 0) return 0;  // normal facing, no swap needed

    BYTE* unitInfo = *reinterpret_cast<BYTE**>(unit + OFF_UNIT_UNITINFO_p);
    if (!unitInfo) return 0;

    BYTE* currentYardmap = *reinterpret_cast<BYTE**>(unitInfo + OFF_UNITINFO_p_YardMap);
    if (!currentYardmap) return 0;  // unit has no yardmap

    // Look up unit type index to find cache entry. Unit has UnitINFOID at 0xA6.
    WORD unitTypeIdx = *reinterpret_cast<WORD*>(unit + 0xA6);
    BYTE* rotatedYardmap = self->GetRotatedYardmap(unitTypeIdx, rotation);
    if (!rotatedYardmap) return 0;

    // If already swapped (rotated yardmap already installed), don't re-swap
    // and don't install a trampoline — an outer call already handled it.
    if (currentYardmap == rotatedYardmap) return 0;

    if (g_yardmapFrameTop >= YARDMAP_FRAME_STACK_SIZE) return 0;  // bail out on overflow

    YardmapFrame& f = g_yardmapFrames[g_yardmapFrameTop++];
    f.unitInfo = unitInfo;
    f.originalYardmap = currentYardmap;
    f.realReturnAddr = *stackTop;

    SafeWriteProtected(unitInfo + OFF_UNITINFO_p_YardMap, &rotatedYardmap, sizeof(BYTE*));
    *stackTop = reinterpret_cast<DWORD>(&YardmapReturnThunk);
    return 0;
}

// FreeGameData entry: TA is about to tear down all UNITINFO state, in
// particular ReleaseUNITINFO does free(UNITINFO[i].p_YardMap). If we
// have an active footprint or yardmap-pointer swap installed, restore
// the originals BEFORE TA's free runs — otherwise it tries to free our
// `new[]`-allocated rotated yardmap copy through TA's CRT, which doesn't
// own the pointer (different DLL/runtime). Crash signature without this
// restore: AV at ntdll!RtlSizeHeap+0x92 with eax=0, called from a free
// chain rooted at ReleaseUNITINFO (see crash dump 2026-04-25).
static int __stdcall FreeGameData_Entry_Proc(PInlineX86StackBuffer)
{
    CUnitRotate* self = CUnitRotate::GetInstance();
    if (self)
    {
        self->ClearRotation();
        self->ClearYardmapRotation();
    }
    // Drop CBuildGhost caches that hold pointers into TA's per-game pools.
    if (CBuildGhost* g = CBuildGhost::GetInstance()) g->OnGameTeardown();
    return 0;
}

static int __stdcall DrawGameScreen_PostDrawGUI_Proc_Inner(PInlineX86StackBuffer);
static int __stdcall DrawGameScreen_PostDrawGUI_Proc(PInlineX86StackBuffer X)
{
    PROFILE_SCOPE("Hook.DrawGameScreen_PostDrawGUI");
    return DrawGameScreen_PostDrawGUI_Proc_Inner(X);
}
static int __stdcall DrawGameScreen_PostDrawGUI_Proc_Inner(PInlineX86StackBuffer)
{
    if (CUnitRotate* self = CUnitRotate::GetInstance())
        self->DrawBuildMenuRotationOverlays();
    return 0;
}

// ============================================================================
// CUnitRotate
// ============================================================================

CUnitRotate::CUnitRotate()
    : m_pendingHeading(-1),
      m_rotation(0),
      m_activeRotatedUnitInfoIdx(ROT_NONE),
      m_activeYardmapRotatedIdx(ROT_NONE),
      m_rotationsKeyIdx(0),
      m_menuClickFeedbackControlIdx(-1),
      m_menuClickFeedbackCardinal(-1),
      m_menuClickFeedbackTimestamp(0),
      m_rotationCycleGameTime(0)
{
    g_instance = this;

    // Key should have been registered during early ddraw init via
    // RegisterUnitDefKeys (called from ddraw.cpp alongside VeterancyHack etc.).
    // If not, register now as a fallback.
    RegisterUnitDefKeys();
    m_rotationsKeyIdx = g_rotationsKeyIdx;

    m_hooks.push_back(std::unique_ptr<InlineSingleHook>(new InlineSingleHook(
        ADDR_TestBuildSpot_Entry, 5, INLINE_5BYTESLAGGERJMP, TestBuildSpot_Entry_Proc)));
    m_hooks.push_back(std::unique_ptr<InlineSingleHook>(new InlineSingleHook(
        ADDR_IssueMobileBuild_Entry, 5, INLINE_5BYTESLAGGERJMP, IssueMobileBuild_Entry_Proc)));
    m_hooks.push_back(std::unique_ptr<InlineSingleHook>(new InlineSingleHook(
        ADDR_NewSubOrder_PostAlloc, 5, INLINE_5BYTESLAGGERJMP, NewSubOrder_PostAlloc_Proc)));
    // Function-wide rotation envelope: install UNITINFO swap at entry, clear
    // at exit via return thunk. Covers case-0 footprint snap, case-1 area-clear
    // check, and ApplyYardmap stamp — all of which fire BEFORE UNITS_CreateUnit.
    m_hooks.push_back(std::unique_ptr<InlineSingleHook>(new InlineSingleHook(
        ADDR_Order_MobileBuild_Entry, 5, INLINE_5BYTESLAGGERJMP, OrderMobileBuild_Entry_Proc)));
    m_hooks.push_back(std::unique_ptr<InlineSingleHook>(new InlineSingleHook(
        ADDR_Order_VTOL_MobileBuild_Entry, 5, INLINE_5BYTESLAGGERJMP, OrderMobileBuild_Entry_Proc)));
    m_hooks.push_back(std::unique_ptr<InlineSingleHook>(new InlineSingleHook(
        ADDR_MobileBuild_PreCreate, 5, INLINE_5BYTESLAGGERJMP, MobileBuild_PreCreate_Proc)));
    m_hooks.push_back(std::unique_ptr<InlineSingleHook>(new InlineSingleHook(
        ADDR_MobileBuild_PostCreate, 5, INLINE_5BYTESLAGGERJMP, MobileBuild_PostCreate_Proc)));
    m_hooks.push_back(std::unique_ptr<InlineSingleHook>(new InlineSingleHook(
        ADDR_VtolBuild_PreCreate, 5, INLINE_5BYTESLAGGERJMP, MobileBuild_PreCreate_Proc)));
    m_hooks.push_back(std::unique_ptr<InlineSingleHook>(new InlineSingleHook(
        ADDR_VtolBuild_PostCreate, 5, INLINE_5BYTESLAGGERJMP, MobileBuild_PostCreate_Proc)));

    // Per-order queue-rect draw: swap UNITINFO->lFpBnd{X,Z} for rotations 1/3
    // so the persistent on-map rectangle matches the queued order's facing.
    m_hooks.push_back(std::unique_ptr<InlineSingleHook>(new InlineSingleHook(
        ADDR_DrawBuildSpotQueue, 5, INLINE_5BYTESLAGGERJMP, DrawBuildSpotQueue_Entry_Proc)));

    // Multiplayer rotation sync. Local: write heading into the unit just before
    // packet 0x09 is broadcast so the wire carries the rotated value. Remote:
    // copy heading out of the spawn record after CreateFromNetwork runs so the
    // building doesn't sit at the engine's default facing. Both peers need the
    // patched DLL — old peers still see the south-facing default until a COB
    // script broadcast eventually corrects it.
    m_hooks.push_back(std::unique_ptr<InlineSingleHook>(new InlineSingleHook(
        ADDR_SendNewUnitsP09, 5, INLINE_5BYTESLAGGERJMP, SendNewUnitsP09_Entry_Proc)));
    m_hooks.push_back(std::unique_ptr<InlineSingleHook>(new InlineSingleHook(
        ADDR_UNITS_CreateFromNetwork, 5, INLINE_5BYTESLAGGERJMP, CreateFromNetwork_Entry_Proc)));

    m_hooks.push_back(std::unique_ptr<InlineSingleHook>(new InlineSingleHook(
        ADDR_Unit_UpdateYardmap, 5, INLINE_5BYTESLAGGERJMP, YardmapReader_Entry_Proc)));
    m_hooks.push_back(std::unique_ptr<InlineSingleHook>(new InlineSingleHook(
        ADDR_UNITS_RebuildFootPrint, 5, INLINE_5BYTESLAGGERJMP, YardmapReader_Entry_Proc)));
    m_hooks.push_back(std::unique_ptr<InlineSingleHook>(new InlineSingleHook(
        ADDR_Unit_ClearMapTileOccupancy, 5, INLINE_5BYTESLAGGERJMP, YardmapReader_Entry_Proc)));
    m_hooks.push_back(std::unique_ptr<InlineSingleHook>(new InlineSingleHook(
        ADDR_UNITS_CanCloseOrOpenYard, 5, INLINE_5BYTESLAGGERJMP, YardmapReader_Entry_Proc)));
    m_hooks.push_back(std::unique_ptr<InlineSingleHook>(new InlineSingleHook(
        ADDR_YardOpen, 5, INLINE_5BYTESLAGGERJMP, YardmapReader_Entry_Proc)));

    // Restore UNITINFO state before TA's exit teardown frees yardmaps.
    m_hooks.push_back(std::unique_ptr<InlineSingleHook>(new InlineSingleHook(
        ADDR_FreeGameData, 5, INLINE_5BYTESLAGGERJMP, FreeGameData_Entry_Proc)));

    // Render build-menu rotation chevrons AFTER DrawGUI repaints the panel,
    // so a dirty-flag panel redraw doesn't overpaint our overlays. Has to
    // sit here rather than alongside the build-cursor overlays at 0x469f30
    // because that hook fires earlier in DrawGameScreen — before the panel
    // repaint at 0x46a303.
    m_hooks.push_back(std::unique_ptr<InlineSingleHook>(new InlineSingleHook(
        ADDR_DrawGameScreen_PostDrawGUI, 5, INLINE_5BYTESLAGGERJMP,
        DrawGameScreen_PostDrawGUI_Proc)));

    IDDrawSurface::OutptTxt("CUnitRotate loaded. Press '/' to cycle build facing.");
}

CUnitRotate::~CUnitRotate()
{
    ClearRotation();
    ClearYardmapRotation();
    // Free cached rotated yardmaps.
    for (auto& kv : m_yardmapCache)
    {
        for (int r = 1; r < 4; ++r)
        {
            if (kv.second.rotated[r] && kv.second.rotated[r] != kv.second.origYardmap)
            {
                delete[] kv.second.rotated[r];
                kv.second.rotated[r] = nullptr;
            }
        }
    }
    if (g_instance == this) g_instance = nullptr;
}

void CUnitRotate::SetRotation(int r)
{
    m_rotation = r & 3;

    // rotation 0 = engine default heading 0x8000 = south facing.
    // '/' cycles 0→1→2→3 = S→E→N→W (CCW around Y when viewed from above,
    // matching TA's heading-increase = CCW convention).
    const char* dir = "S";
    switch (m_rotation)
    {
        case 1: dir = "E"; break;
        case 2: dir = "N"; break;
        case 3: dir = "W"; break;
    }
    char msg[64];
    _snprintf_s(msg, sizeof(msg), _TRUNCATE, "Build facing: %s", dir);
    IDDrawSurface::OutptTxt(msg);
}

void CUnitRotate::ApplyRotationTo(unsigned int idx)
{
    ApplyRotationTo(idx, m_rotation);
}

void CUnitRotate::ApplyRotationTo(unsigned int idx, int rotation)
{
    // If we already have the right footprint-swap state for this idx, nothing to do.
    bool needSwap = (rotation & 1) == 1;  // 90° or 270°
    int desired   = needSwap ? static_cast<int>(idx) : ROT_ACTIVE_NO_SWAP;
    if (m_activeRotatedUnitInfoIdx == desired) return;

    ClearRotation();

    if (needSwap)
    {
        BYTE* ui = GetUnitInfoRaw(idx);
        if (!ui) return;
        SwapUnitInfoFootprintWords(ui);
        m_activeRotatedUnitInfoIdx = static_cast<int>(idx);
    }
    else
    {
        m_activeRotatedUnitInfoIdx = ROT_ACTIVE_NO_SWAP;
    }
}

void CUnitRotate::ClearRotation()
{
    if (m_activeRotatedUnitInfoIdx >= 0)
    {
        BYTE* ui = GetUnitInfoRaw(static_cast<unsigned>(m_activeRotatedUnitInfoIdx));
        if (ui) SwapUnitInfoFootprintWords(ui);  // swap back (swap is its own inverse)
    }
    m_activeRotatedUnitInfoIdx = ROT_NONE;
}

// ============================================================================
// Per-order rotation side-table
// ============================================================================

void CUnitRotate::TagOrderRotation(void* orderPtr, int rotation)
{
    if (!orderPtr) return;
    if (rotation == 0)
    {
        // Default rotation — no need to occupy map space. But if the address
        // was previously used by a rotated order that got freed and reused,
        // remove the stale entry.
        m_orderRotation.erase(orderPtr);
        return;
    }
    m_orderRotation[orderPtr] = rotation & 3;
}

int CUnitRotate::TakeOrderRotation(void* orderPtr)
{
    auto it = m_orderRotation.find(orderPtr);
    if (it == m_orderRotation.end()) return 0;
    return it->second;
}

void CUnitRotate::ClearOrderRotation(void* orderPtr)
{
    m_orderRotation.erase(orderPtr);
}

void CUnitRotate::MarkUnitRotated(void* unitPtr)
{
    if (unitPtr) m_rotatedUnits[unitPtr] = 1;
}

bool CUnitRotate::IsUnitRotated(void* unitPtr) const
{
    return m_rotatedUnits.find(unitPtr) != m_rotatedUnits.end();
}

void CUnitRotate::UnmarkUnitRotated(void* unitPtr)
{
    m_rotatedUnits.erase(unitPtr);
}

bool CUnitRotate::IsRotationAllowed(unsigned int unitTypeIdx, int rotation) const
{
    rotation &= 3;
    if (rotation == 0) return true;  // default facing (S) always ok

    // Rotation index → cardinal facing letter:
    //   0 = S (default — heading 0x8000)
    //   1 = E (heading 0xC000, 90° CCW)
    //   2 = N (heading 0x0000, 180°)
    //   3 = W (heading 0x4000, 270° CCW)
    const std::string& letters =
        UnitDefExtensions::GetInstance()->getString(unitTypeIdx, m_rotationsKeyIdx);
    static const char kDirs[4] = { 'S', 'E', 'N', 'W' };
    char target = kDirs[rotation];
    for (char c : letters)
    {
        if (std::toupper(static_cast<unsigned char>(c)) == target) return true;
    }
    return false;
}

int CUnitRotate::NextAllowedRotation(unsigned int unitTypeIdx, int current, int direction) const
{
    int sign = (direction >= 0) ? +1 : -1;
    for (int step = 1; step <= 4; ++step)
    {
        int candidate = (current + sign * step) & 3;
        if (IsRotationAllowed(unitTypeIdx, candidate)) return candidate;
    }
    return current;
}

// ============================================================================
// Yardmap rotation — cache + swap helpers
// ============================================================================

BYTE* CUnitRotate::GetRotatedYardmap(unsigned int unitTypeIdx, int rotation)
{
    rotation &= 3;
    if (rotation == 0) return nullptr;

    BYTE* ui = GetUnitInfoRaw(unitTypeIdx);
    if (!ui) return nullptr;

    auto it = m_yardmapCache.find(unitTypeIdx);
    if (it == m_yardmapCache.end())
    {
        // First time — snapshot original dims + yardmap pointer. Handle the
        // case where nFootPrintX/Z are currently swapped (footprint-rotation
        // state == this idx) by un-swapping locally.
        WORD w = *reinterpret_cast<WORD*>(ui + OFF_UNITINFO_nFootPrintX);
        WORD h = *reinterpret_cast<WORD*>(ui + OFF_UNITINFO_nFootPrintZ);
        if (m_activeRotatedUnitInfoIdx == static_cast<int>(unitTypeIdx))
        {
            // Footprint is currently in rotated state — recover original dims.
            std::swap(w, h);
        }
        BYTE* src = *reinterpret_cast<BYTE**>(ui + OFF_UNITINFO_p_YardMap);

        // Guard against garbage dims (would cause huge alloc or integer wraparound).
        if (w == 0 || h == 0 || w > 32 || h > 32) return nullptr;

        YardmapCache& cache = m_yardmapCache[unitTypeIdx];
        cache.origW = w;
        cache.origH = h;
        cache.origYardmap = src;
        cache.rotated[0] = src;
        cache.rotated[1] = src ? RotateYardmap(src, w, h, 1) : nullptr;
        cache.rotated[2] = src ? RotateYardmap(src, w, h, 2) : nullptr;
        cache.rotated[3] = src ? RotateYardmap(src, w, h, 3) : nullptr;
        return cache.rotated[rotation];
    }
    return it->second.rotated[rotation];
}

void CUnitRotate::ApplyYardmapRotationTo(unsigned int unitInfoIdx, int rotation)
{
    rotation &= 3;
    if (rotation == 0)
    {
        ClearYardmapRotation();
        return;
    }

    BYTE* rotatedYardmap = GetRotatedYardmap(unitInfoIdx, rotation);
    if (!rotatedYardmap) { ClearYardmapRotation(); return; }  // no yardmap

    BYTE* ui = GetUnitInfoRaw(unitInfoIdx);
    if (!ui) return;

    BYTE* current = *reinterpret_cast<BYTE**>(ui + OFF_UNITINFO_p_YardMap);
    if (current == rotatedYardmap) return;  // already swapped to this rotation

    // If a different UNITINFO has an active swap, clear it first.
    if (m_activeYardmapRotatedIdx >= 0 &&
        m_activeYardmapRotatedIdx != static_cast<int>(unitInfoIdx))
    {
        ClearYardmapRotation();
    }

    // UNITINFO memory is read-only at runtime — SafeWriteProtected handles
    // the page-protection elevation with fallback.
    SafeWriteProtected(ui + OFF_UNITINFO_p_YardMap, &rotatedYardmap, sizeof(BYTE*));
    m_activeYardmapRotatedIdx = static_cast<int>(unitInfoIdx);
}

void CUnitRotate::ClearYardmapRotation()
{
    if (m_activeYardmapRotatedIdx < 0) return;
    auto it = m_yardmapCache.find(static_cast<unsigned>(m_activeYardmapRotatedIdx));
    if (it != m_yardmapCache.end())
    {
        BYTE* ui = GetUnitInfoRaw(static_cast<unsigned>(m_activeYardmapRotatedIdx));
        if (ui)
        {
            BYTE* orig = it->second.origYardmap;
            SafeWriteProtected(ui + OFF_UNITINFO_p_YardMap, &orig, sizeof(BYTE*));
        }
    }
    m_activeYardmapRotatedIdx = ROT_NONE;
}

// ============================================================================
// UnitDef snapshot for ChallengeResponse
// ============================================================================
//
// ChallengeResponse fires its hash threads off the main message thread, so the
// snapshot must be taken on the calling (main) thread BEFORE the std::thread
// detach — that way the hash thread sees a private, frozen copy and we never
// race against ApplyRotationTo / ApplyYardmapRotationTo / DrawBuildSpotQueue's
// inline-hook swap, all of which run on the main thread.
//
// Reverses two kinds of active swap so the snapshot looks pristine:
//   1. FootX/Y (nFootPrintX/Z) — undo the WORD swap on the affected entry.
//   2. YardMap pointer — rewrite to the cached origYardmap so a downstream
//      reader (HashUnits hashes FootX*FootY bytes pointed at by YardMap)
//      sees the original buffer's content. The pointed-at bytes themselves
//      are immutable (yardmap arrays are loaded from FBI and never mutated),
//      so we don't need to copy the bytes.
//
// lFpBnd swaps are short-lived to a single DrawBuildSpotQueue call on the
// main thread; they cannot be active here because the snapshot is itself
// taken on the main thread, so DrawBuildSpotQueue can't be on the stack.
void CUnitRotate::CaptureUnitDefSnapshot(std::vector<UnitDefStruct>& out) const
{
    TAdynmemStruct* ta = GetTA();
    if (!ta || !ta->UnitDef)
    {
        out.clear();
        return;
    }
    out.assign(ta->UnitDef, ta->UnitDef + ta->UNITINFOCount);

    // Reverse footprint swap, if any.
    if (m_activeRotatedUnitInfoIdx >= 0 &&
        static_cast<unsigned>(m_activeRotatedUnitInfoIdx) < out.size())
    {
        UnitDefStruct& u = out[m_activeRotatedUnitInfoIdx];
        short tmp = u.FootX; u.FootX = u.FootY; u.FootY = tmp;
    }

    // Reverse yardmap-pointer swap, if any.
    if (m_activeYardmapRotatedIdx >= 0 &&
        static_cast<unsigned>(m_activeYardmapRotatedIdx) < out.size())
    {
        auto it = m_yardmapCache.find(static_cast<unsigned>(m_activeYardmapRotatedIdx));
        if (it != m_yardmapCache.end())
        {
            out[m_activeYardmapRotatedIdx].YardMap =
                reinterpret_cast<char*>(it->second.origYardmap);
        }
    }
}


// ============================================================================
// Keyboard handling
// ============================================================================

bool CUnitRotate::Message(HWND, UINT Msg, WPARAM wParam, LPARAM)
{
    if (TAInGame != DataShare->TAProgress) return false;
    if (Msg != WM_KEYDOWN) return false;

    // Only act while the build cursor is active (a building type is selected).
    TAdynmemStruct* ta = GetTA();
    if (!ta || ta->BuildUnitID == 0) return false;
    // BuildUnitID lingers after a right-click cancel (it stays on the last
    // build menu selection until the player picks a different builder),
    // so also gate on PrepareOrder_Type to make sure the build cursor is
    // genuinely active. Same check the ghost renderer uses.
    if (ta->PrepareOrder_Type != ordertype::BUILD) return false;

    // Don't intercept if Ctrl is held — that's reserved for other hotkeys.
    // Shift is INTENTIONALLY allowed: line-building (shift-click rows) is
    // exactly when the player most often wants to rotate, and forcing them to
    // lift Shift just to press '/' is friction.
    if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0) return false;

    // The rotation key (default '/' = VK_OEM_2) cycles through the rotations
    // allowed by the current build unit's "Rotations" FBI field. The actual
    // VK code is configurable via the Ctrl-F2 settings dialog
    // (Dialog::GetRotateBuildKey). If the unit only allows ONE facing
    // (typically just S), the keypress is ignored entirely — no sound, no
    // state change, we don't even consume the key. This preserves m_rotation
    // across unit-type switches: rotate ARMVP to E, hover over a shipyard,
    // press the rotate key — does nothing — switch back to ARMVP and you're
    // still on E. Uses TA's "Options" menu-click sound on successful cycle.
    int rotateKey = VK_OEM_2;
    if (LocalShare && LocalShare->Dialog)
        rotateKey = reinterpret_cast<Dialog*>(LocalShare->Dialog)->GetRotateBuildKey();
    if (static_cast<int>(wParam) == rotateKey)
    {
        // Cycle forward (CCW). If the unit only allows ONE facing, the cycle
        // is a no-op and we DON'T consume the key — same key (e.g. '/') is
        // free to act as a normal text/UI character outside our context.
        return TryCycleRotation(+1);
    }
    return false;
}

bool CUnitRotate::TryCycleRotation(int direction)
{
    TAdynmemStruct* ta = GetTA();
    if (!ta || ta->BuildUnitID == 0) return false;
    if (ta->PrepareOrder_Type != ordertype::BUILD) return false;

    unsigned buildIdx = static_cast<unsigned>(ta->BuildUnitID);

    // No-op (and report "didn't cycle") when the unit type only allows one
    // facing — preserves m_rotation across hovers over non-rotatable units.
    int allowedCount = 0;
    for (int r = 0; r < 4; ++r)
        if (IsRotationAllowed(buildIdx, r)) ++allowedCount;
    if (allowedCount < 2) return false;

    int next = NextAllowedRotation(buildIdx, m_rotation, direction);
    if (next == m_rotation) return false;

    SetRotation(next);
    m_rotationCycleGameTime = static_cast<unsigned>(ta->GameTime);
    // "MORE" → button12.wav in ALLSOUND.TDF — soft button click.
    PlaySound_Effect((char*)"MORE", 0);
    if (CBuildGhost* g = CBuildGhost::GetInstance())
        g->SetRotateKeyDiscovered();
    return true;
}

// ============================================================================
// Build-menu rotation overlay (prototype)
// ============================================================================

int CUnitRotate::FindUnitTypeIdxByName(const char* name) const
{
    // GUI button names ("ARMVP", "ARMSOLAR", …) are FBI UnitName values,
    // which are stored in UnitDefStruct::UnitName — NOT the .Name field
    // (which is the human-readable "Vehicle Plant" display name).
    if (!name || !*name) return -1;
    TAdynmemStruct* ta = GetTA();
    if (!ta || !ta->UnitDef) return -1;
    unsigned count = ta->UNITINFOCount;
    for (unsigned i = 0; i < count; ++i)
    {
        if (_stricmp(ta->UnitDef[i].UnitName, name) == 0)
            return static_cast<int>(i);
    }
    return -1;
}

bool CUnitRotate::IsRotatableStructure(unsigned int unitTypeIdx) const
{
    TAdynmemStruct* ta = GetTA();
    if (!ta || !ta->UnitDef || unitTypeIdx >= ta->UNITINFOCount) return false;
    if (ta->UnitDef[unitTypeIdx].bmcode != 0) return false;
    int allowed = 0;
    for (int r = 0; r < 4; ++r)
        if (IsRotationAllowed(unitTypeIdx, r)) ++allowed;
    return allowed >= 2;
}

void CUnitRotate::EnsureRotatableNameCache() const
{
    TAdynmemStruct* ta = GetTA();
    if (!ta || !ta->UnitDef) return;
    const unsigned count = ta->UNITINFOCount;
    if (count == m_rotatableCacheUnitInfoCount && !m_rotatableUnitIdxByLowerName.empty())
        return;

    m_rotatableUnitIdxByLowerName.clear();
    m_rotatableUnitIdxByLowerName.reserve(64);
    char buf[32];
    for (unsigned i = 0; i < count; ++i)
    {
        if (!IsRotatableStructure(i)) continue;
        const char* src = ta->UnitDef[i].UnitName;
        size_t len = 0;
        for (; len < sizeof(buf) - 1 && src[len]; ++len)
            buf[len] = static_cast<char>(std::tolower(static_cast<unsigned char>(src[len])));
        buf[len] = 0;
        m_rotatableUnitIdxByLowerName.emplace(std::string(buf, len), static_cast<int>(i));
    }
    m_rotatableCacheUnitInfoCount = count;
}

int CUnitRotate::FindRotatableUnitIdxByName(const char* name) const
{
    if (!name || !*name) return -1;
    EnsureRotatableNameCache();
    if (m_rotatableUnitIdxByLowerName.empty()) return -1;
    char buf[32];
    size_t len = 0;
    for (; len < sizeof(buf) - 1 && name[len]; ++len)
        buf[len] = static_cast<char>(std::tolower(static_cast<unsigned char>(name[len])));
    buf[len] = 0;
    auto it = m_rotatableUnitIdxByLowerName.find(std::string(buf, len));
    return it == m_rotatableUnitIdxByLowerName.end() ? -1 : it->second;
}

namespace
{
    // Chevron geometry — shared between the renderer and the click
    // hit-tester so the centre dead-zone matches what the chevrons
    // actually occupy.
    const int kChevronArm        = 3;   // half-width of chevron arm
    const int kChevronDepth      = 4;   // distance from arm-tip to apex
    const int kChevronGap        = 4;   // distance between two apices
    const int kChevronInset      = 5;   // distance of front apex from edge
    const int kCardinalMarginPx  = kChevronInset + kChevronGap + kChevronDepth;

    // Identify which cardinal a click landed in, given the click is
    // inside the button rect at relative (rx, ry) and button size (W, H).
    // Returns {0=S, 1=E, 2=N, 3=W} corresponding to the closest edge,
    // or -1 if the click is in the central dead zone.
    int ClickToCardinal(int rx, int ry, int W, int H)
    {
        const int distN = ry;
        const int distS = (H - 1) - ry;
        const int distE = (W - 1) - rx;
        const int distW = rx;
        int minD = distN;
        if (distS < minD) minD = distS;
        if (distE < minD) minD = distE;
        if (distW < minD) minD = distW;
        if (minD >= kCardinalMarginPx) return -1;
        if (minD == distN) return 2;
        if (minD == distS) return 0;
        if (minD == distE) return 1;
        return 3;
    }

    // ---- Surface-direct pixel drawing for the build-menu overlay ----
    //
    // TA's TADrawRect (0x4BF8C0 = DrawTranspRectangle) draws stippled
    // diagonal lines, not solid fills, so it can't render the chevron /
    // triangle shapes we want here. Instead we lock the back-buffer
    // through the same TA helpers CBuildGhost uses and plot pixels
    // directly into the 8-bpp surface.

    using Fn_LockAttackSurface     = int  (__stdcall*)(void*);
    using Fn_UnlockAttackedSurface = void (__stdcall*)(void*);
    const Fn_LockAttackSurface     kFn_LockAttackSurface     = (Fn_LockAttackSurface)    0x004c5e70;
    const Fn_UnlockAttackedSurface kFn_UnlockAttackedSurface = (Fn_UnlockAttackedSurface)0x004c5fa0;

    struct PixelBuf
    {
        unsigned char* data;
        int            pitch;
        int            width;
        int            height;
    };

    inline void Plot(PixelBuf& b, int x, int y, int color)
    {
        if ((unsigned)x < (unsigned)b.width && (unsigned)y < (unsigned)b.height)
            b.data[y * b.pitch + x] = (unsigned char)color;
    }

    // Bresenham line, optionally fattened: each plotted pixel is a
    // (2*half+1) square so thickness 1 = single pixel, thickness 3 = 3x3.
    void DrawLineFat(PixelBuf& b, int x0, int y0, int x1, int y1, int color, int thickness)
    {
        const int half = thickness / 2;
        int dx = std::abs(x1 - x0);
        int dy = -std::abs(y1 - y0);
        int sx = x0 < x1 ? 1 : -1;
        int sy = y0 < y1 ? 1 : -1;
        int err = dx + dy;
        for (;;)
        {
            for (int oy = -half; oy <= half; ++oy)
                for (int ox = -half; ox <= half; ++ox)
                    Plot(b, x0 + ox, y0 + oy, color);
            if (x0 == x1 && y0 == y1) break;
            int e2 = 2 * err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    }

    // Two stacked chevrons whose apices point toward the cardinal edge.
    // Drawn outline-first (3-px line in outlineColor) then a 1-px line of
    // fillColor on top. Highlight reuses the same geometry with different
    // colours, so there's no residual pixels left behind when the
    // highlight expires (TA's GUI panel only repaints on UI events, not
    // every frame).
    void DrawChevronPair(PixelBuf& b, int btnX, int btnY, int btnW, int btnH,
                         int cardinal, int fillColor, int outlineColor)
    {
        const int arm   = kChevronArm;
        const int depth = kChevronDepth;
        const int gap   = kChevronGap;
        const int inset = kChevronInset;

        // Outward = direction the apex points (toward the edge).
        // Perpendicular = sideways spread of the arms.
        static const int outX[4] = {  0, +1,  0, -1 };  // S, E, N, W
        static const int outY[4] = { +1,  0, -1,  0 };
        const int ox = outX[cardinal & 3];
        const int oy = outY[cardinal & 3];
        const int px = oy;       // perpendicular = outward rotated 90°
        const int py = -ox;

        // Edge midpoint, then walk inward by inset for the front apex,
        // and another `gap` for the back apex.
        int edgeMidX, edgeMidY;
        switch (cardinal & 3)
        {
        case 0:  edgeMidX = btnX + btnW / 2; edgeMidY = btnY + btnH;     break;
        case 1:  edgeMidX = btnX + btnW;     edgeMidY = btnY + btnH / 2; break;
        case 2:  edgeMidX = btnX + btnW / 2; edgeMidY = btnY;            break;
        case 3:
        default: edgeMidX = btnX;            edgeMidY = btnY + btnH / 2; break;
        }
        const int a1x = edgeMidX - inset * ox, a1y = edgeMidY - inset * oy;
        const int a2x = a1x      - gap   * ox, a2y = a1y      - gap   * oy;

        auto drawChevron = [&](int ax, int ay, int color, int thick)
        {
            // Arm tips: apex - depth*outward ± arm*perpendicular
            const int leftX  = ax - depth * ox - arm * px;
            const int leftY  = ay - depth * oy - arm * py;
            const int rightX = ax - depth * ox + arm * px;
            const int rightY = ay - depth * oy + arm * py;
            DrawLineFat(b, ax, ay, leftX,  leftY,  color, thick);
            DrawLineFat(b, ax, ay, rightX, rightY, color, thick);
        };
        // Outline pass first, then fill on top.
        drawChevron(a1x, a1y, outlineColor, 3);
        drawChevron(a2x, a2y, outlineColor, 3);
        drawChevron(a1x, a1y, fillColor,    1);
        drawChevron(a2x, a2y, fillColor,    1);
    }

    // ---- Optional 4-frame GAF override for the cardinal arrows ----
    //
    // Mod authors can drop a file named "buildrotate.gaf" in the game
    // directory containing a single sequence with exactly 4 frames in
    // the order S, E, N, W (matching our rotation index). Each frame's
    // hotspot is anchored at the mid-edge of the corresponding cardinal
    // of the build button.
    //
    // Format reference: src/DDraw/gaf.cpp (CopyGafToBits commented body
    // shows the RLE decompression scheme).
    struct GafFrameImg
    {
        int                   width;
        int                   height;
        int                   hotX;
        int                   hotY;
        unsigned char         background;   // pixel value treated as transparent
        std::vector<unsigned char> pixels;  // width * height
    };

    // Idle frames (anims/buildrotate.gaf) and the optional click-flash
    // overlay (anims/buildrotateclick.gaf). When the click GAF is
    // present its frames are blitted in place of the tint-coloured idle
    // frames during the feedback window. When absent we fall back to a
    // white tint on the idle frames; when neither GAF loads we use the
    // built-in chevrons.
    static bool         s_gafLoadAttempted      = false;
    static bool         s_gafLoaded             = false;
    static GafFrameImg  s_gafFrames[4];

    static bool         s_gafClickLoadAttempted = false;
    static bool         s_gafClickLoaded        = false;
    static GafFrameImg  s_gafClickFrames[4];

    // RLE decompression — same per-line scheme TA uses (see gaf.cpp:185+).
    void DecompressGafFrameRLE(const unsigned char* src, int width, int height,
                               unsigned char background, unsigned char* dst,
                               size_t srcRemaining)
    {
        for (int y = 0; y < height; ++y)
        {
            if (srcRemaining < 2) return;
            int byteCount = *reinterpret_cast<const short*>(src);
            src += 2; srcRemaining -= 2;
            if ((size_t)byteCount > srcRemaining) return;
            int x = 0;
            int count = 0;
            unsigned char* line = dst + y * width;
            while (count < byteCount)
            {
                unsigned char mask = src[count++];
                if (mask & 0x01)
                {
                    x += (mask >> 1);
                }
                else if (mask & 0x02)
                {
                    int repeat = (mask >> 2) + 1;
                    if (count >= byteCount) break;
                    unsigned char v = src[count++];
                    while (repeat--)
                        if (x < width) line[x++] = v;
                }
                else
                {
                    int repeat = (mask >> 2) + 1;
                    while (repeat--)
                    {
                        if (count >= byteCount) break;
                        unsigned char v = src[count++];
                        if (x < width) line[x++] = v;
                    }
                }
            }
            src += byteCount; srcRemaining -= byteCount;
        }
    }

    // Parse a 4-frame GAF file from disk into `out`. Returns false on any
    // I/O error, malformed header, frame-count mismatch, or unreasonable
    // sizes — caller falls back to whatever next stage applies.
    bool LoadGaf4Frames(const char* path, GafFrameImg out[4])
    {
        // Use TA's HPI-aware reader so the file is found whether the
        // mod ships it as a loose file under <gamedir>/anims/ OR packs
        // it inside an HPI / GP3 archive alongside its other anims.
        // TA's HPI lookup uses backslashes; mirror that convention.
        char hpiPath[260];
        size_t n = strlen(path);
        if (n >= sizeof(hpiPath)) return false;
        for (size_t i = 0; i <= n; ++i)
            hpiPath[i] = (path[i] == '/') ? '\\' : path[i];

        // Go through the raw readfile_HPI function pointer rather than
        // the _TAHPI C++ wrapper. The wrapper instance (TAHPI*) is only
        // constructed by AddtionInit, which isn't wired up in this DLL
        // configuration; the underlying function pointer
        // (initialised in HPIfunc.cpp from 0x004BBE50) works on its
        // own once TA has loaded its HPI banks. fopen_HPI inside
        // readfile_HPI tries _fopen first (loose file under cwd) before
        // walking the HPI bank list, so loose `anims/foo.gaf` files
        // resolve too.
        unsigned int sz = 0;
        char* raw = readfile_HPI(hpiPath, &sz);
        if (!raw) return false;
        if (sz < 12 || sz > 16u * 1024u * 1024u)
        {
            IDDrawSurface::OutptFmtTxt("[BuildRotateGAF] %s: implausible size %u, skipping",
                hpiPath, sz);
            freeTAMem(raw);
            return false;
        }

        const unsigned char* p = reinterpret_cast<const unsigned char*>(raw);
        bool ok = false;
        do
        {
            if (*reinterpret_cast<const unsigned*>(p) != 0x00010100u) break;
            unsigned numEntries = *reinterpret_cast<const unsigned*>(p + 4);
            if (numEntries < 1 || numEntries > 1024) break;

            unsigned entryOff = *reinterpret_cast<const unsigned*>(p + 12);
            if (entryOff + 40 > sz) break;
            unsigned short frameCount = *reinterpret_cast<const unsigned short*>(p + entryOff);
            if (frameCount != 4)
            {
                IDDrawSurface::OutptFmtTxt("[BuildRotateGAF] %s: entry has %u frames, expected 4 — ignoring",
                    path, (unsigned)frameCount);
                break;
            }

            unsigned frameTableOff = entryOff + 40;
            bool framesOk = true;
            for (int i = 0; i < 4 && framesOk; ++i)
            {
                if (frameTableOff + (size_t)i * 8 + 8 > sz) { framesOk = false; break; }
                unsigned framePtrOff =
                    *reinterpret_cast<const unsigned*>(p + frameTableOff + (size_t)i * 8);
                if (framePtrOff + 24 > sz) { framesOk = false; break; }

                unsigned short  W   = *reinterpret_cast<const unsigned short*>(p + framePtrOff + 0);
                unsigned short  H   = *reinterpret_cast<const unsigned short*>(p + framePtrOff + 2);
                short           XP  = *reinterpret_cast<const short*>         (p + framePtrOff + 4);
                short           YP  = *reinterpret_cast<const short*>         (p + framePtrOff + 6);
                unsigned char   bg  = *(p + framePtrOff + 8);
                unsigned char   cmp = *(p + framePtrOff + 9);
                unsigned        pixOff =
                    *reinterpret_cast<const unsigned*>(p + framePtrOff + 16);
                if (W == 0 || H == 0 || W > 1024 || H > 1024) { framesOk = false; break; }
                if (pixOff >= sz) { framesOk = false; break; }

                GafFrameImg& fr = out[i];
                fr.width      = W;
                fr.height     = H;
                fr.hotX       = XP;
                fr.hotY       = YP;
                fr.background = bg;
                fr.pixels.assign(static_cast<size_t>(W) * H, bg);

                if (cmp)
                {
                    DecompressGafFrameRLE(p + pixOff, W, H, bg, fr.pixels.data(),
                        sz - pixOff);
                }
                else
                {
                    size_t need = static_cast<size_t>(W) * H;
                    if (pixOff + need > sz) { framesOk = false; break; }
                    memcpy(fr.pixels.data(), p + pixOff, need);
                }
            }
            if (!framesOk) break;

            IDDrawSurface::OutptFmtTxt("[BuildRotateGAF] loaded 4 frames from %s "
                "(S=%dx%d, E=%dx%d, N=%dx%d, W=%dx%d)",
                path,
                out[0].width, out[0].height,
                out[1].width, out[1].height,
                out[2].width, out[2].height,
                out[3].width, out[3].height);
            ok = true;
        } while (false);

        freeTAMem(raw);
        return ok;
    }

    // VA of the "anims" directory-name string TotalA.exe passes as the
    // `directory` argument to LocalizedFilePath() inside LoadAnimGaf
    // (xref'd from LoadAnimGaf @ 0x00429758, also LoadFeatureUnit and
    // InitBasicGameData). Some mods hack this string in place to point
    // at a renamed asset directory, so we read it at runtime instead
    // of hardcoding "anims".
    static const char* GetAnimsDirName()
    {
        const char* p = reinterpret_cast<const char*>(0x00502e30);
        size_t n = 0;
        while (n < 32 && p[n] != '\0') ++n;
        if (n == 0 || n >= 32) return "anims";   // defensive fallback
        return p;
    }

    bool TryLoadBuildRotateGaf()
    {
        s_gafLoadAttempted = true;
        char path[260];
        _snprintf_s(path, _TRUNCATE, "%s\\buildrotate.gaf", GetAnimsDirName());
        s_gafLoaded = LoadGaf4Frames(path, s_gafFrames);
        return s_gafLoaded;
    }

    bool TryLoadBuildRotateClickGaf()
    {
        s_gafClickLoadAttempted = true;
        char path[260];
        _snprintf_s(path, _TRUNCATE, "%s\\buildrotateclick.gaf", GetAnimsDirName());
        s_gafClickLoaded = LoadGaf4Frames(path, s_gafClickFrames);
        return s_gafClickLoaded;
    }

    // Blit a GAF frame onto the back buffer, transparent-aware (any pixel
    // matching frm.background is skipped). When `tintColor >= 0`, every
    // non-transparent pixel is recoloured to that index — used to flash
    // the frame for the click-feedback highlight without changing pixel
    // coverage (so the next regular blit fully overwrites the flash with
    // no residual).
    void BlitGafFrame(PixelBuf& b, int dstX, int dstY, const GafFrameImg& frm,
                      int tintColor = -1)
    {
        for (int y = 0; y < frm.height; ++y)
        {
            int sy = dstY + y;
            if (sy < 0 || sy >= b.height) continue;
            const unsigned char* src = frm.pixels.data() + (size_t)y * frm.width;
            unsigned char* dst = b.data + (size_t)sy * b.pitch;
            for (int x = 0; x < frm.width; ++x)
            {
                int sx = dstX + x;
                if ((unsigned)sx >= (unsigned)b.width) continue;
                unsigned char v = src[x];
                if (v == frm.background) continue;
                dst[sx] = (tintColor >= 0) ? (unsigned char)tintColor : v;
            }
        }
    }
}

void CUnitRotate::DrawBuildMenuRotationOverlays()
{
    const bool enabled = !(LocalShare && LocalShare->Dialog
        && !reinterpret_cast<Dialog*>(LocalShare->Dialog)->GetBuildMenuRotationOverlayEnabled());

    TAdynmemStruct* ta = GetTA();

    // On enable→disable transition, dirty all panels so GUI_Draw re-blits
    // and overpaints leftover chevrons. Active_b (+0x14) is GUI_Draw's
    // dirty flag: skipped when 0, cleared after blit.
    if (!enabled && m_lastOverlayEnabled && ta)
    {
        int hops = 0;
        for (GUIMEMSTRUCT* g = ta->desktopGUI.TheActive_GUIMEM;
             g && hops < 32; g = g->per_active, ++hops)
        {
            g->Active_b = 1;
        }
    }
    m_lastOverlayEnabled = enabled;

    if (!enabled || !ta) return;

    // Feedback flash (highlight only renders during this window).
    const DWORD kFeedbackMs = 200;
    const DWORD now         = GetTickCount();
    const DWORD elapsed     = now - m_menuClickFeedbackTimestamp;
    const bool  feedbackOn  = (m_menuClickFeedbackControlIdx > 0
                               && m_menuClickFeedbackCardinal >= 0
                               && elapsed < kFeedbackMs);

    // Fallback chevron colours. Fill = palette index 251 (pure yellow
    // (255,255,0), the same static-palette slot the in-game chat text
    // uses); outline = palette index 0 (black). We deliberately avoid
    // 240..247 here — that's TA's team-colour slot range, overwritten
    // at game start with each player's logo colour, which would render
    // the chevron e.g. black for a black-team player. Highlight = both
    // passes in RadarObjecColor[15] (the bright white HUD slot HUD
    // notifications use), so the chevron flashes solid white. Same
    // pixel coverage as the regular chevron so reverting after the
    // feedback window leaves no residual.
    const int kColorChevronFill    = 251;
    const int kColorChevronOutline = 0;
    const int kColorHighlightFill  = ta->desktopGUI.RadarObjecColor[15];
    const int kColorHighlightOutln = kColorHighlightFill;

    // Walk per_active so chevrons render when chat or the Ctrl-F2 dialog
    // is on top of the build menu.
    constexpr int kMaxChainHops = 32;

    // Track screen rects of panels above the current one in the chain.
    // The in-game tab/options menu is layered ABOVE the build menu, so its
    // rect must occlude chevrons even though the build menu is in the
    // chain. controls[0] holds the panel's own xpos/ypos/width/height.
    struct PanelRect { int left, top, right, bottom; };
    PanelRect upperRects[kMaxChainHops];
    int upperRectCount = 0;

    auto buttonOccluded = [&](int bx, int by, int bw, int bh) -> bool {
        for (int k = 0; k < upperRectCount; ++k) {
            const PanelRect& u = upperRects[k];
            if (bx < u.right && bx + bw > u.left &&
                by < u.bottom && by + bh > u.top) return true;
        }
        return false;
    };

    // Cheap first pass — bail before locking if no rotatable button is
    // visible (panels with no controls don't occlude; per_active walks
    // top-down so each iteration sees only strictly-higher panels).
    bool anyRotatable = false;
    {
        int hops = 0;
        for (GUIMEMSTRUCT* gui = ta->desktopGUI.TheActive_GUIMEM;
             gui && hops < kMaxChainHops && !anyRotatable;
             gui = gui->per_active, ++hops)
        {
            if (!gui->ControlsAry) continue;
            _GUI0IDControl* controls = gui->ControlsAry;
            const int totalGadgets = controls[0].totalgadgets;
            if (totalGadgets <= 0 || totalGadgets > 256) continue;
            const int panelX = controls[0].xpos;
            const int panelY = controls[0].ypos;
            for (int i = 1; i <= totalGadgets; ++i)
            {
                if (!controls[i].active) continue;
                char nm[17]; memcpy(nm, controls[i].name, 16); nm[16] = 0;
                if (FindRotatableUnitIdxByName(nm) < 0) continue;
                if (buttonOccluded(panelX + controls[i].xpos, panelY + controls[i].ypos,
                                   controls[i].width, controls[i].height)) continue;
                anyRotatable = true;
                break;
            }
            if (upperRectCount < kMaxChainHops)
            {
                upperRects[upperRectCount++] = {
                    controls[0].xpos, controls[0].ypos,
                    controls[0].xpos + controls[0].width,
                    controls[0].ypos + controls[0].height
                };
            }
        }
    }
    if (!anyRotatable) return;
    upperRectCount = 0;  // reset for the draw pass below

    // Lazy-load the optional buildrotate.gaf override (and its click
    // companion) on first use. Each attempt is tried at most once.
    if (!s_gafLoadAttempted)      TryLoadBuildRotateGaf();
    if (!s_gafClickLoadAttempted) TryLoadBuildRotateClickGaf();

    OFFSCREEN off;
    memset(&off, 0, sizeof(off));
    if (!kFn_LockAttackSurface(&off)) return;
    if (!off.lpSurface)
    {
        kFn_UnlockAttackedSurface(&off);
        return;
    }

    PixelBuf b;
    b.data   = static_cast<unsigned char*>(off.lpSurface);
    b.pitch  = off.lPitch;
    b.width  = off.Width;
    b.height = off.Height;

    int drawHops = 0;
    for (GUIMEMSTRUCT* gui = ta->desktopGUI.TheActive_GUIMEM;
         gui && drawHops < kMaxChainHops; gui = gui->per_active, ++drawHops)
    {
        if (!gui->ControlsAry) continue;
        _GUI0IDControl* controls = gui->ControlsAry;
        const int totalGadgets = controls[0].totalgadgets;
        if (totalGadgets <= 0 || totalGadgets > 256) continue;

        // Buttons are panel-relative; controls[0] is the panel itself.
        const int panelX = controls[0].xpos;
        const int panelY = controls[0].ypos;

        for (int i = 1; i <= totalGadgets; ++i)
        {
            _GUI0IDControl& c = controls[i];
            if (!c.active) continue;
            if (c.width <= 0 || c.height <= 0) continue;

            char ctrlName[17];
            memcpy(ctrlName, c.name, 16);
            ctrlName[16] = 0;
            int idx = FindRotatableUnitIdxByName(ctrlName);
            if (idx < 0) continue;

            const int btnX = panelX + c.xpos;
            const int btnY = panelY + c.ypos;
            if (buttonOccluded(btnX, btnY, c.width, c.height)) continue;
            for (int r = 0; r < 4; ++r)
            {
                if (!IsRotationAllowed(static_cast<unsigned>(idx), r)) continue;
                const bool highlightThis = feedbackOn
                    && m_menuClickFeedbackControlIdx == i
                    && m_menuClickFeedbackCardinal == r;

                if (s_gafLoaded)
                {
                    // Anchor the GAF frame's hotspot just inside the centre
                    // of the chevron-margin band on the cardinal edge —
                    // same band the chevron fallback occupies, with a
                    // 1-pixel nudge toward the button centre to leave a
                    // small gap between the frame and the panel border.
                    const int halfBand = kCardinalMarginPx / 2 + 1;
                    int anchorX, anchorY;
                    switch (r)
                    {
                    case 0:  anchorX = btnX + c.width / 2;            anchorY = btnY + c.height - halfBand; break;
                    case 1:  anchorX = btnX + c.width - halfBand;     anchorY = btnY + c.height / 2;        break;
                    case 2:  anchorX = btnX + c.width / 2;            anchorY = btnY + halfBand;            break;
                    case 3:
                    default: anchorX = btnX + halfBand;               anchorY = btnY + c.height / 2;        break;
                    }
                    // Use the dedicated click frame when the mod ships
                    // anims/buildrotateclick.gaf. If only the idle GAF is
                    // present we deliberately render it without tint —
                    // mods opting into a custom GAF get no fallback flash;
                    // they need to ship the companion click GAF if they
                    // want one.
                    if (highlightThis && s_gafClickLoaded)
                    {
                        const GafFrameImg& fr = s_gafClickFrames[r];
                        BlitGafFrame(b, anchorX - fr.hotX, anchorY - fr.hotY, fr);
                    }
                    else
                    {
                        const GafFrameImg& fr = s_gafFrames[r];
                        BlitGafFrame(b, anchorX - fr.hotX, anchorY - fr.hotY, fr);
                    }
                    continue;
                }

                if (highlightThis)
                    DrawChevronPair(b, btnX, btnY, c.width, c.height, r,
                        kColorHighlightFill, kColorHighlightOutln);
                else
                    DrawChevronPair(b, btnX, btnY, c.width, c.height, r,
                        kColorChevronFill, kColorChevronOutline);
            }
        }

        if (upperRectCount < kMaxChainHops)
        {
            upperRects[upperRectCount++] = {
                controls[0].xpos, controls[0].ypos,
                controls[0].xpos + controls[0].width,
                controls[0].ypos + controls[0].height
            };
        }
    }

    kFn_UnlockAttackedSurface(&off);
}

bool CUnitRotate::OnBuildMenuClick(int screenX, int screenY)
{
    if (LocalShare && LocalShare->Dialog
        && !reinterpret_cast<Dialog*>(LocalShare->Dialog)->GetBuildMenuRotationOverlayEnabled())
    {
        return false;
    }

    TAdynmemStruct* ta = GetTA();
    if (!ta) return false;
    GUIMEMSTRUCT* gui = ta->desktopGUI.TheActive_GUIMEM;
    if (!gui || !gui->ControlsAry) return false;

    _GUI0IDControl* controls = gui->ControlsAry;
    int totalGadgets = controls[0].totalgadgets;
    if (totalGadgets <= 0 || totalGadgets > 256) return false;

    const int panelX = controls[0].xpos;
    const int panelY = controls[0].ypos;

    for (int i = 1; i <= totalGadgets; ++i)
    {
        _GUI0IDControl& c = controls[i];
        if (!c.active) continue;
        const int btnX = panelX + c.xpos;
        const int btnY = panelY + c.ypos;
        if (screenX < btnX || screenX >= btnX + c.width)  continue;
        if (screenY < btnY || screenY >= btnY + c.height) continue;

        char ctrlName[17];
        memcpy(ctrlName, c.name, 16);
        ctrlName[16] = 0;
        int idx = FindRotatableUnitIdxByName(ctrlName);
        if (idx < 0) continue;

        // Click in the central dead-zone → preserve previously selected
        // rotation. Only the chevron-occupied band on each edge sets a
        // new cardinal.
        const int rx = screenX - btnX;
        const int ry = screenY - btnY;
        int cardinal = ClickToCardinal(rx, ry, c.width, c.height);
        if (cardinal < 0) return false;
        if (!IsRotationAllowed(static_cast<unsigned>(idx), cardinal)) return false;

        SetRotation(cardinal);
        if (TAdynmemStruct* ta = GetTA())
            m_rotationCycleGameTime = static_cast<unsigned>(ta->GameTime);
        m_menuClickFeedbackControlIdx = i;
        m_menuClickFeedbackCardinal   = cardinal;
        m_menuClickFeedbackTimestamp  = GetTickCount();
        if (CBuildGhost* g = CBuildGhost::GetInstance())
            g->SetRotateKeyDiscovered();
        return true;
    }
    return false;
}
