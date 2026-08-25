#pragma once

#define TDRAW_CONFIG_NAME "escalation"

//
// Snap radii
//
#define DEFAULT_MEX_SNAP_RADIUS 0
#define MAX_MEX_SNAP_RADIUS 0
#define DEFAULT_WRECK_SNAP_RADIUS 1
#define MAX_WRECK_SNAP_RADIUS 1

//
// Anti-share-abuse
//
// Structure shares are rate limited, and '.take' is refused when the target's
// commander has already been destroyed.  See ESCALATION_SHARE_GUARD_DESIGN.md.
#define SHARE_ABUSE_GUARD 1

//
// Construction / AI behavior
//
#define FIXED_POSN_GUARDING_CONS_ENABLE 1
#define PATROLING_CONS_RECLAIM_OR_ASSIST_ENABLE 1
#define CONSTRUCTION_KICKOUT_ENABLE 1

//
// Balance
//
// See CLAUDE.md for the derivation. This is a global repair-rate nerf (removes a
// rounding-bug bonus that favored massed cheap repairers over fewer expensive
// ones), not a pure engine-bug fix, so it is scoped to this config only.
#define REPAIR_RATE_FIX_ENABLE 1

// HP delivered per tick is multiplied by one of these two factors -- which one
// applies is decided per call by RepairRateFix.cpp itself (return-address
// check against the one known passive-regen call site), NOT by anything in
// this file. Energy cost is computed independently and is NOT scaled by
// either -- per Wotan, the buff is meant to make healing cheaper in effective
// E/HP, not change the E drain itself.
//
// Deliberately separate knobs: a flat 3x lands very differently on the two
// mechanics. Repair ends up at 154-289% of pre-fix vanilla (a straightforward
// buff), but self-heal for most units is still far below pre-fix vanilla even
// at 3x -- RepairRateFix already cut it 1-2 orders of magnitude for units with
// huge BuildTime (median unit: 4.01% of vanilla regen retained at 1x -> 12.0%
// at 3x; corms: 0.28% -> 0.84%), while four fast-BuildTime units (corcom,
// armcom, cordecom, armdecom) OVERSHOOT vanilla at 3x. One knob can't serve
// both populations. Census: 59 of 547 units have HealTime > 0, re-verified
// 2026-08-18 from the shipping archives -- see ENGINE_NOTES.md SS25.7.1 and
// SS25.7.2 for the full per-unit table.
//
// Both require REPAIR_RATE_FIX_ENABLE 1 (config.h enforces this at compile
// time) and are Escalation-only, same rationale as the fix itself above.
// Agreed value for both is 3x for the initial playtest; flip either constant
// and rebuild to retune independently.
#define REPAIR_RATE_FIX_REPAIR_MULTIPLIER 3
#define REPAIR_RATE_FIX_SELFHEAL_MULTIPLIER 3

//
// Off-map aircraft
//
// Width, in map tiles (16 world units each), of the band outside the map where aircraft can
// still be seen, targeted and killed; stock TA cannot touch them at all.  0 disables the whole
// module, including the separate LOS-shear fix that this number does not bound.
// ~32 covers a whole attack-run overshoot; 1-3 covers only the immediate edge.
// Compile-time only, deliberately -- it decides who can shoot what, so a per-machine
// override would be a mixed-fleet vector.  See OFFMAP_AIRCRAFT.md.
#define OFFMAP_AIRCRAFT_TARGETABLE_MARGIN_TILES 32

//
// Environment / sim sync
//
#define WEATHER_REPORT 1
#define WEATHER_REPORT_WIND 1
#define WEATHER_REPORT_TIDAL 1
#define WIND_SPEED_SYNC 1
#define VISIBLE_MAP_DTS 1

//
// UI enhancements
//
#define TA_HOOK_ENABLE 1
#define USEMEGAMAP 1
#define MEGAMAP_FEATURES 1
#define USEWHITEBOARD 1
