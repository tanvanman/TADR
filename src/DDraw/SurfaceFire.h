#pragma once

#include <memory>
#include "hook/hook.h"

// Patches TotalA.exe to support the "surfacefire" weapon TDF key.
// A waterweapon with surfacefire=1 can target and fire at any unit regardless of depth.
//
// Hooks:
//   WeaponTdfHook (shared): reads "surfacefire" from each weapon TDF and records the weapon def.
//   UnitAutoAim_CheckUnitWeapon @ 0x49AC0F: bypasses REJECT 1 (plain land/surface units
//     with no sub flag and Y > sea level), redirecting to the range check at 0x49AC47.
//   UnitAutoAim_CheckUnitWeapon @ 0x49AC20: bypasses REJECT 2 (units with UnitTypeMask_0
//     & 0x1000 e.g. hovercraft), also redirecting to the range check at 0x49AC47.
//   WeaponCanAim @ 0x49AB18: allows surfacefire weapons to aim regardless of firer depth.
//   ScriptAction_Type2Index @ 0x43F24F: allows a submarine to issue an ATTACK COB action
//     at a surface target when its weapon0 has surfacefire=1.
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
	static int __stdcall CheckRouter(PInlineX86StackBuffer pBuf);
	static int __stdcall CanAimRouter(PInlineX86StackBuffer pBuf);
	static int __stdcall ScriptActionRouter(PInlineX86StackBuffer pBuf);
};
