#pragma once

#include <Windows.h>

// Anti-share-abuse guard (Escalation).
//
// Targets the two ways a losing player can deny the enemy a kill by handing
// their base to an ally: dumping it just before dying, and unplugging so an
// ally can '.take' it. Neither rule tries to detect intent, and both are
// PURELY LOCAL decisions — no replicated state, no cross-client agreement.
//
// See ESCALATION_SHARE_GUARD_DESIGN.md.
//
//  1. Structure-share delay, applied to a whole share at a time. If the last
//     30s of shares plus this one stays within the allowance (default 10
//     structures per 30s) it goes through immediately; otherwise the ENTIRE
//     share is held and released together 30s later, however big it is.
//
//     Deliberately NOT a drip feed: metering the release rate would make a
//     legitimate 1000-structure hand-over take the better part of an hour. The
//     abuse is denied just as well by a flat 30s cost on everything past the
//     first 10 structures — a base cannot be dumped in the moment before dying,
//     and batching gains nothing because a second share inside the window is
//     delayed too.
//
//     Mobile units are deliberately not counted: armies share instantly, as
//     they always have.
//
//     Nothing special is needed if the giver dies mid-delay. With com-ends on
//     their units are wiped; with com-ends off, being dead means having no
//     units left. Either way the held entries fail revalidation and are dropped.
//
//  2. '.take' block on a dead-but-undead commander. A dropped player whose
//     commander has been destroyed is never actually eliminated: elimination
//     needs the owner's client to run UNITS_Send_UnitDeath_P0C, and it is gone.
//     Remote clients just clamp Health to 0 and leave the alive flag set, so
//     the base sits there fully intact and takeable. We detect exactly that
//     state and refuse the take.
//
//     The detector is the state itself, not a heuristic: a commander at
//     Health <= 0 with the alive flag still set can only persist for a player
//     whose client is no longer dispatching deaths. For a live player the same
//     state exists only for the sub-second before their death packet arrives.
//
//     Enforced at _ShowText (0x00463E50), the single choke point for OUTGOING
//     chat — every route to '.take' converges there (typed chat via
//     TALK_OnCommand, TA's timeout dialog, and tdraw's own VoteDialog button).
//     eplayx is an IDirectPlay shim sitting BELOW TA's HAPI layer, so it only
//     sees the command once BroadcastText has sent it; suppressing that call
//     stops OnTake ever running, on every client including the issuer's own.

namespace ShareGuard {

// Install hooks and register the tick callback. Idempotent.
void Install();
void Shutdown();

// True when the feature is compiled in AND this game eliminates on commander
// death (TAdynmemStruct+0x37EF6). With the option off, a destroyed commander
// eliminates nobody and rule 2 is meaningless.
bool IsComEndsGame();

// ---- call-outs from CUnitRotate's UNITS_GiveUnit hook ----------------------
// That address is already hooked by CUnitRotate and hook.h allows exactly one
// router per hook, so we ride its hook rather than adding our own.

// Called at UNITS_GiveUnit entry, BEFORE CUnitRotate's structures-only gate.
// Returns true if this give should be suppressed (it has been queued for
// later release by the rate limiter). The caller must then redirect to
// GetGiveSuppressStub() and return X86STRACKBUFFERCHANGE.
bool ShouldSuppressGive(void* srcUnit, void* targetPlayer, const void* givePkt);

// Naked stub that returns from UNITS_GiveUnit (__stdcall void, RET 0xC)
// without executing it.
void* GetGiveSuppressStub();

} // namespace ShareGuard
