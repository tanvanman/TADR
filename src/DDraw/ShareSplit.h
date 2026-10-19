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
// This module redirects the resource half to a water-filling split: the same pool
// ((stock - threshold) * K, K read from the exe) is divided in equal shares among every eligible
// ally, each capped at its free storage; a share below one unit is folded into fewer, emptier
// receivers. Eligible = a human (PlayerInfo type 1) other than the sender, allied BOTH ways
// (me+0x108[i] and me+0x113[i]), not a spectator, alive, able to receive (+0xEC economy object,
// +0x22 == 0), and LESS FULL than the sender (stock/max). Each transfer is the engine's own
// Player_Transfer{Metal,Energy}Resource with sendPacket 1, so crediting, AI scaling and the 0x16
// packet are vanilla's. The map-share half (kind 3, every 450 ticks) still runs untouched.
//
// Cannot desync: only the owner's client decides, the amount travels in the packet and is applied
// with sendPacket 0 on the other side; a client without this module sends vanilla transfers and
// receives ours like any other.
//
// Patching (two writes, both verified and read back; any mismatch rolls everything back):
//   0x457D5F  `jne 0x457FA1` -> `jmp 0x457FA1` (the resource half becomes dead code, the map half runs)
//   0x49563B  the rel32 of `call 0x457D30` -> ShareSplitStep, which applies vanilla's gates and runs
//             the split, then calls 0x457D30 for the map half.
// Nothing else in the exe or in other modules is touched. See ai-reference/share-resources-split/.

namespace ShareSplit
{
	// Verifies every patched byte and every premise, self-tests, then patches. Idempotent. Logs and
	// installs nothing if anything does not match.
	void Install();

	// Restores every patched byte. Safe to call more than once.
	void Shutdown();
}

#endif // SHARE_SPLIT_ENABLE
