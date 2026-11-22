# Optional unit controls

Each feature is independent. Set its define to `1` to include it or `0` to
exclude it. There is no dependency between the three switches.

The shared TAHook layer must remain enabled (`TA_HOOK_ENABLE=1`), as it is in
all seven shipped profiles. The build rejects enabling these controls with
that layer disabled.

| Define | Feature | Behavior when excluded |
| --- | --- | --- |
| `DRAWN_MOVE_FORMATIONS_ENABLE` | Alt-drag movement formations and brief destination feedback for all accepted moves on the battlefield and megamap; Shift+Alt queues formations | Original movement and selection controls, without the temporary feedback |
| `MEX_UPGRADE_ENABLE` | One-click extractor upgrades with Shift queue/cancel, offset placement and green-arrow/red-X cursors | Original extractor click behavior and cursors |
| `GRID_BUILDING_ENABLE` | Hold the configured Autoclick Key and click–move–click to place grids on the battlefield and megamap; drag–release also works; add Alt for the original diagonal row tool; wheel adjusts spacing | Original row building, including its spacing controls |

## Profile defaults

| Profile | Formations | Extractor upgrades | Grid building |
| --- | --- | --- | --- |
| Escalation | 1 | 1 | 1 |
| ProTA | 1 | 0 | 1 |
| OTA, TA:Zero, BTA, Mayhem, Twilight | 0 | 0 | 0 |

These defaults live in `OptionalUnitControls.h`, included after the selected
mod configuration. Modders can override them in their `config_*.h` or through
compiler definitions. Profiles other than Escalation/ProTA must opt in explicitly.

Extractor upgrades require a supported next-tier unit in the selected
constructor's build menu. Included ARM/CORE mappings are T1 extractor → Moho →
Mantle, for both land and underwater. The Mantle tier is available in Escalation;
an absent or unbuildable replacement leaves normal input behavior intact.
Capability comes from all pages of the actual human build menu, including mod
archive overrides, rather than the separate AI construction list. This supports
vehicle, kbot, air, hover and naval constructors whenever their menu offers the
matching land or underwater extractor.

## Build without editing source

The project accepts matching MSBuild properties in Debug, Release and
ReleasePublic. For example, include formations and grids but exclude upgrades:

```bat
msbuild src\DDraw\ddraw.vcxproj /p:Configuration=ReleasePublic /p:Platform=Win32 /p:TDRAW_CONFIG=TDRAW_CONFIG_ESCALATION /p:DRAWN_MOVE_FORMATIONS_ENABLE=1 /p:MEX_UPGRADE_ENABLE=0 /p:GRID_BUILDING_ENABLE=1
```

Set all three properties to `0` to exclude all three features, or omit them
to use the profile defaults. Invalid values fail the build. Use a clean build
when changing the profile or feature switches so an earlier build's objects
cannot be reused accidentally.

## Runtime preferences

An included formation feature can still be disabled with
`DrawnMoveFormations=FALSE` under `[Preferences]` in the mod's active INI file.
Extractor upgrades similarly honor `MexUpgrade=FALSE`. Restart TA after
changing these preferences. An INI setting cannot enable a compiled-out feature.

Accepted move commands also display a green line and an outlined green arrow at
each destination for 0.5 seconds, without holding Shift. The line follows its
unit while the destination arrow stays fixed. Lines, arrows and dark outlines
blend at approximately 80% opacity so the terrain shows through. Ordinary moves,
Shift-queued moves and committed formations all receive this feedback, including
accepted native moves from commanders. `DrawnMoveFormations=FALSE` disables the
Alt gesture and
`DrawnMovePreview=FALSE` disables its drawing preview; neither disables this
brief feedback. Excluding `DRAWN_MOVE_FORMATIONS_ENABLE` at compile time removes
both formations and destination feedback. See [move order feedback](MOVE_ORDER_FEEDBACK.md).

Grid placement uses the existing Autoclick Key and spacing controls. Hold
that key alone for a filled grid, or add Alt for the original arbitrary-angle
row tool, including diagonal walls and its existing row optimization. Alt can
be pressed or released during the preview to switch between the two shapes.
Both shapes support click–move–click and drag–release on the battlefield and
megamap. While a pattern is active, wheel input changes spacing even with Alt
held; outside a pattern, the existing Alt+wheel building rotation is unchanged.
Excluding grid building at compile time retains the original row-only controls.

## Validation

The dedicated workflow builds all eight switch combinations for Escalation and
the default settings for every other mod profile. It checks binary feature
markers, Windows 7 imports, configuration validation and input/geometry
regressions, including accepted-move capture, commander eligibility, moving
feedback origins, fixed endpoints, palette blending and changes, 500 ms expiry
and actor replacement. Every Windows feature combination also verifies that
feedback is included exactly when formations are compiled in. Disabled-feature tests verify native input
passes through and grid exclusion restores the original row placement behavior.

The combined Escalation feature set was playtested under Wine. The author
reports repeated live multiplayer testing on TAF. These are user playtests;
CI checks code paths and builds rather than establishing multiplayer
compatibility. The new Alt row override still needs its own in-game test;
no replay-watch result is claimed here.

Rapid double-clicks can start formations on both maps. Grid ordering includes
constructors without an AI build plan, and routine gesture logging is bounded.
The `DrawnMoveTestNoOrders` diagnostic preference exists only in Debug builds;
release builds always submit the requested native orders.
