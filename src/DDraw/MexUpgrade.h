#pragma once

struct TAdynmemStruct;
struct _MOUSEEVENT;
struct _OFFSCREEN;
struct UnitStruct;
struct UnitOrdersStruct;

namespace MexUpgrade
{
    bool Enabled();
    bool CheckingSite();
    void UpdateCursor(TAdynmemStruct* ta);
    void DrawFeedback(TAdynmemStruct* ta, _OFFSCREEN* surface);
    bool IsQueuedBuild(const TAdynmemStruct* ta, const UnitStruct* constructor, const UnitOrdersStruct* preceding);
    bool TryCommand(TAdynmemStruct* ta, const _MOUSEEVENT* mouse);
}
