# Native bottom-bar unit statistics

Hover a build-menu item to inspect the unit before building it. Select owned
units of one type to inspect their per-unit stats. Every owned selection also
shows its total count, including mixed groups and selections made while a build
item is hovered. Mixed groups otherwise retain TA's native information display.

TA draws its original 32-pixel bottom bar first: player color, unit name,
health bar, current order and metal/energy figures remain intact. Additional
statistics follow that information, using the same faction font, on the
battlefield and megamap. The actual occupied native area is measured before
adding text. Lower-priority statistics are omitted when they cannot fit;
the total selected count has priority in the appended area.

Native health and metal/energy costs remain available. Added statistics include
numeric health, nominal DPS, weapon range, movement speed and line of sight. Relevant additional fields
include radar, sonar, jammers, build power/range, economy, storage, area of
effect and reload times. Values come from the mod's currently loaded definitions.
A single selected unit shows its current health and measured economy; matching
groups show per-unit values, rather than misleading sums.

## Interpretation

`DPS~` is theoretical base damage per second at the loaded weapon's nominal
reload rate: default damage × burst count × 30 / reload ticks. A zero burst
means one shot. TA's named `NOWEAPON` sentinel is excluded by identity; an
actual weapon with zero reload remains unknown, rather than infinite DPS. Anti-air,
water-weapon and paralysis rates are shown separately. Manual weapons, stockpiled shots and
interceptors are not added to ordinary DPS. Target-specific damage, armour,
health/veterancy reload changes, aim/script delays, projectile misses, splash
against several targets and repeated-contact special weapons are not a claim
about actual combat output.

Movement speed is metres per second, matching TA's native unit-help panel:
16.16 map units per tick × 30 ticks/second × 0.4 metres/map unit. Range and
sensor radii retain TA's native map-distance values.
Build power is nominal worker time; build work is not a duration in seconds.
Wind, tidal and extraction output can depend on the map and live conditions.

## Build and runtime selection

`UNIT_STATS_PANEL_ENABLE` is explicitly selected in all seven mod configurations:
Escalation enables it; ProTA and the other profiles set it to zero. Values other
than zero or one fail the build. `UnitStatsPanel=FALSE` under `[Preferences]`
disables an included feature at runtime. Restart the game after changing it.
The global all-enhancements-off mode also suppresses the added statistics.

The normal-map native information-panel call is checked before patching. A
different executable or another patch owning that call leaves native behavior
in place. Rendering changes no orders, unit definitions or multiplayer packets.
Only the native local redraw cache and temporarily restored hover inputs are
changed to display the selected unit's original information beside its stats.
Fields used from the native structures have names and layout assertions in
`tamem.h`; no raw-offset unit or weapon reads are needed.

## Validation

Portable calculation and production-adapter tests cover loaded values,
unsigned damage/32-bit health and ranges, special weapons, homogeneous/mixed
selection behavior, native-pixel preservation, clipping and disabled fallbacks.
Actual Escalation and ProTA Flash data checks nominal DPS 60 and speeds of
25.8 m/s and 24.0 m/s respectively. The ProTA fixture checks data interpretation;
the ProTA release profile does not enable this feature. Windows builds cover
Escalation and compiled-out profiles. In-game visual checks
are still needed at supported resolutions, on both maps and both factions,
including selection changes, hovering build buttons, menus and match reloads.
