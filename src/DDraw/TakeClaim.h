#pragma once

// '.take' policy, arbitration and target resolution.
//
// dplayx keeps the mechanism — walking the dropped player's unit block and
// injecting the 0x14 gives — and a couple of cheap sanity gates. Whether a take
// may happen, who gets it and what it applies to is decided here, where the
// live engine state is.
//
// It used to be decided in dplayx, from TPlayerData.CanTake (a latched copy of
// one 0x23 ally packet, wrong whenever that packet was lost or raced) and a
// single global TakeStatus shared by every target in the game. That enum was
// set by observing ANY player's '.take' — teammate or enemy — and cleared only
// by an 0x1b rejection. Measured over all 13,293 .tad demos: within one drop
// incident, a player who had seen someone else's '.take' succeeded 55.6% of the
// time if a rejection arrived in between and 4.5% if not (n=18/247, z=7.75);
// across a game, first drop incident 73.0% vs second 32.5% (McNemar p=6.9e-07).
// 290 of the 675 failed takes in the corpus were made by a latched client.
//
// What changed:
//  * State is per target, so a claim on one dropped player cannot block another.
//  * Eligibility is computed at claim time from live TA state, never cached.
//  * Exclusion is an election over broadcast claims, resolved by a rule every
//    client evaluates identically, rather than a first-wins race.
//  * The command names its target ('.take <player>'), so simultaneous reject
//    votes are unambiguous. It travels as ordinary chat, so no .tad parser
//    changes. Bare '.take' still works when only one target is eligible.
//  * Every refusal states its reason.

namespace TakeClaim {

// Install hooks and register handlers. Idempotent.
void Install();
void Shutdown();

// Issue a take for a specific player, so a VoteDialog row button is unambiguous
// with several votes open. Same policy path as typed chat.
void RequestTake(unsigned targetDpid, bool includeCommander);

} // namespace TakeClaim
