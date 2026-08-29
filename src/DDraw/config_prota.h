#pragma once

#define TDRAW_CONFIG_NAME "prota"

//
// Snap radii
//
#define DEFAULT_MEX_SNAP_RADIUS 3
#define MAX_MEX_SNAP_RADIUS 3
#define DEFAULT_WRECK_SNAP_RADIUS 1
#define MAX_WRECK_SNAP_RADIUS 1

//
// Anti-share-abuse
//
// Structure shares are rate limited, and '.take' is refused when the target's
// commander has already been destroyed.  See ESCALATION_SHARE_GUARD_DESIGN.md.
#define SHARE_ABUSE_GUARD 0

//
// '.take' claim arbitration
//
// Per-target claims and a deterministic election replace dplayx's single global
// TakeStatus latch; '.take' gains an explicit target.  See TakeClaim.h.
#define TAKE_CLAIM_ENABLE 1

//
// Construction / AI behavior
//
#define FIXED_POSN_GUARDING_CONS_ENABLE 1
#define PATROLING_CONS_RECLAIM_OR_ASSIST_ENABLE 1
#define CONSTRUCTION_KICKOUT_ENABLE 1

//
// Balance
//
// See CLAUDE.md / config_escalation.h -- Escalation-only balance nerf, not enabled here.
#define REPAIR_RATE_FIX_ENABLE 0
// Multipliers require REPAIR_RATE_FIX_ENABLE 1 (config.h enforces this at
// compile time) -- must be 1/1 here since the fix itself is off.
#define REPAIR_RATE_FIX_REPAIR_MULTIPLIER 1
#define REPAIR_RATE_FIX_SELFHEAL_MULTIPLIER 1

//
// Off-map aircraft
//
// Width, in map tiles (16 world units each), of the band outside the map where aircraft can
// still be seen, targeted and killed; stock TA cannot touch them at all.  0 disables the whole
// module, including the separate LOS-shear fix that this number does not bound.
// ~32 covers a whole attack-run overshoot; 1-3 covers only the immediate edge.
// Compile-time only, deliberately -- it decides who can shoot what, so a per-machine
// override would be a mixed-fleet vector.  See OFFMAP_AIRCRAFT.md.
#define OFFMAP_AIRCRAFT_TARGETABLE_MARGIN_TILES 1

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
