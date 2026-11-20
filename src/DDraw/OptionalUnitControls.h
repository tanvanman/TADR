#pragma once

// Independent build choices for optional unit controls. A mod's config_*.h or
// compiler command line may define any of these before config.h includes this
// file. Runtime preferences can disable included features, never enable an
// excluded feature. See OPTIONAL_UNIT_CONTROLS.md for MSBuild examples.
#ifndef DRAWN_MOVE_FORMATIONS_ENABLE
#if defined(TDRAW_CONFIG_ESCALATION) || defined(TDRAW_CONFIG_PROTA)
#define DRAWN_MOVE_FORMATIONS_ENABLE 1
#else
#define DRAWN_MOVE_FORMATIONS_ENABLE 0
#endif
#endif

#ifndef MEX_UPGRADE_ENABLE
#if defined(TDRAW_CONFIG_ESCALATION)
#define MEX_UPGRADE_ENABLE 1
#else
#define MEX_UPGRADE_ENABLE 0
#endif
#endif

#ifndef GRID_BUILDING_ENABLE
#if defined(TDRAW_CONFIG_ESCALATION) || defined(TDRAW_CONFIG_PROTA)
#define GRID_BUILDING_ENABLE 1
#else
#define GRID_BUILDING_ENABLE 0
#endif
#endif

#if DRAWN_MOVE_FORMATIONS_ENABLE != 0 && DRAWN_MOVE_FORMATIONS_ENABLE != 1
#error "DRAWN_MOVE_FORMATIONS_ENABLE must be 0 or 1"
#endif
#if MEX_UPGRADE_ENABLE != 0 && MEX_UPGRADE_ENABLE != 1
#error "MEX_UPGRADE_ENABLE must be 0 or 1"
#endif
#if GRID_BUILDING_ENABLE != 0 && GRID_BUILDING_ENABLE != 1
#error "GRID_BUILDING_ENABLE must be 0 or 1"
#endif

#if defined(TA_HOOK_ENABLE) && !TA_HOOK_ENABLE && \
    (DRAWN_MOVE_FORMATIONS_ENABLE || MEX_UPGRADE_ENABLE || GRID_BUILDING_ENABLE)
#error "Optional unit controls require TA_HOOK_ENABLE=1"
#endif
