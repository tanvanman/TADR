#include "TransportedExplosions.h"

#include <cstddef>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "GameTickHook.h"
#include "UnitDefExtensions.h"
#include "hook/hook.h"
#include "iddrawsurface.h"
#include "tamem.h"

namespace
{
	const DWORD kExplosionSelectorAddr = 0x0049B017u;
	const DWORD kExplosionSelectedAddr = 0x0049B027u;
	const DWORD kPreDeathTransportCheckAddr = 0x004867BAu;
	const DWORD kPostDeathExplosionDecisionAddr = 0x00486D55u;
	const DWORD kTransportPassengerDeathAddr = 0x004867DAu;
	const DWORD kBeingTransportedByUnitOffset = 0x86u;
	const DWORD kZeroFlag = 0x40u;
	const DWORD kPendingDeathFlag = 0x4000u;

	const BYTE kExplosionSelectorBytes[] = {
		0x74, 0x08, 0x8B, 0x80, 0x24, 0x02, 0x00, 0x00,
		0xEB, 0x06, 0x8B, 0x80, 0x20, 0x02, 0x00, 0x00
	};
	const BYTE kPreDeathTransportCheckBytes[] = { 0x8B, 0x86, 0x86, 0x00, 0x00, 0x00 };
	const BYTE kPostDeathExplosionDecisionBytes[] = { 0x8A, 0x47, 0x0A, 0xA8, 0x0F };
	const BYTE kTransportPassengerDeathBytes[] = { 0x8A, 0x4B, 0x0A, 0x6A, 0x00 };

	const char kTransportedExplodeAsKey[] = "TransportedExplodeAs";
	const char kTransportedSelfDestructAsKey[] = "TransportedSelfDestructAs";

	typedef WeaponStruct* (__stdcall* FindWeaponByNameFn)(const char* weaponName);
	FindWeaponByNameFn FindWeaponByName = reinterpret_cast<FindWeaponByNameFn>(0x0049E5B0u);

	std::unique_ptr<InlineSingleHook> g_explosionSelectorHook;
	std::unique_ptr<InlineSingleHook> g_preDeathTransportCheckHook;
	std::unique_ptr<InlineSingleHook> g_postDeathExplosionDecisionHook;
	std::unique_ptr<InlineSingleHook> g_transportPassengerDeathHook;
	unsigned g_transportedExplodeAsIndex;
	unsigned g_transportedSelfDestructAsIndex;
	UnitStruct* g_pendingTransportedDeath;

	struct TransportDeathPassenger
	{
		UnitStruct* unit;
		short unitInGameIndex;
		short unitTypeId;
		short healthAtCapture;
	};

	static_assert(offsetof(UnitStruct, UnitID) == 0xA6, "UnitStruct.UnitID");
	static_assert(offsetof(UnitStruct, UnitInGameIndex) == 0xA8, "UnitStruct.UnitInGameIndex");
	static_assert(offsetof(UnitStruct, Health) == 0x108, "UnitStruct.Health");
	static_assert(offsetof(UnitStruct, UnitSelected) == 0x110, "UnitStruct.UnitSelected");

	std::vector<TransportDeathPassenger> g_transportDeathPassengers;
	int g_lastGameTime = -1;

	bool HasExpectedBytes(DWORD address, const BYTE* expected, size_t length)
	{
		if (std::memcmp(reinterpret_cast<const void*>(address), expected, length) == 0)
			return true;

		IDDrawSurface::OutptFmtTxt(
			"[TransportedExplosions] disabled: unexpected TotalA.exe bytes at 0x%08X",
			address);
		return false;
	}

	bool ValidateHookSites()
	{
		return HasExpectedBytes(kExplosionSelectorAddr, kExplosionSelectorBytes, sizeof(kExplosionSelectorBytes))
			&& HasExpectedBytes(kPreDeathTransportCheckAddr, kPreDeathTransportCheckBytes, sizeof(kPreDeathTransportCheckBytes))
			&& HasExpectedBytes(kPostDeathExplosionDecisionAddr, kPostDeathExplosionDecisionBytes, sizeof(kPostDeathExplosionDecisionBytes))
			&& HasExpectedBytes(kTransportPassengerDeathAddr, kTransportPassengerDeathBytes, sizeof(kTransportPassengerDeathBytes));
	}

	void OnGameTick(int gameTime)
	{
		if (gameTime < g_lastGameTime)
		{
			g_transportDeathPassengers.clear();
			g_pendingTransportedDeath = nullptr;
		}
		g_lastGameTime = gameTime;

		for (std::vector<TransportDeathPassenger>::iterator it = g_transportDeathPassengers.begin();
			it != g_transportDeathPassengers.end();)
		{
			UnitStruct* unit = it->unit;
			const bool identityChanged = !unit
				|| unit->UnitInGameIndex != it->unitInGameIndex
				|| unit->UnitID != it->unitTypeId
				|| !unit->UnitType;
			const bool pendingDeath = !identityChanged
				&& (unit->UnitSelected & kPendingDeathFlag) != 0;
			const bool survivedObservedDamage = !identityChanged
				&& !pendingDeath
				&& unit->Health > 0
				&& unit->Health != it->healthAtCapture;

			if (identityChanged || survivedObservedDamage)
				it = g_transportDeathPassengers.erase(it);
			else
				++it;
		}
	}

	int __stdcall PreDeathTransportCheckProc(PInlineX86StackBuffer pBuf)
	{
		UnitStruct* unit = reinterpret_cast<UnitStruct*>(pBuf->Esi);
		g_pendingTransportedDeath = unit
			&& *reinterpret_cast<void**>(reinterpret_cast<BYTE*>(unit) + kBeingTransportedByUnitOffset)
			? unit
			: nullptr;
		return 0;
	}

	int __stdcall PostDeathExplosionDecisionProc(PInlineX86StackBuffer)
	{
		g_pendingTransportedDeath = nullptr;
		return 0;
	}

	int __stdcall TransportPassengerDeathProc(PInlineX86StackBuffer pBuf)
	{
		UnitStruct* passenger = reinterpret_cast<UnitStruct*>(pBuf->Eax);
		if (!passenger || !passenger->UnitType)
			return 0;

		TransportDeathPassenger captured = {
			passenger,
			passenger->UnitInGameIndex,
			passenger->UnitID,
			passenger->Health
		};

		for (std::vector<TransportDeathPassenger>::iterator it = g_transportDeathPassengers.begin();
			it != g_transportDeathPassengers.end(); ++it)
		{
			if (it->unit == passenger)
			{
				*it = captured;
				return 0;
			}
		}
		g_transportDeathPassengers.push_back(captured);
		return 0;
	}

	bool ConsumeTransportDeathPassenger(UnitStruct* unit)
	{
		for (std::vector<TransportDeathPassenger>::iterator it = g_transportDeathPassengers.begin();
			it != g_transportDeathPassengers.end(); ++it)
		{
			if (it->unit == unit
				&& it->unitInGameIndex == unit->UnitInGameIndex
				&& it->unitTypeId == unit->UnitID)
			{
				g_transportDeathPassengers.erase(it);
				return true;
			}
			if (it->unit == unit)
			{
				g_transportDeathPassengers.erase(it);
				return false;
			}
		}
		return false;
	}

	int __stdcall ExplosionSelectorProc(PInlineX86StackBuffer pBuf)
	{
		UnitStruct* unit = reinterpret_cast<UnitStruct*>(pBuf->Ecx);
		UnitDefStruct* unitDef = reinterpret_cast<UnitDefStruct*>(pBuf->Eax);
		const bool selfDestruct = (pBuf->EFlags_Dw & kZeroFlag) == 0;
		if (!unit || !unitDef)
			return 0;

		WeaponStruct* selectedWeapon = selfDestruct
			? unitDef->SelfeDestructAs
			: unitDef->ExplodeAs;

		const bool currentlyTransported =
			*reinterpret_cast<void**>(reinterpret_cast<BYTE*>(unit) + kBeingTransportedByUnitOffset) != nullptr;
		const bool wasTransportedAtDeath = unit == g_pendingTransportedDeath;
		const bool wasPassengerOfDestroyedTransport = ConsumeTransportDeathPassenger(unit);
		g_pendingTransportedDeath = nullptr;

		if (currentlyTransported || wasTransportedAtDeath || wasPassengerOfDestroyedTransport)
		{
			const unsigned keyIndex = selfDestruct
				? g_transportedSelfDestructAsIndex
				: g_transportedExplodeAsIndex;
			const std::string& weaponName = UnitDefExtensions::GetInstance()->getString(unitDef->UnitTypeID, keyIndex);

			if (!weaponName.empty())
			{
				WeaponStruct* transportedWeapon = FindWeaponByName(weaponName.c_str());
				if (transportedWeapon)
				{
					selectedWeapon = transportedWeapon;
					IDDrawSurface::OutptFmtTxt(
						"[TransportedExplosions] unit=%s selfDestruct=%u passenger=%u weapon=%s",
						unitDef->UnitName,
						selfDestruct ? 1u : 0u,
						wasPassengerOfDestroyedTransport ? 1u : 0u,
						weaponName.c_str());
				}
				else
				{
					IDDrawSurface::OutptFmtTxt(
						"[TransportedExplosions] weapon '%s' not found for unit %s; using stock explosion",
						weaponName.c_str(),
						unitDef->UnitName);
				}
			}
		}

		pBuf->Eax = reinterpret_cast<DWORD>(selectedWeapon);
		pBuf->rtnAddr_Pvoid = reinterpret_cast<LPVOID>(kExplosionSelectedAddr);
		return X86STRACKBUFFERCHANGE;
	}
}

void TransportedExplosions::Install()
{
	if (g_explosionSelectorHook)
		return;
	if (!ValidateHookSites())
		return;

	UnitDefExtensions* extensions = UnitDefExtensions::GetInstance();
	g_transportedExplodeAsIndex = extensions->registerStringKey(kTransportedExplodeAsKey, "");
	g_transportedSelfDestructAsIndex = extensions->registerStringKey(kTransportedSelfDestructAsKey, "");
	g_transportDeathPassengers.reserve(16);
	IDDrawSurface::OutptTxt("[TransportedExplosions] installed");

	g_preDeathTransportCheckHook.reset(new InlineSingleHook(
		kPreDeathTransportCheckAddr,
		6,
		INLINE_5BYTESLAGGERJMP,
		PreDeathTransportCheckProc));

	g_postDeathExplosionDecisionHook.reset(new InlineSingleHook(
		kPostDeathExplosionDecisionAddr,
		5,
		INLINE_5BYTESLAGGERJMP,
		PostDeathExplosionDecisionProc));

	g_transportPassengerDeathHook.reset(new InlineSingleHook(
		kTransportPassengerDeathAddr,
		5,
		INLINE_5BYTESLAGGERJMP,
		TransportPassengerDeathProc));

	GameTickHook::GetInstance()->addCallback(OnGameTick);

	g_explosionSelectorHook.reset(new InlineSingleHook(
		kExplosionSelectorAddr,
		0x10,
		INLINE_5BYTESLAGGERJMP,
		ExplosionSelectorProc));
}
