#include "AlliedProjectileCollision.h"

#include <windows.h>
#include <cstddef>
#include <cstring>
#include <memory>

#include "config.h"
#include "hook/hook.h"
#include "iddrawsurface.h"
#include "tamem.h"
#include "tafunctions.h"

namespace
{
	// Retail 3.1 / current ProTA TotalA.exe: SHA256
	// 3b9c0fadabf3dc67ed5f05a70f1e1505a0c65deadd1a3c930adfe30e2a84995e.
	// Validate the entire two-slot decision block before installing either hook:
	// includes global/unit-array loads, owner tests, height tests and continuations.
	const DWORD kDecisionStart = 0x0049B1C7u;
	const BYTE kDecisionExpected[] = {
		0x66, 0x8B, 0x03, 0x8B, 0x3D, 0xE8, 0x1D, 0x51, 0x00, 0x66, 0x85, 0xC0, 0x74, 0x4D, 0x25, 0xFF,
		0xFF, 0x00, 0x00, 0x8B, 0xC8, 0xC1, 0xE0, 0x03, 0x2B, 0xC1, 0x8D, 0x14, 0x80, 0x8B, 0x87, 0x57,
		0x43, 0x01, 0x00, 0x8A, 0x8C, 0xD0, 0xFF, 0x00, 0x00, 0x00, 0x8D, 0x04, 0xD0, 0x3A, 0x4E, 0x66,
		0x74, 0x29, 0x8B, 0x90, 0x92, 0x00, 0x00, 0x00, 0x8B, 0x68, 0x6E, 0x8B, 0x8A, 0x6E, 0x01, 0x00,
		0x00, 0x8B, 0x56, 0x08, 0x03, 0xCD, 0x3B, 0xD1, 0x7D, 0x11, 0x50, 0x56, 0xE8, 0x98, 0xEC, 0xFF,
		0xFF, 0x5F, 0x5E, 0x5D, 0x5B, 0x83, 0xC4, 0x18, 0xC2, 0x08, 0x00, 0x66, 0x8B, 0x43, 0x02, 0x66,
		0x85, 0xC0, 0x74, 0x5F, 0x25, 0xFF, 0xFF, 0x00, 0x00, 0x8B, 0xC8, 0xC1, 0xE0, 0x03, 0x2B, 0xC1,
		0x8D, 0x14, 0x80, 0x8B, 0x87, 0x57, 0x43, 0x01, 0x00, 0x8A, 0x8C, 0xD0, 0xFF, 0x00, 0x00, 0x00,
		0x8D, 0x04, 0xD0, 0x3A, 0x4E, 0x66, 0x74, 0x3B, 0x8B, 0x88, 0x92, 0x00, 0x00, 0x00, 0x8B, 0x50,
		0x6E, 0x8B, 0x7E, 0x08, 0x8B, 0xA9, 0x62, 0x01, 0x00, 0x00, 0x03, 0xEA, 0x3B, 0xFD, 0x7C, 0x1D,
		0x8B, 0x89, 0x6E, 0x01, 0x00, 0x00, 0x03, 0xCA, 0x3B, 0xF9, 0x7F, 0x11, 0x50, 0x56, 0xE8, 0x36,
		0xEC, 0xFF, 0xFF, 0x5F, 0x5E, 0x5D, 0x5B, 0x83, 0xC4, 0x18, 0xC2, 0x08, 0x00, 0x8B, 0x3D, 0xE8,
		0x1D, 0x51, 0x00,
	};
	const DWORD kGroundHook = 0x0049B1F9u;
	const DWORD kAirHook = 0x0049B24Fu;
	const DWORD kGroundSkip = 0x0049B222u;
	const DWORD kAirSkip = 0x0049B28Au;
	std::unique_ptr<InlineSingleHook> g_groundHook;
	std::unique_ptr<InlineSingleHook> g_airHook;
	bool g_installed = false;
	bool g_installFailed = false;

	bool MatchesEngine()
	{
		__try
		{
			const BYTE* base = reinterpret_cast<const BYTE*>(GetModuleHandleA(NULL));
			if (reinterpret_cast<DWORD>(base) != 0x00400000u)
				return false;
			const IMAGE_DOS_HEADER* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
			if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0)
				return false;
			const IMAGE_NT_HEADERS32* nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(base + dos->e_lfanew);
			return nt->Signature == IMAGE_NT_SIGNATURE
				&& nt->FileHeader.Machine == IMAGE_FILE_MACHINE_I386
				&& nt->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC
				&& nt->OptionalHeader.SizeOfImage >= kAirSkip - 0x00400000u
				&& std::memcmp(reinterpret_cast<const void*>(kDecisionStart),
					kDecisionExpected, sizeof(kDecisionExpected)) == 0;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return false;
		}
	}

	int __stdcall GroundCollision(PInlineX86StackBuffer state)
	{
		// Native same-owner JE has already run. EAX is the candidate UnitStruct*,
		// ESI the projectile. Skip to the next slot before any impact/effect/damage.
		if (!AlliedProjectileCollision::ShouldPassThrough(
			reinterpret_cast<const ProjectileStruct*>(state->Esi),
			reinterpret_cast<const UnitStruct*>(state->Eax)))
			return 0; // replay mov edx,[eax+92h] and the native height/impact path
		state->rtnAddr_Pvoid = reinterpret_cast<LPVOID>(kGroundSkip);
		return X86STRACKBUFFERCHANGE;
	}

	int __stdcall AirCollision(PInlineX86StackBuffer state)
	{
		if (!AlliedProjectileCollision::ShouldPassThrough(
			reinterpret_cast<const ProjectileStruct*>(state->Esi),
			reinterpret_cast<const UnitStruct*>(state->Eax)))
			return 0; // replay mov ecx,[eax+92h]
		state->rtnAddr_Pvoid = reinterpret_cast<LPVOID>(kAirSkip);
		return X86STRACKBUFFERCHANGE;
	}

	bool HookWritten(const InlineSingleHook& hook)
	{
		return hook.m_LenToModify_Dw == 6
			&& std::memcmp(hook.m_AddrToHook, hook.m_NewBytes_Pbyte, 5) == 0
			&& reinterpret_cast<const BYTE*>(hook.m_AddrToHook)[5] == 0x90;
	}
}

namespace AlliedProjectileCollision
{
	bool IsInstalled() { return g_installed; }

	bool ShouldPassThrough(const ProjectileStruct* projectile, const UnitStruct* candidate)
	{
		if (!projectile || !candidate)
			return false;
		const unsigned shooter = static_cast<unsigned char>(projectile->myLos_PlayerID);
		const unsigned owner = candidate->cOwnerID;
		if (shooter == owner)
			return true;
		// Owner 10 is used by shooter-less map/meteor weapons. Never index it.
		if (!g_installed || shooter >= 10 || owner >= 10)
			return false;
		const TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
		return ta && ta->Players[shooter].AllyFlagAry[owner] != 0;
	}

	void Shutdown()
	{
		g_installed = false;
		g_airHook.reset();
		g_groundHook.reset();
		FlushInstructionCache(GetCurrentProcess(), NULL, 0);
	}

	void Install()
	{
		if (!ALLIED_PROJECTILE_COLLISION_ENABLE || g_installed || g_installFailed)
			return;
		static_assert(offsetof(PlayerStruct, AllyFlagAry) == 0x108, "Alliance flags offset");
		static_assert(offsetof(ProjectileStruct, myLos_PlayerID) == 0x66, "Projectile owner offset");
		static_assert(sizeof(kDecisionExpected) == kAirSkip - kDecisionStart, "Collision guard range");
		if (MatchesEngine())
		{
			try
			{
				g_groundHook.reset(new InlineSingleHook(kGroundHook, 6,
					INLINE_5BYTESLAGGERJMP, GroundCollision));
				g_airHook.reset(new InlineSingleHook(kAirHook, 6,
					INLINE_5BYTESLAGGERJMP, AirCollision));
				if (HookWritten(*g_groundHook) && HookWritten(*g_airHook))
				{
					FlushInstructionCache(GetCurrentProcess(), NULL, 0);
					g_installed = true;
					IDDrawSurface::OutptTxt("[AlliedProjectileCollision] enabled");
					return;
				}
			}
			catch (const std::bad_alloc&) {}
		}
		Shutdown();
		g_installFailed = true;
		IDDrawSurface::OutptTxt("[AlliedProjectileCollision] rejected unsupported/conflicting engine or hook failure");
	}

	void AbortIfInstallFailed()
	{
		if (!g_installFailed)
			return;
		MessageBoxA(NULL, "TADR could not safely enable AlliedProjectileCollision.\n\n"
			"The executable is unsupported, another patch changes its collision code, "
			"or hook installation failed. TA must close to avoid using different "
			"gameplay rules from other players.\n\n"
			"Use a supported executable and a compatible TADR build "
			"for every player.", "TADR collision patch error",
			MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
		ExitProcess(ERROR_BAD_EXE_FORMAT);
	}
}
