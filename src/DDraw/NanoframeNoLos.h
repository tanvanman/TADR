#pragma once

#include "config.h"

#if NANOFRAME_NO_LOS_ENABLE

// A unit under construction (Unit+0x104, the build fraction remaining, not 0) adds nothing to any
// player's sight counts (player+0x7C) or mapping, and does not leave a death eye. It gets its sight the
// next time the engine updates it after it completes (every tick, Game_TickPlayersAndWatchMode 0x465053).
// This changes the simulation: automatic targeting only considers enemies visible to the owner
// (Unit_FindBestTargetInRange 0x40B7B0 via Ai_RebuildUnitTblLists 0x40AA40).
//
// State: a unit is "suppressed" (nothing of it is in any count or mapping) when both shorts of its sight
// cache, Unit+0x7A and +0x7C, are 0x7FFF; +0xF8 is then 0. The engine never produces that cell: true LOS
// derives it from (pos.x >> 16) >> 5, circular LOS from pos.x / 0x200000 minus a GAF frame offset.
//
// Five splices, every one a 5-byte jump to a stub in this module:
//   0x482AC0 Los_RevealNewUnit (spawn)                  nanoframe -> suppress, return
//   0x4827B0 Los_UpdateUnitFromPosition (tick, moves)   suppressed nanoframe -> return;
//                                                       suppressed and complete -> register it;
//                                                       registered nanoframe -> unregister, suppress
//   0x482090 Los_DecUnitFogSightCounts (death)          suppressed -> return
//   0x481796 Los_RebuildAllVisibilityMasks, unit loop   nanoframe -> suppress, next unit
//   0x486748 Unit_HandleKilledPacket, call AddEyeball   nanoframe -> no eye
// Every gate runs before the generic Add 0x482270 / Remove 0x481D50 / Reveal 0x481930, so their
// increments and decrements stay paired, including the recorder's .sharelos patches inside Add and
// Remove (src/Recorder/plugins/LOS_UpdateTables.pas), which copy a unit's sight to its allies' counts.
//
// Known residue: Unit_TransferOwnership 0x488570 creates the receiver's unit complete and copies +0x104
// afterwards (0x48873D), so a nanoframe given, shared, captured or taken counts for the receiver until
// its next update, and with mapping on it maps its sight area for the receiver once.
//
// Install() patches nothing unless every checked span holds the expected bytes, and reads every splice
// back; on any mismatch it logs DISABLED and leaves the engine untouched.

namespace NanoframeNoLos
{
	// Idempotent.
	void Install();

	// Restores the engine's bytes. Safe to call more than once.
	void Shutdown();
}

#endif // NANOFRAME_NO_LOS_ENABLE
