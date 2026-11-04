# Drawn move formations: play-test checklist

This feature is implemented in tdraw.dll. Use the DLL matching your mod; keep
its existing recorder DLL. Enable `DrawnMoveFormations = TRUE` in the
`[Preferences]` section of totala.ini (enabled by default).

Select two or more completed mobile units. Hold the usual movement mouse
button on empty ground, draw a line or curve, then release. Green squares show
one destination per unit. A short click still issues a normal group move.
Explicit Move uses the left mouse button in either interface. Hold Shift when
starting the gesture to append destinations to the existing queues.

Drawing is supported on the normal battlefield and the full-screen megamap.
The small minimap retains its existing controls. Switching views while drawing
cancels the pending gesture.
Destinations are equally spaced along the drawn path. A short or tightly curved
stroke can pack large units too closely; draw a longer line for more spacing.
The engine still handles terrain and pathfinding, so blocked or unreachable
positions may prevent an exact formation. Units move individually; they do not
maintain the wall while travelling. No new network packet types are introduced.

## Automated checks

From the repository root on Linux:

```
g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -g src/DDraw/test/drawn_move_geometry_test.cpp -o /tmp/drawn-move-test
/tmp/drawn-move-test
python3 src/DDraw/test/drawn_move_input_test.py
```

The validation workflow builds Escalation on the test branch and all seven
mod configurations for pull requests with MSVC, and
uploads one DLL artifact per mod. It does not publish releases.

## Required in-game checks before declaring ready for release

- Both left-click and right-click interfaces: normal clicks, straight lines,
  curves, reversed strokes, and very short strokes; two units and large groups.
- In both normal view and megamap, confirm the preview matches the release destinations at multiple resolutions,
  on slopes, near water, near map boundaries and while edge-scrolling.
- Shift queues a distinct destination for each unit without clearing older
  orders; releasing Shift during the gesture does not change the captured mode.
- Explicit Move exits after an ordinary release, and stays prepared with Shift.
- On the megamap, short clicks send one group order, drags send no orders until
  release, and unit-icon clicks retain selection/attack/guard behaviour. Check
  that selection boxes and double-click camera movement still work.
- Test megamap curves near map margins, income/command panels and on slopes.
  The cursor and green markers must follow the pointer; leaving the map cancels.
- Clicks on enemy/friendly units still attack/guard/select normally. Building
  placement, line building, whiteboard drawing and Shift-dragging existing queued
  orders retain their controls. A single selected unit retains stock controls.
- Escape, another command/hotkey, mouse-wheel zoom, the other mouse button,
  moving into the UI, loss of focus and capture loss cancel without sending an
  order. The next click must work; the original group must stay selected.
- Selection changes, a selected unit dying or being transferred, entering the
  other map view, a replay, a spectator view or a new game cancel the old gesture.
- Mixed selections: buildings and incomplete units must not receive formation
  orders, and the complete original selection must be restored.
- Two-player game: inspect the destinations/queues from both clients. Verify
  the normal selected-unit dispatcher emits one move for each intended unit,
  no commands reach other units, and no desynchronisation occurs.
- Record that game and replay it on both clients, including Shift queues.
- Disable DrawnMoveFormations and confirm stock input is restored.

Multiplayer/replay behaviour and terrain projection require the actual game;
geometry tests and a successful DLL build do not establish these properties.
