#pragma once

#include <array>

struct TAdynmemStruct;
struct _Position_Dword;

// A presentation-only snapshot. It owns its name and holds no game pointers.
struct AlliedCursorView
{
    bool visible = false;
    unsigned dpid = 0;
    int playerSlot = -1;
    int x = 0, y = 0, z = 0;
    unsigned char color = 0;
    char name[32] = {};
    unsigned receivedAt = 0;
};

namespace AlliedCursors
{
    void Install();
    void UpdateLocal(const _Position_Dword* position, bool valid);
    std::array<AlliedCursorView, 10> Snapshot(TAdynmemStruct* ta);
}
