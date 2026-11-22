#include "TakeClaim.h"

#include "config.h"

#if TAKE_CLAIM_ENABLE

#include "ChatHijackIds.h"
#include "GameTickHook.h"
#include "PacketChatRouter.h"
#include "ShareGuard.h"
#include "VoteReject.h"
#include "hook/hook.h"
#include "iddrawsurface.h"
#include "tafunctions.h"
#include "tamem.h"

#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace {

// ---- tunables --------------------------------------------------------------

// TWO CLOCKS, and mixing them up cost a live game once already:
//   SimTick()  = GameTime, +1 per executed sim tick in the lockstep loop
//                (0x00495490). Shared by all clients: use for anything that
//                goes on the wire or is compared across machines.
//   WallTick() = GameRunSec(), = GetTickCount()*TicksPerSecond/1000, i.e. THIS
//                machine's uptime. Local deltas only; needed solely because
//                LastMsgTimeStamp is written in these units.
// Ordering claims by WallTick made every received claim look later than the
// local one, so every client elected itself and two allies took the same base.
constexpr int kTicksPerSecond = 30;
constexpr int kSettleTicks    = 45;                    // claim collection, SIM ticks
constexpr int kGraceTicks     = 3 * kTicksPerSecond;   // winner must execute by
constexpr int kSilenceTicks   = 30 * kTicksPerSecond;  // WALL ticks (LastMsgTimeStamp)
constexpr int kElectionKeepTicks = 60 * kTicksPerSecond;

// A premature Complete is worse than a late one: Disconnect_Player destroys
// whatever the target still owns, i.e. the units still in flight.
//
// No fixed settle works. Measured over 40 real prod takes of >=100 units
// (median 455, max 955) on real connections:
//   transfer DURATION : median 1433 ms  p90 7018 ms  p99/max 20820 ms
//   longest GAP in it : median  374 ms  p90 2458 ms  p99/max  2600 ms
// Duration spans two orders of magnitude; the gap does not. So detect
// quiescence and let the duration be whatever it is.
constexpr int kQuietTicks       = 4 * kTicksPerSecond;    // > p99 gap (2.6 s)
constexpr int kSamplePeriod     = 6;                      // 5 Hz is plenty
constexpr int kCompleteBackstop = 60 * kTicksPerSecond;   // ~3x worst duration

// Delay between deciding to reject and doing it, so the reject always happens
// on a game tick rather than inside whatever called us. See OnClaimPacket.
constexpr int kFinishDelayTicks = 6;

// A peer can legitimately be a few sim ticks ahead of or behind us - each
// client runs DeltaTime ticks per frame. Accept a claim tick within this band
// of our own; anything outside it is a different game or a liar, and gets
// clamped so it cannot win by claiming the distant past.
constexpr int kTickSkewTolerance = 2 * kSettleTicks;

constexpr unsigned TA_MAIN_PTR_ADDR = 0x00511de8;
constexpr unsigned kMaxPlayers      = 10;

// _ShowText — the single choke point for OUTGOING chat; every route to '.take'
// converges here. dplayx sits below the HAPI layer and only sees the command
// once BroadcastText has sent it, so suppressing this call stops its OnTake
// everywhere, including on the issuer's own client.
//
// 10-byte prologue: MOV EAX,[ESP+0x10] / SUB ESP,0xC8.
// __stdcall with FOUR args (RET 0x10); Ghidra infers three.
constexpr DWORD kShowTextAddr = 0x00463e50u;
constexpr DWORD kShowTextLen  = 10u;

// Wall clock (see the note above): GetTickCount() * TicksPerSecond / 1000.
typedef int(__cdecl* _GameRunSec)(void);
static const _GameRunSec GameRunSec_fn = (_GameRunSec)0x004b6340u;

// ---- wire format -----------------------------------------------------------

enum class TakeClaimCommand : char {
    Claim    = 1,   // I would like to take targetDpid
    Withdraw = 2,   // ...never mind (target came back, or I lost eligibility)
    Complete = 3,   // I have finished taking targetDpid: everyone reject them now
};

#pragma pack(1)
struct TakeClaimMessage {
    char     chatByte;     // 0x05
    char     nullText;     // 0x00
    char     msgId;        // ChatHijackId::TakeClaim
    short    size;         // sizeof(TakeClaimMessage) = 65
    TakeClaimCommand command;
    unsigned targetDpid;
    int      claimTick;    // GameRunSec() when the claim was made
    char     includeCom;   // 1 if '.takecmd'
    char     pad[50];
};
#pragma pack()

static_assert(sizeof(TakeClaimMessage) == 65,
              "TakeClaimMessage must be exactly 65 bytes (matches CHAT_05 size)");
// Byte 64 must stay zero or gpgnet4ta's TPacket sizer takes its "older recorder
// emitted long chat" fallback and over-reads into the next subpacket.
static_assert(offsetof(TakeClaimMessage, pad) + sizeof(TakeClaimMessage::pad) == 65,
              "pad must occupy through byte 64; byte 64 must remain zero-init");

// ---- state -----------------------------------------------------------------

struct Claim {
    unsigned dpid;
    int      tick;
    bool     includeCom;
};

struct Election {
    std::vector<Claim> claims;
    int      closeTick   = 0;
    int      graceTick   = 0;
    bool     resolved    = false;
    bool     executed    = false;  // somebody's '.take <target>' has been seen
    int      retireTick  = 0;
    bool     awaitDrain  = false;  // we are the taker; waiting for the walk to finish
    int      finishAt    = 0;      // reject locally on this tick, NOT the Complete tick
    int      drainBy     = 0;       // backstop only; quiescence is the real signal
    bool     graceLogged = false;   // "waiting on the winner" said once, not per grace period
    int      liveCount   = -1;      // target's units still above 0 HP, last sample
    int      lastChange  = 0;
    int      nextSample  = 0;
    std::set<unsigned> excluded;   // won, then went silent
};

std::map<unsigned, Election> g_elections;      // keyed by target dpid
std::set<unsigned>           g_giveGrants;     // dpids that typed '.give <me>'
bool                         g_reemitting = false;
InlineSingleHook*            g_showTextHook = nullptr;

// ---- small helpers ---------------------------------------------------------

TAdynmemStruct* GetTA()
{
    TAdynmemStruct** pp = reinterpret_cast<TAdynmemStruct**>(TA_MAIN_PTR_ADDR);
    return pp ? *pp : nullptr;
}

// Lockstep simulation tick. THE clock for anything cross-client.
int SimTick()
{
    TAdynmemStruct* ta = GetTA();
    return ta ? ta->GameTime : 0;
}

// Local wall clock. ONLY for deltas against LastMsgTimeStamp, which is written
// in these units. Never put this on the wire.
int WallTick()
{
    return GameRunSec_fn ? GameRunSec_fn() : 0;
}

PlayerStruct* LocalPlayer()
{
    TAdynmemStruct* ta = GetTA();
    if (!ta) return nullptr;
    return &ta->Players[ta->LocalHumanPlayer_PlayerID];
}

bool IsWatcher(PlayerStruct* p)
{
    return !p || !p->PlayerInfo || (p->PlayerInfo->PropertyMask & WATCH) != 0;
}

bool IsPlayable(PlayerStruct* p)
{
    return p && p->PlayerActive && !IsWatcher(p);
}

void LocalMessage(const char* msg)
{
    // 4th arg is playerIndex, and 10 ('\n') is TA's own sentinel for a system
    // message: NewChatText @0x463ca0 skips the arrival sound on exactly that
    // value, and TA's own death announcement passes PUSH 0xa. Not a bug.
    NewChatText(const_cast<char*>(msg), 1, 0, '\n');
}

void LocalMessagef(const char* fmt, ...)
{
    char buf[192];
    va_list ap;
    va_start(ap, fmt);
    _vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
    va_end(ap);
    buf[sizeof(buf) - 1] = '\0';
    LocalMessage(buf);
}

// Case-insensitive, whole name or unique prefix. TA names can contain spaces,
// so the caller hands us everything after the command word.
PlayerStruct* ResolvePlayerByName(const char* name, bool* ambiguous)
{
    *ambiguous = false;
    TAdynmemStruct* ta = GetTA();
    if (!ta || !name || !*name) return nullptr;

    PlayerStruct* exact  = nullptr;
    PlayerStruct* prefix = nullptr;
    int prefixCount = 0;

    const size_t n = std::strlen(name);
    for (unsigned i = 0; i < kMaxPlayers; ++i)
    {
        PlayerStruct* p = &ta->Players[i];
        if (!p->PlayerActive || !p->Name[0]) continue;
        if (_stricmp(p->Name, name) == 0) { exact = p; break; }
        if (_strnicmp(p->Name, name, n) == 0) { prefix = p; ++prefixCount; }
    }
    if (exact) return exact;
    if (prefixCount == 1) return prefix;
    if (prefixCount > 1) *ambiguous = true;
    return nullptr;
}

// ---- eligibility -----------------------------------------------------------

// AllyTeam is 5+ when no battleroom team was selected.
bool TeamsInUse(TAdynmemStruct* ta)
{
    for (unsigned i = 0; i < kMaxPlayers; ++i)
        if (IsPlayable(&ta->Players[i]) && ta->Players[i].AllyTeam < 5)
            return true;
    return false;
}

// Teams first, deliberately: AllyTeam is a per-player scalar and survives the
// Players[] compaction that happens when somebody leaves, whereas AllyFlagAry's
// COLUMNS are not remapped by it and grow phantom one-way edges exactly when we
// need them. The matrix is still the right answer when no teams were selected
// (in-game '+autoteam' never sets AllyTeam); there we require it to be MUTUAL.
bool AlliedForTake(TAdynmemStruct* ta, PlayerStruct* me, PlayerStruct* other)
{
    if (!me || !other) return false;
    if (g_giveGrants.count((unsigned)other->DirectPlayID)) return true;   // explicit .give

    if (TeamsInUse(ta))
        return me->AllyTeam < 5 && me->AllyTeam == other->AllyTeam;

    const int mine  = me->PlayerAryIndex;
    const int theirs = other->PlayerAryIndex;
    if (mine < 0 || mine >= (int)kMaxPlayers || theirs < 0 || theirs >= (int)kMaxPlayers)
        return false;
    return me->AllyFlagAry[theirs] != 0 && other->AllyFlagAry[mine] != 0;
}

// The precondition that matters is not "silent for a while" but "TA is about to
// destroy these units" — a pending or completed timeout reject. Silence is the
// fallback for paths that never raise a vote.
bool IsPendingRejection(PlayerStruct* target)
{
    VoteReject* vr = VoteReject::GetInstance();
    if (vr)
    {
        if (vr->IsTakeWindowOpen((unsigned)target->DirectPlayID))
            return true;
        std::vector<VoteReject::VoteDisplayInfo> votes;
        vr->GetActiveVotes(votes);
        for (const auto& v : votes)
            if (v.targetDpid == (unsigned)target->DirectPlayID && v.rejectMask == 6)
                return true;
    }
    // WallTick, not SimTick: LastMsgTimeStamp is written in GameRunSec units.
    // Same shape as VoteReject::CanCastVote / TA's CheckForDroppedPlayers,
    // including the max() against GameTimeSec so a timestamp left over from
    // before the game started cannot read as "silent for hours".
    int ts = target->LastMsgTimeStamp;
    const int gameTimeSec = *(int*)0x00512c7c;
    if (ts < gameTimeSec) ts = gameTimeSec;
    return (WallTick() - ts) > kSilenceTicks;
}

// Full verdict for one candidate target. reason[] is filled on refusal.
bool MayTake(PlayerStruct* target, char* reason, size_t reasonSize)
{
    TAdynmemStruct* ta = GetTA();
    PlayerStruct*   me = LocalPlayer();
    if (reason && reasonSize) reason[reasonSize - 1] = 0;   // _snprintf may not
    if (!ta || !me || !target) { _snprintf(reason, reasonSize - 1, "no game state"); return false; }

    const char* who = target->Name[0] ? target->Name : "that player";

    if (target == me)
    { _snprintf(reason, reasonSize - 1, "Cannot take yourself."); return false; }

    if (!IsPlayable(target))
    { _snprintf(reason, reasonSize - 1, "Cannot take %s: not an active player.", who); return false; }

    if (!AlliedForTake(ta, me, target))
    { _snprintf(reason, reasonSize - 1, "Cannot take %s: not allied to you.", who); return false; }

    if (!IsPendingRejection(target))
    {
        int quietFrom = target->LastMsgTimeStamp;
        const int gameTimeSec = *(int*)0x00512c7c;
        if (quietFrom < gameTimeSec) quietFrom = gameTimeSec;
        const int quiet = WallTick() - quietFrom;      // wall: see IsPendingRejection
        const int left  = (kSilenceTicks - quiet + kTicksPerSecond - 1) / kTicksPerSecond;
        _snprintf(reason, reasonSize - 1,
                  "Cannot take %s: still connected%s.", who,
                  left > 0 && left <= 60 ? " (wait a little longer)" : "");
        return false;
    }

    // Escalation's anti-share-abuse rule, now asked about this specific target.
    char sgReason[160] = { 0 };
    if (ShareGuard::RefusesTake((unsigned)target->DirectPlayID, sgReason, sizeof(sgReason)))
    { _snprintf(reason, reasonSize - 1, "%s", sgReason); return false; }

    return true;
}

// Every target the local player could take right now.
void CollectEligibleTargets(std::vector<PlayerStruct*>& out)
{
    TAdynmemStruct* ta = GetTA();
    if (!ta) return;
    char reason[192];
    for (unsigned i = 0; i < kMaxPlayers; ++i)
    {
        PlayerStruct* p = &ta->Players[i];
        if (!p->PlayerActive) continue;
        if (MayTake(p, reason, sizeof(reason)))
            out.push_back(p);
    }
}

// ---- election --------------------------------------------------------------

void Broadcast(TakeClaimCommand cmd, unsigned targetDpid, int claimTick, bool includeCom)
{
    TAdynmemStruct* ta = GetTA();
    if (!ta) return;

    TakeClaimMessage msg;
    std::memset(&msg, 0, sizeof(msg));
    msg.chatByte   = 0x05;
    msg.nullText   = 0x00;
    msg.msgId      = ChatHijackId::TakeClaim;
    msg.size       = sizeof(TakeClaimMessage);
    msg.command    = cmd;
    msg.targetDpid = targetDpid;
    msg.claimTick  = claimTick;
    msg.includeCom = includeCom ? 1 : 0;

    HAPI_BroadcastMessage(ta->Players[ta->LocalHumanPlayer_PlayerID].DirectPlayID,
                          reinterpret_cast<const char*>(&msg), sizeof(msg));
}

void RecordClaim(unsigned targetDpid, unsigned claimantDpid, int tick, bool includeCom)
{
    Election& e = g_elections[targetDpid];
    if (e.executed) return;

    // A closed election is decided. Remember a late claim (the grace path may
    // re-open and need candidates) but never let it lower a tick or reopen the
    // window, or a straggler could rewrite a decision already acted on.
    const bool decided = e.resolved;

    for (auto& c : e.claims)
    {
        if (c.dpid != claimantDpid) continue;
        if (!decided && tick < c.tick) c.tick = tick;   // keep the earliest
        c.includeCom = c.includeCom || includeCom;
        return;
    }
    if (e.claims.empty() && !decided)
        e.closeTick = tick + kSettleTicks;
    Claim c; c.dpid = claimantDpid; c.tick = tick; c.includeCom = includeCom;
    e.claims.push_back(c);
}

// The rule every client runs, on the same inputs, to the same answer: earliest
// claim wins, ties broken by lowest dpid. Order-independent, which first-wins
// is not — and every client sees a different order.
const Claim* PickWinner(const Election& e)
{
    const Claim* best = nullptr;
    for (const auto& c : e.claims)
    {
        if (e.excluded.count(c.dpid)) continue;
        if (!best || c.tick < best->tick ||
            (c.tick == best->tick && c.dpid < best->dpid))
            best = &c;
    }
    return best;
}

// The ONLY place a '.take' is allowed past the ShowText hook. Doubles as the
// execution announcement: the text is broadcast like any other chat.
void EmitTake(PlayerStruct* target, bool includeCom)
{
    TAdynmemStruct* ta = GetTA();
    if (!ta || !target) return;

    char line[96];
    _snprintf(line, sizeof(line) - 1, "%s %s",
              includeCom ? ".takecmd" : ".take", target->Name);
    line[sizeof(line) - 1] = '\0';

    g_reemitting = true;
    ShowText(&ta->Players[ta->LocalHumanPlayer_PlayerID], line, 0, 0);
    g_reemitting = false;

    IDDrawSurface::OutptFmtTxt("[TakeClaim] emitted '%s'", line);
}

// Tell VoteReject a take is running here: hold the row open, relabelled, and
// stop the resume heuristic firing on the take's own injected gives.
void NoteTake(unsigned targetDpid, const char* takerName)
{
    VoteReject* vr = VoteReject::GetInstance();
    if (vr) vr->NoteTakeInProgress(targetDpid, takerName ? takerName : "");
}

// Everyone rejects, together, now that the base has changed hands.
void FinishTake(unsigned targetDpid)
{
    VoteReject* vr = VoteReject::GetInstance();
    if (vr) vr->ExecuteRejectAfterTake(targetDpid);
}

// Units the target still owns above 0 HP. Counting units does NOT work: the
// walk damages each one by 30000 as it hands it over, but the target's client
// is gone to dispatch the deaths, so they sit at Health<=0 with the alive flag
// set and UnitsNumber never decrements.
int TargetLiveUnits(unsigned targetDpid)
{
    PlayerStruct* p = FindPlayerByDPID(targetDpid);
    if (!p || !p->PlayerActive) return 0;
    UnitStruct* u   = p->Units;
    UnitStruct* end = p->UnitsAry_End;
    if (!u || !end) return 0;
    int live = 0;
    for (; u <= end; ++u)
        if ((u->UnitSelected & 0x10000000u) != 0 && u->Health > 0)
            ++live;
    return live;
}

// Demonstrably gone, as opposed to merely quiet - the same test VoteReject uses
// to call somebody dropped. Re-electing on silence alone would have each loser
// independently decide the winner was dead and take as well: the winner's three
// announcements all leave one machine over one link, so a stall loses them
// together. When in doubt we hold: a take that never happens costs the base that
// was dying anyway, while a duplicated army cannot be undone.
bool ClaimantIsGone(unsigned dpid)
{
    TAdynmemStruct* ta = GetTA();
    PlayerStruct*   p  = FindPlayerByDPID(dpid);
    if (!ta) return false;
    if (!p || !p->PlayerActive || p->DirectPlayID == 0) return true;

    int ts = p->LastMsgTimeStamp;
    const int gameTimeSec = *(int*)0x00512c7c;
    if (ts < gameTimeSec) ts = gameTimeSec;
    return (WallTick() - ts) > (int)(ta->NetworkDropoutTimeoutSec * 30);
}

// The walk has finished once the target's block stops changing. Watch the
// TARGET, not ourselves - they are disconnected, so only the walk can change
// them, whereas our own count keeps rising from normal production.
bool TransferQuiescent(Election& e, unsigned targetDpid, int now)
{
    if (now < e.nextSample) return false;
    e.nextSample = now + kSamplePeriod;

    const int live = TargetLiveUnits(targetDpid);
    if (live != e.liveCount)
    {
        e.liveCount  = live;
        e.lastChange = now;
        return false;
    }
    if (live == 0) return true;                       // nothing left: done
    return (now - e.lastChange) >= kQuietTicks;
}

void ResolveElections()
{
    TAdynmemStruct* ta = GetTA();
    PlayerStruct*   me = LocalPlayer();
    if (!ta || !me) return;
    const int now = SimTick();

    for (auto it = g_elections.begin(); it != g_elections.end(); )
    {
        Election& e  = it->second;
        const unsigned targetDpid = it->first;

        // We took this player: once their unit block is empty (or the deadline
        // passes) tell every client to reject them. Until then the vote row
        // stays open on all four clients saying the take is under way.
        if (e.awaitDrain)
        {
            const bool quiet = TransferQuiescent(e, targetDpid, now);
            if (quiet || now >= e.drainBy)
            {
                e.awaitDrain = false;
                IDDrawSurface::OutptFmtTxt(
                    "[TakeClaim] take of %08x complete after %d ticks (%s, %d live units left);"
                    " broadcasting reject",
                    targetDpid, now - (e.drainBy - kCompleteBackstop),
                    quiet ? "transfer quiescent" : "BACKSTOP", e.liveCount);
                Broadcast(TakeClaimCommand::Complete, targetDpid, now, false);

                // Reject from the tick that drains finishAt, not from here, so
                // both the taker and the receivers take the same path. The
                // receiver's copy of this is the one that matters - see
                // OnClaimPacket.
                e.finishAt = now + kFinishDelayTicks;
            }
        }

        if (e.finishAt && now >= e.finishAt)
        {
            e.finishAt = 0;
            FinishTake(targetDpid);
        }

        if (e.executed && e.retireTick && now >= e.retireTick)
        { it = g_elections.erase(it); continue; }

        if (!e.resolved && now >= e.closeTick)
        {
            const Claim* w = PickWinner(e);
            e.resolved  = true;
            e.graceTick = now + kGraceTicks;
            IDDrawSurface::OutptFmtTxt(
                "[TakeClaim] election for %08x closed at tick %d: %u claim(s), winner %08x",
                targetDpid, now, (unsigned)e.claims.size(), w ? w->dpid : 0u);
            if (e.executed)
            {
                // Somebody announced their execution while we were collecting.
                // The tick callback and packet dispatch are not ordered against
                // each other (GameTickHook sits at 0x4969cb, outside the sim
                // loop), so this has to be checked here and not only up front.
                IDDrawSurface::OutptFmtTxt(
                    "[TakeClaim] ...stood down: %08x already taken by a peer", targetDpid);
            }
            else if (!w)
            {
                e.executed   = true;
                e.retireTick = now + kElectionKeepTicks;
            }
            else if (w->dpid == (unsigned)me->DirectPlayID)
            {
                PlayerStruct* target = FindPlayerByDPID(targetDpid);
                char reason[192];
                if (target && MayTake(target, reason, sizeof(reason)))
                {
                    EmitTake(target, w->includeCom);
                    // Our own broadcast never comes back to us, so close the
                    // election here or the grace timer would elect someone else.
                    e.executed   = true;
                    e.retireTick = now + kElectionKeepTicks;
                    e.awaitDrain = true;
                    e.drainBy    = now + kCompleteBackstop;
                    e.liveCount  = -1;
                    e.lastChange = now;
                    e.nextSample = now;
                    NoteTake(targetDpid, me->Name);
                }
                else
                {
                    LocalMessage(reason);
                    e.excluded.insert(w->dpid);
                    e.resolved = false;                 // let the next claimant have it
                    e.closeTick = now;
                }
            }
            else
            {
                PlayerStruct* winner = FindPlayerByDPID(w->dpid);
                PlayerStruct* target = FindPlayerByDPID(targetDpid);
                LocalMessagef("%s is taking %s's units.",
                              winner && winner->Name[0] ? winner->Name : "An ally",
                              target && target->Name[0] ? target->Name : "them");
                IDDrawSurface::OutptFmtTxt("[TakeClaim] lost election for %08x to %08x",
                                           targetDpid, w->dpid);
            }
        }
        // Elected but nothing heard yet. Hand the take on ONLY if the winner is
        // provably gone - never merely because they have been quiet.
        else if (e.resolved && !e.executed && now >= e.graceTick)
        {
            const Claim* w = PickWinner(e);
            if (!w)
            {
                e.executed   = true;
                e.retireTick = now + kElectionKeepTicks;
            }
            else if (ClaimantIsGone(w->dpid))
            {
                e.excluded.insert(w->dpid);
                e.resolved  = false;
                e.closeTick = now;
                IDDrawSurface::OutptFmtTxt(
                    "[TakeClaim] winner %08x has dropped without taking %08x; re-electing",
                    w->dpid, targetDpid);
            }
            else
            {
                // Still connected, just slow. Keep waiting rather than guessing.
                e.graceTick = now + kGraceTicks;
                if (!e.graceLogged)
                {
                    e.graceLogged = true;
                    IDDrawSurface::OutptFmtTxt(
                        "[TakeClaim] winner %08x has not announced a take of %08x yet;"
                        " still connected, holding",
                        w->dpid, targetDpid);
                }
            }
        }
        ++it;
    }
}

// ---- command entry ---------------------------------------------------------

// Shared by typed chat and the VoteDialog buttons.
void BeginClaim(PlayerStruct* target, bool includeCom)
{
    PlayerStruct* me = LocalPlayer();
    if (!me || !target) return;

    const unsigned targetDpid = (unsigned)target->DirectPlayID;

    auto found = g_elections.find(targetDpid);
    if (found != g_elections.end() && found->second.executed)
    {
        LocalMessagef("%s's units have already been claimed.", target->Name);
        return;
    }

    // People mash the button until something happens; one claim is enough.
    if (found != g_elections.end() && !found->second.resolved)
    {
        const unsigned mine = (unsigned)me->DirectPlayID;
        for (const auto& c : found->second.claims)
            if (c.dpid == mine)
            {
                LocalMessagef("Already claiming %s's units - waiting for the other"
                              " players to answer.", target->Name);
                return;
            }
    }

    char reason[192];
    if (!MayTake(target, reason, sizeof(reason)))
    { LocalMessage(reason); return; }

    const int now = SimTick();
    RecordClaim(targetDpid, (unsigned)me->DirectPlayID, now, includeCom);
    Broadcast(TakeClaimCommand::Claim, targetDpid, now, includeCom);
    IDDrawSurface::OutptFmtTxt("[TakeClaim] claiming %08x at tick %d (settle %d)",
                               targetDpid, now, kSettleTicks);
    LocalMessagef("Claiming %s's units...", target->Name);
}

// Returns false if this is not a take command. On true *arg holds the trimmed
// target name, which may be empty.
bool ParseTakeCommand(const char* text, bool* includeCom, std::string* arg)
{
    if (!text) return false;
    while (*text == ' ' || *text == '\t') ++text;
    if (*text != '.') return false;
    ++text;

    struct { const char* name; bool com; } kCmds[] = {
        { "takecmd", true }, { "take", false },      // longest first
    };
    for (const auto& c : kCmds)
    {
        const size_t n = std::strlen(c.name);
        if (_strnicmp(text, c.name, n) != 0) continue;
        const char* rest = text + n;
        if (*rest && *rest != ' ' && *rest != '\t' && *rest != '\r' && *rest != '\n')
            continue;                                 // '.takefoo' is not '.take'
        while (*rest == ' ' || *rest == '\t') ++rest;

        std::string a(rest);
        while (!a.empty() && (a.back() == ' ' || a.back() == '\t' ||
                              a.back() == '\r' || a.back() == '\n'))
            a.pop_back();
        if (a.size() >= 2 && a.front() == '"' && a.back() == '"')
            a = a.substr(1, a.size() - 2);            // ParseParams-style quoting

        *includeCom = c.com;
        *arg = a;
        return true;
    }
    return false;
}

void HandleLocalTakeCommand(const char* text)
{
    bool includeCom = false;
    std::string arg;
    ParseTakeCommand(text, &includeCom, &arg);

    if (!arg.empty())
    {
        bool ambiguous = false;
        PlayerStruct* target = ResolvePlayerByName(arg.c_str(), &ambiguous);
        if (!target)
        {
            LocalMessagef(ambiguous ? "'%s' matches more than one player - use the full name."
                                    : "No player called '%s'.", arg.c_str());
            return;
        }
        BeginClaim(target, includeCom);
        return;
    }

    // Bare '.take' is fine when there is only one thing it could mean.
    std::vector<PlayerStruct*> candidates;
    CollectEligibleTargets(candidates);

    if (candidates.empty())
    {
        LocalMessage("Nobody's units can be taken right now.");
        return;
    }
    if (candidates.size() == 1)
    {
        BeginClaim(candidates[0], includeCom);
        return;
    }

    std::string names;
    for (size_t i = 0; i < candidates.size(); ++i)
    {
        if (i) names += ", ";
        names += candidates[i]->Name;
    }
    LocalMessagef("Several players can be taken (%s) - say '%s <name>'.",
                  names.c_str(), includeCom ? ".takecmd" : ".take");
}

// ---- hooks and handlers ----------------------------------------------------

// Returns from _ShowText (__stdcall, RET 0x10) without running it, so the chat
// is never formatted, broadcast or displayed.
__declspec(naked) void ShowTextSuppressStub()
{
    __asm { ret 0x10 }
}

// _ShowText entry. The prologue has not run, so [Esp+4]=player, [Esp+8]=text.
int __stdcall ShowText_Entry_Proc(PInlineX86StackBuffer pBuf)
{
    DWORD* args = reinterpret_cast<DWORD*>(pBuf->Esp);
    const char* text = reinterpret_cast<const char*>(args[2]);

    bool includeCom = false;
    std::string arg;
    if (!ParseTakeCommand(text, &includeCom, &arg)) return 0;   // ordinary chat

    // Our own re-emission: the one '.take' that reaches dplayx and the wire.
    if (g_reemitting) return 0;

    HandleLocalTakeCommand(text);

    pBuf->rtnAddr_Pvoid = reinterpret_cast<LPVOID>(&ShowTextSuppressStub);
    return X86STRACKBUFFERCHANGE;
}

void OnClaimPacket(unsigned fromDpid, const void* buf)
{
    const TakeClaimMessage* msg = reinterpret_cast<const TakeClaimMessage*>(buf);
    if (!msg || msg->size != sizeof(TakeClaimMessage)) return;

    if (msg->command == TakeClaimCommand::Complete)
    {
        Election& e  = g_elections[msg->targetDpid];
        e.resolved   = true;
        e.executed   = true;
        e.awaitDrain = false;
        e.retireTick = SimTick() + kElectionKeepTicks;
        IDDrawSurface::OutptFmtTxt("[TakeClaim] %08x finished taking %08x; rejecting on next tick",
                                   fromDpid, msg->targetDpid);

        // NEVER reject from here. This runs inside PacketChatRouter's hook at
        // 0x0045522e, i.e. in the middle of TA's chat dispatch. ExecuteReject
        // calls Send_PacketPlayerState_1B, which re-enters the network stack and
        // refills PACKET_DATA - the very buffer TA is about to print from four
        // instructions later at 0x00455250. That is the junk chat line: TA
        // prints the packet that replaced ours (a 6-byte reject) with our
        // 65-byte Complete still resident behind it.
        //
        // Do no network I/O in a packet handler. Hand it to the game tick.
        e.finishAt = SimTick() + kFinishDelayTicks;
        return;
    }

    if (msg->command == TakeClaimCommand::Withdraw)
    {
        auto it = g_elections.find(msg->targetDpid);
        if (it == g_elections.end()) return;
        it->second.excluded.insert(fromDpid);
        return;
    }
    if (msg->command != TakeClaimCommand::Claim) return;

    // Sim ticks ARE comparable across clients, so honour the claimant's own
    // stamp - that is what makes every client pick the same winner. Clamp only
    // what cannot be honest: each client runs a few ticks per frame, so a small
    // skew either way is normal, and anything beyond it would let a client win
    // every election by claiming the distant past.
    int tick = msg->claimTick;
    const int now = SimTick();
    if (tick < now - kTickSkewTolerance || tick > now + kTickSkewTolerance)
    {
        IDDrawSurface::OutptFmtTxt(
            "[TakeClaim] claim from %08x for %08x has out-of-band tick %d (now %d); clamping",
            fromDpid, msg->targetDpid, tick, now);
        tick = now;
    }

    IDDrawSurface::OutptFmtTxt("[TakeClaim] claim received from %08x for %08x at tick %d",
                               fromDpid, msg->targetDpid, tick);
    RecordClaim(msg->targetDpid, fromDpid, tick, msg->includeCom != 0);
}

// Watch chat for the winner's '.take <name>' and for '.give <me>' grants.
void OnIncomingChat(unsigned fromDpid, const char* text)
{
    if (!text) return;

    // "<Name> body" or "<Name->Allies> body". Match the FIRST "> ": the last
    // '>' in the line could be one from the message itself.
    const char* sep  = std::strstr(text, "> ");
    const char* body = sep ? sep + 2 : text;
    while (*body == ' ') ++body;

    bool includeCom = false;
    std::string arg;
    if (ParseTakeCommand(body, &includeCom, &arg))
    {
        // Somebody is executing, so that target is settled.
        PlayerStruct* target = nullptr;
        bool ambiguous = false;
        if (!arg.empty()) target = ResolvePlayerByName(arg.c_str(), &ambiguous);
        if (target)
        {
            Election& e  = g_elections[(unsigned)target->DirectPlayID];
            const bool wasOpen = !e.executed;
            e.resolved   = true;
            e.executed   = true;
            e.retireTick = SimTick() + kElectionKeepTicks;
            if (wasOpen)
                IDDrawSurface::OutptFmtTxt(
                    "[TakeClaim] %08x executed a take of %s; standing down",
                    fromDpid, target->Name[0] ? target->Name : "?");
            // NOT CancelTimeoutVote: cancelling here is what made the taker's
            // dialog vanish while the player stayed unrejected. Hold the row
            // open until the taker's Complete arrives.
            PlayerStruct* taker = FindPlayerByDPID(fromDpid);
            NoteTake((unsigned)target->DirectPlayID, taker ? taker->Name : "An ally");
        }
        return;
    }

    // Second, independent confirmation that a take happened: dplayx broadcasts
    // "<taker> taking <target>s units" from OnTake, with no "<name>" wrapper.
    // The command echo above can be missed; this cannot both be missed for the
    // same take, and missing both is what makes the grace timer hand the units
    // to a second claimant.
    {
        const char* tk = std::strstr(text, " taking ");
        const size_t len = std::strlen(text);
        if (tk && len > 7 && _stricmp(text + len - 7, "s units") == 0 && text[0] != '<')
        {
            std::string victim(tk + 8, text + len - 7);
            bool amb = false;
            PlayerStruct* target = ResolvePlayerByName(victim.c_str(), &amb);
            if (target)
            {
                Election& e  = g_elections[(unsigned)target->DirectPlayID];
                const bool wasOpen = !e.executed;
                e.resolved   = true;
                e.executed   = true;
                e.retireTick = SimTick() + kElectionKeepTicks;
                if (wasOpen)
                {
                    IDDrawSurface::OutptFmtTxt(
                        "[TakeClaim] accept seen for %s; standing down", target->Name);
                    PlayerStruct* taker = FindPlayerByDPID(fromDpid);
                    NoteTake((unsigned)target->DirectPlayID, taker ? taker->Name : "An ally");
                }
            }
            return;
        }
    }

    // '.give <name>' naming us is an explicit grant, standing in for alliance.
    if (_strnicmp(body, ".give", 5) == 0 &&
        (body[5] == ' ' || body[5] == '\t'))
    {
        PlayerStruct* me = LocalPlayer();
        if (!me) return;
        if (std::strstr(body + 5, me->Name))
            g_giveGrants.insert(fromDpid);
        return;
    }
    if (_strnicmp(body, ".stopgive", 9) == 0 &&
        (body[9] == ' ' || body[9] == '\t'))
    {
        PlayerStruct* me = LocalPlayer();
        if (!me) return;
        if (std::strstr(body + 9, me->Name))
            g_giveGrants.erase(fromDpid);
    }
}

void OnGameTick(int)
{
    ResolveElections();
}

} // namespace

namespace TakeClaim {

void Install()
{
    if (g_showTextHook) return;

    g_showTextHook = new InlineSingleHook(
        kShowTextAddr, kShowTextLen,
        INLINE_5BYTESLAGGERJMP,
        (InlineX86HookRouter)ShowText_Entry_Proc);

    PacketChatRouter::GetInstance()->RegisterHandler(
        ChatHijackId::TakeClaim, OnClaimPacket, /*fireInDemo*/ false);
    PacketChatRouter::GetInstance()->RegisterChatHandler(OnIncomingChat);
    GameTickHook::GetInstance()->addCallback(OnGameTick);
}

void Shutdown()
{
    delete g_showTextHook; g_showTextHook = nullptr;
    g_elections.clear();
    g_giveGrants.clear();
    g_reemitting = false;
}

void RequestTake(unsigned targetDpid, bool includeCommander)
{
    PlayerStruct* target = FindPlayerByDPID(targetDpid);
    if (!target)
    {
        LocalMessage("That player is no longer in the game.");
        return;
    }
    BeginClaim(target, includeCommander);
}

} // namespace TakeClaim

#else  // !TAKE_CLAIM_ENABLE

namespace TakeClaim {
void Install() {}
void Shutdown() {}
void RequestTake(unsigned, bool) {}
}

#endif // TAKE_CLAIM_ENABLE
