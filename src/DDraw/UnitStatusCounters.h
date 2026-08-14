#pragma once

#include "tamem.h"

namespace UnitStatusCounters
{
    void Install();
    void Shutdown();
    void DrawForUnit(OFFSCREEN* offscreen, UnitStruct* unit, int centerX, int centerY);
}
