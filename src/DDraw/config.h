#pragma once

//
// Exactly one config must be selected
//
#if (defined(TDRAW_CONFIG_FULL) + \
     defined(TDRAW_CONFIG_ESCALATION) + \
     defined(TDRAW_CONFIG_MINIMAL)) != 1
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
#endif
