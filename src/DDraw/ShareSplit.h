#pragma once

#include "config.h"

#if SHARE_SPLIT_ENABLE

// Fair split of the auto-share surplus among allies (Escalation only).
//
// Vanilla (Share_AutoAllyResourcesAndMap 0x457D30, called once per simulation step from
// Game_RunSimulationStep 0x49563A with the local player) sends, every 60 ticks and per resource, a
// third (metal) or half (energy) of the stock above the share threshold to ONE ally: the last slot
// whose stock is lower than the sender's. Free storage is never compared, so a full ally can be
// picked and the surplus is wasted, and with two qualifying allies the higher slot takes all.
//
// This module replaces the resource half with a water-filling split of the same pool
// ((stock - threshold) * K; K = 1/3 metal, 1/2 energy, read from 0x4FD4B8 / 0x4FD4BC): equal shares
// among every eligible ally, each capped at its free storage (max - stock); when an uncapped share
// would be below 1 unit, the fullest receiver is dropped and the split redone. Eligible receiver:
// remote (+0x73 == 3) human (PlayerInfo+0x94 == 1), allied both ways (me+0x108[i] && me+0x113[i]),
// not a spectator (PlayerInfo+0x9B & 0x40), alive, +0x22 == 0, economy object +0xEC present, at
// least 1 unit of free storage, and less full than the sender (stock/max). The sender additionally
// must be alive, not a spectator and +0x22 == 0 (vanilla debits first and the packet path then
// refuses, so the resources would be lost). Each give is the engine's own
// Player_Transfer{Metal,Energy}Resource (0x464C60 / 0x464B30) with sendPacket 1, in slot order, so
// debit, credit and the 0x16 packet are vanilla's. Same gates as vanilla: WorkStatusMask bit 0,
// GameTime % 60 == 0; nothing while a replay plays (the recorded transfers arrive as packets). The
// map-share half (every 450 ticks) is unchanged.
//
// Only the sender's client runs this; the amount travels in the packet and the receiver applies it
// with sendPacket 0, so clients without the module interoperate (argued from the packet path, not
// tested with two clients).
//
// Patches (both verified before writing and read back after; any mismatch restores everything):
//   0x457D5F  `jne 0x457FA1` -> `jmp 0x457FA1`: vanilla's resource half becomes unreachable.
//   0x49563B  rel32 of `call 0x457D30` in Game_RunSimulationStep -> ShareSplitStep, which runs the
//             split and then calls 0x457D30 for the map half.

namespace ShareSplit
{
	// Verifies every relied-on byte, runs the self-test, then patches. Idempotent. On any mismatch it
	// logs one DISABLED line and patches nothing.
	void Install();

	// Restores every patched byte. Safe to call more than once.
	void Shutdown();
}

#endif // SHARE_SPLIT_ENABLE
