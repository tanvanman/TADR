#pragma once

#include "config.h"

#if SHARE_PERCENT_ENABLE

// Percentage-based resource share thresholds (Escalation only).
//
// +setsharemetal/+setshareenergy store an ABSOLUTE float (ShareMetal/
// ShareEnergy) that is never re-derived when max storage changes, so a
// threshold set early in a game becomes a much smaller fraction of storage
// later. A `%` suffix (`+setshareenergy 50%`) arms a percentage that is kept
// in sync with max storage every tick; a plain integer clears percentage
// mode and behaves exactly like vanilla (so Alt+Shift+Z and existing chat
// presets keep working).
//
// Cannot desync: ShareMetal/ShareEnergy are read only by
// Share_AutoAllyResourcesAndMap (0x00457D30) for the LOCAL player, and the
// resulting transfer is sent over the network as an explicit float --
// nothing recomputes it from a threshold, and a `+command` a client
// receives is never executed by the receiver.
//
// Both command handlers are replaced whole via SingleHook + INLINE_SINGLEJMP
// (plain E9 jmp + NOP pad, no trampoline/relocator involved). Verified no
// direct references land inside the replaced bytes and the handler's return
// value is unused by the dispatcher.
//
// Vanilla limitation, not affected by this module: auto-share only ever
// transfers to a My_PlayerType == 3 (RemoteHuman) recipient, so a local AI
// ally can never receive a share regardless of threshold -- exercising this
// needs a second human client (`+ai` does not help; see SharePercent.cpp).

namespace SharePercent
{
	// Installs both command hooks and the per-tick refresh callback.
	// Idempotent; skips installation (logged) if TotalA.exe doesn't match
	// the expected build.
	void Install();

	// Restores both handlers and clears state. Safe to call more than once.
	void Shutdown();

	// -1 = absolute mode. 0..100 = the armed percentage.
	int GetMetalPercent();
	int GetEnergyPercent();

	// Percentage the SHARE dialog should show: the armed percentage, or the
	// current absolute threshold expressed as a percentage of max storage.
	int GetDisplayPercent(bool isMetal);
}

#endif // SHARE_PERCENT_ENABLE
