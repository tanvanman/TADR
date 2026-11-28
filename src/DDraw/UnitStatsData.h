#pragma once

#include "UnitStatsCore.h"

struct TAdynmemStruct;
struct UnitStruct;

namespace UnitStatsData
{
    // Copies values from the currently loaded mod. No game pointer is retained.
    // singleSelected supplies current HP/economy only for one owned live unit.
    bool Read(TAdynmemStruct* ta, unsigned typeIndex, const UnitStruct* singleSelected,
        unsigned count, UnitStatsCore::Stats& out);
}
