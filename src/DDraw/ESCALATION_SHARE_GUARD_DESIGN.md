# Escalation share-abuse guard

Stops a losing player denying the enemy a kill by handing their base to an ally — both by
dumping it just before dying, and by unplugging so an ally can `.take` it.

Alternative to the creator-tag "unit estate" design (branch `FAF-Style-Anti-Share-Abuse`). It is
far smaller because **neither rule needs replicated state**: both are purely local decisions, so
there is no tag table, no replication packet, no cross-client agreement, and no engine-bug
workarounds.

## The two rules

**1. Structure shares are delayed, a whole share at a time.** A share of N structures goes through
immediately if the structures released in the last 30 s plus N stays within the allowance (default
10 per 30 s); otherwise the **entire** share is held and released together 30 s later, however big
it is. Measured in game ticks (30/s) so lag cannot game it.

Deliberately **not** a drip feed. Metering the release *rate* would make a legitimate
1000-structure hand-over take the better part of an hour — unusable. A flat 30 s cost on everything
past the first 10 structures denies the abuse just as well: a base cannot be dumped in the moment
before dying, and batching gains nothing because a second share inside the window is delayed too.

Mobile units are deliberately not counted; armies share instantly as they always have.

Nothing special is needed if the giver dies mid-queue. With com-ends on their units are wiped;
with com-ends off, being dead means having no units left. Either way queued entries fail
revalidation and are dropped.

**2. `.take` is refused when the target's commander has already been destroyed.**

## Why rule 2 is needed at all (the 30 s gate is not enough)

`.take` already requires 30 s of network silence from the target — verified in eplayx's `OnTake`:
`tsStaleCutoffMs = timeGetTime() - 30000`, gated on `TPlayerData+0x24 LastMsgTimeStamp <
tsStaleCutoffMs` (plus the `CanTake` flag at `+0x11f`). But that is only a *delay*, not a denial —
the ally simply waits it out.

The reason it can't deny anything on its own is the interesting part:

> **A dropped player whose commander has been destroyed is never actually eliminated.**
> Elimination needs the owner's client to run `UNITS_Send_UnitDeath_P0C`, and it is gone. Remote
> clients merely clamp `Health` to 0 and leave the `alive` flag set, because only a unit's owner
> may declare it dead. `UNITS_KillAllForPlayer` only ever fires from `Disconnect_Player` (a
> reject) or from a commander death nobody can dispatch.

So at T+30 s the abuser's base is sitting there fully intact and takeable, with a commander that
is dead in every practical sense but that nothing in the engine will ever declare dead. Plain
`.take` even skips the commander (walk starts at index 2), so the ally takes the base and leaves
the 0-health commander behind to be wiped at reject — all of the loot, none of the liability.

What the 30 s *does* buy is a **window in which the enemy can finish the commander**. Rule 2 is
what turns that work into a denial. The two only have teeth together.

## The detector needs no heuristics

A commander at `Health <= 0` **with the `alive` flag still set** is precisely the "dead but can't
say so" state, and it is not reachable any other way: `UnitTakeDamage_packet` clamps a
remote-owned unit's health instead of killing it, so the state persists only for a player whose
client is no longer dispatching deaths. For a live player it exists for the sub-second before
their death packet arrives.

Commander identification uses TA's own category bitmask,
`FindSpot_CategorysAry("Commander")` (`0x00488c50`) indexed by `UnitINFOID` — race-independent,
unlike comparing against the owner's `RaceSideData.commanderUnitName`, which misidentifies a
commander held by the opposite side.

## Where each rule is enforced

### `_ShowText` (`0x00463E50`) — the single choke point for outgoing chat

```c
_ShowText(player, text, fontColor, arg4) {
    sprintf(buf, "<%s> %s", ...);
    BroadcastText(buf);            // network send — what eplayx eventually sees
    Report_NewChatMessage(buf);    // multiplayer only
    NewChatText(buf, ...);         // local display
}
```

Every route to `.take` converges here:

| Route | Caller |
|---|---|
| User types it in chat | `TALK_OnCommand` @ `0x00493faf` |
| TA's own timeout dialog | `TimeoutDialog_OnCommand` @ `0x0045398b` |
| tdraw's VoteDialog button | calls `ShowText(&Players[local], ".take", 0, 0)` directly |

It is **send-only** — received chat arrives via `Packet_Dispatcher` → `Packet_Chat_0x05` →
`NewChatText` and never passes through — so a hook here only ever sees text the local player is
issuing.

eplayx is an `IDirectPlay` shim sitting **below** TA's HAPI layer, so it only observes the command
once `BroadcastText` has sent it. Suppressing the call stops `OnTake` running on every client,
including the issuer's own. (DirectPlay broadcasts do not loop back to the sender, which is why
eplayx must parse local commands on the *send* side for `Sender.IsSelf` to be reachable at all.)

Hook notes: 10-byte prologue of whole instructions (`MOV EAX,[ESP+0x10]`; `SUB ESP,0xC8`); at the
hook site the prologue has not run, so `[Esp+4]=player`, `[Esp+8]=text`. Suppress by redirecting
to a naked `RET 0x10` stub — **not** to the epilogue at `0x00463ed9`, which does `ADD ESP,0xC8`
and would unbalance the stack.

> **Ghidra signature correction:** `_ShowText` takes **four** args, not three — it ends `RET 0x10`
> and its first instruction reads `[ESP+0x10]`. tdraw's `tafunctions.h` typedef was already right.

Tradeoff accepted: blocking the command also blocks eplayx's trailing `0x1B` reject, so the
dropped player is not rejected by this route. VoteReject and TA's auto-reject
(`NetworkDropoutTimeoutSec` = 120 game-seconds, `TimeoutDialog_Ticker`) still handle that.

### `GiveSelectUnits` (`0x004933e0`) + the `UNITS_GiveUnit` predicate

`GiveSelectUnits` is the only player-initiated share path (reached solely from
`ShareDialog_proc`), and already excludes commanders, transported units and `stateMask&3==2`. Its
entry hook (7-byte prologue) sets an "in player share" flag, cleared by a return thunk.

Because the decision is per-share rather than per-unit, the entry hook cannot rule on it — the
block size is not known until the selection loop has finished. So every structure give is
**collected and suppressed** during the loop, and the return thunk (`FinishPlayerShare`) sizes the
block and either re-issues it immediately or holds it. An immediate block is re-issued on the same
tick, so the player sees no difference; a `g_releasing` guard stops the re-issued gives being
re-collected.

The collection predicate runs inside **CUnitRotate's existing `UNITS_GiveUnit` hook** — that
address allows only one router, so it is extended rather than re-hooked. It distinguishes the
three ways `UNITS_GiveUnit` is reached:

| Caller | `pkt` | flag | metered? |
|---|---|---|---|
| `GiveSelectUnits` (player share) | null | set | **yes** |
| capture (`0x004046c5`) | null | clear | no |
| `Packet_Dispatcher` (network give, and `.take`'s synthetic `0x14`s) | non-null | — | no |

Suppression redirects to a naked `RET 0xC` stub (`UNITS_GiveUnit` is `__stdcall void`).

Held entries store `{unitIndex, unitTypeId, ownerSlot, targetSlot}` and are **revalidated** at
release — unit indices are recycled, so a stale index would otherwise give away an unrelated unit.
Entries that fail revalidation are dropped, which is exactly what makes a giver dying mid-delay a
non-issue.

Each delayed block carries its own `dueTick`, so blocks release in the order they were issued.
Only structures actually released are recorded in the rolling window, so a share that turns out to
be entirely stale does not consume anyone's allowance.

## How `.take` actually moves units (for reference)

eplayx synthesises `0x14` give packets into the local DirectPlay receive stream, one per tick,
forged to look like they came from the dropped player (`lpidFrom := Players[i].ID`), with the
unit's current health patched in at offset 12 — so a taken commander arrives at 0 health. A
trailing `0x1B` reject packet ends the walk.

## Config

`SHARE_ABUSE_GUARD`, 1 in `config_escalation.h` and 0 in every other config.

## Test plan

**Test 0 — confirm the binary first.** Every symptom below is also produced by a DLL with the
feature compiled out, so this is not optional:

```bash
grep -q "transfer will complete in" /d/games/<mod>/tdraw.dll && echo HAS || echo MISSING
```

FileVersion should also end in `-esc`. A whole session was lost on 2026-07-31 to a `Public\`
build that was silently `TDRAW_CONFIG_PROTA` (then named `TDRAW_CONFIG_FULL`).

### A. Share rate limit — skirmish with an allied AI is enough

| # | Action | Expected | If it fails |
|---|---|---|---|
| 1 | Share 1 structure | Immediate | Hook not firing — is the share dialog the route used? `GiveSelectUnits` is only reached from `ShareDialog_proc` |
| 2 | Wait >30 s, share 9 structures | Immediate (window pruned) | As above |
| 3 | Within seconds, share 4 more | **Held 30 s**, chat line, then all 4 arrive together | The discriminating test. If immediate: window pruning too aggressive — suspect `GameTime` units (see below) |
| 4 | Share ~50 structures at once | **All 50 arrive together** after 30 s, not dribbled | Regression guard against the original token-bucket behaviour |
| 5 | Share 20 mobile units | Immediate, never delayed | `IsStructure`/`bmcode` wrong |
| 6 | Trigger a delayed share, then lose your commander before it releases | Held units never transfer | Revalidation not running |
| 7 | Capture an enemy unit | Immediate, unaffected | `g_inPlayerShare` leaking outside `GiveSelectUnits` |

Test 3 is the one that matters most — tests 1 and 2 pass identically whether the feature is
present or absent.

### B. `.take` block — needs a real MP game with a droppable player

| # | Setup | Action | Expected |
|---|---|---|---|
| 8 | Com-ends ON. Ally drops; enemy destroys their commander | Ally types `.take` (or clicks the VoteDialog button) | Refused, chat line naming the player. No units transfer |
| 9 | Com-ends ON. Ally drops with commander **intact** | `.take` | Works normally — full base transfers |
| 10 | Com-ends OFF. Ally drops, commander destroyed | `.take` | Works normally — the rule is inert without com-ends |
| 11 | Same as 8 | `.takecmd` | Also refused (same predicate gates both) |

Test 8 vs 9 is the pair that proves the detector; 10 proves it doesn't over-reach.

### C. Config isolation

| # | Action | Expected |
|---|---|---|
| 12 | Build and run a non-escalation config | None of the above active; string check reports MISSING |

## Open / to verify

- The 10-structures-per-30 s allowance is a first guess and wants play-testing. Both constants are
  at the top of `ShareGuard.cpp`.
- **`GameTime` units are the main unverified assumption.** The window is 900 ticks on the
  assumption that `TAdynmemStruct.GameTime` (0x38A47) counts sim frames at 30/s. If it were
  milliseconds the window would collapse to under a second and *every* share would appear
  immediate — indistinguishable from the feature being absent. Test 3 detects this.
- Whether refusing `.take` outright is the right UX versus refusing only the *structures*.
- Blind spot, accepted by design: a player whose commander is safe can still unplug and have an
  ally take the base. Under com-ends rules they genuinely were not about to die, so what happens
  to their base is ordinary gameplay regardless of who owns it.
