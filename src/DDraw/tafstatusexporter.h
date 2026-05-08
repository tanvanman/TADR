#pragma once
#include "tafgamestate.h"
#include "tamem.h"
#include <windows.h>

// Install engine hooks for dgun-vs-COMMANDER_BLAST disambiguation. Call once
// at DLL init (NOT per-game) — the InlineSingleHook objects must outlive any
// individual game session. Per-game state is reset internally on game restart.
void InstallDgunDetectHooks();

class CTAFStatusExporter
{
public:
    explicit CTAFStatusExporter(TAdynmemStruct* dynmem);
    ~CTAFStatusExporter();

    // Call once per rendered frame from IDDrawSurface::Unlock().
    void FrameUpdate();

    // Append a kill event. Called from UnitsReceiveUnitDeathHookProc.
    void EmitKillEvent(uint32_t victimDpid, uint32_t killerDpid, uint16_t flags);

    // Re-pin GameStateMask VICTORY/DEFEAT bits per the tiebreaker outcome.
    // Called from RunTiebreakerIfReady, from a DrawGameScreen entry hook
    // (the load-bearing call site), and from IDDrawSurface::Lock as a
    // redundant safety net.
    void RepinGameStateMaskBits();

private:
    // Detect game-restart via unit-array-bounds change and reset per-unit tag
    // tables. Kill events themselves come from the death-source hook, not here.
    void ResetTagsOnGameRestart();

    // Detect mutual-elim, run the same tiebreaker rules gpgnet4ta uses, and
    // override ScoreDisplay_WinFlag + GameStateMask so the local player sees
    // the correct verdict. Each peer reaches its conclusion from local data
    // (kill ring + ally state) — no cross-peer collusion.
    void RunTiebreakerIfReady();

    TAdynmemStruct* m_TAdynmem;
    HANDLE          m_hMap;
    void*           m_pView;
    TAFGameState    m_prev;             // last-written snapshot for change detection
    int             m_framesSinceWrite; // heartbeat — force a write periodically so
                                        // gpgnet4ta's staleness check doesn't fall
                                        // back to packet inference during quiet play

    uint32_t        m_killRingHead;
    TAFKillEvent    m_killRing[TAF_KILL_RING_SIZE];

    // 0 = undecided. Once set, persists for the rest of the game.
    uint32_t        m_tiebreakerWinnerDplayId;

    // Used by ResetTagsOnGameRestart to detect game-restart (bounds change ⇒ new
    // session) so per-unit-pointer tag tables are reset before addresses recycle.
    UnitStruct*     m_lastUnitArrayBegin;
    UnitStruct*     m_lastUnitArrayEnd;
};
