# Ctrl+Shift queue prepending

`PREPEND_ORDERS_ENABLE` is explicitly selected in all seven mod configurations.
Escalation includes it; ProTA and all other profiles exclude it. The switch is
independent of formations, extractor upgrades, grid building and other controls.

Hold **Ctrl+Shift** while issuing a command to execute it now, then return to the
previous queue. Shift alone still appends. Ctrl+Shift takes priority
over ordinary Shift queueing, including the extractor-upgrade queue/cancel input.
Keep both keys held until the click or drag finishes.

For an idle unit (empty main queue, Standby, VTOL_Standby or Stop), the command
uses normal native append behavior without a warning. That choice stays with
the unit for the whole gesture, preserving the order of grid/mex commands.

The normal battlefield and megamap order paths use the same dispatcher. A grid
is inserted in its existing build order; a mex upgrade retains reclaim before
build; formations can be committed with Ctrl+Shift+Alt without cancelling the drag.
No extra game overlay is added. Space keeps its native scoreboard/status panels. Text input, keyboard
shortcuts, selection, factory batch counts and replay/watch controls remain native.

Under `[Preferences]`, `QueuePrepend=FALSE` disables the modifier at runtime.
The default is `QueuePrependKey=0` and `QueuePrependModifiers=3` (no extra
key, with Ctrl and Shift both required). Modifier bits are Ctrl=1, Shift=2,
Alt=4. A custom single key can use its Windows virtual-key code and modifier
mask 0. Restart TA after changing these preferences.

## Restarting interrupted work

The engine's raw push-front primitive leaves a running handler's movement and
construction state active. This patch instead creates fresh native urgent and
resume orders before changing the list, removes only the current order through
the native cleanup path, and retains its pending tail. Started mobile
construction resumes as native repair of the same frame, including a frame
that has just been placed before its first build tick; an unstarted build
retains its build type, parameters and rotation. Normal nanoframe decay and
target destruction still apply.

Supported main-queue actions are move, attack, guard, patrol, repair, reclaim,
capture and mobile build. Transport loading/unloading, factory production and
internal/recovery handlers cannot be interrupted safely. A refused insertion
leaves the queue untouched and displays a short message; regular commands remain
available. Idle units use the normal command path instead.

A batch keeps a fixed resume anchor per unit, so inserting multiple commands
does not reverse them or repeatedly interrupt the new urgent head. Native
`0x1000` append-boundary markers remain on the preserved queue, so a later
ordinary Shift command still appends after the original work. Allocation and
constructor preflight failures leave existing commands intact; later commands
for that unit in the failed batch are skipped.

## Native integration and validation

The modifier activates only on mouse command messages; Ctrl+Shift+letter
shortcuts and native factory menus remain outside that scope. TA defers native
battlefield clicks until its next input update. Guarded CALL wrappers at
`0x004993AC` and `0x004995B3` establish the batch when that saved click is
consumed, using the event's Ctrl/Shift flags rather than the current key state.
Synchronous DLL commands keep their window-message batch. The guarded order
CALL wrappers are `0x0048D1F4` and `0x004197A9`, plus the DLL's direct
extractor-upgrade sends. Engine-internal
order creation is unchanged. The actor must be live and owned by the local
human player; demos, watchers and foreign/dead/stale slots are excluded.

Native GUI lookup initializes only the script index's low byte, so active
prepend dispatch normalizes that byte. Inactive dispatch forwards every
original argument unchanged. Raw buffers are preallocated, fresh objects are
constructed through `0x0043A0C0`, and existing orders are removed/inserted using
`0x00439F80` and `0x0043AC60`. Handler state, callbacks, embedded references and
self pointers are never copied from a running object.

Patch-site inspection includes the recorder's nearby OrdersOverride splice at
`0x0048D126` (5+1 bytes), which ends before the command call at `0x0048D1F4`.
Recorder references to the native order constructor and insertion/destruction
helpers are calls, rather than overlapping splices. New call sites fail closed
with a log message when their expected bytes differ.

Portable policy and production-adapter tests run with ASan/UBSan. Native x86
builds verify structure offsets, guarded call sites, feature inclusion and
Windows 7 imports. Gameplay, restart/resume behavior and multiplayer/replay
compatibility on a newly rebased build still need in-game confirmation;
portable tests alone do not establish these results.
