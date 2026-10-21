#pragma once

#include "config.h"

#if GIVE_COMMANDS_ENABLE

// +givemetal / +giveenergy <player> <amount> and a fixed +give (Escalation only).
//
//   +givemetal  <player> <amount>
//   +giveenergy <player> <amount>
//   +give       <player> <amount> metal|energy        (vanilla's argument order)
//
// <player>: exact name (Name or SecondName, case-insensitive, spaces allowed), else a slot number 0-9,
// else a unique case-insensitive prefix of Name (ambiguous: up to 3 candidates are listed). <amount>:
// digits only, >= 1, saturating at 1e9. Sent = min(amount, giver's stock, receiver's free storage).
// Every result and refusal is a local chat line (wrapped at 63 characters, the HUD's line length).
//
// Vanilla +give (Cmd_GiveResource 0x416BD0) takes the slot from atoi and compares only its low byte
// (256 -> slot 0, a name -> slot 0), does not validate the amount, and has no ally, self, room or alive
// check and no feedback. This module registers its own table on every map load through an InlineSingleHook at
// 0x4195C9 (the registration routine, before the second built-in table): the record `Give` (same name
// and case as vanilla's) overwrites vanilla's handler in the registrar; `GiveMetal` / `GiveEnergy` are
// new. All three are run level 1, as vanilla's Give.
//
// Refused: self; receiver not allied by the giver (me+0x108); receiver inactive, spectator, dead,
// without economy object, slot/index mismatch, or (remote) +0x22 != 0; giver without economy object, or
// (to a remote receiver) dead, spectator or +0x22 != 0 -- the packet path would refuse after the debit;
// receiver with < 1 free storage; giver with < 1 stock; outside a game; during a replay. A local AI
// (skirmish) can receive; the engine scales that credit by its difficulty rule. The typed line itself is
// still broadcast as chat (engine behaviour).
//
// The give is the engine's Player_Transfer{Metal,Energy}Resource (0x464C60 / 0x464B30) with sendPacket
// 1: same debit, credit and 0x16 packet as vanilla, so clients without the module interoperate.
//
// Receive guard (GIVE_RECEIVE_GUARD_ENABLE): the engine applies the amount of a received 0x16 resource
// packet without validating it (0 and NaN are no-ops only through the transfer's own zero test). An
// InlineSingleHook on each of the metal and energy arms of the receive switch (0x4558C2, 0x4558E0; the
// hook covers the arm's first two instructions, 8 bytes) tests the float bits with integer instructions
// only (valid: 0 < bits < 0x7F800000, positive and finite) and sends an invalid packet through the case's
// own exit (0x455F50), as if it had not arrived. Valid packets run vanilla's code.
// Protects the receiving client only; a modified sender can still credit itself on its own machine.

namespace GiveCommands
{
	// Verifies every relied-on byte, runs the self-test, then patches the registration site (and the two
	// receive arms). Idempotent. On any mismatch it logs one DISABLED line and patches nothing.
	void Install();

	// Restores every patched byte. Safe to call more than once.
	void Shutdown();
}

#endif // GIVE_COMMANDS_ENABLE
