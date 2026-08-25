#pragma once

#include <memory>
#include "hook/hook.h"

// Terrain-based fire gates. Two complementary weapon TDF keys, both about where the
// FIRING unit is rather than where the target is:
//   notoverwater=1  the weapon will not fire while the firer is over water
//   notoverland=1   the weapon will not fire while the firer is over land
// A weapon carrying both never fires at all.
//
// Hook:
//   UNITS_AutoAim @ 0x49E1D6: skip this weapon slot entirely, jumping to the loop
//     increment at 0x49E541. That kills trajectory calc, the COB Aim script (so the
//     turret stops tracking), the fire callback and the resource debit in one place --
//     and costs less than the vanilla path it replaces. Placed after the reload
//     decrement at 0x49E1C8, so the weapon still reloads while suppressed and is hot
//     the moment the unit crosses the coast.
//
// Not hooked at WeaponCanAim @ 0x49AB18, the obvious spot: SurfaceFire already owns
// that address, and gating there would leave the turret tracking anyway.
class TerrainFireGate
{
public:
	static void Install();

private:
	TerrainFireGate();
	~TerrainFireGate();
	static TerrainFireGate* m_instance;
	static void InstallHooks();
	std::unique_ptr<InlineSingleHook> m_autoAimHook;
	static int __stdcall FireGateRouter(PInlineX86StackBuffer pBuf);
};
