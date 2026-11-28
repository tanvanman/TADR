#pragma once

#include "tamem.h"

namespace UnitStatusCounters
{
    namespace BarOverlayMetrics
    {
        const int kBarHalfWidth = 17;
        const int kBarOuterHeight = 4;
        const int kBarInnerWidth = 32;
        const int kHealthBarTopOffset = -2;
        const int kHealthBarBottomOffset = 2;
        const int kStackedBarGap = 1;
        const int kCounterAboveBarGap = 1;
        const int kCounterStackGap = 1;
        const int kGroupNumberBelowBarOffset = 2;
    }

    void Install();
    void Shutdown();
    void ResetPendingGroupNumberOffset();
    void SetPendingGroupNumberOffset(int expectedX, int expectedY, int extraYOffset);
    void DrawForUnit(OFFSCREEN* offscreen, UnitStruct* unit, int centerX, int centerY);
}
