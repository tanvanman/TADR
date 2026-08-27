#include "GridClaimTieBreak.h"

#include <windows.h>

#include <cstddef>
#include <cstring>

#include "config.h"
#include "iddrawsurface.h"
#include "tamem.h"

namespace
{
	// All `[bin]` VERIFIED against TotalA.exe, Escalation 10.1 GOLD, 1,178,624 bytes,
	// md5 1e677a7f92c79b5ab35440853d822c17.
	//
	// Three structurally identical contest sites inside Unit_ClaimFootprintCells
	// @0x0047C790. At every one of them, on entry:
	//     ecx = incumbent UnitStruct*   (resolved from the cell's stored index)
	//     esi = claiming UnitStruct*    (the function's own argument)
	//     edx = dead (reassigned by the first instruction of the region)
	// and both the KEEP and EVICT branch targets reload edx before using it, so writing
	// dx here clobbers nothing live.

	struct Site
	{
		DWORD       address;      // start of the 17-byte discriminator region
		DWORD       evictTarget;  // branch target when the claimant wins
		const char* what;
	};

	const Site kSites[] = {
		{ 0x0047C847u, 0x0047C87Eu, "yardmap/building" },
		{ 0x0047C950u, 0x0047C987u, "slot A (ground)"  },
		{ 0x0047CA2Cu, 0x0047CA63u, "slot B (air)"     },
	};
	const int kSiteCount = sizeof(kSites) / sizeof(kSites[0]);

	// The original 17 bytes. Byte-for-byte identical at all three sites, including both
	// branch displacements -- which is itself a useful consistency check that we are
	// looking at the same construct three times and not at a coincidental match.
	//
	//   8B 91 96 00 00 00   mov edx, [ecx+0x96]        ; incumbent's PlayerStruct
	//   83 3A 00            cmp dword ptr [edx], 0     ; PlayerActive
	//   74 06               je  KEEP
	//   80 7A 73 03         cmp byte ptr [edx+0x73], 3 ; == Player_RemoteHuman
	//   74 26               je  EVICT
	const BYTE kOriginalBytes[] = {
		0x8B, 0x91, 0x96, 0x00, 0x00, 0x00,
		0x83, 0x3A, 0x00,
		0x74, 0x06,
		0x80, 0x7A, 0x73, 0x03,
		0x74, 0x26
	};
	const size_t kRegionSize = sizeof(kOriginalBytes);
	static_assert(sizeof(kOriginalBytes) == 17, "discriminator region is 17 bytes");

	// UnitInGameIndex lives at UnitStruct+0xA8 -- the same field the grid itself stores.
	static_assert(offsetof(UnitStruct, UnitInGameIndex) == 0xA8, "UnitStruct.UnitInGameIndex");

	bool g_installed  = false;
	bool g_savedValid = false;
	BYTE g_saved[kSiteCount][kRegionSize];

	bool WriteCode(DWORD address, const void* data, size_t length)
	{
		DWORD oldProtect = 0;
		if (!VirtualProtect(reinterpret_cast<void*>(address), length,
			PAGE_EXECUTE_READWRITE, &oldProtect))
		{
			IDDrawSurface::OutptFmtTxt(
				"[GridClaimTieBreak] VirtualProtect failed at 0x%08X", address);
			return false;
		}
		std::memcpy(reinterpret_cast<void*>(address), data, length);
		VirtualProtect(reinterpret_cast<void*>(address), length, oldProtect, &oldProtect);
		FlushInstructionCache(GetCurrentProcess(),
			reinterpret_cast<void*>(address), length);
		return true;
	}

	bool BuildPatch(const Site& site, BYTE* out)
	{
		// 17 bytes replacing 17 bytes:
		//
		//   66 8B 96 A8 00 00 00   mov dx, [esi+0xA8]   ; claimant's UnitInGameIndex
		//   66 3B 91 A8 00 00 00   cmp dx, [ecx+0xA8]   ; vs the incumbent's
		//   72 xx                  jb  EVICT            ; strictly lower index wins
		//   90                     nop                  ; else fall through to KEEP
		//
		// 0xA8 (168) exceeds the signed disp8 range (-128..127), so [reg+disp] MUST use
		// the mod=10 form, which takes a FULL 4-byte little-endian displacement -- not
		// 3. (Confirmed against vanilla's own encoding of an out-of-disp8-range offset:
		// 0x49A149 `mov ax,[ecx+0xD6]` is `66 8B 81 D6 00 00 00`, 7 bytes, same shape.)
		// An earlier build of this file emitted only 3 displacement bytes per
		// instruction; the CPU then consumed the following instruction's leading 0x66
		// prefix as the missing 4th byte, computing esi + 0x660000A8 instead of
		// esi + 0xA8 and reading from a wild address -- confirmed live 2026-08-24,
		// AV at 0x0047C950, faulting address 0xCB4FC528 = 0x654FC480 + 0x660000A8
		// exactly. Fixed here; region size is unchanged (7+7+2+1 = 17).
		//
		// Unsigned compare: UnitInGameIndex is an array ordinal, never negative. A unit
		// that finds ITSELF as the incumbent (it already holds this cell and is
		// re-claiming) compares equal, does not branch, and lands on KEEP -- exactly
		// what vanilla does in that case today.
		std::memset(out, 0x90, kRegionSize);

		out[0] = 0x66; out[1] = 0x8B; out[2] = 0x96;
		out[3] = 0xA8; out[4] = 0x00; out[5] = 0x00; out[6] = 0x00;

		out[7] = 0x66; out[8] = 0x3B; out[9] = 0x91;
		out[10] = 0xA8; out[11] = 0x00; out[12] = 0x00; out[13] = 0x00;

		const DWORD jbNext = site.address + 16;              // IP after the 2-byte jb
		const LONG  delta  = static_cast<LONG>(site.evictTarget) - static_cast<LONG>(jbNext);
		if (delta < -128 || delta > 127)
		{
			IDDrawSurface::OutptFmtTxt(
				"[GridClaimTieBreak] disabled: EVICT target out of short-jump range "
				"at 0x%08X (delta %ld)", site.address, delta);
			return false;
		}

		out[14] = 0x72;                                      // jb rel8
		out[15] = static_cast<BYTE>(delta);
		// out[16] left as the pre-filled nop pad.
		return true;
	}

	bool HasExpectedBytes(DWORD address, const char* what)
	{
		if (std::memcmp(reinterpret_cast<const void*>(address),
			kOriginalBytes, kRegionSize) == 0)
		{
			return true;
		}

		IDDrawSurface::OutptFmtTxt(
			"[GridClaimTieBreak] disabled: unexpected TotalA.exe bytes at 0x%08X (%s)",
			address, what);
		return false;
	}
}

namespace GridClaimTieBreak
{
	void Install()
	{
		if (g_installed)
			return;
		g_installed = true;

#if !GRID_CLAIM_TIEBREAK_ENABLE
		return;
#else
		// Validate every site BEFORE writing any of them. A partial application here
		// would leave the claim function in a state no analysis covers.
		for (int i = 0; i < kSiteCount; ++i)
		{
			if (!HasExpectedBytes(kSites[i].address, kSites[i].what))
				return;
		}

		BYTE patches[kSiteCount][kRegionSize];
		for (int i = 0; i < kSiteCount; ++i)
		{
			if (!BuildPatch(kSites[i], patches[i]))
				return;
		}

		for (int i = 0; i < kSiteCount; ++i)
		{
			std::memcpy(g_saved[i], reinterpret_cast<void*>(kSites[i].address),
				kRegionSize);
		}
		g_savedValid = true;

		for (int i = 0; i < kSiteCount; ++i)
		{
			if (!WriteCode(kSites[i].address, patches[i], kRegionSize))
			{
				IDDrawSurface::OutptFmtTxt(
					"[GridClaimTieBreak] write failed at 0x%08X -- rolling back",
					kSites[i].address);
				Shutdown();
				return;
			}
		}

		IDDrawSurface::OutptFmtTxt(
			"[GridClaimTieBreak] installed at %d sites: contested cells now go to the "
			"lower UnitInGameIndex on every client (was: evict-if-remote-human)",
			kSiteCount);
#endif
	}

	void Shutdown()
	{
		// Only restore bytes actually captured -- otherwise a Shutdown after a failed
		// validation would write an uninitialised buffer over live engine code.
		if (!g_savedValid)
			return;

		for (int i = 0; i < kSiteCount; ++i)
			WriteCode(kSites[i].address, g_saved[i], kRegionSize);

		g_savedValid = false;
	}
}
