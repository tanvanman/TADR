#pragma once

// Each mod explicitly selects these controls in its config_*.h. This header
// validates the choices; it never infers defaults or enables a feature.
#ifndef DRAWN_MOVE_FORMATIONS_ENABLE
#error "Each mod config must define DRAWN_MOVE_FORMATIONS_ENABLE"
#endif

#ifndef MEX_UPGRADE_ENABLE
#error "Each mod config must define MEX_UPGRADE_ENABLE"
#endif

#ifndef GRID_BUILDING_ENABLE
#error "Each mod config must define GRID_BUILDING_ENABLE"
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
