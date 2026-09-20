#pragma once

//
// Exactly one config must be selected
//
#if (defined(TDRAW_CONFIG_PROTA) + \
     defined(TDRAW_CONFIG_ESCALATION) + \
     defined(TDRAW_CONFIG_OTA) + \
     defined(TDRAW_CONFIG_TAZERO) + \
     defined(TDRAW_CONFIG_BTA) + \
     defined(TDRAW_CONFIG_MAYHEM) + \
     defined(TDRAW_CONFIG_TWILIGHT)) != 1
#pragma message ( __FILE__ " - Warning: Exactly one TDRAW_CONFIG_* configurations must be #define'd. defaulting to TDRAW_CONFIG_PROTA" )
#define TDRAW_CONFIG_PROTA
// Implicit-default builds (no explicit config selected) get the profiler
// on; release builds with PROTA/ESC/OTA leave it off.
#define TDRAW_PROFILING 1
// Developer-build fallback: opt in to dumps too large for production.
#define TDRAW_DUMP_MAP_ON_ERROR 1
// Dev-only: auto-dump the live UnitDef table (UnitInfoID -> build costs) once
// per process when a game/replay loads, so unit costs can be recovered offline.
// Implicit-default (local) builds only; any explicit TDRAW_CONFIG_* (CI) -> off.
#define TDRAW_DUMP_UNITS_ON_LOAD 1
#endif

#if defined(TDRAW_CONFIG_PROTA)
#include "config_prota.h"
#elif defined(TDRAW_CONFIG_ESCALATION)
#include "config_escalation.h"
#elif defined(TDRAW_CONFIG_OTA)
#include "config_ota.h"
#elif defined(TDRAW_CONFIG_TAZERO)
#include "config_tazero.h"
#elif defined(TDRAW_CONFIG_BTA)
#include "config_bta.h"
#elif defined(TDRAW_CONFIG_MAYHEM)
#include "config_mayhem.h"
#elif defined(TDRAW_CONFIG_TWILIGHT)
#include "config_twilight.h"
#endif

//
// Weather report rows.  WEATHER_REPORT is the master switch for the whole
// top-of-screen overlay (wind / tidal / game time); these two select which of
// the environment rows are drawn within it.  Mods whose economy has no wind or
// tidal generators (e.g. TA:Zero) turn them off and keep just the game clock.
// Defaulted here so a config_*.h that predates them keeps the old behaviour.
//
#ifndef WEATHER_REPORT_WIND
#define WEATHER_REPORT_WIND WEATHER_REPORT
#endif
#ifndef WEATHER_REPORT_TIDAL
#define WEATHER_REPORT_TIDAL WEATHER_REPORT
#endif

//
// Extended weapon IDs (>= 256) — experimental, OFF by default for now.
//
// When enabled this installs two cooperating patches:
//   * WeaponIdOverflow  — heap-backed overflow weapon slots above TA's
//                         hard-coded Weapons[256] array (lets TDF authors
//                         define ID >= 256 without corrupting adjacent memory).
//   * WeaponFiredExt    — a CHAT_05-hijack network packet that transmits
//                         fire events for those overflow weapon IDs (TA's
//                         native WEAPON_FIRED_0D packet only carries a byte ID).
//
// Both modules ship compiled in but are not installed unless this flag is 1.
// Enable by defining TDRAW_EXTENDED_WEAPON_IDS=1 in a config_*.h or on the
// compiler command line.
//
#ifndef TDRAW_EXTENDED_WEAPON_IDS
#define TDRAW_EXTENDED_WEAPON_IDS 0
#endif

//
// AirCorpseFall: make the wreck of an aircraft killed over land fall to the
// ground instead of hanging at the altitude it died. Changes vanilla behaviour
// for every unit that dies airborne, so it is opt-in per config.
//
#ifndef AIR_CORPSE_FALL_ENABLE
#define AIR_CORPSE_FALL_ENABLE 0
#endif

//
// PlayerMute: the local, display-only `.mute` / `.unmute` typed command --
// see PlayerMute.h. Splices Net_PushChatHudMessage @0x00463CA0 and
// Chat_FormatAndSend @0x00463E50; touches no simulation state and needs no
// version gate in principle (a runtime ini key would do), but every other
// splice this codebase installs unconditionally on every config is either
// engine-version-signature-checked (ChatPosition) or itself opt-in
// (AlliedBuildQueueSync, AirCorpseFall) -- PlayerMute was neither. Gated the
// same way as those: every config_*.h defines this explicitly.
//
#ifndef PLAYER_MUTE_ENABLE
#define PLAYER_MUTE_ENABLE 0
#endif

//
// LagSwitchGuard: freeze the local simulation while every remote peer is
// silent, so a lag switch cannot buy the cheater invulnerable manoeuvring
// time.  Defaulted on so a config_*.h that predates the flag keeps the
// behaviour it shipped with; a mod that would rather ride out packet loss
// than stall sets it to 0.
//
#ifndef LAG_SWITCH_GUARD_ENABLE
#define LAG_SWITCH_GUARD_ENABLE 1
#endif

//
// AlliedBuildQueue: show allies' queued (not yet started) build placements --
// on the game screen while SHIFT is held with an allied builder under the
// cursor/camera, and on the megamap -- and broadcast the local player's own
// queue to allies over a CHAT_05-hijack packet (ChatHijackId::AlliedBuildQueue,
// 0x60) so allies see placements TA never puts on the wire.  Purely additive
// display: it changes no simulation state, and a client built without it
// neither sends nor parses the packet.  It does add periodic packet traffic
// and hands allies information stock TA does not, so it is opt-in per config.
// Defaulted OFF so a config_*.h predating the flag does not pick it up
// silently.  When 0, AlliedBuildQueueSync.cpp compiles to nothing, no hook or
// packet handler is installed, neither overlay is drawn, and the "Show ally
// queues" dialog checkbox is not created.
//
#ifndef ALLIED_BUILD_QUEUE_ENABLE
#define ALLIED_BUILD_QUEUE_ENABLE 0
#endif

//
// COB dispatch-table patch -- Class A (bit-identical, purely faster). See
// ai-reference/simulation-performance/COB_DISPATCH_PROJECT.md and CLAUDE.md.
// Escalation-only -- the splice window and every hardcoded address are
// specific to that exact binary. Every config_*.h defines this explicitly;
// this fallback is only for a future one that forgets to.
//
#ifndef COB_DISPATCH_TABLE_ENABLE
#define COB_DISPATCH_TABLE_ENABLE 0
#endif

//
// BuildWeaponSlotGuard -- fixes the stockpile ("Nanolathing") build-percent
// divide-by-zero crash. Real cause (corrected 2026-09-14, PR #26 review): a
// unit-identity divergence can make a legitimately-issued order reference a weapon
// slot that is locally unarmed, resolving to WeaponsTypedefArray[0] -- TA's permanent
// "no weapon" sentinel, whose reload divisor is 0 by construction. Both
// Unit_GetLinkedBuildWeaponPercent and MissionTick_BuildWeapon divide by that field
// unchecked; also bounds-checks the adjacent unchecked weapon-slot index in the same
// two functions. See BuildWeaponSlotGuard.h for the full derivation.
//
// Gated to Escalation only -- this project has verified these addresses against
// Escalation's TotalA.exe alone. PR #26's review found the same six hook/bail-out
// signatures byte-identical on all seven shipped TotalA.exe builds (stock TA engine
// code, not Escalation-specific), but this project has not re-run that verification
// itself, so this stays a staged rollout rather than an assumption either way. Every
// config_*.h defines this explicitly; this fallback is only for a future one that
// forgets to.
//
#ifndef BUILD_WEAPON_SLOT_GUARD_ENABLE
#define BUILD_WEAPON_SLOT_GUARD_ENABLE 0
#endif

//
// ReceiveWeaponFired: take the projectile-kind branch from the firing unit's own weapon slot
// rather than from the weapon id in the packet.
//
// TA picks the branch at 0x0049D42A from WeaponsTypedefArray[pkt[0x19]], but all three
// UNITS_FireProjectile_* callees build from -- and Ballistic divides by the weaponvelocity of --
// shooter->UnitWeapons[pkt[0x23]].p_Weapon, with nothing checking the two agree. When a client's
// copy of the shooter has the wrong unit type its slot can be the all-zero "no weapon" entry
// while the packet names a ballistic weapon: divide by zero, on a well-formed packet, killing
// only that client. Reading from p_Weapon is what TA's own local firing path does at 0x0049D742,
// so this is a no-op unless the client is already diverged. (The meteor test at the top of
// ReceiveWeaponFired still uses the packet weapon, correctly -- it has no unit at all.)
//
// Compile-time only per the standing rule: a fleet split over which projectile gets created is
// exactly the divergence that rule exists to prevent. 0 still installs the hook, but it only
// records the TRACE_CAT_WPNX breadcrumb.
//
#ifndef WEAPONFIRE_DISPATCH_FROM_SLOT
#define WEAPONFIRE_DISPATCH_FROM_SLOT 1
#endif

//
// Unit-identity audit: every ~900 ticks, walk each player's block of the unit array, compare the
// walked live count against that player's nNumUnits, and broadcast the owner's own count+digest
// on CHAT_05 hijack msgId 0x31 (ChatHijackId::UnitIdentityDigest) so every client can check its
// copy of that block against the authority. TA has never had an "am I in sync?" signal.
//
// Diagnostic only -- no simulation state. One block walk and one 65-byte packet per player per
// ~30 s. A disagreement must survive three consecutive audits before it is reported: the two
// clients sample different instants and TA is not lockstep. A client built without this neither
// sends nor parses the packet, and an old client ignores the unregistered msgId.
//
// The MORF/GHST/WPNX breadcrumbs in UnitIdentity.cpp are NOT gated by this -- always on.
//
#ifndef UNIT_IDENTITY_AUDIT_ENABLE
#define UNIT_IDENTITY_AUDIT_ENABLE 1
#endif

//
// 0x2C dirty-entry bailout. OBSERVE-ONLY until the 2CBD breadcrumbs explain what produces the bad
// entries. Prod bundles show 18 fatals inside Receive_UnitStatAndMove_2C (13 at 0048BA07 on a null
// move-class, 5 at 0048B9AD on a wild unit pointer), and the wild pointers are ~1000x further from
// the unit array than a 16-bit slotDelta can reach -- so this is not simply an unvalidated index,
// and a guard that skipped the entry would hide the fault while leaving it active. 1 bails to
// 0x0048BA28, the engine's own end-of-list fall-through.
//
#ifndef TDRAW_2C_ENTRY_BAILOUT
#define TDRAW_2C_ENTRY_BAILOUT 0
#endif

//
// Sound instance limiting: drop a local playback whose sound object already started inside this
// window, in milliseconds. 0 disables.
//
// TA plays a 3D sound per projectile event and every play reaches DirectSound, which does registry
// lookups per buffer play -- sampling put ~26% of the main thread in DSOUND, ~20% of it in
// RegOpenKeyExA. Hooks DSoundP_PlayBuffer @0x004CF582, the single choke point for 2D, 3D and
// remote players' sounds (Packet_Dispatcher @0045563F feeds wire sounds into PlaySound_3D_ID_P13),
// so one hook covers every origin. Audio only: no simulation state, no wire effect.
//
// A companion SOUND_BROADCAST_LIMIT_MS was removed 2026-09-09 -- it read "sent=0 dropped=0" in
// every log, because every caller passes priority 0 and TA never broadcasts sounds here. Do not
// re-add it without first confirming a caller that passes a non-zero priority.
//
#ifndef SOUND_INSTANCE_LIMIT_MS
#define SOUND_INSTANCE_LIMIT_MS 50
#endif

//
// Repair-rate fix heal multipliers -- see config_escalation.h for the tunable
// values and RepairRateFix.cpp for how they're applied. Every config_*.h must
// define both explicitly (same convention as REPAIR_RATE_FIX_ENABLE itself,
// SS9), so there is no #ifndef fallback here on purpose: a config that forgets
// one gets a hard compiler error at the #ifndef check below, not a silent 1.
//
#ifndef REPAIR_RATE_FIX_REPAIR_MULTIPLIER
#error "config_*.h must define REPAIR_RATE_FIX_REPAIR_MULTIPLIER explicitly (1 if REPAIR_RATE_FIX_ENABLE is 0)"
#endif
#ifndef REPAIR_RATE_FIX_SELFHEAL_MULTIPLIER
#error "config_*.h must define REPAIR_RATE_FIX_SELFHEAL_MULTIPLIER explicitly (1 if REPAIR_RATE_FIX_ENABLE is 0)"
#endif

// Rule: both multipliers are meaningless -- and forbidden -- unless the fix
// itself is installed. They scale RepairRateFix's accumulator formula, which
// only exists when REPAIR_RATE_FIX_ENABLE is 1 (ddraw.cpp guards Install()
// with it). A config that sets a multiplier != 1 while the fix is off would
// silently do nothing at runtime -- fail the build instead, loudly, so that
// mistake can't ship unnoticed.
#if !REPAIR_RATE_FIX_ENABLE && \
    (REPAIR_RATE_FIX_REPAIR_MULTIPLIER != 1 || REPAIR_RATE_FIX_SELFHEAL_MULTIPLIER != 1)
#error "REPAIR_RATE_FIX_REPAIR_MULTIPLIER / SELFHEAL_MULTIPLIER require REPAIR_RATE_FIX_ENABLE 1 -- they scale RepairRateFix's accumulator, which is not installed in this config."
#endif

//
// SharePercent: accept a `%` suffix on +setsharemetal / +setshareenergy
// (e.g. `+setshareenergy 50%`) so the share threshold tracks a percentage of
// max storage instead of a fixed absolute that never adjusts as storage
// grows. A plain integer keeps vanilla behaviour unchanged. Local per-client
// state only -- cannot desync. Defaulted OFF; when 0, SharePercent.cpp
// compiles to nothing.
//
#ifndef SHARE_PERCENT_ENABLE
#define SHARE_PERCENT_ENABLE 0
#endif
