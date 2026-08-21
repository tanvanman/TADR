#pragma once

// Real 32-bit unit health. See ai-reference/units-hp-rework/CLAUDE.md and the
// project plan for the full design, the Phase 1 live-test results that validated
// it (T1-T5, all exact), and ai-reference/units-hp-rework/verify_proxy_math.py
// (33,385,047-case exhaustive proof of the proxy round-trip below).
//
// THE PROBLEM: Escalation's high-tier units fake their stat-panel HP. A `corkrog`
// shows "180,000" but its real `MaxDamage` is 24,480 -- the rest is simulated by a
// `DamageModifier` that scales incoming damage down. This is forced by two engine
// ceilings: `Unit+0x108` (current HP) is a signed 16-bit field (hard ceiling
// 32,767), and 30,000 is the engine's universal "kill this unit" damage token, a
// tighter ceiling that self-destruct/reverse-build/kill-all/ownership-transfer all
// rely on. Both are `[game]` VERIFIED, not inferred (T4, T5).
//
// THE FIX: authoritative HP moves to a DLL-side int32 table (`realHP`), keyed by
// `UnitInGameIndex` so it survives array-slot recycling correctly (H3) and never
// depends on a pointer value (determinism). The engine's own `Unit+0x108` becomes a
// *scaled proxy* -- small enough to fit its 16-bit home and stay under the 30,000
// token -- computed as:
//
//     S        = ceil(realMax / 24480)
//     proxyMax = realMax / S              (written over UnitDef+0x1FA at load)
//     proxyHP  = clamp(realHP * proxyMax / realMax, 0, proxyMax)
//                with proxyHP >= 1 whenever realHP >= 1   <-- load-bearing:
//                alive must never look dead to the engine (see H1)
//
// Every *ratio*-consuming vanilla reader (health bar, `CobGet_HEALTH`, the ~25
// "is this unit damaged?" mission-tick tests) keeps working completely unmodified,
// because the proxy is calibrated to preserve that ratio. Correction 2026-08-19:
// an earlier draft of this design assumed a raw-*number* HP reader existed (the
// unit selection panel) and would need its own hook. It doesn't -- see
// "Hud_DrawSelectionInfo" below, which turned out to be a false alarm. The only
// remaining raw-number concerns are persistence paths (savegame, net sync).
//
// SCOPE OF THIS BUILD -- read before assuming anything not listed here is covered:
//
//   Implemented, hooked, and installed by Install():
//     - Def-load: UnitDef+0x1FA becomes proxyMax instead of raw MaxDamage (0x42C39E)
//     - Spawn: both branches of Unit_InitInstanceFields (0x485B1E full HP,
//       0x485B37 nanoframe = 0), each clearing-then-setting the table slot (H3)
//     - The two live HP-changing writes in Unit_ApplyNetDamagePacket: heal-add
//       (0x489D80) and damage-subtract (0x489EB5). The third write in that
//       function, the zero-on-death at 0x489EF1, needs NO hook at all -- see the
//       comment on WideHealth_DamageSubtractWrite for why that isn't an oversight.
//     - Resurrect (0x405226): realHP = 1
//     - The kill-token hole (H2): Unit_HandleKilledPacket (0x4867FE) forwards a
//       damage TYPE read from the network packet, not a constant, so keying
//       instakill on type alone misses it. Hooked by call site instead
//       (post-call at 0x486810): realHP forced to 0 unconditionally, regardless
//       of what type value reached the type-dispatch table.
//     - H10, fixed 2026-08-19: self-destruct/kill-all (type 3), transfer (type 4)
//       and reverse-build (type 9) all push exactly 30,000 as an unconditional
//       kill idiom. At MaxDamage <= 24480 that always worked; under real 32-bit
//       HP it didn't, because nothing checked the type. Fixed inside the existing
//       damage-subtract hook (0x489EB5) -- the type rides unscaled in the packet
//       at +8, so no new hook site was needed.
//     - kBigDamageInstakill, decided 2026-08-19: anything using the engine's own
//       armour-bypass idiom (raw damage >= 30,000 -- a D-Gun, in practice) must
//       still one-shot a widened unit, matching the guarantee it already has
//       against every stock unit. Unlike H10, this genuinely needed a new hook
//       (0x489BCD, inside Unit_ApplyDamageAndBroadcast, before the armour/
//       veterancy scaling) because ordinary weapon damage carries no dedicated
//       type -- keying on the post-scaling *amount* would be the exact Hard
//       Rule 6 mistake (a vet-5 target sees 24,000 for the same 30,000 token).
//
//   Deliberately NOT YET hooked -- each of these needs its own dedicated
//   verification pass (the kind RepairRateFix.cpp got, not a first pass), and
//   until then falls back safely rather than doing nothing or doing something
//   wrong. See "Graceful degradation" below for exactly what that fallback is.
//     - Unit_ApplyBuildProgressDelta (0x41BC22 / 0x41BC89): a unit under
//       construction ramps its PROXY up at vanilla's rate, which targets
//       proxyMax, not realMax -- so it converges S times too fast relative to
//       what realHP should be doing. Needs its own delta-source trace before a
//       hook is written, the same way RepairRateFix's formula was extracted
//       before it was ported.
//     - RepairRateFix's own repair formula (0x41BD10, already fully replaced by
//       that module) reads `nMaxHP` -- which is now the proxy -- so a widened
//       unit repairs S times too slowly. The fix is a one-line change in
//       RepairRateFix.cpp once this module can hand it `GetRealMax()` (declared
//       below, ready for that call), but the two modules are not wired together
//       yet.
//     - Savegame (0x487846 / 0x4871B5), ownership transfer (0x488730 / 0x48877E),
//       scenario/map-placed units (0x4884A5), and net unit-state sync
//       (0x48B235 / 0x48B4B2) don't persist or migrate `realHP` yet.
//
//   NOT a gap -- verified and corrected 2026-08-19: the selection panel around
//   0x46B052 (`[bin]` VERIFIED, deep inside an un-prologued HUD draw routine) was
//   assumed to show a raw HP number and need its own hook. It doesn't need one
//   because it doesn't exist: that code computes a fill fraction from
//   `UnitDef+0x1FA` and `Unit+0x108` and draws two coloured bar segments through
//   generic primitives (0x4C14F0, 0x4BF6F0) -- no text-formatting call anywhere in
//   that span. (The nearby resource readouts -- "+%.1f" etc. -- ARE numeric text,
//   but they're Metal/Energy rates, not HP; confirmed no "%d/%d"-shaped or
//   HP-labelled string exists anywhere in .rdata/.data.) A ratio-based bar is
//   already correct by construction under this design -- the whole point of the
//   proxy is that every ratio consumer needs no hook at all.
//
//   Graceful degradation for everything above: the two hooked HP-write sites
//   (heal-add, damage-subtract) compare the engine's *live* proxy value against
//   what this module's own table says it should be before touching anything. A
//   mismatch means something changed `Unit+0x108` through a path this module
//   doesn't yet track -- exactly what an unhooked build-progress tick, savegame
//   load, or ownership transfer looks like. On mismatch, `realHP` is silently
//   re-adopted from the live proxy (`liveProxy * S`) and a canary counter ticks.
//   Consequence: no crash, no exploit, no silent HP loss -- just a precision reset
//   to within one `S` of the last unhooked change, resolved automatically the next
//   time the unit takes damage or is healed through a hooked path. This is the
//   same tolerance already accepted and proven safe for net-resync precision
//   (`verify_proxy_math.py`'s worst case: 250 HP on `corms`, 0.006% of its health).

#include "tamem.h"

namespace WideHealth {

    // Installs every hook listed above. Idempotent. Tables are allocated lazily on
    // first use (Install() runs at DLL_PROCESS_ATTACH, before the engine has
    // allocated the unit or UnitDef arrays -- there is nothing to size against yet).
    void Install();

    // Restores every hooked site and frees both tables. Safe to call multiple times.
    void Shutdown();

    // For future integration (RepairRateFix's nMaxHP -> realMax switch, and any
    // other module that legitimately needs a unit type's *real* max HP rather than
    // the small proxy the engine itself sees). Returns the type's realMax, or -1 if
    // the type table isn't ready yet or `def` doesn't resolve to a known type --
    // callers MUST treat -1 as "fall back to vanilla `def->nMaxHP`", never as 0.
    __int32 GetRealMax(UnitDefStruct* def);

}
