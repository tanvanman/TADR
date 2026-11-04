#pragma once

namespace DrawnMove
{
    void LogBuild();
    // Called before other consumers while a gesture owns the mouse, so UI
    // overlays cannot lose its release or focus/capture cancellation.
    bool OwnsInput();
    bool Message(HWND window, UINT message, WPARAM keys, LPARAM position, bool allowStart, bool fromMegamap = false);
    void Draw(LPDIRECTDRAWSURFACE surface);
}
