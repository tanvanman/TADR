#pragma once

#define TDRAW_CONFIG_NAME "tazero"

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

// See WideHealth.h -- Escalation-only data rework, not enabled here.
#define WIDE_HEALTH_ENABLE 0

//
// Environment / sim sync
//
// TA:Zero's economy has no wind or tidal generators, so those two rows of the
// weather report are meaningless clutter — the overlay stays on for the game
// clock only.  WIND_SPEED_SYNC is left enabled: it is a desync fix rather than
// a display feature, and costs nothing on a mod that ignores wind.
//
#define WEATHER_REPORT 1
#define WEATHER_REPORT_WIND 0
#define WEATHER_REPORT_TIDAL 0
#define WIND_SPEED_SYNC 1
#define VISIBLE_MAP_DTS 1

//
// UI enhancements
//
#define TA_HOOK_ENABLE 1
#define USEMEGAMAP 1
#define MEGAMAP_FEATURES 1
#define USEWHITEBOARD 1
