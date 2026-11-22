#pragma once

// BuildWeaponSlotGuard -- fixes the stockpile ("Nanolathing") build-percent
// divide-by-zero crash and a separate, adjacent out-of-bounds bug in the same order
// type. Full derivation, byte evidence and test plan: local project notes referenced
// from the PR; PR #26 (Axle1975 review, 2026-09-14) supplied the real root cause and
// is cited throughout this file by section number below.
//
// ---------------------------------------------------------------------------------
// ROOT CAUSE, corrected 2026-09-14. `[bin]` VERIFIED against real production crashes.
// ---------------------------------------------------------------------------------
// This module originally shipped believing a degenerate stockpile weapon's `reloadtime`
// TDF value was the cause (see "FIX 1" below) and treated the weapon-slot index bug as
// unconfirmed insurance (see "FIX 2"). Both binary claims were and are correct; the
// causal claim was not. PR #26's review recovered FIVE independent production crashes
// at the exact fault (0x00439D65, `idiv esi`, `esi==0`) from real `game_logs` and, for
// every one, reconstructed the faulting `WeaponStruct*` (`ecx` at the fault, recovered
// via `ebp`'s known displacement from a static viewport-table base -- see the review
// for the arithmetic). **All five are `&WeaponsTypedefArray[0]`** -- TA's own permanent
// "no weapon" sentinel entry -- not a weapon with a bad TDF, and not an out-of-range
// index either (the resolved pointer was a perfectly valid array entry).
//
// The real mechanism, re-verified against this project's own binary, not merely taken
// on the review's word:
//   - `LoadUNITINFO` (0x0042CDE3-0x0042CE0C) resolves each of a unit type's three
//     weapon names; on a failed/absent lookup it falls back to `&WeaponsTypedefArray[0]`
//     (`lea esi,[eax+0x2CF3]` then `mov eax,esi` when the lookup returned 0). So every
//     unarmed weapon slot on every unit type points at entry 0, always.
//   - `LoadWeapons_Tdf` (0x0042E31C) initialises the whole 256-entry array with
//     `ID=index` and an empty name; entry 0 is NEVER subsequently parsed from any TDF.
//     Its `reloadtime` (+0xE4) is therefore 0 by construction, forever, in every mod --
//     "FIX 1" below cannot touch it: that fix only runs from inside the TDF loader, and
//     the sentinel never passes through the TDF loader.
//   - `UNITS_StartWeaponsScripts` (0x0049E070) copies `UNITINFO::weaponN` straight into
//     `UnitWeapons[N].p_Weapon`, so `p_Weapon` is a pure function of the LOCAL unit
//     type. If this client's copy of a unit has the wrong type, and a legitimately
//     issued order references a weapon slot that is unarmed on this client's (wrong)
//     copy, `p_Weapon == &WeaponsTypedefArray[0]` and the HUD divides by zero the
//     moment anyone inspects that unit.
//   - `DrawUnitBottomState` (0x0046A860, reaches this whole call chain) filters the
//     displayed unit by `UnitINFOID != 0` and LOS only -- there is NO owner check --
//     so the crashing unit can be, and for this mechanism must be, a REMOTE player's
//     unit. This is why the crash killed only 2 of 9 players in one real game: a bad
//     TDF would kill everyone who inspects that weapon; a wrong local copy of one
//     specific remote unit kills only the client(s) whose copy diverged.
//   - This is the exact mechanism `tamem.h`'s `WeaponStruct::weaponvelocity` comment
//     (the neighbouring `+0x68` field) and `config.h`'s `WEAPONFIRE_DISPATCH_FROM_SLOT`
//     block already document, for a different consumer of the same sentinel. It should
//     have been connected to this bug from the start via a sibling-defect scan and
//     was not -- recorded here as an accountability note, not just a citation.
//
// This is unit-identity divergence, the same family `UnitIdentity.{h,cpp}` exists to
// diagnose and repair. It is a per-client state bug, not a data-authoring bug, and no
// TDF edit can fix it.
//
// ---------------------------------------------------------------------------------
// THE FIX for the above: ClassifyWeaponSlot(), called from both Fix 2 routers below.
// ---------------------------------------------------------------------------------
// Resolves the order's weapon slot to the actual WeaponStruct* and checks the divisor
// at the point of use, whatever made it zero. Verdicts: ok, zeroDivisor (the sentinel),
// badIndex, unreadableOrder / unreadableUnit / unreadableWeapon (corruption). What each
// consumer does with a non-ok verdict:
//   - HUD (an integer `idiv`, where all five PR #26 prod crashes are): return 0 -- no
//     progress bar. Display only, no state touched.
//   - Sim, zeroDivisor: OBSERVE ONLY, bit-identical to vanilla. Its divides are masked
//     x87 and survive a zero divisor (no integer divide in the function), and PR #26
//     reports 0 of 252 prod crash bundles faulting there. Why not bail out: every exit
//     of that function changes order state, and the only generic one (return 7) makes
//     the dispatcher free ALL of the unit's orders -- on a client that is already
//     diverged, making it worse (PR #26 F7).
//   - Sim, corrupt order: bail out via return 7, the exit vanilla itself takes for a
//     corrupt State byte on the same order. Prevents a write outside the weapon-slot
//     array. Never observed live.
// So the sim side only ever changes behaviour for an order that is already corrupt.
//
// ---------------------------------------------------------------------------------
// FIX 1 -- kept as insurance, NOT the fix for the crashes above. `[bin]` VERIFIED.
// ---------------------------------------------------------------------------------
// WeaponDef_LoadTdfProperties @0x0042E440 stores a weapon's `reloadtime` TDF key as
// `WeaponStruct+0xE4 = (WORD)(int)(reloadtime * 30.0)` (0x0042E54B-0x0042E561) and its
// `stockpile` key into bit 28 of `WeaponStruct+0x111` (0x0042EA39-0x0042EA5F). Nothing
// checks that a stockpile weapon actually got a usable reloadtime: an absent key
// (TA's TDF getters default to 0), an explicit 0/negative, or a reloadtime whose *30
// lands on an exact multiple of 65536 (the `mov word` truncation, smallest case
// ~2184.53s) all produce `WeaponStruct+0xE4 == 0`. This clamps that field from 0 to 1
// (the smallest non-degenerate value) once, at the moment TDF loading finishes for a
// weapon that is already degenerate (stockpile set AND the stored divisor is 0). A
// correctly authored weapon is untouched, bit for bit.
//
// An offline scan of the operator's live Escalation data found zero weapons matching
// this shape (240 weapons scanned, all ten stockpile weapons have healthy explicit
// reloadtime values) -- consistent with, not contradicted by, this fix never being the
// explanation for a real crash. It stays in as free, zero-behaviour-change hardening
// for any mod's data (or a future edit) that hits this authoring mistake for real; it
// is not required for, and does not explain, any crash this module has actual evidence
// for. It is the only guard for such a weapon on the sim side, which just observes a
// zero divisor: there, vanilla builds the shot instantly and at zero cost (both cost
// deltas cancel to 0). Hook site: 0x0042F313, the function's own epilogue
// (`push ebp; call 0x49E010`), which runs after both fields are stored. EBP (the WeaponStruct* this function
// operates on) is written exactly once, at 0x0042E489, and never reassigned before
// 0x0042F313 -- confirmed by disassembling the complete function and enumerating every
// instruction that writes EBP.
//
// ---------------------------------------------------------------------------------
// FIX 2 -- a separate, adjacent defect found in the same code paths. `[bin]` VERIFIED.
// ---------------------------------------------------------------------------------
// UnitOrdersStruct::BuildUnitID (+0x36) is read as a weapon-slot index by both
// consumers above with NO bounds check against the only three real slots, {0,1,2}
// (UnitStruct::Weapon1/2/3 are the only weapon pointers a unit has -- tamem.h,
// static_assert-confirmed strides). An index of 3 or more walks off the end of the
// per-slot array into adjacent UnitStruct fields:
//   - Sim (MissionTick_BuildWeapon, unconditional load at 0x00402B89, BEFORE the
//     order's State dispatch): idx=3 lands on YPos__'s low byte -- silent desync;
//     idx=4 lands on FirstUnit's low byte -- a pointer smash.
//   - HUD (Unit_GetLinkedBuildWeaponPercent, 0x00439D51): the same stride reads a
//     non-pointer as a WeaponStruct*, then dereferences it at +0xE4.
//
// +0x36 is reused with DIFFERENT meaning by other, unrelated mission-tick handlers
// elsewhere in the binary (confirmed by a whole-.text scan for the same struct
// offset: other functions compare it against 2 or 3 for entirely different order
// types). That is exactly why this fix hooks INSIDE the two functions that are only
// ever reached for a BuildWeapon order, rather than attempting any kind of global
// clamp on the field -- a global clamp would silently reinterpret data other order
// types depend on. Scoping the check to these two call sites is what makes it safe.
//
// This is a real, independent hazard confirmed correct by PR #26's review -- but it
// is NOT what caused the five known production crashes (the fault is at the `idiv`,
// not at the earlier weapon-slot dereference, and `ecx` at the fault was a valid
// array entry every time; an out-of-range index overwhelmingly produces an
// ACCESS_VIOLATION at 0x00439D5D instead). Kept as a genuine guard on a genuinely
// unchecked read/write; the claim that it might explain the original crash is
// withdrawn.
//
// FIX: bounds-check the index at the top of each function and, if it is out of range
// (or the order pointer itself is not safely readable), redirect to that SAME
// function's own existing bail-out path rather than fabricating a new one:
//   - Sim: redirect to 0x00402BA4, vanilla's `return 7` for a corrupt State byte. Not
//     harmless: the dispatcher's case 7 (0x0043BA3D) frees every order on the unit's
//     main and background lists. Acceptable only for an order that is already corrupt,
//     which is why the zero-divisor case above does not use it.
//   - HUD: redirect to 0x00439D6B, the function's own `xor eax,eax` return-0, already
//     an exercised path today whenever BackgroundOrder is null or the order list is
//     exhausted.
// Every non-ok verdict logs and drops a TRACE_CAT_BWSG breadcrumb (unit, slot, resolved
// weapon, verdict, GameTime), both throttled so a persistently diverged unit cannot
// flood the log or the crash-trace ring. Same two hooks for both checks -- no new patch
// surface.
//
// A note on the relative CALL stolen by the Fix 1 hook (`call 0x49E010`): copying a
// relative `E8` displacement into a trampoline reads like a bug until you check that
// it is relocated. `X86RedirectOpcodeToNewBase` (hook/etc.cpp, `case 0xe8`) does
// relocate `E8` displacements whose target lands outside the stolen byte range, so
// this is safe -- re-verified by reading that code, not assumed.
//
// ---------------------------------------------------------------------------------
// Gating: BUILD_WEAPON_SLOT_GUARD_ENABLE (config.h / config_*.h). 1 on Escalation,
// 0 elsewhere. Corrected 2026-09-14: this used to say every address here was specific
// to Escalation's TotalA.exe. That was not established and PR #26's review disproved
// it -- all six hook/bail-out signatures match byte-for-byte on all seven shipped
// TotalA.exe builds (Escalation included), because this is stock TA engine code no
// mod has patched. This project has not independently re-verified that itself, so the
// gate stays Escalation-only until it does -- a staged rollout, not a belief that the
// addresses differ elsewhere. Disabled arm still identifies itself in the log so a
// module that failed its own byte check cannot be mistaken for one compiled out.
// ---------------------------------------------------------------------------------

namespace BuildWeaponSlotGuard
{
    // Byte-validates all three sites before installing any of them. Idempotent.
    // Runs an in-process self-test against synthetic data first (see the .cpp) and
    // refuses to install if that fails -- logic that misbehaves on synthetic input is
    // not trusted against live game data either.
    void Install();

    // Restores original bytes. Safe to call if Install() failed, was disabled, or did
    // nothing.
    void Shutdown();
}
