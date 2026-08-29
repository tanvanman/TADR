#include "GridClaimTieBreak.h"

#include <windows.h>

#include <cstddef>
#include <cstring>

#include "config.h"
#include "iddrawsurface.h"
#include "tamem.h"

namespace
{
	// All `[bin]` VERIFIED against TotalA.exe, Escalation GOLD 10.2.0, 1,178,624 bytes,
	// (version read from the PE resource: FileVersion 10.2.0.0; 10.1 is code-identical
	// apart from 8 bytes, so these addresses are valid for both),
	// md5 1e677a7f92c79b5ab35440853d822c17. Byte signatures on other TAF builds are
	// unverified here -- the install-time check is what makes that safe rather than a
	// claim: a build that does not match disables the module instead of corrupting it.
	//
	// SIX structurally identical contest sites, in TWO functions. Getting this wrong --
	// patching only the first three -- is the mistake this file shipped with until
	// 2026-08-27; see the function map below, and GridClaimTieBreak.h.
	//
	// Unit_ClaimFootprintCells @0x0047C790  -- the RE-CLAIM path.
	//     Gated on UnitStateMask bit 27 ("I am displaced"): `shr ecx,0x1B / test cl,1 /
	//     je ret` @0x0047C7A0. Reached only from Unit_SetYardOpen and from
	//     SpatialQueryCb_ClaimFootprint (vftable 0x004FD660), which
	//     Unit_ClearFootprintFromMap invokes over a vacated rect when the departing unit
	//     owned a cell.
	//     Registers: ecx = incumbent, esi = claimant, edx = dead (reassigned by the first
	//     instruction of both branch targets).
	//
	// Unit_LinkToSpatialGrid @0x0047CC30    -- the STAMP path, and the one that matters
	//     for anything that moves. Called from UnitMotion_ApplyDeltaAndRelink @0x0043DA41
	//     (every tick a unit moves), Unit_CreateFromDefId, Unit_SetWorldPosAndRelink,
	//     Unit_ApplyUnitPosePacket9 and Net_DeserializeUnitStateBits.
	//     Registers: eax = incumbent, esi = claimant, ecx = dead (both branch targets open
	//     with `mov ecx,[eax+0x110]`).
	//     *** edx holds the LIVE tile pointer here (`mov [edx],ax` / `mov [edx+2],ax`,
	//     `add edx,0xd`), so the variant-A encoding below MUST NOT be used at these
	//     sites -- writing dx would corrupt the cell walk. ***

	struct Variant
	{
		const BYTE* original;    // 17-byte signature at every site of this variant
		const BYTE* replace;     // first 14 bytes of the replacement
		const char* description;
	};

	// ---- variant A: Unit_ClaimFootprintCells (ecx = incumbent, dx scratch) ----
	//   8B 91 96 00 00 00   mov edx, [ecx+0x96]        ; incumbent's PlayerStruct
	//   83 3A 00            cmp dword ptr [edx], 0     ; PlayerActive
	//   74 06               je  KEEP
	//   80 7A 73 03         cmp byte ptr [edx+0x73], 3 ; == Player_RemoteHuman
	//   74 26               je  EVICT
	const BYTE kOriginalA[] = {
		0x8B, 0x91, 0x96, 0x00, 0x00, 0x00,
		0x83, 0x3A, 0x00,
		0x74, 0x06,
		0x80, 0x7A, 0x73, 0x03,
		0x74, 0x26
	};
	//   66 8B 96 A8 00 00 00   mov dx, [esi+0xA8]
	//   66 3B 91 A8 00 00 00   cmp dx, [ecx+0xA8]
	const BYTE kReplaceA[] = {
		0x66, 0x8B, 0x96, 0xA8, 0x00, 0x00, 0x00,
		0x66, 0x3B, 0x91, 0xA8, 0x00, 0x00, 0x00
	};

	// ---- variant B: Unit_LinkToSpatialGrid (eax = incumbent, cx scratch, EDX LIVE) ----
	//   8B 88 96 00 00 00   mov ecx, [eax+0x96]
	//   83 39 00            cmp dword ptr [ecx], 0
	//   74 06               je  KEEP
	//   80 79 73 03         cmp byte ptr [ecx+0x73], 3
	//   74 25               je  EVICT
	const BYTE kOriginalB[] = {
		0x8B, 0x88, 0x96, 0x00, 0x00, 0x00,
		0x83, 0x39, 0x00,
		0x74, 0x06,
		0x80, 0x79, 0x73, 0x03,
		0x74, 0x25
	};
	//   66 8B 8E A8 00 00 00   mov cx, [esi+0xA8]
	//   66 3B 88 A8 00 00 00   cmp cx, [eax+0xA8]
	const BYTE kReplaceB[] = {
		0x66, 0x8B, 0x8E, 0xA8, 0x00, 0x00, 0x00,
		0x66, 0x3B, 0x88, 0xA8, 0x00, 0x00, 0x00
	};

	const size_t kRegionSize  = sizeof(kOriginalA);
	const size_t kReplaceSize = sizeof(kReplaceA);
	static_assert(sizeof(kOriginalA) == 17, "discriminator region is 17 bytes");
	static_assert(sizeof(kOriginalB) == 17, "discriminator region is 17 bytes");
	static_assert(sizeof(kReplaceA) == 14, "replacement leaves 3 bytes for jb + pad");
	static_assert(sizeof(kReplaceB) == 14, "replacement leaves 3 bytes for jb + pad");

	const Variant kVariantA = { kOriginalA, kReplaceA, "ecx=incumbent, dx scratch" };
	const Variant kVariantB = { kOriginalB, kReplaceB, "eax=incumbent, cx scratch" };

	struct Site
	{
		DWORD           address;      // start of the 17-byte discriminator region
		DWORD           evictTarget;  // branch target when the claimant wins
		const Variant*  variant;
		const char*     what;
	};

	const Site kSites[] = {
		// Unit_ClaimFootprintCells -- re-claim after a cell is vacated.
		{ 0x0047C847u, 0x0047C87Eu, &kVariantA, "reclaim: yardmap/building" },
		{ 0x0047C950u, 0x0047C987u, &kVariantA, "reclaim: slot A (ground)"  },
		{ 0x0047CA2Cu, 0x0047CA63u, &kVariantA, "reclaim: slot B (air)"     },
		// Unit_LinkToSpatialGrid -- the per-tick stamp for anything that moves.
		{ 0x0047CDCAu, 0x0047CE00u, &kVariantB, "stamp: yardmap/building"   },
		{ 0x0047CF00u, 0x0047CF36u, &kVariantB, "stamp: slot A (ground)"    },
		{ 0x0047CFDAu, 0x0047D010u, &kVariantB, "stamp: slot B (air)"       },
	};
	const int kSiteCount = sizeof(kSites) / sizeof(kSites[0]);

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
		//   66 8B <r> A8 00 00 00   mov <tmp>, [esi+0xA8]  ; claimant's UnitInGameIndex
		//   66 3B <r> A8 00 00 00   cmp <tmp>, [inc+0xA8]  ; vs the incumbent's
		//   72 xx                   jb  EVICT              ; strictly lower index wins
		//   90                      nop                    ; else fall through to KEEP
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
		std::memcpy(out, site.variant->replace, kReplaceSize);

		// The jb occupies out[14..15], so the IP after it is site + 16. Note this is one
		// byte EARLIER than vanilla's `je`, so the displacement is NOT the 0x26/0x25 the
		// original bytes carry -- it is computed here rather than copied for that reason.
		const DWORD jbNext = site.address + static_cast<DWORD>(kReplaceSize) + 2;
		const LONG  delta  = static_cast<LONG>(site.evictTarget) - static_cast<LONG>(jbNext);
		if (delta < -128 || delta > 127)
		{
			IDDrawSurface::OutptFmtTxt(
				"[GridClaimTieBreak] disabled: EVICT target out of short-jump range "
				"at 0x%08X (delta %ld)", site.address, delta);
			return false;
		}

		out[kReplaceSize]     = 0x72;                        // jb rel8
		out[kReplaceSize + 1] = static_cast<BYTE>(delta);
		// out[16] left as the pre-filled nop pad.
		return true;
	}

	bool HasExpectedBytes(const Site& site)
	{
		if (std::memcmp(reinterpret_cast<const void*>(site.address),
			site.variant->original, kRegionSize) == 0)
		{
			return true;
		}

		IDDrawSurface::OutptFmtTxt(
			"[GridClaimTieBreak] disabled: unexpected TotalA.exe bytes at 0x%08X (%s)",
			site.address, site.what);
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
		// Say so out loud. A paired patched/control experiment is only interpretable if
		// each log states which arm produced it -- identifying the control by the
		// ABSENCE of a line is unsafe, because a module that failed its byte check, or
		// was never installed at all, looks exactly the same. This project already lost
		// a session to a control build that was not what it appeared to be
		// (CLAUDE.md 0.11), so the control now identifies itself positively.
		IDDrawSurface::OutptTxt(
			"[GridClaimTieBreak] DISABLED at compile time (GRID_CLAIM_TIEBREAK_ENABLE 0) "
			"-- contested cells keep vanilla's evict-if-remote-human rule. This is the "
			"CONTROL arm.");
		return;
#else
		// Validate every site BEFORE writing any of them. A partial application here
		// would leave the claim function in a state no analysis covers -- and a
		// partial application ACROSS THE TWO FUNCTIONS is worse than either extreme,
		// because the stamp path and the re-claim path would then resolve the same
		// contest by two different rules on alternating ticks.
		for (int i = 0; i < kSiteCount; ++i)
		{
			if (!HasExpectedBytes(kSites[i]))
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
			"[GridClaimTieBreak] installed at %d sites (3 re-claim + 3 stamp): contested "
			"cells now go to the lower UnitInGameIndex on every client "
			"(was: evict-if-remote-human)",
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
