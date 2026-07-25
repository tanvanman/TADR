#pragma once

//
// Exactly one config must be selected
//
#if (defined(TDRAW_CONFIG_FULL) + \
     defined(TDRAW_CONFIG_ESCALATION) + \
     defined(TDRAW_CONFIG_MINIMAL) + \
     defined(TDRAW_CONFIG_NO_MEGAMAP_FEATURES)) != 1
#pragma message ( __FILE__ " - Warning: Exactly one TDRAW_CONFIG_* configurations must be #define'd. defaulting to TDRAW_CONFIG_FULL" )
#define TDRAW_CONFIG_FULL
// Implicit-default builds (no explicit config selected) get the profiler
// on; release builds with FULL/ESC/MINIMAL leave it off.
#define TDRAW_PROFILING 1
// Developer-build fallback: opt in to dumps too large for production.
#define TDRAW_DUMP_MAP_ON_ERROR 1
// Dev-only: auto-dump the live UnitDef table (UnitInfoID -> build costs) once
// per process when a game/replay loads, so unit costs can be recovered offline.
// Implicit-default (local) builds only; any explicit TDRAW_CONFIG_* (CI) -> off.
#define TDRAW_DUMP_UNITS_ON_LOAD 1
#endif

#if defined(TDRAW_CONFIG_FULL)
#include "config_full.h"
#elif defined(TDRAW_CONFIG_ESCALATION)
#include "config_escalation.h"
#elif defined(TDRAW_CONFIG_MINIMAL)
#include "config_minimal.h"
#elif defined(TDRAW_CONFIG_NO_MEGAMAP_FEATURES)
#include "config_no_megamap_features.h"
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
