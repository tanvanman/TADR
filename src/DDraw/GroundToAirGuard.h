#pragma once

// GroundToAirGuard -- lets a ground CanGuard unit be given an explicit Guard order on a
// flying ally, so a ground constructor can assist an air constructor's build or repair.
// Vanilla allows this when the GUARDIAN flies (air->ground, air->air); a non-flying
// guardian could only guard a non-flying target.
//
// Hard requirement: reachable ONLY via the explicit Guard command (button / hotkey G).
// Two separate engine paths can otherwise turn a click into a Guard order -- the
// "Move-click onto a unit becomes Guard" convenience and the smart/right-click default
// order. P3 and P6 close them respectively.
//
// All `[bin]` VERIFIED against TotalA.exe, Escalation GOLD 10.2.0, 1,178,624 bytes,
// md5 1e677a7f92c79b5ab35440853d822c17, ImageBase 0x400000. Struct offsets cross-checked
// against tamem.h.
//
// ---------------------------------------------------------------------------------
// Why six sites and not one
// ---------------------------------------------------------------------------------
// A Guard order is not one mission: it is `Follow_Ground` (ground guardian, tick
// MissionTick_Follow_Ground @0x00406300) or `VTOL_Follow` (air guardian), chosen by the
// GUARDIAN's canfly bit, never the target's. Two functions decide whether a click can
// produce one, and they do not share logic:
//   - Unit_ResolveCursorOrderType @0x0043E490 -- cursor feedback only. Its cached result
//     gates dispatch (`cmp dl,0x11 / jl` @0x00499044). DEFEND case blocks ground->air (P1).
//   - Orders_ResolveMissionNameFromIntent @0x0043F0E0 -- the click-time resolver, called
//     from Unit_IssueOrdersToSelected. Re-derives everything from the intent byte and
//     never reads the cursor's cached code, so a cursor-only fix cannot gate a click.
//     Its DEFEND case already permits ground->air, which is why P1 alone makes the
//     explicit command work. Its MOVE and STOP cases have no target-canfly test, which
//     is why P3 and P6 are mandatory.
//
// Without P4, a ground->air guard order can be issued but is inert:
// MissionTick_Follow_Ground vetoes a flying target every tick (`test dl,1 / je`
// @0x00406350, `mov eax,8`). P4 removes that veto -- which is precisely why P3/P6 are
// not optional: once P4 lands, the implicit paths' already-issuable orders become real.
//
// ---------------------------------------------------------------------------------
// Collision check against this project's own hooks
// ---------------------------------------------------------------------------------
// A scan for every literal address in [0x401000,0x407000) and [0x438000,0x441000) across
// the codebase found exactly one hit in the ranges this module touches: TABugFix.cpp's
// FixedPositionGuardingConsProc, an InlineSingleHook already installed AT 0x004066AC
// (FIXED_POSN_GUARDING_CONS_ENABLE, on in every config that ships this feature). That is
// P5's plain-follow REDIRECT TARGET, not a byte range this module writes. Composing is
// safe: control flow reaching that address enters the same trampoline whether it arrives
// by vanilla fall-through or by this module's redirect, so the STAY/SCATTER guard-spacing
// feature still applies.
//
// CONSEQUENCE: TABugFix installs earlier in DLL_PROCESS_ATTACH than this module
// (ddraw.cpp constructs FixTABug before GroundToAirGuard::Install()), so 0x004066AC holds
// TABugFix's trampoline, not vanilla bytes, by the time this module runs. A vanilla-byte
// CheckBytes there would fail in every shipped config. This module deliberately does not
// byte-check that address; the redirect's safety rests on the control-flow argument above,
// which holds either way.
//
// A byte-granular branch-target scan of the whole .text section confirms nothing branches
// into the middle of any instruction this module steals or rewrites.
//
// Outside src/DDraw, and therefore missed by the sweep above: the Delphi recorder
// (src/Recorder, shipped as eplayx, registered for Escalation via `IniSettings.ModId > 1`)
// splices 9 bytes at 0x0043E4F5 in Unit_ResolveCursorOrderType and 9 at 0x0043F144 in
// Orders_ResolveMissionNameFromIntent -- the same two functions P1/P2 and P3/P6 patch.
// No byte overlap with any of the six windows here. It matters anyway, because P6's
// reachability proof is a CFG walk over VANILLA bytes and 0x0043F144 replaces that
// function's `cmp ecx,0Dh / ja` dispatch prologue at runtime. The proof still holds: the
// wrapper re-implements that test and returns to 0x0043F14D -- the untouched
// `jmp [ecx*4+0x4401EC]` -- for every intent <= 0x0D, which covers STOP(1), MOVE(2) and
// DEFEND(7); its custom-order exits (0x00440194, 0x004401DC, and 0x0043EA5A on the cursor
// side) are all function epilogues ending in `ret`, so no recorder path can enter a hook
// site here. Recorded so a future change on either side can see the coupling.
//
// ---------------------------------------------------------------------------------
// P1 -- cursor DEFEND unblock. In-place byte rewrite.
// ---------------------------------------------------------------------------------
//   0043E637  jne 0x43EAE8                 ; guardian flies -> success, no target check
//   0043E63D  mov edx,[edi+0x92]           ; else edx = TARGET's UnitDefStruct
//   0043E643  test [edx+0x241],eax         ; eax = 0x800 (canfly)
//   0043E649  jne 0x43F098                 ; *** target flies -> FAIL. The block. ***
//   0043E64F  mov eax,5                    ; ground guardian, ground target -> success
// NOP the 6-byte jne so it falls through to the success return ground->ground already
// takes. Reached only after CanGuard+allied passed and only when the guardian cannot fly,
// so every other pairing is byte-identical.
//
// ---------------------------------------------------------------------------------
// P2 -- cursor MOVE cosmetic refusal. InlineSingleHook, 7 bytes. NOT load-bearing.
// ---------------------------------------------------------------------------------
//   0043EAD7  mov eax,[esp+0x20]           ; guardian UnitDefStruct
//   0043EADB  test byte [eax+0x245],0x20   ; <- hook site, 7 bytes
//   0043EAE2  je 0x43EAF5                  ; not guard-capable -> code 0xE (no guard)
// Router: guardian doesn't fly AND target flies -> redirect to 0x43EAF5, the gate's own
// "no guard" exit. Edi (target) verified unwritten across the whole MOVE case body.
// Cursor icon only; without P3 the click still goes through.
//
// ---------------------------------------------------------------------------------
// P3 -- resolver MOVE refusal. InlineSingleHook, 7 bytes. MANDATORY.
// ---------------------------------------------------------------------------------
//   0043F9A4  test byte [esi+0x245],0x20   ; <- hook site. esi = guardian UnitDefStruct
//   0043F9AB  je 0x43F9CD                  ; not guard-capable -> plain move
//   0043F9AD  test ebx,ebx                 ; allied (set in the function prologue)
//   0043F9C2  jne 0x4401C7                 ; guardian flies -> VTOL_FOLLOW (unaffected)
//   0043F9C8  jmp 0x43FBA9                 ; ground -> FOLLOW_GROUND
// Edi (target) verified unwritten from the MOVE case entry (0x43F845) to this gate.
// Router redirects to 0x43F9CD, the gate's own plain-move continuation. The DEFEND entry
// (0x0043F4C7) is deliberately untouched -- it already permits ground->air.
//
// ---------------------------------------------------------------------------------
// P6 -- resolver SMART-CLICK refusal. InlineSingleHook, 6 bytes. MANDATORY.
// ---------------------------------------------------------------------------------
// The resolver's jump table @0x004401EC is indexed `intent-1`. Intent STOP (=1, the "no
// command button active" sentinel a plain click carries) has its OWN handler at
// 0x0043F9E9 with its OWN guard tail, which P3 never touches:
//   0043F9E9  cmp dword [ebp+0x37efa],1    ; interface setting; !=1 reaches no guard site
//   0043FB81  mov ecx,[esp+0x20]           ; guardian UnitDefStruct (stored @0x43F13D)
//   0043FB85  mov eax,[ecx+0x245]
//   0043FB8B  test al,0x20                 ; guardian CanGuard
//   0043FB8D  je 0x43FBC3                  ; -.
//   0043FB8F  test ebx,ebx                 ;  |  allied
//   0043FB91  je 0x43FBC3                  ; -+-> vanilla's "cannot guard" continuation
//   0043FB93  mov eax,[ecx+0x241]          ; <- HOOK SITE, 6 bytes
//   0043FBA9  mov eax,0x5052FC             ; "FOLLOW_GROUND"
// This tail tests CanGuard, allied and the GUARDIAN's canfly -- never the target's.
//
// Why this cannot break the explicit Guard command: a CFG walk of the whole function
// shows 0x0043FB93 is reachable ONLY from the intent-STOP handler. DEFEND (intent 7 --
// the Guard button and hotkey G) and MOVE (intent 2) each do their own checks and
// `jmp 0x0043FBA9` directly, landing PAST this site. Verified mechanically by walking
// from 0x0043F4C7 and 0x0043F845: both reach 0x0043FBA9, neither reaches 0x0043FB93.
//
// Router redirects to 0x0043FBC3, vanilla's own rejection continuation for this tail.
// The stolen instruction is deliberately NOT replayed: that leaves Eax holding
// [ecx+0x245] from 0x0043FB85, exactly the state vanilla's own two `je`s deliver to
// 0x0043FBC3, whose first instruction is `test esi,eax`. Hooking earlier would hand that
// label a stale Eax. DO NOT move this hook.
//
// The `!=1` arm (0x0043FE35) reaches no guard site at all, which is why the Left-Click
// Interface never produced this and why P6 changes nothing for it.
//
// ---------------------------------------------------------------------------------
// P4 -- simulation veto removal. In-place byte rewrite. Class B.
// ---------------------------------------------------------------------------------
//   0040634D  test dl,1                    ; target canfly?
//   00406350  je 0x406361                  ; <- 2-byte patch site
//   00406352  mov eax,8                    ; flying target -> bail, every tick, forever
// je -> jmp. Ground->ground: dl&1 was already 0, so both go to the same place.
//
// Removing the veto exposes TWO order-construction sites further down the tick, not one.
// P5 covers the verbatim mirror; the other needs no patch but should not surprise anyone:
//   0x004064C2  if the guarded unit is damaged ([unit+0x108] < [def+0x1FA]) and the
//               GUARDIAN is a builder ([def+0x241] & 0x40), the tick calls
//               Orders_ResolveMissionNameFromIntent(out, intent=8 REPAIR, guardian,
//               target, 0) at 0x004064F5 and constructs whatever comes back, through its
//               own allocate/ctor/assign at 0x00406502-0x00406537 (id read from
//               [esp+0x14], a different slot from P5's [esp+0x10]).
// Correct without help: intent 8's resolver arm (0x0043F46C) branches on the GUARDIAN's
// canfly only (0x0043F48F / 0x0043F4AB), so a ground guardian resolves the GROUND mission
// -- HELPBUILD or REPAIRUNIT -- whatever the target is, then returns immediately
// (0x0043FA8C / 0x0043FAD5, both `ret 0x14`), reaching neither P3's nor P6's hook site.
// Net effect: a ground constructor guarding a DAMAGED air constructor repairs it, via
// vanilla code. That is the intended behaviour, not a side effect to be suppressed.
//
// ---------------------------------------------------------------------------------
// P5 -- assist translation. InlineSingleHook, 5 bytes (auto-extends to 7). Class B.
// ---------------------------------------------------------------------------------
// With P4 in, MissionTick_Follow_Ground's assist-mirroring logic (0x406549 onward)
// becomes reachable for a flying target. When the guarded unit's order name-matches
// "MobileBuild"/"BuildingBuild" the engine constructs "HelpBuild" instead (0x406636).
// Anything else with the right Order_State bits is mirrored VERBATIM -- which would hand
// a ground unit a flying-unit mission. An air constructor placing ANY build order runs
// VTOL_MobileBuild (the resolver's BUILD entry @0x43F7A0 picks "VTOL_MOBILEBUILD"
// whenever the builder has canfly; no "VTOL_BuildingBuild" mission exists), and that name
// matches neither, so the verbatim mirror is what this hook exists to intercept.
//
//   00406612  mov eax,[ebx+0x16]           ; ebx = guard order (arg, stable); eax = guarded unit
//   00406615  mov ecx,[eax+0x5c]           ; ecx = guarded unit's current order
//   00406618  mov dl,[ecx+4]               ; <- hook site (steals this + next mov, 7B)
//   0040661B  mov [esp+0x10],dl            ; the verbatim mirror
//   0040661F  jmp 0x40664A                 ; -> allocate / construct / assign
//
// Router reads Eax (guarded unit) and Ecx (its order), and dispatches on the order's
// COBHandler_index:
//   - guarded unit doesn't fly -> return 0. Ground->ground is untouched.
//   - guarded unit's order has no target yet (AttackTargat == 0) -> redirect to
//     0x004066AC. Mirrors vanilla's own guard at 0x00406629, which both redirects below
//     would otherwise jump past. See the 'no target yet' note -- this is reachable.
//   - VTOL_MobileBuild, or VTOL_HelpBuild -> redirect to 0x406636 (vanilla's own
//     HelpBuild lookup+construct). Both carry the being-built unit in the same
//     AttackTargat field. See "guard chains" below for why VTOL_HelpBuild belongs here.
//   - VTOL_RepairUnit -> write the resolved ground "RepairUnit" id into the stack slot
//     vanilla reads the id back from, then redirect to 0x40664A. See below.
//   - anything else -> redirect to 0x4066AC, the plain-follow tail (composes with the
//     TABugFix hook there, per the collision note).
// Stack-correct: 0x406618, 0x406636 and 0x40664A are reached from the same ancestor
// (0x4065E7) with net-zero stack delta -- each intervening call is thiscall-with-one-arg
// and cleans its own bytes. Verified by counting pushes on every path, not assumed.
//
// Mission ids are table positions, not constants. Resolved lazily on first live use via
// MissionOrder_FindByName@0x00438760, __thiscall(void* outByteBuf, const char* name),
// which binary-searches a global vector of 25-byte mission templates -- begin/end/capacity
// at 0x00512344/0x00512348/0x0051234C -- and returns (entry - begin) / 25.
//
// That vector is constructed by a CRT static initialiser (0x00438450, registered in the
// .data init table at 0x0050100C) and filled through a single call site on the startup
// path, 0x0049136E -> 0x0043C050, from .rdata templates. Every instruction in the image
// that writes its begin/end pointers belongs to that construct/populate/grow sequence or
// to the atexit destructor (0x00438461/66, 0x00438490/95, 0x0043BD85/8D, 0x0043C135/40,
// 0x0043C355) -- nothing rebuilds or clears it mid-game. Two consequences, both stated
// wrongly in an earlier revision of this file:
//   - Resolution must still be lazy, but because of LOAD ORDER, not because the table is
//     per-map: this DLL's DLL_PROCESS_ATTACH runs before WinMain, so it really is empty
//     at Install() time.
//   - The ids are per-BINARY constants. Mod files do not feed this table, so two clients
//     on the same TotalA.exe cannot resolve different ids, and no mod can fail to supply
//     one of these names.
// The module records the table's extent alongside the ids and re-resolves if it ever
// changes, which turns the paragraph above from an argument into a runtime invariant, and
// skips the lookups entirely while the table is empty. Distinctness is still cross-checked
// to catch a failed lookup before it can collide with index 0. Install() byte-checks
// MissionOrder_FindByName's prologue, covering both table pointers and the /25 stride.
//
// ---- no target yet: why the null check is not optional ----
// Vanilla reaches 0x00406636 from exactly one place, and only past a null-target guard:
//   00406623  je 0x4066AC                  ; name did not match -> plain follow
//   00406629  cmp dword [esi+0x16],ebp     ; guarded unit's order has a target?
//   0040662C  je 0x4066AC                  ; null -> plain follow, do NOT construct
//   00406636  push 0x5013F0                ; HelpBuild
// That guard is reachable, not theoretical. The mirror-branch discriminator at 0x004065E7
// admits an order either because Order_State bit 0x200 is set AND its target is non-null,
// or because bit 0x400 is set -- and the 0x400 arm tests no target at all. The order
// constructor fixes what those bits mean: 0x0043A1A1 clears 0x200 when the target argument
// is null, 0x0043A1B3 clears 0x400 when the position argument is null. Template defaults
// (.rdata, 25-byte entries): VTOL_MobileBuild 0x00100508 (0x400, no 0x200), VTOL_HelpBuild
// 0x00100208 and VTOL_RepairUnit 0x00100200 (both 0x200).
//
// So VTOL_MobileBuild -- the headline case -- takes the arm with no target test, and an air
// constructor holds it with AttackTargat null for the whole flight to the build site: the
// nanoframe only exists on arrival (0x004140A1 writes the order's target slot). Building a
// HelpBuild order on that null target makes MissionTick_HelpBuild bail at 0x00403F9D with
// a Construction-terminated announcement -- one allocate/construct/assign and one
// announcement per tick, for the whole flight. The router mirrors the guard instead.
// VTOL_HelpBuild and VTOL_RepairUnit carry 0x200, so they cannot arrive here null.
//
// ---- repair: why a stack write rather than a redirect ----
// Unlike MobileBuild there is no vanilla path that constructs a generic-target
// "RepairUnit" order, so there is nothing to redirect into. What IS reused is vanilla's
// own construction sequence at 0x40664A (allocate 0x56 / construct / assign), which the
// untouched steady-state mirror already falls into. Both repair ticks read their own
// order's target from the same generic field:
//   MissionTick_RepairUnit      @0x00405300: 0040530B  mov eax,[esi+0x16]
//   MissionTick_VTOL_RepairUnit @0x00414E70: 00414E7B  mov eax,[esi+0x16]
// (tamem.h: UnitOrdersStruct::AttackTargat @0x16 -- the same field P5's existing
// HelpBuild mirror already reads at 0x406668.)
//
// Stack-offset proof that the written byte is the slot vanilla reads back, counting every
// push from the hook site:
//   00406618  ESP here = ESP0;  vanilla would write the mirrored id to [ESP0+0x10]
//   0040664A  push 0x56         ; call+ret nets back to ESP0 after `add esp,4`
//   0040665B  push ebp x3       ; esp = ESP0-0xC
//   00406667  push ecx          ; esp = ESP0-0x10
//   0040666B  mov edx,[esp+0x20] ; = [ESP0+0x10]  <- SAME SLOT, read as missionId arg
// buf->Esp == ESP0 (captured by the trampoline's pushad before any stolen byte runs).
// Writing through buf->Esp is an established idiom here -- TABugFix.cpp,
// TenPlayerReplay.cpp, UnicodeSupport.cpp, unitrotate.cpp and others do the same.
// Skipping 0x406636 is safe: the only thing it computes beyond the id byte is a throwaway
// FindByName scratch buffer at [esp+0x38] that nothing downstream reads. Edx is stale on
// arrival but is overwritten at 0x406658 before any use.
//
// If the repair target is null or dead by the tick this runs, the new order fails through
// the engine's own bailout (0x405312, "cannot comply", return 5) -- the same path a
// player issuing an impossible Repair order takes.
//
// Repair ids are resolved and flagged INDEPENDENTLY of the build ids, so a failure in one
// cannot disable the other.
//
// ---- guard chains: why VTOL_HelpBuild is also translated ----
// If the guarded air unit is itself guarding a further air constructor, it never holds a
// raw VTOL_MobileBuild order. MissionTick_VTOL_Follow @0x0040FBE0 has its own mirror
// branch (0x0040FE68) whose match cascade (0x0040FF52-0x0040FFD3) tests the followed
// unit's order against "MobileBuild"/"BuildingBuild"/"VTOL_MobileBuild" and, on any
// match, constructs **VTOL_HelpBuild** -- same ctor shape, target taken from
// [order+0x16] (0x004100F4). So the first air hop always resolves to VTOL_HelpBuild
// before a ground guardian can see it.
//
// Deeper chains converge on the same id rather than producing new ones: VTOL_Follow's
// other branch (0x0040FFE3-0x004100D6) translates exactly four GROUND-side names to their
// VTOL_ counterparts (REPAIRUNIT, RECLAIMUNIT, VTOL_RECLAIM's ground pair, HELPBUILD).
// None of those match an already-VTOL-prefixed id, so a second or Nth air guardian falls
// through unmatched and the cascade's fallback re-mirrors the incoming id verbatim.
// Recognizing this one id therefore covers a chain of any depth.
//
// Repair needs no equivalent: no "VTOL_HelpRepair" mission exists (multiple simultaneous
// repairers is already normal vanilla behavior, so repair never needed an assist variant),
// and that cascade never rewrites an id that is already VTOL_RepairUnit.
//
// ---------------------------------------------------------------------------------
// Scope and known limits
// ---------------------------------------------------------------------------------
// Any CanGuard ground unit may guard a flying ally, not constructors only. The engine's
// gate has no builder-only test on this path, and Cursor_ResolveModeForSelection keeps the
// MINIMUM cursor code across a selection, so a builder-scoped rule would still let a
// non-builder in a mixed selection receive a real FOLLOW_GROUND order. A non-builder
// simply walks to and holds under the flying ally; the assist machinery above engages only
// for a builder-builder pair (both sides tested against UnitTypeMask_0 bit 0x40,
// UNITINFOMASK_0::builder, at 0x40656A/0x406577).
//
// PRE-EXISTING, not introduced here: the assist-mirror path performs no alliance re-check.
// The alliance test at 0x406393 gates the separate combat-assist sub-feature, not this
// branch, and MissionTick_VTOL_Follow's mirror has no alliance check either. A guard order
// requires allied at issue time; if the guarded unit later changes owner, the assist would
// follow it. This property is identical for the three pairings vanilla already ships;
// P4 extends its surface to ground->air. Fixing it would change behavior for pairings
// outside this feature's scope.
//
// ---------------------------------------------------------------------------------
// Gating: GROUND_TO_AIR_GUARD_ENABLE (config.h / config_*.h). Compile-time only, no runtime
// switch. 1 on Escalation (the build these addresses were verified against), 0 elsewhere --
// a staged rollout, not a belief that the addresses differ.
//
// Class B (uniform simulation change: every client in a game must run the same build).
// FOUR sites change simulation behaviour, not two:
//   - P4 and P5, as described above.
//   - P3 and P6 too, for the refused pairing only. Vanilla DID construct a FOLLOW_GROUND
//     order on a move-click or smart-click onto a friendly aircraft; it was simply inert,
//     because MissionTick_Follow_Ground vetoed it at 0x00406350 and the unit stopped. P3/P6
//     refuse earlier, so the click now resolves to the ordinary position order it would
//     otherwise have produced and the unit walks there. Better behaviour, and required by
//     the hard constraint above -- but a change for that pairing, not a preservation.
// ---------------------------------------------------------------------------------

namespace GroundToAirGuard
{
	// Byte-validates every site and redirect target, and runs a self-test against
	// synthetic data, before installing anything. All-or-nothing: a partial application
	// is not a state this project has reasoned about.
	void Install();

	// Restores original bytes and removes the hooks. Safe to call if Install() failed,
	// was disabled, or did nothing.
	void Shutdown();
}
