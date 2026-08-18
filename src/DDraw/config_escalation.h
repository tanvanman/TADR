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

// Real 32-bit unit health -- see WideHealth.h for the full design and, importantly,
// its "SCOPE OF THIS BUILD" section: several write paths (build progress, savegame,
// ownership transfer, map-spawn, net sync) are not hooked yet and fall back to a
// drift-detection self-heal instead. Off by default even here: this is a first-draft
// implementation that has not been compiled or live-tested (only the design's
// assumptions were, in Phase 1's T1-T5). Flip to 1 only after working through the
// plan's Verification steps.
#define WIDE_HEALTH_ENABLE 0

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
