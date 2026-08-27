#pragma once

// GridClaimTieBreak -- makes "which unit wins a contested map cell" produce the same
// answer on every client, instead of an answer that depends on whose screen you are
// looking at.
//
// This is Option D from ai-reference/roach-muat-fix/CLAUDE.md §5.4, and bug B in the
// "Found along the way" section of Air_Transport_Stacking_Report_for_Wotan.pdf. It is
// SEPARATE from AreaDamageOverflow (bug A / the stacking immunity) and is toggled
// independently -- see the scope warning at the bottom.
//
// THE BUG (`[bin]` VERIFIED):
//   When a unit claims a map cell that is already occupied, Unit_ClaimFootprintCells
//   @0x0047C790 decides whether to evict the incumbent with:
//
//       mov edx, [ecx+0x96]           ; incumbent's owning PlayerStruct
//       cmp dword ptr [edx], 0        ; PlayerActive?
//       je  KEEP
//       cmp byte ptr [edx+0x73], 3    ; My_PlayerType == Player_RemoteHuman?
//       je  EVICT
//
//   i.e. "evict the incumbent if it belongs to a remote human." But local-vs-remote is
//   not a property of a player, it is a property of a player AS SEEN FROM ONE MACHINE.
//   The same human is Player_LocalHuman (1) on their own client and Player_RemoteHuman
//   (3) on everyone else's. So for the same two units:
//     * on the owning client   -> incumbent is type 1, never evicted, FIRST claimer holds
//     * on every other client  -> incumbent is type 3, always evicted, LAST claimer holds
//   Two clients genuinely disagree about which unit is recorded on the cell. Confirmed
//   live 2026-08-22 (T2): in single player, where everything is type 1, the occupant was
//   stable across 60s with three transports contesting -- the "never evicted" half of
//   the prediction, and a direct refutation of the original repro doc's claim that the
//   occupant flips unconditionally.
//
// WHY IT HAS NOT VISIBLY BROKEN VANILLA (INFERRED, not proven):
//   Damage is applied through a single net-broadcast choke point rather than recomputed
//   independently per client, so one machine decides who got hurt and tells the others.
//   That contains the disagreement for damage. It does NOT contain it for anything else
//   that reads the grid -- projectile collision @0x0049B090 and the VTOL landing check
//   @0x0047E2D0 both do.
//
// THE FIX:
//   Replace the client-relative discriminator with a total order over engine-assigned
//   simulation state: THE LOWER UnitInGameIndex WINS THE CELL.
//
//       mov dx, [esi+0xA8]            ; claimant's index
//       cmp dx, [ecx+0xA8]            ; vs incumbent's index
//       jb  EVICT                     ; strictly lower -> take the cell
//                                     ; otherwise fall through to KEEP
//
//   UnitInGameIndex is what the grid already stores, is assigned by the engine, and is
//   explicitly safe to key on in lockstep (ENGINE_NOTES.md §10). Because it is a total
//   order the outcome cannot oscillate and does not depend on arrival history: whichever
//   unit has the lower index ends up holding the cell and stays there. A unit that
//   re-claims a cell it already owns compares equal to itself, does not take the EVICT
//   branch, and therefore behaves exactly as vanilla does today in that case.
//
//   Applied at all THREE structurally identical sites in Unit_ClaimFootprintCells:
//     0x0047C847  yardmap / building path
//     0x0047C950  slot A (ground layer)
//     0x0047CA2C  slot B (air layer)
//
// SIDE BENEFIT: the incumbent's PlayerStruct is no longer dereferenced at all, so a unit
// carrying a null or stale owner pointer can no longer fault here.
//
// ---------------------------------------------------------------------------------
// SCOPE WARNING -- READ BEFORE ENABLING
// ---------------------------------------------------------------------------------
// Unlike AreaDamageOverflow, which is air-only by construction, this patch sits in the
// claim path used by EVERY unit type on BOTH layers -- ground, air, naval, buildings.
// It changes which unit is recorded on a contested cell, which in turn can change:
//   * who direct-fire projectiles collide with (0x0049B090 reads the same slot)
//   * VTOL landing decisions (0x0047E2D0)
// It also drops vanilla's PlayerActive check, so an inactive player's lingering unit can
// now be displaced by a lower-index claimant where previously it could not.
//
// On the owning client this is a genuine behaviour change: today the FIRST claimer holds
// a contested cell, afterwards the LOWEST-INDEX claimer does. That is the price of making
// every client agree, and it cannot be avoided by any rule that is not client-relative.
//
// Class B patch (ENGINE_NOTES.md §23.4): every player must run the same build, and old
// demos will not replay correctly.

namespace GridClaimTieBreak
{
    // Idempotent. Byte-signature validates all three sites and disables itself with a
    // logged message rather than patching anything if any site does not match.
    void Install();

    // Restores the original bytes. Safe to call if Install() failed or did nothing.
    void Shutdown();
}
