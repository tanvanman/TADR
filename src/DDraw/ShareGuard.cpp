#include "ShareGuard.h"

#include "config.h"

#if SHARE_ABUSE_GUARD

#include "GameTickHook.h"
#include "tamem.h"
#include "tafunctions.h"
#include "hook/hook.h"
#include "iddrawsurface.h"

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <deque>
#include <utility>
#include <vector>

namespace {

// ---- tunables --------------------------------------------------------------
// TA runs at 30 ticks/second.
constexpr int kTicksPerSecond   = 30;
constexpr int kWindowTicks      = 30 * kTicksPerSecond;  // 30s
constexpr int kStructuresPerWin = 10;                    // free allowance per window

// Whole-block semantics, NOT a drip feed. A share of N structures either goes
// through immediately (if the last 30s of shares plus N stays within the
// allowance) or the ENTIRE block is held and released together 30s later,
// however big it is. Metering release rate instead would make a legitimate
// 1000-structure hand-over take the better part of an hour.
//
// This still denies the abuse: everything past the first 10 structures costs a
// flat 30 seconds, so a base cannot be dumped in the moment before dying, and
// batching gains nothing — a second block inside the window is delayed too.

// ---- addresses -------------------------------------------------------------

constexpr unsigned TA_MAIN_PTR_ADDR = 0x00511de8;

// _ShowText — the single choke point for OUTGOING chat. 10-byte prologue of
// whole instructions:
//   8B 44 24 10          MOV EAX,[ESP+0x10]
//   81 EC C8 00 00 00    SUB ESP,0xC8
// NOTE it is __stdcall with FOUR args (RET 0x10); Ghidra infers three.
constexpr DWORD kShowTextAddr = 0x00463e50u;
constexpr DWORD kShowTextLen  = 10u;

// GiveSelectUnits — the only player-initiated share path (reached solely from
// ShareDialog_proc). 7-byte prologue:
//   83 EC 10       SUB ESP,0x10
//   8A 44 24 14    MOV AL,[ESP+0x14]
constexpr DWORD kGiveSelectAddr = 0x004933e0u;
constexpr DWORD kGiveSelectLen  = 7u;

typedef void(__stdcall* _UNITS_GiveUnit)(void* unit, void* targetPlayer, const void* pkt);
static const _UNITS_GiveUnit UNITS_GiveUnit_fn = (_UNITS_GiveUnit)0x00488570u;

// Returns a bitmask indexed by UnitINFOID. This is TA's OWN commander test —
// race-independent, unlike comparing against the owner's RaceSideData
// commanderUnitName (which misidentifies a commander held by the other side).
typedef unsigned*(__stdcall* _FindSpot_CategorysAry)(const char* name);
static const _FindSpot_CategorysAry FindSpot_CategorysAry =
    (_FindSpot_CategorysAry)0x00488c50u;

// ---- UnitStruct.UnitSelected (the engine's UnitStateMask) bits -------------
// tamem.h keeps the legacy name "UnitSelected" for 0x110 because most callers
// only touch its selection bits; these are the two we need.
constexpr unsigned UNIT_STATE_ALIVE = 0x10000000u;
// "suppressedSpeech" in Ghidra's enum, but it is really the "marked dead,
// awaiting dispatch" flag set by UnitTakeDamage_packet.
constexpr unsigned UNIT_STATE_PENDING_DEATH = 0x4000u;

constexpr unsigned kMaxPlayers = 10;

// Layout guards. This module walks unit arrays by pointer arithmetic, indexes
// TAdynmemStruct::Players, and reads fields whose offsets were verified against
// Ghidra; if tamem.h ever drifts from the engine these become compile errors
// rather than silent memory corruption.
static_assert(sizeof(UnitStruct)   == 0x118, "UnitStruct stride must match the engine");
static_assert(sizeof(PlayerStruct) == 0x14B, "PlayerStruct stride must match the engine");
static_assert(offsetof(UnitStruct, UnitType)        == 0x92,  "UnitStruct.UnitType");
static_assert(offsetof(UnitStruct, Owner_PlayerPtr0)== 0x96,  "UnitStruct.Owner_PlayerPtr0");
static_assert(offsetof(UnitStruct, UnitID)          == 0xA6,  "UnitStruct.UnitID (unit TYPE index)");
static_assert(offsetof(UnitStruct, UnitInGameIndex) == 0xA8,  "UnitStruct.UnitInGameIndex");
static_assert(offsetof(UnitStruct, cOwnerID)        == 0xFF,  "UnitStruct.cOwnerID");
static_assert(offsetof(UnitStruct, Health)          == 0x108, "UnitStruct.Health");
static_assert(offsetof(UnitStruct, UnitSelected)    == 0x110, "UnitStruct.UnitSelected (UnitStateMask)");
static_assert(offsetof(UnitDefStruct, bmcode)       == 0x22F, "UnitDefStruct.bmcode");
static_assert(offsetof(TAdynmemStruct, ActiveCommanderDeath) == 0x37EF6,
              "TAdynmemStruct.ActiveCommanderDeath");
static_assert(offsetof(TAdynmemStruct, Players)     == 0x1B63, "TAdynmemStruct.Players");

// ---- state -----------------------------------------------------------------

struct QueuedShare
{
    unsigned short unitIndex;    // UnitStruct.UnitInGameIndex
    unsigned short unitTypeId;   // unit TYPE, revalidated (indices are recycled)
    unsigned char  ownerSlot;    // giver, revalidated
    unsigned char  targetSlot;   // recipient
};

// One GiveSelectUnits invocation's worth of structures, held as a unit.
struct ShareBlock
{
    int                      dueTick;
    std::vector<QueuedShare> units;
};

// Structures actually released, for the rolling 30s window.
struct ReleaseRecord { int tick; int count; };

std::deque<ReleaseRecord> g_windowLog;    // pruned to the last kWindowTicks
std::deque<ShareBlock>    g_delayedBlocks;
std::vector<QueuedShare>  g_pendingBlock; // accumulating during a GiveSelectUnits call
bool g_inPlayerShare = false;
bool g_releasing     = false;             // guards our own re-issued gives
int  g_lastTick      = 0;

DWORD g_giveSelectRealReturn = 0;

InlineSingleHook* g_showTextHook   = nullptr;
InlineSingleHook* g_giveSelectHook = nullptr;

// ---- helpers ---------------------------------------------------------------

TAdynmemStruct* GetTA()
{
    return *reinterpret_cast<TAdynmemStruct**>(TA_MAIN_PTR_ADDR);
}

inline UnitStruct* U(void* p) { return reinterpret_cast<UnitStruct*>(p); }

inline unsigned short UnitIndex(void* u)  { return (unsigned short)U(u)->UnitInGameIndex; }
inline unsigned short UnitTypeId(void* u) { return (unsigned short)U(u)->UnitID; }
inline unsigned char  UnitOwner(void* u)  { return U(u)->cOwnerID; }
inline short          UnitHealth(void* u) { return U(u)->Health; }
inline bool           UnitAlive(void* u)  { return (U(u)->UnitSelected & UNIT_STATE_ALIVE) != 0; }
inline bool           UnitPendingDeath(void* u) { return (U(u)->UnitSelected & UNIT_STATE_PENDING_DEATH) != 0; }

bool IsStructure(void* unit)
{
    UnitDefStruct* def = U(unit)->UnitType;
    return def && def->bmcode == 0;   // 0=building, 1=mobile, 2=feature
}

bool IsCommanderType(unsigned short unitTypeId)
{
    unsigned* mask = FindSpot_CategorysAry("Commander");
    if (!mask) return false;
    return (mask[unitTypeId >> 5] & (1u << (unitTypeId & 0x1f))) != 0;
}

int CurrentTick()
{
    TAdynmemStruct* ta = GetTA();
    return ta ? ta->GameTime : 0;
}

// A commander sitting at Health <= 0 with the alive flag STILL SET.
//
// That combination is only reachable for a player whose client is no longer
// dispatching deaths — i.e. one who has dropped. UnitTakeDamage_packet clamps
// a remote-owned unit's Health to 0 rather than killing it, because only the
// owner may declare a unit dead; if the owner is gone, nobody ever does. For a
// live player the same state exists only for the sub-second before their death
// packet arrives.
bool HasDestroyedCommander(PlayerStruct* ps)
{
    if (!ps || !ps->PlayerActive) return false;

    UnitStruct* unit = ps->Units;
    UnitStruct* end  = ps->UnitsAry_End;
    if (!unit || !end) return false;

    for (; unit <= end; ++unit)
    {
        const unsigned short typeId = UnitTypeId(unit);
        if (typeId == 0) continue;
        if (!UnitAlive(unit)) continue;
        if (!IsCommanderType(typeId)) continue;
        if (UnitHealth(unit) <= 0) return true;
    }
    return false;
}

void LocalMessage(const char* msg)
{
    NewChatText(const_cast<char*>(msg), 1, 0, '\n');
}

// ---- rate limiter ----------------------------------------------------------

void ResetState()
{
    g_windowLog.clear();
    g_delayedBlocks.clear();
    g_pendingBlock.clear();
    g_inPlayerShare = false;
    g_releasing     = false;
}

// Drop window records older than kWindowTicks, and reset everything if the
// game clock went backwards (new game / replay rewind).
void PruneWindow()
{
    const int now = CurrentTick();
    if (now < g_lastTick) { ResetState(); g_lastTick = now; return; }
    g_lastTick = now;
    while (!g_windowLog.empty() && now - g_windowLog.front().tick >= kWindowTicks)
        g_windowLog.pop_front();
}

int StructuresSharedInWindow()
{
    int n = 0;
    for (const ReleaseRecord& r : g_windowLog) n += r.count;
    return n;
}

// Re-resolve a queued entry. Unit indices are recycled, so verify the slot
// still holds the same unit type, still belongs to the giver, and is alive.
UnitStruct* ResolveQueued(const QueuedShare& q)
{
    TAdynmemStruct* ta = GetTA();
    if (!ta || !ta->BeginUnitsArray_p) return nullptr;

    UnitStruct* unit = ta->BeginUnitsArray_p + q.unitIndex;
    if (UnitTypeId(unit) != q.unitTypeId) return nullptr;
    if (UnitOwner(unit) != q.ownerSlot)   return nullptr;
    if (!UnitAlive(unit))                 return nullptr;
    if (UnitPendingDeath(unit))           return nullptr;
    return unit;
}

// Hand over a whole block at once. Entries that no longer validate (unit died,
// index recycled, recipient gone) are simply dropped — which is what makes a
// giver dying mid-delay a non-issue.
void ReleaseBlock(const std::vector<QueuedShare>& units)
{
    TAdynmemStruct* ta = GetTA();
    if (!ta) return;

    int released = 0;
    g_releasing = true;                      // stop ShouldSuppressGive re-catching these
    for (const QueuedShare& q : units)
    {
        UnitStruct* unit = ResolveQueued(q);
        if (!unit) continue;
        PlayerStruct* target = &ta->Players[q.targetSlot];
        if (!target->PlayerActive) continue;
        UNITS_GiveUnit_fn(unit, target, nullptr);
        ++released;
    }
    g_releasing = false;

    if (released > 0)
    {
        ReleaseRecord r; r.tick = CurrentTick(); r.count = released;
        g_windowLog.push_back(r);
    }
}

// Called from the GiveSelectUnits return thunk, once the whole selection has
// been seen and we know how big the block is.
void FinishPlayerShare()
{
    g_inPlayerShare = false;
    if (g_pendingBlock.empty()) return;

    std::vector<QueuedShare> block;
    block.swap(g_pendingBlock);

    PruneWindow();
    const int n = (int)block.size();

    if (StructuresSharedInWindow() + n <= kStructuresPerWin)
    {
        ReleaseBlock(block);                 // within allowance — go now
        return;
    }

    ShareBlock sb;
    sb.dueTick = CurrentTick() + kWindowTicks;
    sb.units.swap(block);
    g_delayedBlocks.push_back(std::move(sb));

    char msg[160];
    _snprintf(msg, sizeof(msg) - 1,
              "Sharing %d structures - transfer will complete in %d seconds.",
              n, kWindowTicks / kTicksPerSecond);
    msg[sizeof(msg) - 1] = '\0';
    LocalMessage(msg);
}

void OnGameTick(int)
{
    PruneWindow();
    const int now = CurrentTick();
    while (!g_delayedBlocks.empty() && g_delayedBlocks.front().dueTick <= now)
    {
        ShareBlock sb = std::move(g_delayedBlocks.front());
        g_delayedBlocks.pop_front();
        ReleaseBlock(sb.units);
    }
}

// ---- hooks -----------------------------------------------------------------

// Returns from UNITS_GiveUnit (__stdcall void, RET 0xC) without running it.
__declspec(naked) void GiveSuppressStub()
{
    __asm { ret 0xC }
}

// Returns from _ShowText (__stdcall, RET 0x10) without running it, so the
// chat is never formatted, broadcast, or displayed.
__declspec(naked) void ShowTextSuppressStub()
{
    __asm { ret 0x10 }
}

// Case-insensitive match for ".take" / ".takecmd", tolerating surrounding
// whitespace. Anything with arguments is not a take command.
bool IsTakeCommand(const char* text)
{
    if (!text) return false;
    while (*text == ' ' || *text == '\t') ++text;
    if (*text != '.') return false;
    ++text;

    const char* kCmds[] = { "takecmd", "take" };   // longest first
    for (const char* cmd : kCmds)
    {
        const size_t n = std::strlen(cmd);
        if (_strnicmp(text, cmd, n) != 0) continue;
        const char* rest = text + n;
        while (*rest == ' ' || *rest == '\t' || *rest == '\r' || *rest == '\n') ++rest;
        if (*rest == '\0') return true;
    }
    return false;
}

// _ShowText entry. At the hook site the prologue has not run, so
// [Esp+4]=player, [Esp+8]=text.
int __stdcall ShowText_Entry_Proc(PInlineX86StackBuffer pBuf)
{
    DWORD* args = reinterpret_cast<DWORD*>(pBuf->Esp);
    const char* text = reinterpret_cast<const char*>(args[2]);
    if (!IsTakeCommand(text)) return 0;

    if (!ShareGuard::IsComEndsGame()) return 0;   // rule is inert without com-ends

    TAdynmemStruct* ta = GetTA();
    if (!ta) return 0;

    const unsigned char localSlot = (unsigned char)ta->LocalHumanPlayer_PlayerID;
    for (unsigned p = 0; p < kMaxPlayers; ++p)
    {
        if (p == localSlot) continue;
        PlayerStruct* ps = &ta->Players[p];
        if (!HasDestroyedCommander(ps)) continue;

        char msg[160];
        _snprintf(msg, sizeof(msg) - 1,
                  "Cannot take %s: their commander has been destroyed.",
                  ps->Name[0] ? ps->Name : "that player");
        msg[sizeof(msg) - 1] = '\0';
        LocalMessage(msg);
        IDDrawSurface::OutptFmtTxt("[ShareGuard] blocked take of slot %u (commander at 0 HP)", p);

        pBuf->rtnAddr_Pvoid = reinterpret_cast<LPVOID>(&ShowTextSuppressStub);
        return X86STRACKBUFFERCHANGE;
    }
    return 0;
}

// GiveSelectUnits return thunk — the whole selection has now been offered, so
// this is where the block is sized and either released or held.
extern "C" void __cdecl GiveSelectEnvelopeCleanup()
{
    FinishPlayerShare();
}

__declspec(naked) void GiveSelectReturnThunk()
{
    __asm
    {
        pushad
        pushfd
        call GiveSelectEnvelopeCleanup
        popfd
        popad
        jmp dword ptr [g_giveSelectRealReturn]
    }
}

// GiveSelectUnits entry. Marks the gives that follow as a player-initiated
// share so ShouldSuppressGive can distinguish them from capture (pkt==null,
// no flag) and from network/'.take' gives (pkt!=null).
//
// Every structure in the selection is collected and suppressed here; the
// decision is deferred to the return thunk, because only then do we know how
// big the block is. Immediate blocks are simply re-issued from there on the
// same tick, so the player sees no difference.
int __stdcall GiveSelect_Entry_Proc(PInlineX86StackBuffer pBuf)
{
    DWORD* stackTop = reinterpret_cast<DWORD*>(pBuf->Esp);
    g_inPlayerShare = true;
    g_pendingBlock.clear();
    PruneWindow();
    g_giveSelectRealReturn = stackTop[0];
    stackTop[0] = reinterpret_cast<DWORD>(&GiveSelectReturnThunk);
    return 0;
}

} // namespace

namespace ShareGuard {

bool IsComEndsGame()
{
    TAdynmemStruct* ta = GetTA();
    if (!ta) return false;
    return ta->ActiveCommanderDeath != 0;
}

void* GetGiveSuppressStub()
{
    return reinterpret_cast<void*>(&GiveSuppressStub);
}

bool ShouldSuppressGive(void* srcUnit, void* targetPlayer, const void* givePkt)
{
    if (!srcUnit || !targetPlayer) return false;
    if (givePkt) return false;          // network / '.take' give — never metered
    if (g_releasing) return false;      // our own re-issue of a block
    if (!g_inPlayerShare) return false; // capture and other engine paths
    if (!IsStructure(srcUnit)) return false;   // mobile units share instantly

    TAdynmemStruct* ta = GetTA();
    if (!ta) return false;

    PlayerStruct* target = reinterpret_cast<PlayerStruct*>(targetPlayer);
    const int targetSlot = (int)(target - ta->Players);
    if (targetSlot < 0 || targetSlot >= (int)kMaxPlayers) return false;

    // Collect, don't decide. The block is sized and ruled on in the
    // GiveSelectUnits return thunk (FinishPlayerShare), which re-issues it
    // immediately if it fits inside the allowance.
    QueuedShare q;
    q.unitIndex  = UnitIndex(srcUnit);
    q.unitTypeId = UnitTypeId(srcUnit);
    q.ownerSlot  = UnitOwner(srcUnit);
    q.targetSlot = (unsigned char)targetSlot;
    g_pendingBlock.push_back(q);
    return true;
}

void Install()
{
    if (g_showTextHook) return;

    g_showTextHook = new InlineSingleHook(
        kShowTextAddr, kShowTextLen,
        INLINE_5BYTESLAGGERJMP,
        (InlineX86HookRouter)ShowText_Entry_Proc);

    g_giveSelectHook = new InlineSingleHook(
        kGiveSelectAddr, kGiveSelectLen,
        INLINE_5BYTESLAGGERJMP,
        (InlineX86HookRouter)GiveSelect_Entry_Proc);

    GameTickHook::GetInstance()->addCallback(OnGameTick);
}

void Shutdown()
{
    delete g_showTextHook;   g_showTextHook   = nullptr;
    delete g_giveSelectHook; g_giveSelectHook = nullptr;
    ResetState();
}

} // namespace ShareGuard

#else  // !SHARE_ABUSE_GUARD

namespace ShareGuard {
void  Install() {}
void  Shutdown() {}
bool  IsComEndsGame() { return false; }
bool  ShouldSuppressGive(void*, void*, const void*) { return false; }
void* GetGiveSuppressStub() { return nullptr; }
}

#endif // SHARE_ABUSE_GUARD
