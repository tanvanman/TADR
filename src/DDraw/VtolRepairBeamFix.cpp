#include "VtolRepairBeamFix.h"

#include <windows.h>

#include <cstring>
#include <memory>

#include "config.h"
#include "iddrawsurface.h"
#include "hook/hook.h"

namespace
{
	// All `[bin]` VERIFIED against TotalA.exe, Escalation GOLD 10.2.0, md5
	// 1e677a7f92c79b5ab35440853d822c17. Full derivation: VtolRepairBeamFix.h.

	bool CheckBytes(DWORD address, const BYTE* expected, size_t len, const char* what)
	{
		if (std::memcmp(reinterpret_cast<const void*>(address), expected, len) == 0)
			return true;

		IDDrawSurface::OutptFmtTxt(
			"[VtolRepairBeamFix] DISABLED: unexpected TotalA.exe bytes at 0x%08X (%s)",
			address, what);
		return false;
	}

	// One region covering the call whose result this hook reads AND the two instructions
	// it steals. Checking the call too is what makes "Eax holds
	// Unit_ApplyRepairHealProgress's return value" a verified fact rather than a claim:
	// E8 CE 6B 00 00 at 0x0041513D encodes exactly `call 0x0041BD10`.
	const DWORD kRegionAddr = 0x0041513Du;
	const BYTE kRegionExpectedBytes[12] = {
		0xE8, 0xCE, 0x6B, 0x00, 0x00,   // call 0x0041BD10  Unit_ApplyRepairHealProgress
		0x8B, 0x56, 0x0E,               // mov edx,[esi+0x0E]   <- hook site
		0x8D, 0x4C, 0x24, 0x10          // lea ecx,[esp+0x10]
	};

	const DWORD kHookAddr = 0x00415142u;
	const DWORD kHookLen = 5u;   // extends to 7 (whole-instruction rule, see the header)

	// Vanilla's shared tail, past the nano-piece query and the beam spawn. The ground
	// repair tick jumps to its own identical tail the same way.
	const DWORD kSkipBeamTarget = 0x004151F1u;
	const BYTE kSkipBeamTargetExpectedBytes[4] = { 0x6A, 0x01, 0x8B, 0xCE };  // push 1 / mov ecx,esi

	DWORD g_beamSuppressed = 0;

	// Eax is Unit_ApplyRepairHealProgress's return value: 1 only if the repair was paid
	// for and applied, 0 if the payer stalled or the target is already at full HP.
	// Reads no memory, so there is nothing to probe -- the decision is a register test.
	int __stdcall VtolRepairBeamProc(PInlineX86StackBuffer buf)
	{
		if (buf->Eax != 0)
			return 0;   // repaired this tick: vanilla, beam plays

		++g_beamSuppressed;
		buf->rtnAddr_Pvoid = reinterpret_cast<LPVOID>(kSkipBeamTarget);
		return X86STRACKBUFFERCHANGE;
	}

	bool RunSelfTest()
	{
		InlineX86StackBuffer buf;

		std::memset(&buf, 0, sizeof(buf));
		buf.Eax = 0;
		const DWORD before = g_beamSuppressed;
		const bool suppressOk = (VtolRepairBeamProc(&buf) == X86STRACKBUFFERCHANGE
			&& buf.rtnAddr_Pvoid == reinterpret_cast<LPVOID>(kSkipBeamTarget)
			&& g_beamSuppressed == before + 1);

		std::memset(&buf, 0, sizeof(buf));
		buf.Eax = 1;
		const bool vanillaOk = (VtolRepairBeamProc(&buf) == 0 && buf.rtnAddr_Pvoid == NULL);

		if (!suppressOk)
			IDDrawSurface::OutptTxt(
				"[VtolRepairBeamFix][selftest] FAIL sub-case: no repair progress -> skip beam");
		if (!vanillaOk)
			IDDrawSurface::OutptTxt(
				"[VtolRepairBeamFix][selftest] FAIL sub-case: repair progressed -> vanilla, no redirect");

		g_beamSuppressed = 0;   // do not let the self-test show up in the live counter
		return suppressOk && vanillaOk;
	}

	bool g_installed = false;
	std::unique_ptr<InlineSingleHook> g_hook;
}

namespace VtolRepairBeamFix
{
	void Install()
	{
		if (g_installed)
			return;
		g_installed = true;

#if !VTOL_REPAIR_BEAM_FIX_ENABLE
		IDDrawSurface::OutptTxt(
			"[VtolRepairBeamFix] DISABLED at compile time (VTOL_REPAIR_BEAM_FIX_ENABLE 0) "
			"-- an air constructor still emits the nanolathe beam while its repair is "
			"stalled, exactly as vanilla. This is the CONTROL arm.");
		return;
#else
		bool ok = true;
		ok = CheckBytes(kRegionAddr, kRegionExpectedBytes, sizeof(kRegionExpectedBytes),
			"VTOL repair progress call + hook site") && ok;
		ok = CheckBytes(kSkipBeamTarget, kSkipBeamTargetExpectedBytes,
			sizeof(kSkipBeamTargetExpectedBytes), "beam-skip redirect target") && ok;
		if (!ok)
			return;

		if (!RunSelfTest())
		{
			IDDrawSurface::OutptTxt(
				"[VtolRepairBeamFix] DISABLED: self-test failed -- see [selftest] lines "
				"above. Refusing to trust this logic against live game data.");
			return;
		}

		g_hook.reset(new InlineSingleHook(
			kHookAddr, kHookLen, INLINE_5BYTESLAGGERJMP, VtolRepairBeamProc));

		IDDrawSurface::OutptFmtTxt(
			"[VtolRepairBeamFix] installed: nanolathe beam @0x%08X now gated on "
			"Unit_ApplyRepairHealProgress, matching the three ground repair ticks",
			kHookAddr);
#endif
	}

	void Shutdown()
	{
		// Reported once, at teardown, so nothing is logged per simulation tick.
		if (g_hook)
		{
			IDDrawSurface::OutptFmtTxt(
				"[VtolRepairBeamFix] counters: beam suppressed on %lu unpaid repair ticks",
				g_beamSuppressed);
		}

		g_hook.reset();
		g_installed = false;
	}
}
