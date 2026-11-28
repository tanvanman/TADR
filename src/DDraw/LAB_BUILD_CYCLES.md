# Lab build cycles

Select a completed factory and use **Cycle Off / Cycle On** in its normal
orders panel, or press **C** to toggle it. The native button underlines its
shortcut and uses fixed, embedded faction button images. A successful mouse
or keyboard toggle plays TA's normal `specialorders` sound.
Cycling starts off for each factory. Add units with the ordinary
build buttons and quantities; for example, five Flashes followed by one
constructor will repeat in that order while cycling is on.

Each successfully completed unit is added once to the end of that factory's
native production queue. No separate saved template or copied order nodes are
used. Turning cycling on adopts the queue that remains at that moment. Turning
it off stops replenishing the queue; the units already queued finish normally.
Native quantity adjustments, right-click removals and Stop still edit or clear
the ordinary queue. Stop does not change the cycle setting. An empty queue
does not create units until the player adds a new build order.

Cancelled construction and unfinished units destroyed before completion are
handled normally by TA and are not recycled as completed units. A blocked
factory exit, insufficient resources, pause or a unit limit does not count as
a completion and cannot add extra orders.

The button is a native GUI gadget, drawn and hit-tested by TA alongside the
existing orders. In the standard filled lab panels it uses the unavailable,
greyed-out Cloak position. A usable Cloak action is never displaced; a genuinely
empty row can be used instead. The original placeholder is restored when it is
needed again. Caption-free button artwork is embedded in the DLL, with native
normal, pressed and disabled frames. No extra files or overlay window are required.
It is offered for one owned, completed factory at a time, rather than mobile
constructors or stockpile weapons. Other commands keep their original behavior.
If a mod's panel leaves no room for a button, the panel is left intact.

## Mod selection

`LAB_BUILD_CYCLES_ENABLE` is explicitly defined in every `config_*.h`:
Escalation opts in; ProTA and the other five profiles leave the feature out.
The switch removes both cycling and the menu code when zero. Each factory's
Cycle Off/On state is separate from this compile-time mod choice.

Cycling is independent of the other player upgrades included in this branch;
none of those features is required for it to work.

## Native queue and compatibility

TA runs production orders on the owning player. Cycling is restricted to the
local human's factories and uses TA's existing production-queue adjustment
function. Completed units continue through the native creation, builder-link
and unit-state paths. No new multiplayer packet, changed packet meaning or
recording-format change is introduced. Replay playback never generates cycles;
it plays back the normally recorded results.

The successful BuildingBuild path at `0x402B3C` has already cleared the completed
target and decremented its count. The hook adds one of that type through
`AdjustBuildCount` (`0x43B0B0`) and lets the original build-menu refresh run.
Abort/cancel paths are separate. Factory state is discarded on destruction,
ownership loss, slot initialization, leaving a match or restarting a match.

The second hook is six bytes at the unit initializer's entry `0x485A40`, which
clears any old cycle state before a slot is initialized again. The recorder's
`UnitInfoExpand` splice at `0x485A75` is separate. The DDraw and recorder sources
were checked for nearby splices; neither production completion nor this entry
overlaps an existing patch. The menu adds an in-memory gadget and wraps only
its native layer callback; it does not patch GUI executable code.

Hook bytes, GUI allocation layout, sound playback and native entry points are
checked before installation. An
incompatible executable or an existing patch at a required site leaves this
feature disabled with a diagnostic. The addresses have been checked against
the installed Escalation and ProTA executables; this is not a claim of in-game
or multiplayer playtesting.

## Validation

Production-adapter tests exercise queue rotation, counts, toggles, native edits,
ownership and lifecycle restrictions, native menu controls and disabled builds.
The combined workflow compiles enabled Escalation, excluded profiles and each
feature independently, checks feature markers and Windows 7 imports. In-game testing
should cover both factions, different factories and build-menu pages, normal
map and megamap input, pause, queue editing, restarting a match, multiplayer
and replay playback.
