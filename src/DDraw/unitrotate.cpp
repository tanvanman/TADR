#include "unitrotate.h"

#include "buildghost.h"
#include "dialog.h"
#include "iddrawsurface.h"
#include "hook/hook.h"
#include "tafunctions.h"
#include "tamem.h"
#include "UnitDefExtensions.h"

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
static int __stdcall DrawBuildSpotQueue_Entry_Proc(PInlineX86StackBuffer X86StrackBuffer)
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

// Just before UNITS_CreateUnit runs in Order_MobileBuild / Order_VTOL_MobileBuild.
// ESI = OrderStruct* at this site (confirmed by disassembly). Read the order's
// stored rotation from the side-table (defaulting to 0 for untagged orders).
static int __stdcall MobileBuild_PreCreate_Proc(PInlineX86StackBuffer X86StrackBuffer)
{
    CUnitRotate* self = CUnitRotate::GetInstance();
    if (!self) return 0;

    BYTE* order = reinterpret_cast<BYTE*>(X86StrackBuffer->Esi);
    if (!order) return 0;

    int rotation = self->TakeOrderRotation(order);
    if (rotation == 0)
    {
        self->m_pendingHeading = -1;
        return 0;  // no rotation for this order
    }

    unsigned unitTypeIdx = *reinterpret_cast<unsigned*>(order + OFF_ORDER_build_unitType);
    if (unitTypeIdx == 0) { self->m_pendingHeading = -1; return 0; }

    BYTE* ui = GetUnitInfoRaw(unitTypeIdx);
    if (!ui) { self->m_pendingHeading = -1; return 0; }
    if (*(ui + OFF_UNITINFO_bmcode) != 0) { self->m_pendingHeading = -1; return 0; }

    // Defensive: ignore stale entries from a freed-and-reused order address.
    // If the unit type's "Rotations" FBI key doesn't allow this rotation, the
    // tag can't have been put there by us — drop it so AI builds at a recycled
    // OrderStruct slot don't inherit a previous player rotation.
    if (!self->IsRotationAllowed(unitTypeIdx, rotation))
    {
        self->m_pendingHeading = -1;
        return 0;
    }

    // Save rotation for the footprint swap + PostCreate heading write.
    // We reuse the existing ApplyRotationTo/ClearRotation machinery but drive
    // it from the ORDER's stored rotation rather than the current global.
    int savedRotation = self->GetRotation();
    self->SetRotation(rotation);   // briefly make ApplyRotationTo's needSwap use this order's rotation
    self->ApplyRotationTo(unitTypeIdx);
    self->SetRotation(savedRotation);

    // Swap UNITINFO.p_YardMap so the initial stamp in UNITS_CreateUnit's
    // internal UNITS_RebuildFootPrint → Unit_UpdateYardmap uses the rotated
    // yardmap. Restored in PostCreate.
    self->ApplyYardmapRotationTo(unitTypeIdx, rotation);

    self->m_pendingHeading = rotation;
    return 0;
}

// Just after UNITS_CreateUnit returned. The preceding instruction was `PUSH EAX`
// (the new unit pointer), so [ESP] at router entry holds newUnit. Apply the
// rotation stashed by PreCreate + remove the order's map entry.
static int __stdcall MobileBuild_PostCreate_Proc(PInlineX86StackBuffer X86StrackBuffer)
{
    CUnitRotate* self = CUnitRotate::GetInstance();
    if (!self || self->m_pendingHeading < 0) return 0;

    int rotation = self->m_pendingHeading;
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
    (void)rotation;  // no longer used here; kept above for consistency / log lines

    // Intentionally NOT clearing m_orderRotation here. The order is still in
    // the builder's queue throughout the actual construction, and engine
    // functions like DrawBuildSpotQueue keep iterating it to draw the build
    // rectangle — those callers consult TakeOrderRotation, so the entry must
    // outlive PostCreate. Stale entries (from completed/cancelled orders
    // whose addresses get reused) are handled by:
    //   (a) TagOrderRotation overwriting on any new player-issued build, and
    //   (b) DrawBuildSpotQueue / MobileBuild_PreCreate gating their effect
    //       behind IsRotationAllowed(unitType, rotation).

    // Restore UNITINFO.p_YardMap now that UNITS_CreateUnit has finished
    // stamping the grid. Subsequent yardmap reads go through the runtime
    // hooks (Unit_UpdateYardmap etc.) which do their own scoped swap.
    self->ClearYardmapRotation();

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
      m_rotationsKeyIdx(0)
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
    // If we already have the right footprint-swap state for this idx, nothing to do.
    bool needSwap = (m_rotation & 1) == 1;  // 90° or 270°
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
    // "MORE" → button12.wav in ALLSOUND.TDF — soft button click.
    PlaySound_Effect((char*)"MORE", 0);
    if (CBuildGhost* g = CBuildGhost::GetInstance())
        g->SetRotateKeyDiscovered();
    return true;
}
