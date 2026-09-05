#pragma once

#define TDRAW_CONFIG_NAME "mayhem"

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
// Lag-switch mitigation
//
// Freezes the local simulation while every remote peer is silent, so a player
// pulling the plug on his own connection cannot manoeuvre while immune to
// incoming damage.  Purely local -- it only suppresses this client's own sim
// ticks, so it neither desyncs nor changes the wire protocol.  It does,
// however, stall the game on ordinary packet loss too.  See LagSwitchGuard.h.
#define LAG_SWITCH_GUARD_ENABLE 1

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
// Extended weapon IDs (>= 256)
//
// Installs WeaponIdOverflow (heap-backed weapon slots above TA's hard-coded
// Weapons[256]) plus WeaponFiredExt (the CHAT_05-hijack packet that carries
// fire events for those overflow IDs, which the native WEAPON_FIRED_0D byte ID
// cannot address).  See config.h for the full description.
#define TDRAW_EXTENDED_WEAPON_IDS 1

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

//
// Allied build-queue overlay -- see AlliedBuildQueueSync.h, and config.h for
// the full description.  Draws allies' queued build placements (game screen
// while SHIFT is held with an allied builder under the cursor/camera, and the
// megamap) and broadcasts the local player's own queue to allies on
// CHAT_05-hijack msgId 0x60.  Off: nothing is hooked, sent, parsed or drawn,
// and the "Show ally queues" dialog checkbox is not created.
#define ALLIED_BUILD_QUEUE_ENABLE 0

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
