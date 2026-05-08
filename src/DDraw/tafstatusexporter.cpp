#include "tafstatusexporter.h"
#include "hook/etc.h"       // PInlineX86StackBuffer
#include "hook/hook.h"      // InlineSingleHook, INLINE_5BYTESLAGGERJMP
#include "Profiler.h"
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <memory>
#include <unordered_map>

// Forward declarations: defined later in the file but referenced from hooks inside
// the anonymous namespace below.
static int64_t  WallClockMs();
static uint32_t GetCurrentGameTime();
static bool     IsRecentCommandfireFirer(UnitStruct* pUnit);

// Singleton pointer to the live CTAFStatusExporter, set by its constructor and
// cleared by its destructor. Hook procs use this to write into the kill ring
// without holding an instance pointer themselves.
extern CTAFStatusExporter* g_exporterSingleton;

// Force a write at least this often even if nothing changed, so consumers can detect that
// the exporter is alive. IDDrawSurface::Unlock fires roughly per render frame (~30 Hz), so
// 30 ≈ 1 second. Must be comfortably below GameMonitor2's EXTERNAL_STATUS_STALENESS_MS.
static const int HEARTBEAT_FRAMES = 30;

// UnitStateMask bit for "alive" (per Ghidra's UnitStateMask enum). The legacy tdraw code
// uses the same DWORD field via UnitStruct::UnitSelected.
static const unsigned int UNITSTATEMASK_ALIVE = 0x10000000u;

// "Is this unit a commander" — engine uses case-insensitive name match against
// RaceSideData[Raceside].commanderUnitName (Send_UnitDeath_P0C @ 0x4864e7,
// CreatePlayerCommander @ 0x496f24). The IDA enum `UNITINFOMASK_1::commander
// = 0x40000` in tamem.h is misleading; that bit is unrelated and false-positives
// on e.g. ProTA's ARMTHOVR. Helper UnitIsCommander() lives in the anon namespace.

// Dgun-vs-COMMANDER_BLAST discrimination. Both are commander-fired AoE that
// one-shots, so HP-delta + killer-is-commander alone can't tell them apart.
// The clean signal is the weapon's WeaponTypeMask::commandfire bit
// (0x04000000) — engine semantics are "do NOT auto-acquire targets; user must
// explicitly order each shot" (engine reads the bit at >>0x1a&1 in the auto-aim
// loops e.g. UnitAutoAim_FindGoodTarget). Set in vanilla on dgun, plane bombs
// (ARMBOMB/ARMADVBOMB/COR…), and stockpile nukes — NOT on COMMANDER_BLAST,
// which is a death-trigger the engine fires automatically. Bombs and nukes
// also have commandfire=1 but they're plane/silo weapons; we additionally gate
// on killer-is-commander so they don't false-positive as dgun.
//
// The bit position is engine-hardcoded; whether a weapon has it is data-driven
// from the TDF `commandfire=1` field (LoadWeaponTdf @ 0x42e440).
//
// Captured symmetrically on both peers:
//   - firer-side via WEAPONS_ProjectileDamage @ 0x499eb0
//   - receiver-side via ReceiveWeaponFired @ 0x49d270 (reads the 0x0d packet's
//     weaponID and looks up the WeaponTypedef)
// Firer tags keyed by UnitStruct* are reset on game-restart
// (ResetTagsOnGameRestart) so recycled addresses don't false-positive.

namespace {

static const unsigned int WEAPONTYPEMASK_COMMANDFIRE     = 0x04000000u;
static const unsigned int WEAPONS_PROJECTILE_DAMAGE_ADDR = 0x00499eb0u;
static const unsigned int RECEIVE_WEAPON_FIRED_ADDR      = 0x0049d270u;
static const unsigned int UNITS_RECEIVE_UNIT_DEATH_ADDR  = 0x004866d0u;
static const unsigned int DRAW_GAME_SCREEN_ADDR          = 0x00468cf0u;
// UnitKilled0CPacket::cause upper-nibble value indicating self-destruct or
// resignation. Trace: Order_SelfDestruct → UNITS_MakeDamage(p,p,30000,3) →
// LastDamageType=3; UNITS_KillAllForPlayer (resignation) also passes 3.
// Both are "deliberately gave up unit" — neither a deserved kill.
static const unsigned int DEATH_CAUSE_SELF_DESTRUCT       = 3u;
// Pointer to the live TAdynmemStruct (the canonical engine-wide TA state).
// Same address used by every other tdraw module that touches engine memory.
static TAdynmemStruct* GetTAdynmem()
{
    return *reinterpret_cast<TAdynmemStruct**>(0x00511de8u);
}

// Verify the struct layout we rely on hasn't drifted (raw byte arithmetic in
// Ghidra disasm uses these specific offsets).
static_assert(sizeof(UnitStruct) == 0x118,            "UnitStruct stride changed");
static_assert(sizeof(WeaponStruct) == 0x115,          "WeaponStruct stride changed");
static_assert(sizeof(RaceSideData) == 0x232,          "RaceSideData stride changed");
static_assert(sizeof(ProjectileStruct) == 0x6B,       "ProjectileStruct stride changed");
static_assert(offsetof(TAdynmemStruct, LocalHumanPlayer_PlayerID) == 0x2A42, "cControlPlayerID offset changed");
static_assert(offsetof(TAdynmemStruct, Weapons) == 0x2CF3,                   "Weapons[] offset changed");
static_assert(offsetof(TAdynmemStruct, BeginUnitsArray_p) == 0x14357,        "BeginUnitsArray_p offset changed");
static_assert(offsetof(TAdynmemStruct, RaceSideDataAry) == 0x37F3D,          "RaceSideDataAry offset changed");
static_assert(offsetof(TAdynmemStruct, GameTime) == 0x38A47,                 "GameTime offset changed");
static_assert(offsetof(TAdynmemStruct, ScoreDisplay_WinFlag) == 0x391AF,     "ScoreDisplay_WinFlag offset changed");
static_assert(offsetof(TAdynmemStruct, GameStateMask) == 0x3923B,            "GameStateMask offset changed");
static_assert(offsetof(WeaponStruct, WeaponTypeMask) == 0x111,        "WeaponTypeMask offset changed");
static_assert(offsetof(RaceSideData, commanderUnitName) == 0x22,      "commanderUnitName offset changed");
static_assert(offsetof(ProjectileStruct, AttackerUnitPtr) == 0x52,    "AttackerUnitPtr offset changed");

// "Player recently self-destructed a unit they own", keyed by dplayId → game-tick.
// Populated when a 0x0c packet's cause-byte upper-nibble == 3. Both peers see the
// same cause byte (HAPI-broadcast) so each reaches the same conclusion locally.
// Freshness window covers the SD countdown (~5s) plus cascade.
static std::unordered_map<uint32_t, uint32_t> g_recentSelfDestructByPlayer;
static const uint32_t SELF_DESTRUCT_FRESHNESS_TICKS = 300;  // 10 s @ 30 tps
// "Unit recently fired a commandfire projectile", keyed by firer UnitStruct* →
// fire game-tick. Freshness uses GameTime (not wall-clock) so it's immune to
// game-speed and pause. 90 ticks covers projectile travel (~36) + cascade (~15)
// with margin. Tag the FIRER (not victim) because WEAPONS_ProjectileDamage only
// runs on the projectile owner's peer; receivers learn the weapon class from the
// 0x0d WEAPON_FIRED packet via ReceiveWeaponFired and tag in symmetry.
// Cascade case: commander_blast (the death-trigger) has commandfire=0 so the
// dying commander never gets tagged; collateral commander_blast deaths are
// correctly classified as non-dgun.
static std::unordered_map<UnitStruct*, uint32_t>     g_recentCommandfireFirer;
static const uint32_t COMMANDFIRE_FIRER_FRESHNESS_TICKS = 90;  // 3 s @ 30 tps
static std::unique_ptr<InlineSingleHook>             g_weaponsProjectileDamageHook;
static std::unique_ptr<InlineSingleHook>             g_receiveWeaponFiredHook;
static std::unique_ptr<InlineSingleHook>             g_receiveUnitDeathHook;
// Re-pin GameStateMask at DrawGameScreen entry. Lock-time override alone is
// insufficient — the renderer reads the mask before our IDDrawSurface::Lock
// hook fires (likely renders to an offscreen buffer pre-Lock).
static std::unique_ptr<InlineSingleHook>             g_drawGameScreenEntryHook;

// SEH-protected memcpy: simultaneously validates the source pointer and
// captures the bytes into a local POD. Must live in its own helper because
// __try can't share a frame with C++ objects that need unwinding (C2712).
static bool SafeCopyBytes(const void* src, void* dst, size_t bytes)
{
    if (!src || !dst) return false;
    __try { memcpy(dst, src, bytes); return true; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}

// Wire-format subpacket layouts. #pragma pack(1) so unaligned fields land on
// the right offsets and the compiler emits unaligned-safe loads.
#pragma pack(push, 1)
struct UnitKilled0CPacket {        // subpacket 0x0c, 11 bytes
    uint8_t  subpacketId;          // 0x00 = 0x0c
    uint16_t dyingUnitId;          // 0x01  slot into BeginUnitsArray_p
    uint32_t killerDplayId;        // 0x03  authoritative (set by victim's owner peer)
    uint16_t killerUnitId;         // 0x07  slot into BeginUnitsArray_p (0 = unknown)
    uint8_t  _pad9;                // 0x09
    uint8_t  cause;                // 0x0A  upper nibble = death-type param_2
};
static_assert(sizeof(UnitKilled0CPacket) == 11, "0x0c wire layout");

struct WeaponFired0DPacket {       // subpacket 0x0d (we read up to byte 0x22)
    uint8_t  data_00[0x19];        // 0x00..0x18
    uint8_t  weaponID;             // 0x19
    uint8_t  data_1a[0x21 - 0x1a]; // 0x1A..0x20
    uint16_t firerUnitSlot;        // 0x21  slot into BeginUnitsArray_p
};
static_assert(sizeof(WeaponFired0DPacket) == 0x23, "0x0d wire layout up to firer");
#pragma pack(pop)

static int __stdcall WeaponsProjectileDamageHookProc(PInlineX86StackBuffer X)
{
    PROFILE_SCOPE("Hook.WeaponsProjectileDamage");
    if (!X) return 0;
    // First argument [Esp+4]: ProjectileStruct*.
    ProjectileStruct* proj = *reinterpret_cast<ProjectileStruct**>(X->Esp + 4);
    if (!proj || !proj->Weapon) return 0;
    if ((proj->Weapon->WeaponTypeMask & WEAPONTYPEMASK_COMMANDFIRE) == 0) return 0;

    // Runs on the projectile owner's peer only; receiver peers tag the firer
    // via ReceiveWeaponFired's 0x0d packet path, so both peers converge on
    // the same firer-tag set from local data.
    if (proj->AttackerUnitPtr)
        g_recentCommandfireFirer[proj->AttackerUnitPtr] = GetCurrentGameTime();
    return 0;
}

// Receiver-side WEAPON_FIRED (subpacket 0x0d) handler @ 0x49d270. The 0x0B HAPI
// damage packet that arrives later carries no weapon ID, so this is the receiver's
// only chance to learn weapon class.
//
// TODO(WeaponFiredExt): when the in-flight branch adding ChatHijackId::WeaponFiredExt
// (0x2d) lands, weapon IDs become uint16 staged in a thread-local. Read that
// thread-local instead of pkt.weaponID, else IDs >= 256 misclassify.
static int __stdcall ReceiveWeaponFiredHookProc(PInlineX86StackBuffer X)
{
    PROFILE_SCOPE("Hook.ReceiveWeaponFired");
    if (!X) return 0;
    // Stack at hook entry: [Esp+4] = pPlayerInfo, [Esp+8] = pPacketData.
    WeaponFired0DPacket pkt;
    if (!SafeCopyBytes(*(void**)(X->Esp + 8), &pkt, sizeof(pkt))) return 0;

    TAdynmemStruct* dynmem = GetTAdynmem();
    if (!dynmem) return 0;
    if ((dynmem->Weapons[pkt.weaponID].WeaponTypeMask & WEAPONTYPEMASK_COMMANDFIRE) == 0) return 0;

    if (pkt.firerUnitSlot == 0) return 0;
    UnitStruct* unitsBase = dynmem->BeginUnitsArray_p;
    if (!unitsBase) return 0;
    g_recentCommandfireFirer[unitsBase + pkt.firerUnitSlot] = GetCurrentGameTime();
    return 0;
}

// Engine's canonical commander check: case-insensitive compare unit->UnitName
// against RaceSideDataAry[RaceSide].commanderUnitName.
static bool UnitIsCommander(UnitStruct* pUnit)
{
    if (!pUnit || !pUnit->UnitType) return false;
    if (!pUnit->Owner_PlayerPtr0 || !pUnit->Owner_PlayerPtr0->PlayerInfo) return false;
    TAdynmemStruct* dynmem = GetTAdynmem();
    if (!dynmem) return false;

    const int raceside = pUnit->Owner_PlayerPtr0->PlayerInfo->RaceSide;
    if (raceside < 0 || raceside >= 5) return false;

    return _stricmp(dynmem->RaceSideDataAry[raceside].commanderUnitName,
                    pUnit->UnitType->UnitName) == 0;
}

static uint32_t ReadOwnerDplayId(UnitStruct* pUnit)
{
    if (!pUnit || !pUnit->Owner_PlayerPtr0) return 0;
    return static_cast<uint32_t>(pUnit->Owner_PlayerPtr0->DirectPlayID);
}

static bool IsRecentSelfDestructPlayer(uint32_t dpid)
{
    if (dpid == 0u || dpid == 0xFFFFFFFFu) return false;
    auto it = g_recentSelfDestructByPlayer.find(dpid);
    if (it == g_recentSelfDestructByPlayer.end()) return false;
    const uint32_t now = GetCurrentGameTime();
    if (now == 0) return false;  // GameTime read failed
    const uint32_t when = it->second;
    if (now < when) return false;  // wraparound sanity
    return (now - when) <= SELF_DESTRUCT_FRESHNESS_TICKS;
}

// Build kill-event flags. SELF_DESTRUCT is gated on the cause-byte upper-nibble
// (set in g_recentSelfDestructByPlayer when the victim's owner is observed
// initiating SD), NOT on `victim == killer` — the latter false-positives on
// cascade self-AOE clip when commander_blast rewrites the victim's Attacker_p
// to self mid-frame (see game 102 TheCore misflag).
static uint16_t BuildKillEventFlags(UnitStruct* pVictim, UnitStruct* pKiller,
                                    uint32_t victimDpid, uint32_t killerDpid)
{
    uint16_t flags = TAF_KILL_FLAG_VICTIM_COMMANDER;
    const bool killerIsCommander = UnitIsCommander(pKiller);
    if (killerIsCommander) flags |= TAF_KILL_FLAG_KILLER_COMMANDER;
    if (IsRecentSelfDestructPlayer(victimDpid))
        flags |= TAF_KILL_FLAG_SELF_DESTRUCT;
    if (pKiller && killerIsCommander && victimDpid != killerDpid
        && IsRecentCommandfireFirer(pKiller))
        flags |= TAF_KILL_FLAG_PRESUMED_DGUN;
    return flags;
}

// Hook UNITS_ReceiveUnitDeath @ 0x4866d0 — single source of truth for commander
// deaths on every peer. UNITS_Send_UnitDeath_P0C synchronously calls Receive on
// the local path, so we get every death exactly once via this hook (don't also
// hook Send — would double-count). The packet is killerDplayId-authoritative
// because it's set by the victim's owner peer, which broadcasts it.
// Suppress kill-event emission for the first ~3 s of game time. Empirically the
// engine sometimes Receives an unattributed UNIT_KILLED for one peer's commander
// at game-tick ~30 that doesn't replicate to the other peer's local Send, leaving
// asymmetric kill rings that poison the tiebreaker. A real MP commander can't
// die in the first 3 s (SD has a 5 s countdown, no enemy units exist yet), so
// gating here is safe.
static const uint32_t KILL_EVENT_EMIT_MIN_GAMETIME = 90;  // 3 s @ 30 tps

static int __stdcall UnitsReceiveUnitDeathHookProc(PInlineX86StackBuffer X)
{
    PROFILE_SCOPE("Hook.UnitsReceiveUnitDeath");
    if (!X) return 0;

    UnitKilled0CPacket pkt;
    if (!SafeCopyBytes(*(void**)(X->Esp + 4), &pkt, sizeof(pkt))) return 0;
    if (pkt.dyingUnitId == 0) return 0;

    TAdynmemStruct* dynmem = GetTAdynmem();
    if (!dynmem) return 0;
    if (static_cast<uint32_t>(dynmem->GameTime) < KILL_EVENT_EMIT_MIN_GAMETIME) return 0;
    UnitStruct* unitsBase = dynmem->BeginUnitsArray_p;
    if (!unitsBase) return 0;
    UnitStruct* pVictim = unitsBase + pkt.dyingUnitId;
    // Mirror engine's alive-gate at 0x4866ec — Receive can fire multiple times
    // during cleanup but no-ops after first call clears the alive bit.
    if ((pVictim->UnitSelected & UNITSTATEMASK_ALIVE) == 0) return 0;

    // Tag any unit's owner on SD-cause death — covers transport-kamikaze
    // (Arm SDs transport, carried commander dies later: tag carries through).
    if ((pkt.cause >> 4) == DEATH_CAUSE_SELF_DESTRUCT) {
        const uint32_t ownerDpid = ReadOwnerDplayId(pVictim);
        if (ownerDpid != 0u && ownerDpid != 0xFFFFFFFFu)
            g_recentSelfDestructByPlayer[ownerDpid] = GetCurrentGameTime();
    }

    if (!UnitIsCommander(pVictim)) return 0;

    UnitStruct* pKiller = (pkt.killerUnitId != 0) ? (unitsBase + pkt.killerUnitId) : nullptr;
    const uint32_t victimDpid = ReadOwnerDplayId(pVictim);
    const uint16_t flags = BuildKillEventFlags(pVictim, pKiller, victimDpid, pkt.killerDplayId);

    if (victimDpid != 0u && g_exporterSingleton)
        g_exporterSingleton->EmitKillEvent(victimDpid, pkt.killerDplayId, flags);
    return 0;
}

static int __stdcall DrawGameScreenEntryHookProc(PInlineX86StackBuffer /*X*/)
{
    PROFILE_SCOPE("Hook.DrawGameScreen_Entry");
    if (g_exporterSingleton)
        g_exporterSingleton->RepinGameStateMaskBits();
    return 0;
}

}  // namespace

void InstallDgunDetectHooks()
{
    g_weaponsProjectileDamageHook.reset(new InlineSingleHook(
        WEAPONS_PROJECTILE_DAMAGE_ADDR, 5, INLINE_5BYTESLAGGERJMP,
        WeaponsProjectileDamageHookProc));
    g_receiveWeaponFiredHook.reset(new InlineSingleHook(
        RECEIVE_WEAPON_FIRED_ADDR, 5, INLINE_5BYTESLAGGERJMP,
        ReceiveWeaponFiredHookProc));
    g_receiveUnitDeathHook.reset(new InlineSingleHook(
        UNITS_RECEIVE_UNIT_DEATH_ADDR, 5, INLINE_5BYTESLAGGERJMP,
        UnitsReceiveUnitDeathHookProc));
    g_drawGameScreenEntryHook.reset(new InlineSingleHook(
        DRAW_GAME_SCREEN_ADDR, 5, INLINE_5BYTESLAGGERJMP,
        DrawGameScreenEntryHookProc));
}

static bool IsRecentCommandfireFirer(UnitStruct* pUnit)
{
    auto it = g_recentCommandfireFirer.find(pUnit);
    if (it == g_recentCommandfireFirer.end()) return false;
    const uint32_t now = GetCurrentGameTime();
    if (now == 0) return false;  // GameTime read failed — fail closed
    const uint32_t fireTick = it->second;
    if (now < fireTick) return false;  // sanity (would only happen on game-restart wraparound)
    return (now - fireTick) <= COMMANDFIRE_FIRER_FRESHNESS_TICKS;
}

// Reset per-game tag state. Called on game restart so recycled UnitStruct
// addresses don't false-positive against pointer-keyed entries.
static void ResetDgunDetectMap()
{
    g_recentCommandfireFirer.clear();
    g_recentSelfDestructByPlayer.clear();
}

static int64_t WallClockMs()
{
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    // 100ns ticks since 1601, convert to ms since 1970
    static const int64_t EPOCH_DIFF_MS = 11644473600000LL;
    int64_t ticks100ns = (static_cast<int64_t>(ft.dwHighDateTime) << 32)
                       |  static_cast<int64_t>(ft.dwLowDateTime);
    return ticks100ns / 10000 - EPOCH_DIFF_MS;
}

// TAdynmemStruct::GameTime — game tick counter (30 tps, frozen during pause).
// Robust against pause exploit and game-speed changes. 0 if dynmem not yet up.
static uint32_t GetCurrentGameTime()
{
    TAdynmemStruct* dynmem = GetTAdynmem();
    return dynmem ? static_cast<uint32_t>(dynmem->GameTime) : 0u;
}

CTAFStatusExporter* g_exporterSingleton = nullptr;

CTAFStatusExporter::CTAFStatusExporter(TAdynmemStruct* dynmem)
    : m_TAdynmem(dynmem), m_hMap(NULL), m_pView(nullptr), m_prev{},
      m_framesSinceWrite(HEARTBEAT_FRAMES),
      m_killRingHead(0),
      m_tiebreakerWinnerDplayId(0),
      m_lastUnitArrayBegin(nullptr),
      m_lastUnitArrayEnd(nullptr)
{
    memset(m_killRing, 0, sizeof(m_killRing));
    m_hMap  = CreateFileMapping((HANDLE)0xFFFFFFFF, NULL, PAGE_READWRITE,
                                 0, sizeof(TAFGameState),
                                 TAFGAMESTATE_SHMEM_NAME);
    m_pView = m_hMap ? MapViewOfFile(m_hMap, FILE_MAP_ALL_ACCESS,
                                     0, 0, sizeof(TAFGameState))
                     : nullptr;
    if (m_pView)
        memset(m_pView, 0, sizeof(TAFGameState)); // magic=0 until first update
    g_exporterSingleton = this;
}

CTAFStatusExporter::~CTAFStatusExporter()
{
    if (g_exporterSingleton == this) g_exporterSingleton = nullptr;
    if (m_pView) { UnmapViewOfFile(m_pView); m_pView = nullptr; }
    if (m_hMap)  { CloseHandle(m_hMap);       m_hMap  = NULL;   }
}

void CTAFStatusExporter::EmitKillEvent(uint32_t victimDpid, uint32_t killerDpid, uint16_t flags)
{
    TAFKillEvent ev{};
    ev.wallClockMs   = WallClockMs();
    ev.victimDplayId = victimDpid;
    ev.killerDplayId = killerDpid;
    ev.flags         = flags;
    m_killRing[m_killRingHead % TAF_KILL_RING_SIZE] = ev;
    ++m_killRingHead;
}

// Detect game-restart by watching unit-array bounds change, and clear the
// per-unit-pointer-keyed tag tables so recycled UnitStruct addresses can't
// false-positive against stale entries. (Kill events themselves are emitted
// from UnitsReceiveUnitDeathHookProc, not this function.)
void CTAFStatusExporter::ResetTagsOnGameRestart()
{
    UnitStruct* begin = m_TAdynmem ? m_TAdynmem->BeginUnitsArray_p : nullptr;
    UnitStruct* end   = m_TAdynmem ? m_TAdynmem->EndOfUnitsArray_p : nullptr;
    if (!begin || !end || end <= begin) {
        if (m_lastUnitArrayBegin || m_lastUnitArrayEnd) {
            ResetDgunDetectMap();
            m_tiebreakerWinnerDplayId = 0;
            m_lastUnitArrayBegin = nullptr;
            m_lastUnitArrayEnd   = nullptr;
        }
        return;
    }
    if (begin != m_lastUnitArrayBegin || end != m_lastUnitArrayEnd) {
        ResetDgunDetectMap();
        m_tiebreakerWinnerDplayId = 0;
        m_lastUnitArrayBegin = begin;
        m_lastUnitArrayEnd   = end;
    }
}

// GameStateMask bits driving the in-game VICTORY/DEFEAT animation in DrawGameScreen
// @ 0x468cf0:
//   bit 0x20 → igvictory.gaf (read independently of 0x40 — both set = both render)
//   bit 0x40 → igdefeat.gaf
//   bit 0x10 → winner status (score tally; not read by the animation)
// Engine victory path sets 0x10|0x20; we mirror that. Engine writes happen every
// 30 ticks via Game_PlayerPerTickUpdate @ 0x464f80; RepinGameStateMaskBits is
// called from a DrawGameScreen entry hook to win the race per render.
static const uint16_t  GAMESTATEMASK_VICTORY_BITS = 0x0030;  // 0x10 | 0x20
static const uint16_t  GAMESTATEMASK_DEFEAT_BITS  = 0x0040;

static const uint16_t PLAYER_PROPERTY_MASK_WATCH = 0x40;

// Mutual-elim detection and tiebreaker resolution must agree across peers, but
// each peer's local kill ring may contain stale events (e.g. an early-game
// asymmetric phantom from a peer-only Receive call) that arrived seconds before
// the actual game-end deaths. Only consider events within this window of the
// most-recent VICTIM_COMMANDER event when judging mutual-elim or picking the
// tiebreaker winner. A 5 s window comfortably covers dgun-cascade timing
// (commander → blast → second commander) while filtering stale events by
// orders of magnitude.
static const int64_t TIEBREAKER_RECENT_WINDOW_MS = 5000;

void CTAFStatusExporter::RunTiebreakerIfReady()
{
    if (!m_TAdynmem) return;

    // First pass: find the most-recent VICTIM_COMMANDER timestamp. Anything more
    // than TIEBREAKER_RECENT_WINDOW_MS older is treated as stale and ignored
    // for both mutual-elim detection and tiebreaker rules below.
    int64_t latestMs = 0;
    for (int i = 0; i < TAF_KILL_RING_SIZE; ++i) {
        const TAFKillEvent& ev = m_killRing[i];
        if ((ev.flags & TAF_KILL_FLAG_VICTIM_COMMANDER) == 0) continue;
        if (ev.wallClockMs > latestMs) latestMs = ev.wallClockMs;
    }
    if (latestMs == 0) return;
    const int64_t cutoffMs = latestMs - TIEBREAKER_RECENT_WINDOW_MS;

    // Mutual-elim detection by kill-event coverage (NOT UnitsNumber=0): the engine's
    // unit cleanup lags the commander-death edge by many frames, but TA's in-game
    // DEFEAT overlay fires at the death-edge — waiting for UnitsNumber would let
    // the wrong banner draw before our override lands. Trigger as soon as every
    // active non-watcher dplayId appears as a recent VICTIM_COMMANDER.
    int activeNonWatcherCount = 0;
    int diedNonWatcherCount = 0;
    for (int i = 0; i < 10; ++i) {
        const PlayerStruct& p = m_TAdynmem->Players[i];
        if (!p.PlayerActive) continue;
        const bool isWatcher = p.PlayerInfo
            && (p.PlayerInfo->PropertyMask & PLAYER_PROPERTY_MASK_WATCH) != 0;
        if (isWatcher) continue;
        ++activeNonWatcherCount;
        const uint32_t dpid = static_cast<uint32_t>(p.DirectPlayID);
        for (int j = 0; j < TAF_KILL_RING_SIZE; ++j) {
            const TAFKillEvent& ev = m_killRing[j];
            if ((ev.flags & TAF_KILL_FLAG_VICTIM_COMMANDER) == 0) continue;
            if (ev.wallClockMs < cutoffMs) continue;
            if (ev.victimDplayId == dpid) { ++diedNonWatcherCount; break; }
        }
    }
    if (activeNonWatcherCount < 2) return;
    if (diedNonWatcherCount < activeNonWatcherCount) return;
    if (m_tiebreakerWinnerDplayId == 0) {
        // Mirror of gpgnet4ta's resolveDrawTiebreaker. Rule 3 (last-man-standing)
        // intentionally omitted here — gpgnet4ta still applies it as a fallback
        // if our exported decision is empty. Both rules ignore events older than
        // the recent-window cutoff to filter stale phantoms.
        int64_t bestMs = 0;
        uint32_t winner = 0;

        // Rule 1: latest PRESUMED_DGUN victim wins (dgun-victim's team).
        for (int i = 0; i < TAF_KILL_RING_SIZE; ++i) {
            const TAFKillEvent& ev = m_killRing[i];
            if ((ev.flags & TAF_KILL_FLAG_PRESUMED_DGUN) == 0) continue;
            if (ev.victimDplayId == 0u) continue;
            if (ev.wallClockMs < cutoffMs) continue;
            if (ev.wallClockMs > bestMs) {
                bestMs = ev.wallClockMs;
                winner = ev.victimDplayId;
            }
        }

        // Rule 2: latest non-SD VICTIM_COMMANDER wins (that victim).
        if (winner == 0) {
            for (int i = 0; i < TAF_KILL_RING_SIZE; ++i) {
                const TAFKillEvent& ev = m_killRing[i];
                if ((ev.flags & TAF_KILL_FLAG_VICTIM_COMMANDER) == 0) continue;
                if ((ev.flags & TAF_KILL_FLAG_SELF_DESTRUCT) != 0) continue;
                if (ev.victimDplayId == 0u) continue;
                if (ev.wallClockMs < cutoffMs) continue;
                if (ev.wallClockMs > bestMs) {
                    bestMs = ev.wallClockMs;
                    winner = ev.victimDplayId;
                }
            }
        }

        if (winner == 0) return;
        m_tiebreakerWinnerDplayId = winner;
    }

    const uint8_t localSlot = static_cast<uint8_t>(m_TAdynmem->LocalHumanPlayer_PlayerID);
    if (localSlot >= 10) return;

    bool localWon = false;
    for (int i = 0; i < 10; ++i) {
        const PlayerStruct& p = m_TAdynmem->Players[i];
        if (!p.PlayerActive) continue;
        if (static_cast<uint32_t>(p.DirectPlayID) != m_tiebreakerWinnerDplayId) continue;
        if (i == localSlot) {
            localWon = true;
        }
        else {
            // AllyFlagAry[i][j] != 0 ⇒ slot i is allied with slot j.
            const PlayerStruct& localP = m_TAdynmem->Players[localSlot];
            if (localP.PlayerActive && localP.AllyFlagAry[i]) localWon = true;
        }
        break;
    }

    // ScoreDisplay_WinFlag drives the post-game result screen.
    m_TAdynmem->ScoreDisplay_WinFlag = localWon ? 1u : 0u;

    // GameStateMask drives the in-game banner; pinned here AND from Lock-time.
    RepinGameStateMaskBits();
}

// Re-pin GameStateMask VICTORY/DEFEAT bits per the tiebreaker outcome. Called
// from RunTiebreakerIfReady (Unlock-time), from a DrawGameScreen entry hook
// (immediately before the renderer reads — the load-bearing call), and from
// IDDrawSurface::Lock (redundant safety net). No-op until tiebreaker decides.
void CTAFStatusExporter::RepinGameStateMaskBits()
{
    if (!m_TAdynmem) return;
    if (m_tiebreakerWinnerDplayId == 0u) return;

    const uint8_t localSlot = static_cast<uint8_t>(m_TAdynmem->LocalHumanPlayer_PlayerID);
    if (localSlot >= 10) return;

    bool localWon = false;
    for (int i = 0; i < 10; ++i) {
        const PlayerStruct& p = m_TAdynmem->Players[i];
        if (!p.PlayerActive) continue;
        if (static_cast<uint32_t>(p.DirectPlayID) != m_tiebreakerWinnerDplayId) continue;
        if (i == localSlot) {
            localWon = true;
        }
        else {
            const PlayerStruct& localP = m_TAdynmem->Players[localSlot];
            if (localP.PlayerActive && localP.AllyFlagAry[i]) localWon = true;
        }
        break;
    }

    uint16_t mask = static_cast<uint16_t>(m_TAdynmem->GameStateMask);
    mask &= static_cast<uint16_t>(~(GAMESTATEMASK_VICTORY_BITS | GAMESTATEMASK_DEFEAT_BITS));
    mask |= localWon ? GAMESTATEMASK_VICTORY_BITS : GAMESTATEMASK_DEFEAT_BITS;
    m_TAdynmem->GameStateMask = static_cast<short>(mask);
}

void CTAFStatusExporter::FrameUpdate()
{
    if (!m_pView || !m_TAdynmem) return;

    TAFGameState next = {};
    next.magic = TAFGAMESTATE_MAGIC;
    for (int i = 0; i < 10; i++) {
        const PlayerStruct& p = m_TAdynmem->Players[i];

        next.playerActive[i]      = (p.PlayerActive != 0) ? 1u : 0u;
        next.playerUnitsNumber[i] = static_cast<int16_t>(p.UnitsNumber);

        if (p.PlayerActive != 0) {
            memcpy(next.playerAllyFlags[i], p.AllyFlagAry, 10);
            next.playerAllyTeam[i]     = static_cast<uint8_t>(p.AllyTeam);
            next.playerDirectPlayId[i] = static_cast<uint32_t>(p.DirectPlayID);
            if (p.PlayerInfo) {
                next.playerPropertyMask[i] = static_cast<uint16_t>(p.PlayerInfo->PropertyMask);
            }
        }
        // inactive slots leave the rest zeroed (units number too — any stale value on a
        // freshly-cleared slot would be misleading to the consumer's latch logic).
    }

    ResetTagsOnGameRestart();
    RunTiebreakerIfReady();

    next.killRingHead = m_killRingHead;
    memcpy(next.killRing, m_killRing, sizeof(next.killRing));
    next.tiebreakerWinnerDplayId = m_tiebreakerWinnerDplayId;

    ++m_framesSinceWrite;
    const bool dataChanged =
        m_prev.magic != TAFGAMESTATE_MAGIC ||
        memcmp(m_prev.playerAllyFlags,    next.playerAllyFlags,    sizeof(next.playerAllyFlags))    != 0 ||
        memcmp(m_prev.playerAllyTeam,     next.playerAllyTeam,     sizeof(next.playerAllyTeam))     != 0 ||
        memcmp(m_prev.playerActive,       next.playerActive,       sizeof(next.playerActive))       != 0 ||
        memcmp(m_prev.playerUnitsNumber,  next.playerUnitsNumber,  sizeof(next.playerUnitsNumber))  != 0 ||
        memcmp(m_prev.playerPropertyMask, next.playerPropertyMask, sizeof(next.playerPropertyMask)) != 0 ||
        memcmp(m_prev.playerDirectPlayId, next.playerDirectPlayId, sizeof(next.playerDirectPlayId)) != 0 ||
        m_prev.killRingHead != next.killRingHead ||
        m_prev.tiebreakerWinnerDplayId != next.tiebreakerWinnerDplayId;
    const bool heartbeatDue = m_framesSinceWrite >= HEARTBEAT_FRAMES;

    if (dataChanged || heartbeatDue)
    {
        TAFGameState* shm = static_cast<TAFGameState*>(m_pView);

        // Seqlock write: odd sequenceNumber signals write-in-progress to readers
        uint32_t writingSeq = m_prev.sequenceNumber + 1;  // always odd (prev was even or 0)
        shm->sequenceNumber = writingSeq;
        MemoryBarrier();

        shm->magic = next.magic;
        memcpy(shm->playerAllyFlags,    next.playerAllyFlags,    sizeof(next.playerAllyFlags));
        memcpy(shm->playerAllyTeam,     next.playerAllyTeam,     sizeof(next.playerAllyTeam));
        memcpy(shm->playerActive,       next.playerActive,       sizeof(next.playerActive));
        memcpy(shm->playerUnitsNumber,  next.playerUnitsNumber,  sizeof(next.playerUnitsNumber));
        memcpy(shm->playerPropertyMask, next.playerPropertyMask, sizeof(next.playerPropertyMask));
        memcpy(shm->playerDirectPlayId, next.playerDirectPlayId, sizeof(next.playerDirectPlayId));
        shm->killRingHead = next.killRingHead;
        memcpy(shm->killRing, next.killRing, sizeof(next.killRing));
        shm->tiebreakerWinnerDplayId = next.tiebreakerWinnerDplayId;

        MemoryBarrier();
        shm->sequenceNumber = writingSeq + 1;  // even: write complete

        m_prev = next;
        m_prev.sequenceNumber = writingSeq + 1;
        m_framesSinceWrite = 0;
    }
}
