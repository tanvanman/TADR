#pragma once

//
// Exactly one config must be selected
//
#if (defined(TDRAW_CONFIG_PROTA) + \
     defined(TDRAW_CONFIG_ESCALATION) + \
     defined(TDRAW_CONFIG_OTA) + \
     defined(TDRAW_CONFIG_TAZERO) + \
     defined(TDRAW_CONFIG_BTA)) != 1
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
