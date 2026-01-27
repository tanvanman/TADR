#pragma once

//
// Exactly one config must be selected
//
#if (defined(TDRAW_CONFIG_FULL) + \
     defined(TDRAW_CONFIG_NOSNAP) + \
     defined(TDRAW_CONFIG_MINIMAL)) != 1
#define TDRAW_CONFIG_FULL
#endif

#if defined(TDRAW_CONFIG_FULL)
#include "config_full.h"
#elif defined(TDRAW_CONFIG_NOSNAP)
#include "config_nosnap.h"
#elif defined(TDRAW_CONFIG_MINIMAL)
#include "config_minimal.h"
#endif
