#pragma once

#include "AlliedCursors.h"

namespace AlliedCursorDraw
{
    // Passive sampling: does not capture input or change the native cursor.
    bool Sample(TAdynmemStruct* ta, _Position_Dword& position);
    // Called while the composed, indexed-color back buffer is locked.
    void Draw(char* pixels, int width, int height, int pitch,
        const TAdynmemStruct& ta, const std::array<AlliedCursorView, 10>& views);
}
