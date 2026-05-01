#include "WeaponFiredExt.h"
#include "ChatHijackIds.h"
#include "PacketChatRouter.h"
#include "WeaponIdOverflow.h"
#include "tamem.h"
#include "tafunctions.h"
#include "hook/hook.h"

#include <cstring>

namespace {

// ---- thread-local context shared between WeaponFiredExt receive handler ----
// ---- and the ReceiveWeaponFired byte-read hook ----------------------------

// When non-zero, an overflow id is "in flight": the next ReceiveWeaponFired
// byte-read at packet+0x19 (engine path 0x49D27B) is replaced with this id
// via EDX-substitution. Cleared after each invocation. Only set when WE
// drive ReceiveWeaponFired ourselves from the WeaponFiredExt handler.
thread_local uint16_t t_pendingFullId = 0;

// Re-entrance guard for the HAPI_BroadcastMessage hook — when WE call the
// function from inside the hook (to broadcast the extended packet), the
// hook must not recursively interpret it.
thread_local bool t_inBroadcastHook = false;

// ---- hook objects ----------------------------------------------------------

InlineSingleHook* g_broadcastHook   = nullptr;
InlineSingleHook* g_receiveEdxHook  = nullptr;

// ---- helpers ---------------------------------------------------------------

bool IsWeaponFiredExtMessage(const WeaponFiredExtMessage& m)
{
    return m.chatByte == 0x05
        && m.nullText == 0x00
        && m.msgId    == ChatHijackId::WeaponFiredExt
        && m.size     == sizeof(WeaponFiredExtMessage);
}

// Look up the firing weapon from a 0x0D packet payload. shooterIdx is the
// UnitInGameIndex at +0x21; weaponSlot at +0x23 selects which of the unit's
// up to three UnitWeapons fired. Returns nullptr if any lookup fails.
WeaponStruct* WeaponFromPacket0D(const uint8_t* buf)
{
    const uint16_t shooterIdx = *(const uint16_t*)(buf + 0x21);
    const uint8_t  weaponSlot = (uint8_t)(buf[0x23] & 0x03);
    if (shooterIdx == 0) return nullptr;

    TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
    if (!taPtr || !taPtr->BeginUnitsArray_p) return nullptr;

    UnitStruct* shooter = (UnitStruct*)((char*)taPtr->BeginUnitsArray_p
                                        + (size_t)shooterIdx * sizeof(UnitStruct));
    if (!shooter || shooter->IsUnit == 0) return nullptr;

    // UnitStruct embeds three UnitWeapon-shaped sub-records starting at
    // offset 0x10 (Weapon1 ptr), stride 0x1C. Match Ghidra's layout used by
    // the engine's ReceiveWeaponFired (UnitWeapons + slot * sizeof(UnitWeapon)).
    constexpr size_t kUnitWeaponStride = 0x1C;
    constexpr size_t kFirstWeaponPtrOffset = 0x10;
    const size_t off = kFirstWeaponPtrOffset + (size_t)weaponSlot * kUnitWeaponStride;
    return *(WeaponStruct**)((char*)shooter + off);
}

// ---- send-side: HAPI_BroadcastMessage entry hook ---------------------------

// Patch site at 0x451DF0 (6 bytes — function prologue):
//   53           PUSH EBX
//   55           PUSH EBP
//   8B 6C 24 0C  MOV  EBP, dword ptr [ESP + 0x0C]
// Total 6 bytes. INLINE_5BYTESLAGGERJMP re-executes them after our router.
//
// At hook entry ESP points at the caller's frame:
//   pBuf->Esp + 0x00 = retaddr
//   pBuf->Esp + 0x04 = arg0 (fromPID)
//   pBuf->Esp + 0x08 = arg1 (buffer)
//   pBuf->Esp + 0x0C = arg2 (size)
//
// Behaviour:
//   * Re-entrance guard ON              -> pass through (we are emitting an ext packet)
//   * Not a WEAPON_FIRED_0D packet      -> pass through
//   * Firing weapon resolves to id<256  -> pass through (legacy 0x0D unchanged)
//   * Firing weapon resolves to id>=256 -> emit WeaponFiredExt instead, suppress original
constexpr DWORD kBroadcastHookAddr = 0x00451DF0u;
constexpr DWORD kBroadcastHookLen  = 6u;

// Naked epilogue used to short-circuit HAPI_BroadcastMessage when we have
// suppressed it. EAX = 0 (success), pops 12 bytes of __stdcall args.
__declspec(naked) static void __stdcall BroadcastSuppressEpilogue()
{
    __asm
    {
        xor eax, eax
        ret 0x0C
    }
}

int __stdcall BroadcastEntryHook(PInlineX86StackBuffer pBuf)
{
    if (t_inBroadcastHook)
        return 0;

    DWORD* args = (DWORD*)pBuf->Esp;
    const int       fromPID = (int)args[1];
    const uint8_t*  buffer  = (const uint8_t*)args[2];
    const int       size    = (int)args[3];

    if (size != 0x24 || !buffer || buffer[0] != 0x0D)
        return 0;

    WeaponStruct* w = WeaponFromPacket0D(buffer);
    if (!w)
        return 0;
    const int fullId = WeaponIdOverflow::GetId(w);
    if (fullId < 0 || fullId < WeaponIdOverflow::kBaseCapacity)
        return 0;  // base-array weapon — let legacy 0x0D broadcast as normal

    // Build extended packet and broadcast it. Re-entrance flag prevents the
    // recursive HAPI_BroadcastMessage call from being re-interpreted.
    WeaponFiredExtMessage ext;
    std::memset(&ext, 0, sizeof(ext));
    ext.chatByte     = 0x05;
    ext.nullText     = 0x00;
    ext.msgId        = ChatHijackId::WeaponFiredExt;
    ext.size         = sizeof(ext);
    ext.fullWeaponId = (uint16_t)fullId;
    std::memcpy(ext.packet0D, buffer, sizeof(ext.packet0D));

    t_inBroadcastHook = true;
    HAPI_BroadcastMessage(fromPID, (const char*)&ext, sizeof(ext));
    t_inBroadcastHook = false;

    // Suppress the original 0x0D broadcast — redirect to a stub that
    // returns 0 with __stdcall arg cleanup.
    pBuf->rtnAddr_Pvoid = (LPVOID)&BroadcastSuppressEpilogue;
    return X86STRACKBUFFERCHANGE;
}

// ---- receive-side: WeaponFiredExt handler ----------------------------------

// Engine entry — same address ReceiveWeaponFired is hooked at downstream.
typedef void (__stdcall *_ReceiveWeaponFired)(int pPlayerInfo, const void* pPacketData);
static const _ReceiveWeaponFired kReceiveWeaponFired = (_ReceiveWeaponFired)0x0049D270u;

void HandleWeaponFiredExt(unsigned /*fromDpid*/, const void* buf)
{
    const WeaponFiredExtMessage* m = (const WeaponFiredExtMessage*)buf;
    if (!m || !IsWeaponFiredExtMessage(*m))
        return;

    // Stage the full id so the byte-read hook in ReceiveWeaponFired (below)
    // synthesizes EDX to point at our overflow slot instead of trusting the
    // legacy byte at packet0D[0x19].
    t_pendingFullId = m->fullWeaponId;

    // The engine's first arg (pPlayerInfo) is unused for the dispatch we
    // exercise here — pass 0; pPacketData is the embedded 36-byte payload.
    kReceiveWeaponFired(0, m->packet0D);

    t_pendingFullId = 0;
}

// ---- receive-side: ReceiveWeaponFired EDX substitution ---------------------

// Patch site at 0x49D27E (5 bytes). At entry to ReceiveWeaponFired:
//   0x49D27B  MOV DL, byte ptr [EAX + 0x19]   ; weaponTypeIdx = packet[+0x19]
//   0x49D27E  PUSH ESI                          \  these 5 bytes are our patch:
//   0x49D27F  PUSH EDI                           > LAGGERJMP re-executes them
//   0x49D280  LEA  ECX, [EDX + EDX*2]           /  after our router runs
//   0x49D283  SHL  ECX, 3                         (next instr after patch)
// Subsequent code computes EBP = g_TAMainStruct + EDX*0x115 + 0x2CF3
// (the slot pointer) using EDX. By rewriting EDX before the LEA chain runs
// we redirect EBP to our overflow slot — same trick as the LoadWeaponTdf
// hook, just substituting EDX rather than EAX.
//
// We only intervene when a WeaponFiredExt message is actively being
// dispatched (t_pendingFullId != 0). Legacy 0x0D packets are unaffected.
constexpr DWORD kReceiveEdxHookAddr = 0x0049D27Eu;
constexpr DWORD kReceiveEdxHookLen  = 5u;

int __stdcall ReceiveEdxHookProc(PInlineX86StackBuffer pBuf)
{
    if (t_pendingFullId == 0)
        return 0;  // legacy path — let engine read byte at packet+0x19 unchanged
    const uint16_t id = t_pendingFullId;
    if (id < WeaponIdOverflow::kBaseCapacity)
    {
        // Caller staged a low id but went through the extended path anyway.
        // Just write it into EDX so the engine's lookup is consistent.
        pBuf->Edx = id;
        return 0;
    }
    if (!WeaponIdOverflow::GetDef(id))
        return 0;  // overflow slot not populated — fall through, will likely no-op

    // Mirror the synthetic-EAX trick from WeaponIdOverflow's LoadWeaponTdf
    // hook. We need ((WeaponsTypedefArray + EDX*0x115) - g_overflowSlot) == 0.
    WeaponStruct* def = WeaponIdOverflow::GetDef(id);
    TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
    if (!def || !taPtr)
        return 0;
    const ptrdiff_t deltaBytes =
        (const char*)def - (const char*)&taPtr->Weapons[0];
    pBuf->Edx = (DWORD)(deltaBytes / (ptrdiff_t)sizeof(WeaponStruct));
    return 0;
}

} // namespace

namespace WeaponFiredExt {

void Install()
{
    if (g_broadcastHook)
        return;
    g_broadcastHook = new InlineSingleHook(
        kBroadcastHookAddr, kBroadcastHookLen,
        INLINE_5BYTESLAGGERJMP,
        (InlineX86HookRouter)BroadcastEntryHook);
    g_receiveEdxHook = new InlineSingleHook(
        kReceiveEdxHookAddr, kReceiveEdxHookLen,
        INLINE_5BYTESLAGGERJMP,
        (InlineX86HookRouter)ReceiveEdxHookProc);
    PacketChatRouter::GetInstance()->RegisterHandler(
        ChatHijackId::WeaponFiredExt, HandleWeaponFiredExt, /*fireInDemo=*/true);
}

void Shutdown()
{
    delete g_broadcastHook;
    g_broadcastHook = nullptr;
    delete g_receiveEdxHook;
    g_receiveEdxHook = nullptr;
}

} // namespace WeaponFiredExt
