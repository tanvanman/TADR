#include "GroundToAirGuard.h"

#include <windows.h>

#include <cstddef>
#include <cstring>
#include <memory>

#include "config.h"
#include "iddrawsurface.h"
#include "hook/hook.h"
#include "tamem.h"

namespace
{
	// All `[bin]` VERIFIED against TotalA.exe, Escalation GOLD 10.2.0, 1,178,624 bytes,
	// md5 1e677a7f92c79b5ab35440853d822c17. Full derivation and evidence for every
	// address below: GroundToAirGuard.h. The install-time byte check is what makes
	// trusting these addresses on an unverified build safe rather than a guess: a
	// mismatch disables the module instead of patching the wrong bytes.

	bool SafeIsBadReadPtr(const void* p, size_t n)
	{
		if (!p) return true;
		__try
		{
			const volatile unsigned char* b = (const volatile unsigned char*)p;
			for (size_t i = 0; i < n; ++i) (void)b[i];
			return false;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return true;
		}
	}

	bool CheckBytes(DWORD address, const BYTE* expected, size_t len, const char* what)
	{
		if (std::memcmp(reinterpret_cast<const void*>(address), expected, len) == 0)
			return true;

		IDDrawSurface::OutptFmtTxt(
			"[GroundToAirGuard] DISABLED: unexpected TotalA.exe bytes at 0x%08X (%s)",
			address, what);
		return false;
	}

	bool WriteCode(DWORD address, const void* data, size_t length)
	{
		DWORD oldProtect = 0;
		if (!VirtualProtect(reinterpret_cast<void*>(address), length,
			PAGE_EXECUTE_READWRITE, &oldProtect))
		{
			IDDrawSurface::OutptFmtTxt(
				"[GroundToAirGuard] VirtualProtect failed at 0x%08X", address);
			return false;
		}
		std::memcpy(reinterpret_cast<void*>(address), data, length);
		VirtualProtect(reinterpret_cast<void*>(address), length, oldProtect, &oldProtect);
		FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(address), length);
		return true;
	}

	// ---- P1: cursor DEFEND unblock -------------------------------------------------
	//
	// Unit_ResolveCursorOrderType, DEFEND case. Checked as one 23-byte region (the
	// context leading into the jne, the jne itself, and its fall-through landing are
	// all contiguous) so a single CheckBytes call covers everything this site depends
	// on being exactly what it was verified to be.
	const DWORD kP1RegionAddr = 0x0043E63Du;
	const BYTE kP1ExpectedBytes[23] = {
		0x8B, 0x97, 0x92, 0x00, 0x00, 0x00,             // mov edx,[edi+0x92]
		0x85, 0x82, 0x41, 0x02, 0x00, 0x00,             // test [edx+0x241],eax
		0x0F, 0x85, 0x49, 0x0A, 0x00, 0x00,             // jne 0x43F098  <- patched
		0xB8, 0x05, 0x00, 0x00, 0x00                    // mov eax,5
	};
	const DWORD kP1PatchOffset = 12;   // the jne, within the region above
	const BYTE kP1PatchBytes[6] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };

	// ---- P2: cursor MOVE cosmetic refusal ------------------------------------------
	const DWORD kP2HookAddr = 0x0043EADBu;
	const DWORD kP2HookLen = 7u;
	const BYTE kP2ExpectedBytes[7] = { 0xF6, 0x80, 0x45, 0x02, 0x00, 0x00, 0x20 };
	const DWORD kP2NoGuardTarget = 0x0043EAF5u;
	const BYTE kP2NoGuardTargetExpectedBytes[5] = { 0xB8, 0x0E, 0x00, 0x00, 0x00 }; // mov eax,0xe

	// ---- P3: resolver MOVE refusal (mandatory) -------------------------------------
	const DWORD kP3HookAddr = 0x0043F9A4u;
	const DWORD kP3HookLen = 7u;
	const BYTE kP3ExpectedBytes[7] = { 0xF6, 0x86, 0x45, 0x02, 0x00, 0x00, 0x20 };
	const DWORD kP3PlainMoveTarget = 0x0043F9CDu;
	const BYTE kP3PlainMoveTargetExpectedBytes[6] =
		{ 0x8B, 0x96, 0x41, 0x02, 0x00, 0x00 }; // mov edx,[esi+0x241]

	// ---- P4: simulation veto removal (Class B) -------------------------------------
	//
	// MissionTick_Follow_Ground. Checked as one 20-byte region ending at the je.
	const DWORD kP4RegionAddr = 0x0040633Eu;
	const BYTE kP4ExpectedBytes[20] = {
		0x8B, 0x88, 0x92, 0x00, 0x00, 0x00,             // mov ecx,[eax+0x92]
		0x8B, 0x91, 0x41, 0x02, 0x00, 0x00,             // mov edx,[ecx+0x241]
		0xC1, 0xEA, 0x0B,                               // shr edx,0xB
		0xF6, 0xC2, 0x01,                               // test dl,1
		0x74, 0x0F                                      // je 0x406361  <- patched
	};
	const DWORD kP4PatchOffset = 18;
	const BYTE kP4PatchBytes[2] = { 0xEB, 0x0F };        // jmp 0x406361

	// ---- P5: suppress the VTOL_MobileBuild mirror leak -----------------------------
	const DWORD kP5HookAddr = 0x00406618u;
	const DWORD kP5HookLen = 5u;   // auto-extends to 7 (whole-instruction rule, see .h)
	const BYTE kP5ExpectedBytes[7] =
		{ 0x8A, 0x51, 0x04, 0x88, 0x54, 0x24, 0x10 };
	const DWORD kP5HelpBuildTarget = 0x00406636u;
	const BYTE kP5HelpBuildTargetExpectedBytes[5] =
		{ 0x68, 0xF0, 0x13, 0x50, 0x00 };  // push 0x5013f0  ("HelpBuild")
	// NOT byte-checked, deliberately: TABugFix.cpp's FixedPositionGuardingConsProc is
	// already hooked here and installs earlier in DLL_PROCESS_ATTACH, so these bytes are
	// its trampoline, not vanilla, in every shipped config. The redirect is safe either
	// way -- see the collision note in GroundToAirGuard.h.
	const DWORD kP5PlainFollowTarget = 0x004066ACu;

	// ---- P5b: translate VTOL_RepairUnit -> ground RepairUnit -----------------------
	//
	// Same hook as the VTOL_MobileBuild case, different redirect. There is no vanilla
	// path that builds a generic-target RepairUnit order, so instead of calling the
	// engine's allocator/ctor/assign directly, this writes the resolved ground id into
	// the stack slot vanilla reads it back from and redirects into vanilla's own
	// construction sequence. 0x0040664A is reached in vanilla from this hook's own
	// fallthrough on every steady-state tick, so it is an existing path, not a new one.
	// Stack-offset proof and register liveness: GroundToAirGuard.h.
	const DWORD kP5RepairConstructTarget = 0x0040664Au;
	const BYTE kP5RepairConstructTargetExpectedBytes[2] = { 0x6A, 0x56 };  // push 0x56
	const DWORD kP5RepairStackIdOffset = 0x10u;

	// ---- P6: resolver SMART-CLICK refusal (mandatory) ------------------------------
	//
	// The smart-order (intent STOP) arm has its own guard tail, separate from the MOVE
	// arm P3 covers. Reachable only from intent STOP, never from intent DEFEND, so
	// refusing here cannot break the Guard button or hotkey G -- proof in the header.
	const DWORD kP6HookAddr = 0x0043FB93u;
	const DWORD kP6HookLen = 6u;
	const BYTE kP6ExpectedBytes[6] =
		{ 0x8B, 0x81, 0x41, 0x02, 0x00, 0x00 };  // mov eax,[ecx+0x241]
	// Vanilla's own "this pairing cannot guard" continuation, the shared target of the
	// CanGuard and allied-flag rejections six and ten bytes above the hook site.
	const DWORD kP6NoGuardTarget = 0x0043FBC3u;
	const BYTE kP6NoGuardTargetExpectedBytes[6] =
		{ 0x85, 0xC6, 0x8B, 0x74, 0x24, 0x24 };  // test esi,eax / mov esi,[esp+0x24]

	// WARNING for future hooks anywhere in this project: do NOT hook an instruction whose
	// opcode is 0xC6 (MOV r/m8, imm8) with InlineSingleHook. hook/etc.cpp's length
	// disassembler has table_1[0xC6] = C_MODRM+C_DATA66 (a copy of the 0xC7 entry), so it
	// sizes the imm8 as 4 bytes and reports the instruction 3 bytes too long; the hook
	// steals past its end and the jump-back lands mid-instruction. Confirmed by crashing
	// this way on a temporary hook at 0x004991D9. Correct entry would be C_MODRM+C_DATAW0.
	// Not fixed here: etc.cpp is shared by every hook in the project, out of scope for
	// this module. None of the sites below are 0xC6.

	// MissionOrder_FindByName @0x00438760: __thiscall(void* outByteBuf, const char* name).
	// Binary-searches the sorted mission template table and writes ONE byte -- the
	// matched entry's runtime index, or 0 if not found -- to *outByteBuf. Confirmed by
	// disassembling the whole function, including both write-back sites.
	typedef void (__thiscall* MissionOrderFindByNameFn)(void*, const char*);
	const DWORD kMissionOrderFindByNameAddr = 0x00438760u;

	// The table that function searches is a global vector of 25-byte mission templates
	// copied from .rdata: begin/end pointers below, read fresh on every call. The
	// install-time check covers both loads AND the /25 stride magic between them, so
	// these addresses are validated by the same all-or-nothing gate as the patch sites.
	const DWORD kMissionTableEndPtrAddr = 0x00512348u;
	const DWORD kMissionTableBeginPtrAddr = 0x00512344u;
	const DWORD kMissionTableProbeAddr = 0x00438763u;
	const BYTE kMissionTableProbeExpectedBytes[20] = {
		0x8B, 0x2D, 0x44, 0x23, 0x51, 0x00,      // mov ebp,[0x512344]
		0x56,                                    // push esi
		0x8B, 0xF1,                              // mov esi,ecx
		0xB8, 0x1F, 0x85, 0xEB, 0x51,            // mov eax,0x51EB851F  (reciprocal for /25)
		0x8B, 0x0D, 0x48, 0x23, 0x51, 0x00       // mov ecx,[0x512348]
	};

	DWORD MissionTableSize()
	{
		return *reinterpret_cast<const volatile DWORD*>(kMissionTableEndPtrAddr)
			- *reinterpret_cast<const volatile DWORD*>(kMissionTableBeginPtrAddr);
	}

	// Table extent the cached ids were resolved against.
	DWORD g_missionTableSize = 0;

	BYTE g_vtolMobileBuildId = 0;
	BYTE g_groundMobileBuildId = 0;
	// A guarded air constructor that is itself guarding another one never holds a raw
	// VTOL_MobileBuild order: MissionTick_VTOL_Follow (0x0040FE68) translates that to
	// VTOL_HelpBuild before this hook sees it, and deeper chains converge on the same id.
	// Handled identically to g_vtolMobileBuildId -- same translation, same target field.
	BYTE g_vtolHelpBuildId = 0;
	bool g_missionIdsResolved = false;

	bool ResolveMissionIds()
	{
		MissionOrderFindByNameFn findByName =
			reinterpret_cast<MissionOrderFindByNameFn>(kMissionOrderFindByNameAddr);

		BYTE vtolId = 0, groundId = 0, vtolHelpId = 0;
		findByName(&vtolId, "VTOL_MobileBuild");
		findByName(&groundId, "MobileBuild");
		findByName(&vtolHelpId, "VTOL_HelpBuild");

		// All three must resolve to distinct ids -- if any lookup failed (the engine's
		// own "not found" sentinel is 0), trusting id 0 to mean a real mission would
		// risk matching whatever real entry happens to sit at index 0. All three names
		// are entries in the same static template table, so they resolve or fail
		// together; this is a sanity check on the lookup, not a mod-data dependency.
		// Not logged here: the caller already logs a throttled message for the
		// unresolved case, and logging here too would just double it, unthrottled.
		if (vtolId == groundId || vtolHelpId == vtolId || vtolHelpId == groundId)
			return false;

		g_vtolMobileBuildId = vtolId;
		g_groundMobileBuildId = groundId;
		g_vtolHelpBuildId = vtolHelpId;
		g_missionIdsResolved = true;
		IDDrawSurface::OutptFmtTxt(
			"[GroundToAirGuard] mission ids resolved: VTOL_MobileBuild=%u MobileBuild=%u "
			"VTOL_HelpBuild=%u",
			vtolId, groundId, vtolHelpId);
		return true;
	}

	// Repair-mirror ids -- resolved and flagged INDEPENDENTLY of the build pair above.
	// Deliberately not folded into ResolveMissionIds(): if a mod's game data ever fails
	// to resolve "VTOL_RepairUnit"/"RepairUnit" distinctly, that must not also disable
	// the already-shipped, already-verified VTOL_MobileBuild -> HelpBuild translation.
	// Each capability fails closed on its own.
	BYTE g_vtolRepairUnitId = 0;
	BYTE g_groundRepairUnitId = 0;
	bool g_repairIdsResolved = false;

	bool ResolveRepairMissionIds()
	{
		MissionOrderFindByNameFn findByName =
			reinterpret_cast<MissionOrderFindByNameFn>(kMissionOrderFindByNameAddr);

		BYTE vtolId = 0, groundId = 0;
		findByName(&vtolId, "VTOL_RepairUnit");
		findByName(&groundId, "RepairUnit");

		if (vtolId == groundId)
			return false;

		g_vtolRepairUnitId = vtolId;
		g_groundRepairUnitId = groundId;
		g_repairIdsResolved = true;
		IDDrawSurface::OutptFmtTxt(
			"[GroundToAirGuard] repair mission ids resolved: VTOL_RepairUnit=%u RepairUnit=%u",
			vtolId, groundId);
		return true;
	}

	DWORD g_p2Refused = 0;
	DWORD g_p3Refused = 0;
	DWORD g_p5Translated = 0;
	DWORD g_p5RepairTranslated = 0;
	DWORD g_p5NullTarget = 0;
	DWORD g_p5Suppressed = 0;
	DWORD g_p6Refused = 0;

	// True iff `def` (a UnitDefStruct*) is readable and has the canfly bit set. Used by
	// P2, P3 and P6 -- same predicate, same safety rules, one place to get it right.
	bool DefCanFly(const UnitDefStruct* def)
	{
		// Probe only the field about to be read, not the whole struct prefix:
		// SafeIsBadReadPtr is a byte-at-a-time volatile loop and this runs per guard order
		// per unit per tick. Same rule as TABugFix.cpp's order-pointer probe.
		if (!def || SafeIsBadReadPtr(&def->UnitTypeMask_0, sizeof(def->UnitTypeMask_0)))
			return false;   // unreadable -> treated as "does not fly" (the common case)
		return (def->UnitTypeMask_0 & canfly) != 0;
	}

	// Shared predicate for P2, P3 and P6: true iff this (guardian, target) pairing is
	// the one case vanilla did not allow before P1/P4 -- a non-flying guardian and a
	// flying target -- and should therefore be refused on every IMPLICIT order path.
	// "Implicit" is the whole point of this module's hard constraint: ground-to-air
	// guarding must be reachable only by explicitly choosing Guard (button or hotkey
	// G), never as a side effect of a Move-click or a smart/right-click. Any read that
	// cannot be done safely resolves to "do not refuse" (i.e. behave exactly as
	// vanilla), which is the safe default for a refusal hook: it can make this module
	// do nothing, never something wrong.
	bool ShouldRefuseImplicitGuard(const UnitDefStruct* guardianDef, const UnitStruct* target)
	{
		// The guardian's readability is checked HERE rather than leaning on DefCanFly,
		// which maps "unreadable" to "does not fly". That mapping is the safe direction
		// for the target but the wrong one for the guardian: "does not fly" is precisely
		// the guardian state that leads to a refusal, so an unverifiable guardian would
		// refuse an order vanilla allows. Unknown guardian -> never refuse.
		if (!guardianDef || SafeIsBadReadPtr(&guardianDef->UnitTypeMask_0,
			sizeof(guardianDef->UnitTypeMask_0)))
			return false;

		if (DefCanFly(guardianDef))
			return false;   // flying guardian: never restricted, unaffected by this module

		if (!target || SafeIsBadReadPtr(&target->UnitType, sizeof(target->UnitType)))
			return false;

		return DefCanFly(target->UnitType);
	}

	// ---------------------------------------------------------------------------
	// P2 router -- cosmetic only. Never redirects unless P3 would also refuse the
	// same pairing; the cursor icon and the click outcome must agree.
	// ---------------------------------------------------------------------------
	int __stdcall P2MoveCursorGuardProc(PInlineX86StackBuffer buf)
	{
		const UnitDefStruct* guardianDef = reinterpret_cast<const UnitDefStruct*>(buf->Eax);
		const UnitStruct* target = reinterpret_cast<const UnitStruct*>(buf->Edi);

		if (!ShouldRefuseImplicitGuard(guardianDef, target))
			return 0;

		++g_p2Refused;
		buf->rtnAddr_Pvoid = reinterpret_cast<LPVOID>(kP2NoGuardTarget);
		return X86STRACKBUFFERCHANGE;
	}

	// ---------------------------------------------------------------------------
	// P3 router -- the load-bearing refusal. A Move-click that would otherwise
	// resolve onto a friendly flying unit with a non-flying guardian selected is
	// redirected to this same gate's own plain-move continuation instead.
	// ---------------------------------------------------------------------------
	int __stdcall P3MoveResolverGuardProc(PInlineX86StackBuffer buf)
	{
		const UnitDefStruct* guardianDef = reinterpret_cast<const UnitDefStruct*>(buf->Esi);
		const UnitStruct* target = reinterpret_cast<const UnitStruct*>(buf->Edi);

		if (!ShouldRefuseImplicitGuard(guardianDef, target))
			return 0;

		++g_p3Refused;
		buf->rtnAddr_Pvoid = reinterpret_cast<LPVOID>(kP3PlainMoveTarget);
		return X86STRACKBUFFERCHANGE;
	}

	// ---------------------------------------------------------------------------
	// P6 router -- the second load-bearing refusal, and the one that actually stops
	// a right-click from guarding. Same predicate and same shape as P3, different
	// arm of the same function: Ecx = guardian's UnitDefStruct (vanilla loaded it
	// from the arg slot ten bytes above), Edi = target UnitStruct (the function's
	// arg4, never written between entry and here).
	//
	// DO NOT move this hook earlier. The redirect deliberately does NOT replay the
	// stolen `mov eax,[ecx+0x241]`, which is exactly what makes it correct: Eax still
	// holds `[ecx+0x245]` from vanilla's load six bytes above, and that is precisely
	// the register state vanilla's own two `je 0x0043FBC3` rejections deliver to the
	// same label. Hooking one instruction earlier would hand 0x0043FBC3 a stale Eax
	// and its `test esi,eax` would then branch on garbage.
	// ---------------------------------------------------------------------------
	int __stdcall P6SmartClickGuardProc(PInlineX86StackBuffer buf)
	{
		const UnitDefStruct* guardianDef = reinterpret_cast<const UnitDefStruct*>(buf->Ecx);
		const UnitStruct* target = reinterpret_cast<const UnitStruct*>(buf->Edi);

		if (!ShouldRefuseImplicitGuard(guardianDef, target))
			return 0;

		++g_p6Refused;
		buf->rtnAddr_Pvoid = reinterpret_cast<LPVOID>(kP6NoGuardTarget);
		return X86STRACKBUFFERCHANGE;
	}

	// ---------------------------------------------------------------------------
	// P5 router -- runs only inside MissionTick_Follow_Ground's already-gated
	// verbatim-mirror branch (builder-builder pair, target has a qualifying active
	// order). Eax = guarded unit, Ecx = guarded unit's current order, exactly as
	// vanilla has them at this instruction.
	// ---------------------------------------------------------------------------
	int __stdcall P5FollowGroundMirrorProc(PInlineX86StackBuffer buf)
	{
		const UnitStruct* target = reinterpret_cast<const UnitStruct*>(buf->Eax);
		const UnitOrdersStruct* targetOrder =
			reinterpret_cast<const UnitOrdersStruct*>(buf->Ecx);

		if (!target || SafeIsBadReadPtr(&target->UnitType, sizeof(target->UnitType)))
			return 0;
		if (!DefCanFly(target->UnitType))
			return 0;   // ground target: vanilla mirror, unchanged

		// Both fields this router reads, probed individually. The order pointer can
		// already be freed memory, and probing the whole 0x56-byte struct on a per-tick
		// path costs far more for no added safety -- TABugFix.cpp makes the same call.
		if (!targetOrder
			|| SafeIsBadReadPtr(&targetOrder->COBHandler_index,
				sizeof(targetOrder->COBHandler_index))
			|| SafeIsBadReadPtr(&targetOrder->AttackTargat,
				sizeof(targetOrder->AttackTargat)))
			return 0;

		// Vanilla only ever enters the assist construction at 0x00406636 past its own
		// null-target guard at 0x00406629 (`cmp [order+0x16],ebp / je 0x004066AC`), and
		// both redirects below land after it. That guard is reachable, not theoretical:
		// VTOL_MobileBuild's Order_State carries 0x400 (has a position) and not 0x200
		// (has a unit target), so the mirror-branch discriminator at 0x004065E7 admits it
		// without testing the target -- and an air constructor holds that order with
		// AttackTargat still null for the whole flight to the build site, because the
		// nanoframe is only created on arrival (0x004140A1 writes the slot). Constructing
		// HelpBuild on a null target makes MissionTick_HelpBuild bail at 0x00403F9D with a
		// "Construction terminated" announcement, once per tick. Mirror the guard.
		// VTOL_HelpBuild and VTOL_RepairUnit both carry 0x200 and cannot reach here null.
		if (!targetOrder->AttackTargat)
		{
			++g_p5NullTarget;
			buf->rtnAddr_Pvoid = reinterpret_cast<LPVOID>(kP5PlainFollowTarget);
			return X86STRACKBUFFERCHANGE;
		}

		// Resolved lazily, not at Install() time: the table MissionOrder_FindByName
		// searches ([0x512344,0x512348)) is filled once from WinMain, and this DLL's
		// DLL_PROCESS_ATTACH runs before that, so an eager resolve would fail every
		// launch. This router only ever runs inside a live simulation tick, well after
		// WinMain, so resolution succeeds on its first real invocation. Build and repair
		// ids are resolved and flagged independently (see ResolveRepairMissionIds) so a
		// failure in one can never suppress the other.
		//
		// The ids are POSITIONS in that table, so re-resolve if its extent ever changes.
		// It does not change -- but that is an argument, and this makes it an invariant.
		// An empty table also short-circuits the five binary searches instead of retrying
		// them every tick. Both reads are of engine state identical on every client, so
		// neither branch can diverge a lockstep game.
		const DWORD tableSize = MissionTableSize();
		if (tableSize != g_missionTableSize)
		{
			g_missionTableSize = tableSize;
			g_missionIdsResolved = false;
			g_repairIdsResolved = false;
		}
		if (tableSize != 0)
		{
			if (!g_missionIdsResolved)
				ResolveMissionIds();
			if (!g_repairIdsResolved)
				ResolveRepairMissionIds();
		}

		// If it has NOT succeeded yet (should not happen in practice, kept as a
		// belt-and-suspenders case): do not fall through to `return 0` here -- at
		// this specific instruction that means "let vanilla mirror the order
		// verbatim", which is the exact flying-mission-onto-ground-unit hazard this
		// hook exists to prevent. Redirect to plain follow instead; safe either way.
		if (!g_missionIdsResolved && !g_repairIdsResolved)
		{
			++g_p5Suppressed;
			buf->rtnAddr_Pvoid = reinterpret_cast<LPVOID>(kP5PlainFollowTarget);
			return X86STRACKBUFFERCHANGE;
		}

		// VTOL_MobileBuild is the direct case (target itself is building); VTOL_HelpBuild
		// is what a chained air guardian's own order becomes. Both carry the being-built
		// unit in the same AttackTargat field, so both take the identical translation.
		if (g_missionIdsResolved && (targetOrder->COBHandler_index == g_vtolMobileBuildId
			|| targetOrder->COBHandler_index == g_vtolHelpBuildId))
		{
			++g_p5Translated;
			buf->rtnAddr_Pvoid = reinterpret_cast<LPVOID>(kP5HelpBuildTarget);
			return X86STRACKBUFFERCHANGE;
		}

		// VTOL_RepairUnit -> ground RepairUnit, aimed at whatever the target is repairing
		// (targetOrder->AttackTargat -- the same generic field both repair ticks read at
		// their own entry, and the same one the build translation already uses).
		if (g_repairIdsResolved && targetOrder->COBHandler_index == g_vtolRepairUnitId)
		{
			++g_p5RepairTranslated;
			*reinterpret_cast<BYTE*>(buf->Esp + kP5RepairStackIdOffset) = g_groundRepairUnitId;
			buf->rtnAddr_Pvoid = reinterpret_cast<LPVOID>(kP5RepairConstructTarget);
			return X86STRACKBUFFERCHANGE;
		}

		// Any other flying-unit mission: do not mirror a flying-unit order onto a
		// ground unit. Fall back to plain follow.
		++g_p5Suppressed;
		buf->rtnAddr_Pvoid = reinterpret_cast<LPVOID>(kP5PlainFollowTarget);
		return X86STRACKBUFFERCHANGE;
	}

	// ---------------------------------------------------------------------------
	// Self-test -- calls the router functions directly against synthetic,
	// heap/stack-local data, exactly as BuildWeaponSlotGuard.cpp's self-test does and
	// for the same reason: exercises the real decision logic with none of the risk of
	// forcing it at a live hook address. A failed self-test aborts installation.
	//
	// Each sub-case is named and reported individually: a bare pass/fail for a whole
	// group forces whoever reads the log to re-derive which assertion broke.
	// ---------------------------------------------------------------------------
	struct SubCase { bool passed; const char* name; };

	bool ReportSubCases(const SubCase* subs, int count)
	{
		bool ok = true;
		for (int i = 0; i < count; ++i)
		{
			if (!subs[i].passed)
			{
				IDDrawSurface::OutptFmtTxt(
					"[GroundToAirGuard][selftest] FAIL sub-case: %s", subs[i].name);
				ok = false;
			}
		}
		return ok;
	}

	bool SelfTestShouldRefuseImplicitGuard()
	{
		UnitDefStruct groundDef; std::memset(&groundDef, 0, sizeof(groundDef));
		UnitDefStruct airDef; std::memset(&airDef, 0, sizeof(airDef));
		airDef.UnitTypeMask_0 = canfly;

		UnitStruct groundTarget; std::memset(&groundTarget, 0, sizeof(groundTarget));
		groundTarget.UnitType = &groundDef;
		UnitStruct airTarget; std::memset(&airTarget, 0, sizeof(airTarget));
		airTarget.UnitType = &airDef;

		const SubCase subs[] = {
			{ ShouldRefuseImplicitGuard(&groundDef, &groundTarget) == false, "ground guardian + ground target -> allow" },
			{ ShouldRefuseImplicitGuard(&groundDef, &airTarget)    == true,  "ground guardian + air target -> refuse" },
			{ ShouldRefuseImplicitGuard(&airDef,    &groundTarget) == false, "air guardian + ground target -> allow" },
			{ ShouldRefuseImplicitGuard(&airDef,    &airTarget)    == false, "air guardian + air target -> allow" },
			{ ShouldRefuseImplicitGuard(&groundDef, nullptr)       == false, "unreadable target -> allow (vanilla)" },
			{ ShouldRefuseImplicitGuard(nullptr,    &airTarget)    == false, "unreadable guardian -> allow (vanilla)" },
		};
		return ReportSubCases(subs, sizeof(subs) / sizeof(subs[0]));
	}

	// P3 and P6 share a predicate but read the guardian def out of different registers
	// (Esi vs Ecx). That wiring is the part a refactor can silently break, so exercise
	// both routers through the register layout each one actually sees in the engine.
	bool SelfTestImplicitGuardRouters()
	{
		UnitDefStruct groundDef; std::memset(&groundDef, 0, sizeof(groundDef));
		UnitDefStruct airDef; std::memset(&airDef, 0, sizeof(airDef));
		airDef.UnitTypeMask_0 = canfly;

		UnitStruct groundTarget; std::memset(&groundTarget, 0, sizeof(groundTarget));
		groundTarget.UnitType = &groundDef;
		UnitStruct airTarget; std::memset(&airTarget, 0, sizeof(airTarget));
		airTarget.UnitType = &airDef;

		InlineX86StackBuffer buf;

		std::memset(&buf, 0, sizeof(buf));
		buf.Esi = reinterpret_cast<DWORD>(&groundDef);
		buf.Edi = reinterpret_cast<DWORD>(&airTarget);
		const bool p3Refuses = (P3MoveResolverGuardProc(&buf) == X86STRACKBUFFERCHANGE
			&& buf.rtnAddr_Pvoid == reinterpret_cast<LPVOID>(kP3PlainMoveTarget));

		std::memset(&buf, 0, sizeof(buf));
		buf.Ecx = reinterpret_cast<DWORD>(&groundDef);
		buf.Edi = reinterpret_cast<DWORD>(&airTarget);
		const bool p6Refuses = (P6SmartClickGuardProc(&buf) == X86STRACKBUFFERCHANGE
			&& buf.rtnAddr_Pvoid == reinterpret_cast<LPVOID>(kP6NoGuardTarget));

		// The pairings that must stay vanilla. A false redirect here would break
		// ground-to-ground guarding, which every player already relies on.
		std::memset(&buf, 0, sizeof(buf));
		buf.Ecx = reinterpret_cast<DWORD>(&groundDef);
		buf.Edi = reinterpret_cast<DWORD>(&groundTarget);
		const bool p6AllowsGround = (P6SmartClickGuardProc(&buf) == 0
			&& buf.rtnAddr_Pvoid == NULL);

		std::memset(&buf, 0, sizeof(buf));
		buf.Ecx = reinterpret_cast<DWORD>(&airDef);
		buf.Edi = reinterpret_cast<DWORD>(&airTarget);
		const bool p6AllowsAirGuardian = (P6SmartClickGuardProc(&buf) == 0
			&& buf.rtnAddr_Pvoid == NULL);

		const SubCase subs[] = {
			{ p3Refuses,           "P3 (Esi=guardian) ground+air -> plain move" },
			{ p6Refuses,           "P6 (Ecx=guardian) ground+air -> no-guard continuation" },
			{ p6AllowsGround,      "P6 ground+ground -> vanilla, no redirect" },
			{ p6AllowsAirGuardian, "P6 air guardian -> vanilla, no redirect" },
		};
		return ReportSubCases(subs, sizeof(subs) / sizeof(subs[0]));
	}

	bool SelfTestP5Mirror()
	{
		UnitDefStruct groundDef; std::memset(&groundDef, 0, sizeof(groundDef));
		UnitDefStruct airDef; std::memset(&airDef, 0, sizeof(airDef));
		airDef.UnitTypeMask_0 = canfly;

		UnitStruct groundTarget; std::memset(&groundTarget, 0, sizeof(groundTarget));
		groundTarget.UnitType = &groundDef;
		UnitStruct airTarget; std::memset(&airTarget, 0, sizeof(airTarget));
		airTarget.UnitType = &airDef;

		// Real state saved/restored around this test: production resolves ids lazily on
		// first live use (see P5FollowGroundMirrorProc). This self-test runs at Install()
		// time, before WinMain has filled the mission table, so a real resolve here would
		// fail exactly as it does in production and every air-target case below would
		// take the "not resolved" fallback instead of exercising translate/suppress.
		// Force known synthetic ids instead. Build and repair ids are saved/forced/
		// restored as two independent pairs, matching how the router treats them.
		const bool savedResolved = g_missionIdsResolved;
		const BYTE savedVtolId = g_vtolMobileBuildId;
		const BYTE savedGroundId = g_groundMobileBuildId;
		const BYTE savedVtolHelpId = g_vtolHelpBuildId;
		const bool savedRepairResolved = g_repairIdsResolved;
		const BYTE savedVtolRepairId = g_vtolRepairUnitId;
		const BYTE savedGroundRepairId = g_groundRepairUnitId;
		const DWORD savedTableSize = g_missionTableSize;

		InlineX86StackBuffer buf;
		// A real, writable stack slot for buf.Esp: the repair-translate branch writes
		// through it (see kP5RepairStackIdOffset), so every sub-case below needs a
		// valid address there even when that branch isn't the one under test.
		BYTE espScratch[64];

		// Ids unresolved + flying target. Drives the router through the real
		// ResolveMissionIds()/ResolveRepairMissionIds(), whose success depends on whether
		// the mission table happens to be filled yet -- so it asserts only the invariant
		// that holds either way, and the one that matters: a flying target must NEVER
		// return 0 here, because at this instruction that means "let vanilla mirror the
		// order verbatim". Which safe redirect it picks is incidental.
		g_missionIdsResolved = false;
		g_vtolMobileBuildId = 0;
		g_groundMobileBuildId = 0;
		g_vtolHelpBuildId = 0;
		g_repairIdsResolved = false;
		g_vtolRepairUnitId = 0;
		g_groundRepairUnitId = 0;
		UnitOrdersStruct someOrder; std::memset(&someOrder, 0, sizeof(someOrder));
		// Every synthetic order below carries a non-null AttackTargat: a null target is
		// its own sub-case at the end, and would otherwise short-circuit the branch each
		// of these exists to exercise.
		someOrder.COBHandler_index = 5;
		someOrder.AttackTargat = &groundTarget;
		std::memset(&buf, 0, sizeof(buf));
		buf.Esp = reinterpret_cast<DWORD>(espScratch);
		buf.Eax = reinterpret_cast<DWORD>(&airTarget);
		buf.Ecx = reinterpret_cast<DWORD>(&someOrder);
		int rc = P5FollowGroundMirrorProc(&buf);
		const bool unresolvedOk = (rc == X86STRACKBUFFERCHANGE
			&& (buf.rtnAddr_Pvoid == reinterpret_cast<LPVOID>(kP5PlainFollowTarget)
				|| buf.rtnAddr_Pvoid == reinterpret_cast<LPVOID>(kP5HelpBuildTarget)));

		g_vtolMobileBuildId = 7;
		g_groundMobileBuildId = 3;
		g_vtolHelpBuildId = 9;
		g_missionIdsResolved = true;
		g_vtolRepairUnitId = 11;
		g_groundRepairUnitId = 13;
		g_repairIdsResolved = true;

		UnitOrdersStruct mobileBuildOrder; std::memset(&mobileBuildOrder, 0, sizeof(mobileBuildOrder));
		mobileBuildOrder.COBHandler_index = g_vtolMobileBuildId;
		mobileBuildOrder.AttackTargat = &groundTarget;
		UnitOrdersStruct chainedHelpBuildOrder; std::memset(&chainedHelpBuildOrder, 0, sizeof(chainedHelpBuildOrder));
		chainedHelpBuildOrder.COBHandler_index = g_vtolHelpBuildId;
		chainedHelpBuildOrder.AttackTargat = &groundTarget;
		UnitOrdersStruct repairOrder; std::memset(&repairOrder, 0, sizeof(repairOrder));
		repairOrder.COBHandler_index = g_vtolRepairUnitId;
		repairOrder.AttackTargat = &groundTarget;
		UnitOrdersStruct otherOrder; std::memset(&otherOrder, 0, sizeof(otherOrder));
		otherOrder.COBHandler_index = 8;  // matches none of the ids above
		otherOrder.AttackTargat = &groundTarget;
		// The case this module had wrong until PR review: an air constructor flying to
		// its build site holds VTOL_MobileBuild with no target yet.
		UnitOrdersStruct noTargetBuildOrder; std::memset(&noTargetBuildOrder, 0, sizeof(noTargetBuildOrder));
		noTargetBuildOrder.COBHandler_index = g_vtolMobileBuildId;

		// Ground target -> always vanilla (return 0), regardless of order type.
		std::memset(&buf, 0, sizeof(buf));
		buf.Esp = reinterpret_cast<DWORD>(espScratch);
		buf.Eax = reinterpret_cast<DWORD>(&groundTarget);
		buf.Ecx = reinterpret_cast<DWORD>(&mobileBuildOrder);
		DWORD before = g_p5Translated;
		rc = P5FollowGroundMirrorProc(&buf);
		const bool groundOk = (rc == 0 && buf.rtnAddr_Pvoid == NULL && g_p5Translated == before);

		// Air target, VTOL_MobileBuild -> redirect to HelpBuild construction.
		std::memset(&buf, 0, sizeof(buf));
		buf.Esp = reinterpret_cast<DWORD>(espScratch);
		buf.Eax = reinterpret_cast<DWORD>(&airTarget);
		buf.Ecx = reinterpret_cast<DWORD>(&mobileBuildOrder);
		before = g_p5Translated;
		rc = P5FollowGroundMirrorProc(&buf);
		const bool translateOk = (rc == X86STRACKBUFFERCHANGE
			&& buf.rtnAddr_Pvoid == reinterpret_cast<LPVOID>(kP5HelpBuildTarget)
			&& g_p5Translated == before + 1);

		// Air target, VTOL_HelpBuild (a CHAINED air guardian's own order, per
		// MissionTick_VTOL_Follow's mirror logic) -> same redirect as the direct
		// VTOL_MobileBuild case. This is the specific case a guard chain needs.
		std::memset(&buf, 0, sizeof(buf));
		buf.Esp = reinterpret_cast<DWORD>(espScratch);
		buf.Eax = reinterpret_cast<DWORD>(&airTarget);
		buf.Ecx = reinterpret_cast<DWORD>(&chainedHelpBuildOrder);
		before = g_p5Translated;
		rc = P5FollowGroundMirrorProc(&buf);
		const bool chainedTranslateOk = (rc == X86STRACKBUFFERCHANGE
			&& buf.rtnAddr_Pvoid == reinterpret_cast<LPVOID>(kP5HelpBuildTarget)
			&& g_p5Translated == before + 1);

		// Air target, VTOL_RepairUnit -> redirect to the repair construct site, AND
		// the resolved ground RepairUnit id must have actually landed in the stack
		// slot vanilla's own construction sequence reads it back from.
		std::memset(&buf, 0, sizeof(buf));
		std::memset(espScratch, 0xCC, sizeof(espScratch));
		buf.Esp = reinterpret_cast<DWORD>(espScratch);
		buf.Eax = reinterpret_cast<DWORD>(&airTarget);
		buf.Ecx = reinterpret_cast<DWORD>(&repairOrder);
		before = g_p5RepairTranslated;
		rc = P5FollowGroundMirrorProc(&buf);
		const bool repairTranslateOk = (rc == X86STRACKBUFFERCHANGE
			&& buf.rtnAddr_Pvoid == reinterpret_cast<LPVOID>(kP5RepairConstructTarget)
			&& espScratch[kP5RepairStackIdOffset] == g_groundRepairUnitId
			&& g_p5RepairTranslated == before + 1);

		// Ground target, VTOL_RepairUnit id -> still always vanilla. A flying-only id
		// must never trigger construction for a ground-to-ground pairing.
		std::memset(&buf, 0, sizeof(buf));
		buf.Esp = reinterpret_cast<DWORD>(espScratch);
		buf.Eax = reinterpret_cast<DWORD>(&groundTarget);
		buf.Ecx = reinterpret_cast<DWORD>(&repairOrder);
		before = g_p5RepairTranslated;
		rc = P5FollowGroundMirrorProc(&buf);
		const bool repairGroundOk = (rc == 0 && buf.rtnAddr_Pvoid == NULL
			&& g_p5RepairTranslated == before);

		// Air target, some other order -> redirect to plain follow, not mirrored.
		std::memset(&buf, 0, sizeof(buf));
		buf.Esp = reinterpret_cast<DWORD>(espScratch);
		buf.Eax = reinterpret_cast<DWORD>(&airTarget);
		buf.Ecx = reinterpret_cast<DWORD>(&otherOrder);
		before = g_p5Suppressed;
		rc = P5FollowGroundMirrorProc(&buf);
		const bool suppressOk = (rc == X86STRACKBUFFERCHANGE
			&& buf.rtnAddr_Pvoid == reinterpret_cast<LPVOID>(kP5PlainFollowTarget)
			&& g_p5Suppressed == before + 1);

		// Air target, VTOL_MobileBuild but no build target yet -> plain follow. Must NOT
		// construct: vanilla's own guard at 0x00406629 refuses exactly this.
		std::memset(&buf, 0, sizeof(buf));
		buf.Esp = reinterpret_cast<DWORD>(espScratch);
		buf.Eax = reinterpret_cast<DWORD>(&airTarget);
		buf.Ecx = reinterpret_cast<DWORD>(&noTargetBuildOrder);
		before = g_p5Translated;
		const DWORD beforeNull = g_p5NullTarget;
		rc = P5FollowGroundMirrorProc(&buf);
		const bool nullTargetOk = (rc == X86STRACKBUFFERCHANGE
			&& buf.rtnAddr_Pvoid == reinterpret_cast<LPVOID>(kP5PlainFollowTarget)
			&& g_p5Translated == before
			&& g_p5NullTarget == beforeNull + 1);

		// Unsafe target pointer -> vanilla, never a wild redirect.
		std::memset(&buf, 0, sizeof(buf));
		buf.Esp = reinterpret_cast<DWORD>(espScratch);
		buf.Eax = 0;
		buf.Ecx = reinterpret_cast<DWORD>(&mobileBuildOrder);
		rc = P5FollowGroundMirrorProc(&buf);
		const bool unsafeOk = (rc == 0 && buf.rtnAddr_Pvoid == NULL);

		g_missionIdsResolved = savedResolved;
		g_vtolMobileBuildId = savedVtolId;
		g_groundMobileBuildId = savedGroundId;
		g_vtolHelpBuildId = savedVtolHelpId;
		g_repairIdsResolved = savedRepairResolved;
		g_vtolRepairUnitId = savedVtolRepairId;
		g_groundRepairUnitId = savedGroundRepairId;
		g_missionTableSize = savedTableSize;

		const SubCase subs[] = {
			{ unresolvedOk,       "mission ids unresolved + air target -> plain follow, never mirror" },
			{ groundOk,           "ground target -> vanilla mirror untouched" },
			{ translateOk,        "air target + VTOL_MobileBuild -> HelpBuild" },
			{ chainedTranslateOk, "air target + VTOL_HelpBuild (chained) -> HelpBuild" },
			{ repairTranslateOk,  "air target + VTOL_RepairUnit -> RepairUnit, id written to stack" },
			{ repairGroundOk,     "ground target + VTOL_RepairUnit id -> vanilla, no redirect" },
			{ suppressOk,         "air target + other order -> plain follow" },
			{ nullTargetOk,       "air target + VTOL_MobileBuild, no target yet -> plain follow" },
			{ unsafeOk,           "unreadable target -> vanilla, no redirect" },
		};
		const bool ok = ReportSubCases(subs, sizeof(subs) / sizeof(subs[0]));

		return ok;
	}

	bool RunSelfTest()
	{
		struct Case { bool passed; const char* name; };
		const Case cases[] = {
			{ SelfTestShouldRefuseImplicitGuard(),  "P2/P3/P6 shared refusal predicate, all four pairings + unsafe pointers" },
			{ SelfTestImplicitGuardRouters(),      "P3/P6 routers, per-site register wiring + vanilla pairings" },
			{ SelfTestP5Mirror(),                  "P5 mirror translation/suppression/vanilla-fallthrough" },
		};
		const int total = sizeof(cases) / sizeof(cases[0]);
		int passed = 0;
		for (int i = 0; i < total; ++i)
		{
			IDDrawSurface::OutptFmtTxt("[GroundToAirGuard][selftest] %s: %s",
				cases[i].passed ? "PASS" : "FAIL", cases[i].name);
			if (cases[i].passed) ++passed;
		}
		IDDrawSurface::OutptFmtTxt("[GroundToAirGuard][selftest] %s: %d/%d",
			(passed == total) ? "PASS" : "FAIL", passed, total);

		// The self-test above calls P5's router directly, which increments the same
		// module-level counters production does. Reset so a live log's counters and
		// heartbeat reflect real gameplay only -- same reasoning, same fix, as
		// BuildWeaponSlotGuard.cpp's RunSelfTest.
		g_p5Translated = 0;
		g_p5RepairTranslated = 0;
		g_p5NullTarget = 0;
		g_p5Suppressed = 0;
		g_p2Refused = 0;
		g_p3Refused = 0;
		g_p6Refused = 0;

		return passed == total;
	}

	bool g_installed = false;
	bool g_savedValid = false;
	BYTE g_savedP1[sizeof(kP1ExpectedBytes)];
	BYTE g_savedP4[sizeof(kP4ExpectedBytes)];
	std::unique_ptr<InlineSingleHook> g_p2Hook;
	std::unique_ptr<InlineSingleHook> g_p3Hook;
	std::unique_ptr<InlineSingleHook> g_p5Hook;
	std::unique_ptr<InlineSingleHook> g_p6Hook;
}

namespace GroundToAirGuard
{
	void Install()
	{
		if (g_installed)
			return;
		g_installed = true;

#if !GROUND_TO_AIR_GUARD_ENABLE
		IDDrawSurface::OutptTxt(
			"[GroundToAirGuard] DISABLED at compile time (GROUND_TO_AIR_GUARD_ENABLE 0) "
			"-- a ground CanGuard unit still cannot be given a Guard order on a flying "
			"ally, exactly as vanilla. This is the CONTROL arm.");
		return;
#else
		// Validate every site, and every redirect target, BEFORE installing any of
		// them. A partial application (e.g. P1 active but P4 not) would leave a real,
		// clickable Guard order that the simulation still vetoes every tick -- worse
		// than either extreme -- so this is all-or-nothing.
		bool ok = true;
		ok = CheckBytes(kP1RegionAddr, kP1ExpectedBytes, sizeof(kP1ExpectedBytes),
			"P1 cursor DEFEND region") && ok;
		ok = CheckBytes(kP2HookAddr, kP2ExpectedBytes, sizeof(kP2ExpectedBytes),
			"P2 cursor MOVE hook") && ok;
		ok = CheckBytes(kP2NoGuardTarget, kP2NoGuardTargetExpectedBytes,
			sizeof(kP2NoGuardTargetExpectedBytes), "P2 redirect target") && ok;
		ok = CheckBytes(kP3HookAddr, kP3ExpectedBytes, sizeof(kP3ExpectedBytes),
			"P3 resolver MOVE hook") && ok;
		ok = CheckBytes(kP3PlainMoveTarget, kP3PlainMoveTargetExpectedBytes,
			sizeof(kP3PlainMoveTargetExpectedBytes), "P3 redirect target") && ok;
		ok = CheckBytes(kP4RegionAddr, kP4ExpectedBytes, sizeof(kP4ExpectedBytes),
			"P4 sim veto region") && ok;
		ok = CheckBytes(kP5HookAddr, kP5ExpectedBytes, sizeof(kP5ExpectedBytes),
			"P5 mirror hook") && ok;
		ok = CheckBytes(kP5HelpBuildTarget, kP5HelpBuildTargetExpectedBytes,
			sizeof(kP5HelpBuildTargetExpectedBytes), "P5 HelpBuild redirect target") && ok;
		// kP5PlainFollowTarget is intentionally not byte-checked -- see its definition
		// above.
		ok = CheckBytes(kP5RepairConstructTarget, kP5RepairConstructTargetExpectedBytes,
			sizeof(kP5RepairConstructTargetExpectedBytes), "P5 repair construct target") && ok;
		ok = CheckBytes(kP6HookAddr, kP6ExpectedBytes, sizeof(kP6ExpectedBytes),
			"P6 resolver smart-click hook") && ok;
		ok = CheckBytes(kP6NoGuardTarget, kP6NoGuardTargetExpectedBytes,
			sizeof(kP6NoGuardTargetExpectedBytes), "P6 redirect target") && ok;
		ok = CheckBytes(kMissionTableProbeAddr, kMissionTableProbeExpectedBytes,
			sizeof(kMissionTableProbeExpectedBytes),
			"mission table pointers in MissionOrder_FindByName") && ok;
		if (!ok)
			return;

		// Mission ids are NOT resolved here. The table MissionOrder_FindByName searches
		// is filled once from WinMain, and this DLL's DLL_PROCESS_ATTACH runs before
		// that, so an eager resolve would fail every launch and would wrongly abort
		// P1-P4, none of which need it. P5's router resolves lazily on first real use
		// (see its definition) and fails safe until it succeeds.

		if (!RunSelfTest())
		{
			IDDrawSurface::OutptTxt(
				"[GroundToAirGuard] DISABLED: self-test failed against synthetic data "
				"-- see [selftest] lines above. Refusing to trust this logic against "
				"live game data.");
			return;
		}

		std::memcpy(g_savedP1, reinterpret_cast<const void*>(kP1RegionAddr), sizeof(g_savedP1));
		std::memcpy(g_savedP4, reinterpret_cast<const void*>(kP4RegionAddr), sizeof(g_savedP4));
		g_savedValid = true;

		BYTE p1Patched[sizeof(kP1ExpectedBytes)];
		std::memcpy(p1Patched, kP1ExpectedBytes, sizeof(p1Patched));
		std::memcpy(p1Patched + kP1PatchOffset, kP1PatchBytes, sizeof(kP1PatchBytes));
		if (!WriteCode(kP1RegionAddr, p1Patched, sizeof(p1Patched)))
		{
			Shutdown();
			return;
		}

		BYTE p4Patched[sizeof(kP4ExpectedBytes)];
		std::memcpy(p4Patched, kP4ExpectedBytes, sizeof(p4Patched));
		std::memcpy(p4Patched + kP4PatchOffset, kP4PatchBytes, sizeof(kP4PatchBytes));
		if (!WriteCode(kP4RegionAddr, p4Patched, sizeof(p4Patched)))
		{
			Shutdown();
			return;
		}

		g_p2Hook.reset(new InlineSingleHook(
			kP2HookAddr, kP2HookLen, INLINE_5BYTESLAGGERJMP, P2MoveCursorGuardProc));
		g_p3Hook.reset(new InlineSingleHook(
			kP3HookAddr, kP3HookLen, INLINE_5BYTESLAGGERJMP, P3MoveResolverGuardProc));
		g_p5Hook.reset(new InlineSingleHook(
			kP5HookAddr, kP5HookLen, INLINE_5BYTESLAGGERJMP, P5FollowGroundMirrorProc));
		g_p6Hook.reset(new InlineSingleHook(
			kP6HookAddr, kP6HookLen, INLINE_5BYTESLAGGERJMP, P6SmartClickGuardProc));

		IDDrawSurface::OutptFmtTxt(
			"[GroundToAirGuard] installed: cursor unblock @0x%08X, cursor/resolver "
			"move-click refusal @0x%08X/@0x%08X, smart-click refusal @0x%08X, sim veto "
			"removed @0x%08X, mirror fix @0x%08X (build+repair assist, mission ids "
			"resolve lazily on first live guard tick)",
			kP1RegionAddr, kP2HookAddr, kP3HookAddr, kP6HookAddr, kP4RegionAddr,
			kP5HookAddr);
#endif
	}

	void Shutdown()
	{
		// The counters are the only way to tell "feature never triggered" from "module
		// never installed". Reported once, at teardown, so nothing is logged per click or
		// per simulation tick during play.
		if (g_savedValid)
		{
			IDDrawSurface::OutptFmtTxt(
				"[GroundToAirGuard] counters: implicit guard refused cursor=%lu move=%lu "
				"smart=%lu; assist translated build=%lu repair=%lu; not translated "
				"flying-mission=%lu no-target-yet=%lu",
				g_p2Refused, g_p3Refused, g_p6Refused, g_p5Translated,
				g_p5RepairTranslated, g_p5Suppressed, g_p5NullTarget);
		}

		g_p2Hook.reset();
		g_p3Hook.reset();
		g_p5Hook.reset();
		g_p6Hook.reset();

		// Restores the whole saved region, not just the patched bytes. Correct today
		// (nothing else in the project hooks inside either region) and this only runs at
		// process detach, but a future module hooking in those ranges after this one
		// would be clobbered here. The reverse order is already safe: our CheckBytes
		// would fail and the whole module would decline to install.
		if (g_savedValid)
		{
			WriteCode(kP1RegionAddr, g_savedP1, sizeof(g_savedP1));
			WriteCode(kP4RegionAddr, g_savedP4, sizeof(g_savedP4));
			g_savedValid = false;
		}

		g_installed = false;
	}
}
