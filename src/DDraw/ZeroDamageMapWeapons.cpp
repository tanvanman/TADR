#include "ZeroDamageMapWeapons.h"

#include "hook/hook.h"
#include "iddrawsurface.h"
#include "tamem.h"
#include "WeaponTdfHook.h"

#include <memory>
#include <unordered_set>

namespace
{
	const DWORD kProjectileAreaDamageCallAddr = 0x0049A0A7u;
	const DWORD kProjectileAreaDamageCallLen = 7u;
	const DWORD kProjectileAreaDamageReturnAddr = 0x0049A0AEu;
	const DWORD kNativeMinimapProjectileDrawAddr = 0x00467206u;
	const DWORD kNativeMinimapProjectileDrawLen = 8u;
	const DWORD kNativeMinimapProjectileNextAddr = 0x00467406u;

	const char kNoMapWeaponAlertKey[] = "nomapweaponalert";

	std::unique_ptr<InlineSingleHook> g_projectileAreaDamageHook;
	std::unique_ptr<InlineSingleHook> g_nativeMinimapProjectileDrawHook;
	std::unordered_set<DWORD> g_noMapWeaponAlertWeapons;

	bool IsNoMapWeaponAlertProjectile(const ProjectileStruct* projectile)
	{
		WeaponStruct* weapon = projectile ? projectile->Weapon : nullptr;

		return projectile
			&& weapon
			&& weapon->Damage == 0
			&& projectile->AttackerUnitPtr == nullptr
			&& g_noMapWeaponAlertWeapons.count((DWORD)weapon);
	}

	int __stdcall ProjectileAreaDamageCallProc(PInlineX86StackBuffer pBuf)
	{
		ProjectileStruct* projectile = reinterpret_cast<ProjectileStruct*>(pBuf->Esi);

		if (IsNoMapWeaponAlertProjectile(projectile))
		{
			pBuf->rtnAddr_Pvoid = (LPVOID)kProjectileAreaDamageReturnAddr;
			return X86STRACKBUFFERCHANGE;
		}

		return 0;
	}

	int __stdcall NativeMinimapProjectileDrawProc(PInlineX86StackBuffer pBuf)
	{
		ProjectileStruct* projectile = reinterpret_cast<ProjectileStruct*>(pBuf->Ecx);
		if (IsNoMapWeaponAlertProjectile(projectile))
		{
			pBuf->rtnAddr_Pvoid = (LPVOID)kNativeMinimapProjectileNextAddr;
			return X86STRACKBUFFERCHANGE;
		}

		return 0;
	}
}

namespace ZeroDamageMapWeapons
{
	void Install()
	{
		if (g_projectileAreaDamageHook || g_nativeMinimapProjectileDrawHook)
			return;

		WeaponTdfHook::Register([](const WeaponTdfHook::Context& ctx) {
			if (ctx.getInt(kNoMapWeaponAlertKey) & 1)
				g_noMapWeaponAlertWeapons.insert((DWORD)ctx.pWeaponDef);
		});

		g_projectileAreaDamageHook.reset(new InlineSingleHook(
			kProjectileAreaDamageCallAddr,
			kProjectileAreaDamageCallLen,
			INLINE_5BYTESLAGGERJMP,
			ProjectileAreaDamageCallProc));

		g_nativeMinimapProjectileDrawHook.reset(new InlineSingleHook(
			kNativeMinimapProjectileDrawAddr,
			kNativeMinimapProjectileDrawLen,
			INLINE_5BYTESLAGGERJMP,
			NativeMinimapProjectileDrawProc));

		IDDrawSurface::OutptTxt("[ZeroDamageMapWeapons] installed");
	}

	void Shutdown()
	{
		g_projectileAreaDamageHook.reset();
		g_nativeMinimapProjectileDrawHook.reset();
		g_noMapWeaponAlertWeapons.clear();
	}

	bool ShouldHideProjectileMarker(const ProjectileStruct* projectile)
	{
		return IsNoMapWeaponAlertProjectile(projectile);
	}
}
