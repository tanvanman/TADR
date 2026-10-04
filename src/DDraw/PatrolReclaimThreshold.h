#pragma once

#include "config.h"

#if PATROL_RECLAIM_THRESHOLD_ENABLE

// Per-player patrol reclaim thresholds (Escalation only).
//
// A patrolling constructor (ground: MissionTick_RepairPatrol 0x405980, air:
// MissionTick_VTOL_RepairPatrol 0x4152F0) calls a resource "low" when its
// current stock is below 0.2 * max storage (strict, per resource). Ground:
// energy not low -> look for something to assist; energy and metal both not
// low -> keep patrolling; otherwise pick a reclaimable feature. Air has no
// "both not low" step. Eight fmul operands read the 0.2 (read-only doubles at
// 0x4FC950 ground, 0x4FCC58 air).
//
// +setreclaimmetal / +setreclaimenergy <0-100>[%] set the percentage for the
// typing slot; no argument prints both. The eight operands are repointed at
// two doubles that a splice at each handler entry fills from the unit owner's
// slot. A slot never typed for holds a byte copy of the engine's 0.2, so the
// default is bit-identical to vanilla and other players and AIs are never
// affected. Reset at every game start. Energy also moves the "stop assisting"
// check, as the single constant did.
//
// PATROL_RECLAIM_AIR_GATE_ENABLE: the air handler's call to the feature picker
// (0x47EA40) is retargeted to a stub that applies the ground "both not low"
// step to air constructors. This changes default air behaviour for every
// player of the config, including air constructors set to "Reclaim Only" (the
// Hold Pos default), which vanilla never gated: they now reclaim only while a
// stock is below its threshold, as ground constructors in that mode already
// do. Thresholds reset every game; 100% for both resources restores the old
// air behaviour except while both stocks are exactly full (and, through the
// energy value, limits assisting to full energy).
//
// Cannot desync: mission handlers run only on the machine that owns the unit
// (Unit_TickAllPlayersScriptsAndWeapons skips them for remote owners), the
// thresholds are read only there, and a +command typed elsewhere is never
// executed here.
//
// Patching: SingleHook operand writes and INLINE_SINGLEJMP splices (no
// LAGGERJMP); no branch in TotalA.exe lands inside a replaced range. Not
// overlapping TADR's patrol-mode hooks (0x4059E4, 0x405B18, 0x41547D,
// 0x415621) or AutoTeam's (0x4195DD). Install checks every byte first and
// disables the module (logged) on a mismatch, self-tests before patching, and
// rolls back if a read-back differs.
//
// Two checked ranges abut hooks installed earlier: the ground step at 0x405B1E
// follows PatrolDisableReclaim (0x405B18, 6 bytes) and the registration check
// ends at 0x4195DC, where AutoTeam's hook begins. If either hook is lengthened,
// Install() reads patched bytes, logs DISABLED and patches nothing.
//
// If the recorder's OrdersOverride plugin replaced the RepairPatrol table
// entry, the ground thresholds have no effect: logged at game start, and the
// command warns.

namespace PatrolReclaimThreshold
{
	// Verifies every patched byte, self-tests, then patches. Idempotent. Logs and installs nothing
	// if anything does not match.
	void Install();

	// Restores every patched byte and clears state. Safe to call more than once.
	void Shutdown();
}

#endif // PATROL_RECLAIM_THRESHOLD_ENABLE
