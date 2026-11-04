#pragma once

namespace DrawnMove
{
    // Called before other consumers while a gesture owns the mouse, so UI
    // overlays cannot lose its release or focus/capture cancellation.
    bool OwnsInput();
    bool Message(HWND window, UINT message, WPARAM keys, LPARAM position, bool allowStart);
    void Draw(LPDIRECTDRAWSURFACE surface);
}
