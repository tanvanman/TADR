#pragma once

// OffMapAircraft -- lets anti-air see, target and kill aircraft that stock TotalA.exe treats as
// unreachable near the map edge. Two independent causes, both closed here; see
// OFFMAP_AIRCRAFT.md.
//   1. Outside the map: UNITS_RebuildFootPrint @0x47cc30 buckets the unit and stamps no tile,
//      and tile occupancy is how the engine finds every victim.
//   2. On the map but sheared off the LOS grid: CheckUnitInPlayerLOS samples
//      row = (worldZ - alt/2) >> 5, which goes negative near the top edge and fails an
//      unsigned bounds test. Only needs the visibility fix -- such a unit stamps tiles.
//
// OFFMAP_AIRCRAFT_TARGETABLE_MARGIN_TILES (config_*.h) bounds case 1 only; 0 disables the
// module. Compile-time only: it decides who can shoot what, so a per-machine override would be
// a mixed-fleet vector.
namespace OffMapAircraft
{
	// marginTiles must be > 0; the caller guards with #if. Non-positive installs nothing.
	void Install(int marginTiles);
}
