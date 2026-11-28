# Escalation player upgrades

Each feature is selected independently in the mod's `config_*.h` before
compiling. Escalation sets the following seven switches to `1`. ProTA and
the other five profiles explicitly set all seven to `0`.

| Define | Player behavior |
| --- | --- |
| `DRAWN_MOVE_FORMATIONS_ENABLE` | Hold Alt and draw a movement line or curve; Shift+Alt queues it. Green footprints preview destinations. |
| `MEX_UPGRADE_ENABLE` | Click an owned extractor with a compatible constructor to reclaim and replace it with the next tier. Shift queues or removes an upgrade. |
| `GRID_BUILDING_ENABLE` | Hold the configured Autoclick Key and click–move–click, or drag and release, to place a filled grid. Wheel adjusts spacing; Alt restores arbitrary-angle rows. |
| `PREPEND_ORDERS_ENABLE` | Hold Ctrl+Shift while issuing an order to interrupt now and resume the previous queue afterward. Idle units accept a normal queued order. |
| `ALLIED_CURSORS_ENABLE` | Show compatible allies' cursor positions as translucent spots with names on the battlefield and megamap. |
| `UNIT_STATS_PANEL_ENABLE` | Append unit statistics to TA's native bottom bar and always show the selected-unit count. |
| `LAB_BUILD_CYCLES_ENABLE` | Use the native Cycle Off/On factory button, or C, to repeat the production queue in order. |

Set any switch to `0` to exclude that feature, then rebuild the selected mod.
There are no MSBuild feature overrides or inferred per-mod defaults. The first
three input features require `TA_HOOK_ENABLE=1`; missing or invalid switches
are compile errors. The remaining features do not require those input features.

```bat
msbuild src\DDraw\ddraw.vcxproj /p:Configuration=ReleasePublic /p:Platform=Win32 /p:TDRAW_CONFIG=TDRAW_CONFIG_ESCALATION
```

Formations, grid placement and extractor interactions work on both the normal
map and megamap. Extractor upgrades cover ARM and CORE land/underwater
T1 → T2 → T3, using each constructor's actual build menu. Offset placement can
fit around nearby buildings while keeping the original extraction cells
covered; the cursor shows a green arrow or a red X for the placement result.
Construction, movement, production and queued orders use TA's native commands.

Cursor sharing uses a reserved chat-hijack ID and exchanges capability messages
before sending positions to a peer. Older peers need no update. No existing
packet size, meaning or recording format changes. The information panel only
reads loaded definitions and visible native selection/build-menu state; it
preserves the original faction square, health bar, order and resource display.

Feature details, hook locations, limitations and tests are documented in
[OPTIONAL_UNIT_CONTROLS.md](OPTIONAL_UNIT_CONTROLS.md),
[PREPEND_ORDERS.md](PREPEND_ORDERS.md), [ALLIED_CURSORS.md](ALLIED_CURSORS.md),
[UNIT_STATISTICS.md](UNIT_STATISTICS.md) and
[LAB_BUILD_CYCLES.md](LAB_BUILD_CYCLES.md).

The combined workflow checks all 128 configuration combinations, production
adapters and portable calculations. Windows builds cover all seven profiles,
Escalation with everything excluded, and each feature by itself. Each resulting
DLL is checked for its selected feature markers and Windows 7 imports.
