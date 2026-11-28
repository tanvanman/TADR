#pragma once

namespace DrawnMove
{
    // DRAWN_MOVE_FORMATIONS_ENABLE=0 builds no-op entry points so shared input
    // and rendering consumers can retain the native path without this feature.
    void LogBuild();
    // Called before other consumers while a gesture owns the mouse, so UI
    // overlays cannot lose its release or focus/capture cancellation.
    bool OwnsInput();
    bool Message(HWND window, UINT message, WPARAM keys, LPARAM position, bool allowStart, bool fromMegamap = false);
    void Draw(LPDIRECTDRAWSURFACE surface);
}
