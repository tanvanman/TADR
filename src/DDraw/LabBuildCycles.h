#pragma once

struct UnitStruct;

// Local factory queue control. No production state or custom packets are
// installed on remote players' units; the engine's usual unit sync remains
// responsible for the completed units.
namespace LabBuildCycles
{
    void Install();
    void Shutdown();
    bool SelectionState(bool& enabled);
    bool ToggleSelection();
    bool IsEnabled(UnitStruct* unit);
}
