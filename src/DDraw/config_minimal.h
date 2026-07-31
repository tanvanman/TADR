#pragma once

#define TDRAW_CONFIG_NAME "minimal"

//
// Snap radii
//
#define DEFAULT_MEX_SNAP_RADIUS 0
#define MAX_MEX_SNAP_RADIUS 0
#define DEFAULT_WRECK_SNAP_RADIUS 0
#define MAX_WRECK_SNAP_RADIUS 0

//
// Anti-share-abuse
//
// Structure shares are rate limited, and '.take' is refused when the target's
// commander has already been destroyed.  See ESCALATION_SHARE_GUARD_DESIGN.md.
#define SHARE_ABUSE_GUARD 0

//
// Construction / AI behavior
//
#define FIXED_POSN_GUARDING_CONS_ENABLE 0
#define PATROLING_CONS_RECLAIM_OR_ASSIST_ENABLE 0
#define CONSTRUCTION_KICKOUT_ENABLE 0

//
// Environment / sim sync
//
#define WEATHER_REPORT 0
#define WEATHER_REPORT_WIND 0
#define WEATHER_REPORT_TIDAL 0
#define WIND_SPEED_SYNC 0
#define VISIBLE_MAP_DTS 0

//
// UI enhancements
//
#define TA_HOOK_ENABLE 1
#define USEMEGAMAP 1
#define MEGAMAP_FEATURES 0
#define USEWHITEBOARD 1
