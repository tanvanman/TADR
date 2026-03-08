#pragma once

#include <memory>
#include "hook/hook.h"

// Patches TotalA.exe to support the "nottoair" weapon TDF key (bit 31 of WeaponTypeMask).
// A weapon with nottoair=1 cannot target or fire at flying units.
//
// Two hooks:
//   LoadWeaponTdf @ 0x42E4AB: reads "nottoair" from the TDF and sets WTM_NotToAir.
//   UnitAutoAim_CheckUnitWeapon @ 0x49AD07: rejects flying targets for nottoair weapons.
class NotToAir
{
public:
	static void Install();

private:
	NotToAir();
	~NotToAir();
	static NotToAir* m_instance;
	std::unique_ptr<InlineSingleHook> m_weaponCheckHook;
	std::unique_ptr<InlineSingleHook> m_tdfParseHook;
	static int __stdcall CheckRouter(PInlineX86StackBuffer pBuf);
	static int __stdcall ParseRouter(PInlineX86StackBuffer pBuf);
};
