#pragma once

// VtolRepairBeamFix -- stop an air constructor emitting the nanolathe beam while its
// repair is not actually being paid for.
//
// Vanilla defect, unrelated to any other module here: MissionTick_VTOL_RepairUnit is the
// ONLY repair tick in the engine that ignores whether the repair tick succeeded before
// spawning the beam effect. Its own ground counterpart and both other repair ticks all
// test it and skip the beam.
//
// All `[bin]` VERIFIED against TotalA.exe, Escalation GOLD 10.2.0, 1,178,624 bytes,
// md5 1e677a7f92c79b5ab35440853d822c17, ImageBase 0x400000.
//
// ---------------------------------------------------------------------------------
// The asymmetry
// ---------------------------------------------------------------------------------
// Unit_ApplyRepairHealProgress @0x0041BD10 returns 1 only when it actually healed:
//   0041BD33  cmp edx,eax / jl        ; already at full HP -> return 0
//   0041BDB7  call 0x00401180         ; UnitResourceSlot_AddEnergyUseIfNotStalled
//   0041BDBC  test eax,eax / je       ; could not pay -> ebp stays 0
//   0041BDCC  mov ebp,1               ; paid and healed
//   0041BDD3  mov eax,ebp             ; <- the return value
//
// Every caller in the image, and what it does with that value:
//   0x00402518  (anon repair tick)        test eax,eax / je  -> skips beam   OK
//   0x0040561A  MissionTick_RepairUnit    test eax,eax / je 0x004056D6       OK
//   0x0040583F  MissionTick_RepairUnitNoMove  test eax,eax / je 0x004058FB   OK
//   0x0041513D  MissionTick_VTOL_RepairUnit   result DISCARDED               BUG
//
// At 0x00415142 the VTOL tick goes straight on to UnitScript_QueryNanoPiece and then
// Fx_SpawnNanoBeamEffect @0x004151EC, unconditionally. So a stalled (or finished) air
// constructor keeps drawing the beam; a ground one stops. The build ticks are NOT
// affected -- MobileBuild/HelpBuild and their VTOL_ counterparts all carry branches
// that skip their beam, so this is the single outlier.
//
// ---------------------------------------------------------------------------------
// The patch -- InlineSingleHook @0x00415142, 5 bytes requested (extends to 7)
// ---------------------------------------------------------------------------------
//   0041513D  call 0x0041BD10              ; E8 CE 6B 00 00  -> Unit_ApplyRepairHealProgress
//   00415142  mov edx,[esi+0x0E]           ; <- HOOK SITE  (8B 56 0E,     3 bytes)
//   00415145  lea ecx,[esp+0x10]           ;               (8D 4C 24 10,  4 bytes)
//   00415149  push ecx / push edx
//   0041514B  call 0x0043E400              ; UnitScript_QueryNanoPiece,  ret 8
//   00415153  push 6 ... push ecx / push edx
//   004151EC  call 0x004720D0              ; Fx_SpawnNanoBeamEffect,     ret 0xC
//   004151F1  push 1                       ; <- REDIRECT TARGET, the shared tail
//
// Router: Eax == 0 -> rtnAddr_Pvoid = 0x004151F1, X86STRACKBUFFERCHANGE. Else return 0
// and the two stolen instructions are replayed. Eax still holds the call's return value
// at the hook site -- nothing writes it in between, and neither stolen instruction does.
//
// The install-time byte check covers the CALL as well as the hook site, so "Eax is the
// return of Unit_ApplyRepairHealProgress" is verified against the live image rather than
// assumed from a disassembly listing.
//
// Stack correctness is not an argument, it is vanilla's own behaviour: the ground tick
// performs the identical jump (`je 0x004056D6` at 0x00405621) over a structurally
// identical region. Both regions push 20 bytes in total and both callees clean their own
// arguments (`ret 8` and `ret 0xC`), so esp at the redirect target equals esp at the hook
// site. Counted per instruction on both sides, not assumed. The target needs only Esi
// (the order), which this hook does not touch.
//
// Opcode lengths under hook/etc.cpp's length disassembler: table_1[0x8B] and table_1[0x8D]
// are both C_MODRM, giving 3 + 4 = 7 -- exactly what the hook steals. Neither is the
// mis-sized 0xC6 entry documented in GroundToAirGuard.h.
//
// ---------------------------------------------------------------------------------
// Scope
// ---------------------------------------------------------------------------------
// Fires only for a flying unit running VTOL_RepairUnit whose repair tick did not heal,
// i.e. stalled on energy or the target already at full HP. Both are exactly the cases the
// three sibling ticks already treat this way.
//
// Class B (uniform simulation change; every client in a game must run the same build).
// The skipped region is not purely cosmetic: UnitScript_QueryNanoPiece runs a COB script
// function, and a unit whose script cycles nano emitters advances state there. Skipping it
// is precisely what the ground ticks already do under the same condition, but it is a
// simulation-visible difference, so it is gated and rolled out like one rather than being
// called a VFX-only change.
//
// Gating: VTOL_REPAIR_BEAM_FIX_ENABLE (config.h / config_*.h). Compile-time only. 1 on
// Escalation (the build these addresses were verified against), 0 elsewhere.
// ---------------------------------------------------------------------------------

namespace VtolRepairBeamFix
{
	// Byte-validates the hook site, the preceding call and the redirect target, and runs
	// a self-test against synthetic register state, before installing anything.
	void Install();

	// Removes the hook. Safe to call if Install() failed, was disabled, or did nothing.
	void Shutdown();
}
