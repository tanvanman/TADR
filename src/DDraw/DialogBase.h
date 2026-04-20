#pragma once

#include "oddraw.h"

#include <memory>
#include <string>
#include <vector>

class Widget;

// Base class for non-modal DirectDraw overlay dialogs.
//
// Owns the dialog surface, PCX background, the four font surfaces, and the
// cursor surface.  Provides default BlitDialog / Message implementations that
// cover visibility, surface-lost recovery, drag, widget routing, and cursor
// blitting.  Subclasses implement RenderDialog() and, if needed, override
// RestoreAll(), OnDragMoved(), and OnMouseInsideDialog().
class DialogBase
{
    friend class Widget;
    friend class Button;
    friend class Label;

public:
    DialogBase(bool vidMem, int width, int height, int rowHeight);
    virtual ~DialogBase();

    // Drawing utilities — called by Widget subclasses and subclass RenderDialog().
    void DrawText(LPDIRECTDRAWSURFACE dest, int x, int y, const char* text);
    void DrawSmallText(LPDIRECTDRAWSURFACE dest, int x, int y, const char* text);
    void DrawTexture(int x, int y, int width, int height,
                     LPDIRECTDRAWSURFACE texture, int texturePosX, int texturePosY);
    int  DrawTextField(int posX, int posY, int width, int height,
                       const std::string& text, char color);

    // Blit the dialog (and cursor) to destSurf.  No-op if not visible.
    // Calls RenderDialog() every frame, then blits lpDialogSurf.
    virtual void BlitDialog(LPDIRECTDRAWSURFACE destSurf);

    // Route a window message.  Returns true if the message was consumed.
    // Handles widget focus/routing, drag, cursor tracking, WM_LBUTTONDBLCLK.
    virtual bool Message(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // Cursor blitting — public so external renderers (whiteboard, income bar, etc.)
    // can blit the cursor at an arbitrary position on top of their own surfaces.
    void BlitCursor(LPDIRECTDRAWSURFACE destSurf, int x, int y);

    // Public so Button / Label can access them directly via a DialogBase*.
    LPDIRECTDRAWSURFACE lpDialogSurf;
    int posX, posY;

protected:
    // Re-draw the dialog: RenderBackground() then draw all widgets.
    // Subclasses that need pre-render work (e.g. updating Label text) override
    // this, do their updates, then call DialogBase::RenderDialog().
    virtual void RenderDialog();

    // Restore all surfaces after a device-lost event.
    // Base: shared surfaces (lpDialogSurf, background, fonts) + cursor.
    // Subclasses override to also restore their own button-skin surfaces,
    // then call DialogBase::RestoreAll() first.
    virtual void RestoreAll();

    // Called after the dialog position changes due to a drag move.
    // Dialog overrides to call CorrectPos().
    virtual void OnDragMoved() {}

    // Called when the mouse is within the dialog's cursor-tracking zone.
    // Dialog overrides to clear the MegaMap cursor.
    virtual void OnMouseInsideDialog(int /*mx*/, int /*my*/) {}

    // Restore shared surfaces from PCX resources.
    void RestoreSharedSurfaces();

    // Blit lpBackground onto lpDialogSurf.
    void RenderBackground();

    void RestoreCursor();

    bool m_visible;

    bool VidMem;
    int  m_dialogWidth;
    int  m_dialogHeight;
	int m_rowHeight;

    LPDIRECTDRAWSURFACE lpBackground;
    LPDIRECTDRAWSURFACE lpUCFont;
    LPDIRECTDRAWSURFACE lpLCFont;
    LPDIRECTDRAWSURFACE lpSmallUCFont;
    LPDIRECTDRAWSURFACE lpSmallLCFont;
    LPDIRECTDRAWSURFACE lpCursor;
    int CursorBackground;
    int CursorPosX, CursorPosY;

    std::vector<std::shared_ptr<Widget>> m_widgets;

    bool Move;
    int  posXPrev, posYPrev;

private:
    // Working state for DrawTextField (set during Lock, used by helpers).
    LPVOID SurfaceMemory;
    int    lPitch;

    void DrawTinyText(char* str, int posx, int posy, char color);
    void FillRect(int x, int y, int x2, int y2, char color);
};
