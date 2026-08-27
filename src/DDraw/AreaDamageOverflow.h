#pragma once

// AreaDamageOverflow -- lets a single explosion damage EVERY airborne unit standing
// on a map cell, instead of only the one that happens to own the cell's air slot.
//
// THE BUG (all `[bin]` VERIFIED, see ai-reference/roach-muat-fix/CLAUDE.md):
//   The map is a grid of 13-byte cells. Each cell has exactly TWO unit slots:
//     cell+0x00 = slot A, claimed by units with (UnitStateMask & 3) == 1  (ground)
//     cell+0x02 = slot B, claimed by everything else -- aircraft live here
//   Weapon_ApplyAreaDamageAndBroadcast @0x0049A120 finds its victims by walking the
//   cells inside the blast and reading those two slots. Stack three transports on one
//   cell and the grid can only name one of them, so the other two are not "tanking"
//   the blast -- they are structurally invisible to it. Confirmed live 2026-08-22: a
//   stacked CORMUAT took 0 of ~20 missile impacts over ~1000 ticks.
//
// THE FIX -- extend the victim SOURCE, change none of the damage math:
//   1. Splice the per-cell slot dispatch at 0x0049A214 so slot selectors 0 and 1 still
//      return vanilla's own slot A / slot B reads byte-for-byte, while selectors 2..N-1
//      return additional airborne occupants from a DLL-side index.
//   2. Widen the per-cell loop bound at 0x0049A41A from 2 iterations to N.
//   3. Rebuild that index from scratch every game tick from the live unit array.
//   4. Dedup victims per explosion by UnitInGameIndex, which also repairs a
//      pre-existing vanilla bug (see below).
//   The blast rect math, distance falloff, EdgeEffectivnes curve, damage values, the
//   firer self-exclusion at 0x0049A259 and the network broadcast are ALL untouched.
//   No new floating point anywhere in this module.
//
// SCOPE -- AIR ONLY, deliberately:
//   The index is built exclusively from units with (UnitStateMask & 3) == 2. Ground,
//   naval and building splash takes the identical slot A / slot B path it takes today.
//   Live-confirmed 2026-08-22 (T4) that ground units cannot stack in the first place --
//   three CORCKs ordered onto one spot settled on three distinct cells -- so air-only
//   scope is not merely conservative, it is complete for the reachable bug.
//
// SIDE EFFECT, INTENTIONAL -- the 20-victim dedup cap:
//   Vanilla keeps a per-explosion victim list so a multi-cell unit is not hit once per
//   cell it occupies. That list is a 20-entry stack array (`cmp ecx, 0x14` @0x0049A28F).
//   Past 20 distinct victims it STOPS RECORDING but KEEPS DAMAGING, so unit 21+ takes
//   the same blast once per cell it stands on -- up to 25x for a 5x5 CORMUAT. Widening
//   the victim set makes that cap easier to reach, so this module MUST dedup or it would
//   turn a latent bug into a reliable exploit. Our dedup is by UnitInGameIndex with no
//   fixed cap, so for blasts under 20 victims behaviour is identical to vanilla and past
//   20 it is strictly more correct. Set AREA_DAMAGE_OVERFLOW_FIX_DEDUP_CAP to 0 to
//   restrict dedup to overflow slots only and leave vanilla's ground behaviour bit-exact.
//
// DETERMINISM (lockstep -- ENGINE_NOTES.md §10, §23.4):
//   Integer arithmetic only. Identity is UnitInGameIndex, never a pointer value. The
//   index is rebuilt in unit-array order, so insertion order is identical on every
//   client. This is a Class B patch: every player must run the same build, and old
//   demos will not replay correctly.

namespace AreaDamageOverflow
{
    // Idempotent. Validates the exact original bytes at every patch site first and
    // disables itself with a logged message rather than corrupting the game if any
    // site does not match.
    void Install();

    // Reverts every patch and frees the index. Safe to call if Install() failed.
    void Shutdown();
}
