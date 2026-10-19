#pragma once

#include "config.h"

#if GIVE_COMMANDS_ENABLE

// +givemetal / +giveenergy <player> <amount> and a fixed +give (Escalation only).
//
//   +givemetal  <player> <amount>
//   +giveenergy <player> <amount>
//   +give       <player> <amount> metal|energy        (vanilla's argument order)
//
// <player> is an exact name (case-insensitive, spaces allowed), else a slot number 0-9, else a unique
// prefix of a name. <amount> is a whole number of at least 1 (digits only). The amount actually sent is
// capped at the giver's stock and at the receiver's free storage; the chat line says so.
//
// Vanilla +give (Cmd_GiveResource 0x416BD0) reads the slot with atoi and tests only the low byte (256
// -> slot 0), so a name means slot 0; it has no ally, self, sign, room or alive checks and prints
// nothing. This module registers its own table on every map load through a splice in the command
// registration routine (0x4195C9); the exact-case name `Give` replaces vanilla's handler in the
// registrar (a same-name, same-case entry overwrites), `GiveMetal` and `GiveEnergy` are new. Every
// refusal and result is a local chat line.
//
// Refused: the giver or the receiver is not an active, living, non-spectating player that can use the
// transfer path (a transfer to a player the network path would refuse is debited and lost); receiver
// not allied BY THE GIVER; self; no free storage. Works in skirmish (credit to a local AI is scaled by
// the engine's own difficulty rule). Does nothing while watching a replay. The typed line is still
// broadcast to everyone as chat (engine behaviour).
//
// The transfer is the engine's own Player_Transfer{Metal,Energy}Resource with sendPacket 1, so the
// amount travels in the 0x16 packet exactly as vanilla's did: no desync, interoperable with clients
// without this module. See ai-reference/share-resources-split/.
//
// Receive guard (GIVE_RECEIVE_GUARD_ENABLE): the engine applies a received 0x16 resource packet with no
// sign check. A zero or NaN amount is ignored by the transfer's own zero test, but a NEGATIVE or infinite
// amount is credited exactly as received, so a peer can drain or flood the receiver's stock. The two arms
// of the packet switch that read the amount (metal 0x4558C2, energy 0x4558E0) are spliced (8 bytes each) to
// a stub that tests the float bits with integer arithmetic only and leaves through the arm's own exit when
// the amount is not positive and finite. It protects the receiving client's own view; it cannot stop a
// cheater's client from crediting itself. Valid packets take exactly vanilla's path.

namespace GiveCommands
{
	// Verifies every premise and self-tests, then patches the registration site. Idempotent. Logs and
	// installs nothing if anything does not match.
	void Install();

	// Restores the registration site. Safe to call more than once.
	void Shutdown();
}

#endif // GIVE_COMMANDS_ENABLE
