# Optional unit controls

These three input features have independent compile-time switches. Each mod
selects every switch explicitly in its own `config_*.h`; `OptionalUnitControls.h`
only validates those values and the shared `TA_HOOK_ENABLE=1` dependency.

| Define | Feature | Behavior when excluded |
| --- | --- | --- |
| `DRAWN_MOVE_FORMATIONS_ENABLE` | Alt-drag movement formations on the battlefield and megamap; Shift+Alt queues them | Original movement and selection controls |
| `MEX_UPGRADE_ENABLE` | Extractor upgrades with Shift queue/cancel, offset placement and green-arrow/red-X cursors | Original extractor click behavior and cursors |
| `GRID_BUILDING_ENABLE` | Hold the configured Autoclick Key to place grids; add Alt for arbitrary-angle rows; wheel adjusts spacing | Original row building and spacing controls |

## Mod selection

Escalation enables all three switches. ProTA, OTA, TA:Zero, BTA, Mayhem and
Twilight set all three to zero. Modders change these values in their profile
headers, then rebuild that profile. There are no inferred defaults or MSBuild
feature-property overrides. A disabled feature cannot be enabled through an
INI preference.

For example, compile the Escalation profile after editing its header:

```bat
msbuild src\DDraw\ddraw.vcxproj /p:Configuration=ReleasePublic /p:Platform=Win32 /p:TDRAW_CONFIG=TDRAW_CONFIG_ESCALATION
```

## Player controls

Hold Alt and draw with the usual movement mouse button to place selected mobile
units along a line or curve. Green squares preview the individual destinations.
Shift+Alt appends formations; Alt alone replaces current movement orders.
Ordinary Shift selection and queued movement retain their original controls.
This feature does not draw temporary movement lines or arrows after an order.

Click an owned extractor with a compatible constructor to reclaim it and build
the next-tier replacement. All pages of the constructor's human build menu are
checked, including mod archive overrides; the separate AI construction list
does not determine eligibility. ARM/CORE mappings cover T1 extractor → Moho →
Mantle on land and underwater. Vehicle, kbot, air, hover and naval constructors
can upgrade when their menu contains the matching replacement.

Placement keeps the original extraction cells covered and searches aligned,
offset footprints when nearby buildings block a centered replacement.
A green arrow indicates a legal upgrade; a red X indicates a blocked site.
Shift adds an upgrade or removes its queued reclaim/build pair. Clicking without
Shift replaces the previous upgrade. Existing native reclaim/build orders and
refund rules perform the work; no unit definitions or resource amounts change.
When queue prepending is enabled, Ctrl+Shift uses its prepend behavior instead
of being interpreted as Shift queue/cancel.

Hold the configured Autoclick Key and click–move–click to place a filled grid.
Dragging and releasing the mouse also commits it. Both gestures work on the
battlefield and megamap. Wheel input changes spacing. Add Alt for the original
row tool, including diagonals and its row optimizer; pressing or releasing Alt
during a preview changes the planned shape. Placement follows horizontal rows,
working toward the selected constructor's side of the grid. The native engine
still decides building legality, reachability and pathfinding.

An included formation feature can be disabled locally with
`DrawnMoveFormations=FALSE` under `[Preferences]` in the active mod INI.
Extractor upgrades similarly honor `MexUpgrade=FALSE`. Restart TA after changing
those preferences.

## Validation

Portable geometry and production-adapter tests cover formation input and
previews, grid gesture ownership, row/grid queue ordering, wheel spacing,
diagonal fallback, constructor menu lookup, extractor tiers and footprint
placement. Disabled variants check that native input remains available.
The shared configuration checks cover every mod and independent feature mix;
Windows builds verify enabled and excluded modules and Windows 7 imports.

Actual in-game testing remains necessary for terrain/pathfinding, both mouse
interfaces and maps, restarting matches, multiplayer and replay behavior.
A successful automated test or build does not establish those properties.
