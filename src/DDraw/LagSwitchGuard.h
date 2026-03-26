#pragma once

#include "HudNotifications.h"
#include "hook/hook.h"

#include <memory>
#include <windows.h>

// Lag-switch countermeasure: detects when the local player's network goes dark
// (no packets from ANY remote peer) and freezes the local simulation so the
// cheater cannot maneuver while immune to incoming damage.
//
// Detection: Each render frame, compares each remote human player's
// LastMsgTimeStamp against a cached copy.  When the timestamp advances,
// records the wall-clock time (GetTickCount).  If ALL remote humans go silent
// for longer than FREEZE_THRESHOLD_MS, the simulation is frozen.
//
// Enforcement: DeltaTime hook at 0x4967f8 (after CALL ApplyDeltaTime in
// GameTickFunction).  While frozen, forces DeltaTime=0 to suppress
// simulation.  Every TICK_LEAK_INTERVAL_MS, forces DeltaTime=1 instead
// so Packet_Dispatcher runs and updates LastMsgTimeStamp (avoids
// detection deadlock).  The player has no way to override DeltaTime.
//
// Skips detection when IsGamePaused is set (player-initiated pause also
// stops LastMsgTimeStamp from advancing, which would false-trigger).
class LagSwitchGuard
{
public:
	static LagSwitchGuard* GetInstance();

	// Called from the GUI thread (IDDrawSurface::Unlock) each render frame.
	void Tick();

	bool IsFrozen() const { return m_frozen; }

	static int __stdcall PostApplyDeltaTimeRouter(PInlineX86StackBuffer pBuf);
	static int __stdcall PauseKeyRouter(PInlineX86StackBuffer pBuf);

private:
	LagSwitchGuard();

	static LagSwitchGuard* m_instance;
	std::unique_ptr<InlineSingleHook> m_postApplyDeltaTimeHook;
	std::unique_ptr<InlineSingleHook> m_pauseKeyHook;

	// Per-player tracking: wall-clock time of last observed LastMsgTimeStamp change
	struct PlayerTrack {
		int  lastSeenTimestamp;   // cached PlayerStruct.LastMsgTimeStamp value
		DWORD lastReceiveTickMs;  // GetTickCount() when we last saw it change
	};
	PlayerTrack m_playerTrack[10];

	bool  m_frozen;
	DWORD m_frozenSinceMs;
	DWORD m_lastTickLeakMs;    // GetTickCount() when we last let a tick through while frozen
	HudLineId m_hudLineId;

	// Transient "resumed" notification
	HudLineId m_resumeLineId;
	DWORD     m_resumeLineExpiry;

	static const DWORD FREEZE_THRESHOLD_MS   = 500;
	static const DWORD RESUME_NOTICE_MS      = 5000;
	static const DWORD TICK_LEAK_INTERVAL_MS = 500;
};
