#pragma once

#include <memory>
#include "hook/hook.h"

// Patches TotalA.exe to support the "surfacefire" and "nottounderwater" weapon TDF keys.
// These are complementary waterweapon-targeting modifiers, so they share one module to keep
// their interacting hooks (and the reject-address coordination between them) in one place:
//   surfacefire=1     a waterweapon can also target/fire at surface units regardless of depth.
//   nottounderwater=1 the weapon cannot target/fire at fully-submerged units (topY <= sea level),
//                     the underwater mirror of nottoair. Only meaningful for waterweapons; plain
//                     weapons already reject submerged targets natively (0x49ACF7).
//
// Hooks:
//   WeaponTdfHook (shared): reads "surfacefire" and "nottounderwater" from each weapon TDF and
//     records the weapon def in the matching set.
//   UnitAutoAim_CheckUnitWeapon @ 0x49AC0F: bypasses REJECT 1 (plain land/surface units
//     with no sub flag and Y > sea level), redirecting to the range check at 0x49AC47.
//   UnitAutoAim_CheckUnitWeapon @ 0x49AC20: bypasses REJECT 2 (units with UnitTypeMask_0
//     & 0x1000 e.g. hovercraft), also redirecting to the range check at 0x49AC47.
//   UnitAutoAim_CheckUnitWeapon @ 0x49AC47: nottounderwater gate at the range-check
//     convergence; rejects fully-submerged targets (redirect to 0x49AC3B, NOT 0x49AC0F
//     which this module also hooks -- see SurfaceFire.cpp for the ping-pong-freeze note).
//   WeaponCanAim @ 0x49AB18: allows surfacefire weapons to aim regardless of firer depth.
//   ScriptAction_Type2Index @ 0x43F24F: allows a submarine to issue an ATTACK COB action
//     at a surface target when its weapon0 has surfacefire=1.
//   ProjectilesEngine @ 0x49B9EB: allows surfacefire missile guidance above sea level.
//     Without this, waterweapon+twophase missiles have guidance killed when above sea level
//     (gravity-only, elevation zeroed, Projectile_Cruise/Turn skipped).
//
// Terrain/projectile detonation notes:
//   The waterweapon pass-through in ProjectileUnitCollisionDetection @ 0x49B3A1 fires when
//   proj_Y >= terrain_height (projectile is above the terrain surface). This must be left
//   intact for surfacefire torpedoes too — an underwater torpedo is always above the sea
//   floor as it travels, and detonating there would cause instant explosion.
//   Detonation on the sea floor or dry terrain occurs via the proj_Y < terrain_height branch
//   of the same function, which has no waterweapon check and fires for all weapons.
class SurfaceFire
{
public:
	static void Install();

private:
	SurfaceFire();
	~SurfaceFire();
	static SurfaceFire* m_instance;
	std::unique_ptr<InlineSingleHook> m_rejectHook;      // REJECT 1 @ 0x49AC0F
	std::unique_ptr<InlineSingleHook> m_weaponCheckHook; // REJECT 2 @ 0x49AC20
	std::unique_ptr<InlineSingleHook> m_canAimHook;
	std::unique_ptr<InlineSingleHook> m_scriptActionHook;
	std::unique_ptr<InlineSingleHook> m_guidanceHook;
	std::unique_ptr<InlineSingleHook> m_rangeCheckHook;   // nottounderwater gate @ 0x49AC47
	static int __stdcall CheckRouter(PInlineX86StackBuffer pBuf);
	static int __stdcall CanAimRouter(PInlineX86StackBuffer pBuf);
	static int __stdcall ScriptActionRouter(PInlineX86StackBuffer pBuf);
	static int __stdcall GuidanceRouter(PInlineX86StackBuffer pBuf);
	static int __stdcall RangeCheckRouter(PInlineX86StackBuffer pBuf);
};
