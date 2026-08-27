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

//
// Air-unit stacking / area-damage immunity -- see AreaDamageOverflow.h.
// Lets one explosion damage every airborne unit on a cell instead of only the one
// holding the cell's air slot. Air-only; ground/naval splash is unchanged.
// Class B patch: every player must run the same build, old demos will not replay.
#define AREA_DAMAGE_OVERFLOW_ENABLE 1
// 1 = dedup victims with no cap, which also repairs vanilla's 20-victim overflow bug
//     (unit 21+ otherwise takes one blast once per cell it occupies).
// 0 = restrict dedup to overflow slots, leaving vanilla ground behaviour bit-exact.
#define AREA_DAMAGE_OVERFLOW_FIX_DEDUP_CAP 1

// Contested-cell tie-break -- see GridClaimTieBreak.h, and READ ITS SCOPE WARNING.
// Replaces the client-relative "evict if the incumbent belongs to a remote human" rule
// with "the lower UnitInGameIndex wins", so every client agrees on who holds a cell.
// WIDER SCOPE THAN AreaDamageOverflow: this sits in the claim path used by every unit
// type on both layers, not just aircraft. Class B patch.
#define GRID_CLAIM_TIEBREAK_ENABLE 1

