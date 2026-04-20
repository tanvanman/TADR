#include "DialogBase.h"

#include "font.h"
#include "gaf.h"
#include "pcxread.h"
#include "tamem.h"
#include "tafunctions.h"
#include "Widgets/Widget.h"
#include "iddrawsurface.h"

#include <cstring>

#define ROW_HEIGHT 18

// PCX resource indices shared by all dialogs:
//   2  = panel background
//   3  = UC font
//   7  = LC font
//   8  = small UC font
//   9  = small LC font

DialogBase::DialogBase(bool vidMem, int width, int height, int rowHeight)
    : lpDialogSurf(nullptr)
    , posX(0)
    , posY(0)
    , m_visible(false)
    , VidMem(vidMem)
    , m_dialogWidth(width)
    , m_dialogHeight(height)
	, m_rowHeight(rowHeight)
    , lpBackground(nullptr)
    , lpUCFont(nullptr)
    , lpLCFont(nullptr)
    , lpSmallUCFont(nullptr)
    , lpSmallLCFont(nullptr)
    , lpCursor(nullptr)
    , CursorBackground(-1)
    , CursorPosX(-1)
    , CursorPosY(-1)
    , Move(false)
    , posXPrev(0)
    , posYPrev(0)
    , SurfaceMemory(nullptr)
    , lPitch(0)
{
    LPDIRECTDRAW taDD = (LPDIRECTDRAW)LocalShare->TADirectDraw;

    DDSURFACEDESC ddsd;
    DDRAW_INIT_STRUCT(ddsd);
    ddsd.dwFlags        = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    ddsd.dwWidth        = width;
    ddsd.dwHeight       = height;
    taDD->CreateSurface(&ddsd, &lpDialogSurf, NULL);

    lpBackground  = CreateSurfPCXResource(2, vidMem);
    lpUCFont      = CreateSurfPCXResource(3, vidMem);
    lpLCFont      = CreateSurfPCXResource(7, vidMem);
    lpSmallUCFont = CreateSurfPCXResource(8, vidMem);
    lpSmallLCFont = CreateSurfPCXResource(9, vidMem);

    PGAFSequence CursorSequence = (*TAmainStruct_PtrPtr)->cursor_ary[cursornormal];
    if (CursorSequence != NULL)
    {
        PGAFFrame GafFrame = CursorSequence->PtrFrameAry[0].PtrFrame;
        lpCursor = CreateSurfByGafFrame(taDD, GafFrame, vidMem);
        CursorBackground = GafFrame->Background;
    }
}

DialogBase::~DialogBase()
{
    if (lpDialogSurf)  { lpDialogSurf->Release();  lpDialogSurf  = nullptr; }
    if (lpBackground)  { lpBackground->Release();  lpBackground  = nullptr; }
    if (lpUCFont)      { lpUCFont->Release();      lpUCFont      = nullptr; }
    if (lpLCFont)      { lpLCFont->Release();      lpLCFont      = nullptr; }
    if (lpSmallUCFont) { lpSmallUCFont->Release(); lpSmallUCFont = nullptr; }
    if (lpSmallLCFont) { lpSmallLCFont->Release(); lpSmallLCFont = nullptr; }
    if (lpCursor)      { lpCursor->Release();      lpCursor      = nullptr; }
}

// -----------------------------------------------------------------------
// RenderDialog: clear the canvas and draw all widgets.
// Subclasses that need pre-render work override this, do their updates,
// then call DialogBase::RenderDialog().
// -----------------------------------------------------------------------
void DialogBase::RenderDialog()
{
    RenderBackground();
    for (auto& w : m_widgets)
        w->Draw(this);
}

// -----------------------------------------------------------------------
// RenderBackground: blit the panel PCX onto lpDialogSurf.
// -----------------------------------------------------------------------
void DialogBase::RenderBackground()
{
    if (lpDialogSurf->Blt(NULL, lpBackground, NULL, DDBLT_ASYNC, NULL) != DD_OK)
        lpDialogSurf->Blt(NULL, lpBackground, NULL, DDBLT_WAIT, NULL);
}

// -----------------------------------------------------------------------
// RestoreSharedSurfaces: restore lost surfaces and re-fill from PCX.
// -----------------------------------------------------------------------
void DialogBase::RestoreSharedSurfaces()
{
    lpDialogSurf->Restore();
    lpBackground->Restore();
    lpUCFont->Restore();
    lpLCFont->Restore();
    lpSmallUCFont->Restore();
    lpSmallLCFont->Restore();

    RestoreFromPCX(2, lpBackground);
    RestoreFromPCX(3, lpUCFont);
    RestoreFromPCX(7, lpLCFont);
    RestoreFromPCX(8, lpSmallUCFont);
    RestoreFromPCX(9, lpSmallLCFont);
}

// -----------------------------------------------------------------------
// RestoreAll: restore all surfaces after a device-lost event.
// Base restores shared surfaces + cursor.  Subclasses call this first,
// then restore their own button-skin surfaces and re-render.
// -----------------------------------------------------------------------
void DialogBase::RestoreAll()
{
    RestoreSharedSurfaces();
    RestoreCursor();
}

// -----------------------------------------------------------------------
// BlitDialog: render and blit the dialog to destSurf each frame.
// -----------------------------------------------------------------------
void DialogBase::BlitDialog(LPDIRECTDRAWSURFACE destSurf)
{
    if (!m_visible || !lpDialogSurf)
        return;

    if (lpDialogSurf->IsLost() != DD_OK)
        RestoreAll();

    if (!m_visible)
        return;

    RenderDialog();

    RECT src  = { 0, 0, m_dialogWidth, m_dialogHeight };
    RECT dest = { posX, posY, posX + m_dialogWidth, posY + m_dialogHeight };
    if (destSurf->Blt(&dest, lpDialogSurf, &src, DDBLT_ASYNC, NULL) != DD_OK)
        destSurf->Blt(&dest, lpDialogSurf, &src, DDBLT_WAIT, NULL);

    if (CursorPosX != -1 && CursorPosY != -1)
        BlitCursor(destSurf, CursorPosX, CursorPosY);
}

// -----------------------------------------------------------------------
// Message: route window messages for the dialog.
// Handles widget focus/routing, drag, cursor tracking, and standard
// consume-inside-dialog behaviour.
// -----------------------------------------------------------------------
bool DialogBase::Message(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (!m_visible)
        return false;

    int mx = (int)(short)LOWORD(lParam);
    int my = (int)(short)HIWORD(lParam);

    bool insideDialog = (mx >= posX && mx < posX + m_dialogWidth &&
                         my >= posY && my < posY + m_dialogHeight);

    // Update widget focus on mouse-down.
    if (msg == WM_LBUTTONDOWN)
    {
        int x = mx - posX;
        int y = my - posY;
        for (auto& w : m_widgets)
        {
            bool isInside = !w->m_hidden && !w->m_disabled && w->IsInside(x, y);
            if (isInside != w->m_focused)
            {
                w->m_focused = isInside;
                w->Draw(this);
            }
        }
    }

    // Route to widgets first.
    // Take a snapshot so that a widget callback that rebuilds m_widgets
    // (e.g. a YES/NO vote button calling Refresh() -> RebuildRows() ->
    // m_widgets.clear()) does not invalidate the iterator or destroy the
    // widget we are about to call Draw() on after Message() returns.
    {
        std::vector<std::shared_ptr<Widget>> snapshot = m_widgets;
        for (auto& w : snapshot)
        {
            if (w->Message(this, hWnd, msg, wParam, lParam))
            {
                w->Draw(this);
                return true;
            }
        }
    }

    switch (msg)
    {
    case WM_LBUTTONDBLCLK:
        return insideDialog;

    case WM_LBUTTONDOWN:
        if (insideDialog)
        {
            Move     = true;
            posXPrev = mx;
            posYPrev = my;
            return true;
        }
        break;

    case WM_LBUTTONUP:
        if (Move) { Move = false; return insideDialog; }
        break;

    case WM_MOUSEMOVE:
        if (mx >= posX - 10 && mx < posX + m_dialogWidth &&
            my >= posY - 20 && my < posY + m_dialogHeight)
        {
            CursorPosX = mx;
            CursorPosY = my;
            OnMouseInsideDialog(mx, my);
        }
        else
        {
            CursorPosX = -1;
            CursorPosY = -1;
        }
        if (Move)
        {
            posX    += mx - posXPrev;
            posY    += my - posYPrev;
            OnDragMoved();
            posXPrev = mx;
            posYPrev = my;
            return true;
        }
        posXPrev = mx;
        posYPrev = my;
        return insideDialog;
    }

    return false;
}

// -----------------------------------------------------------------------
// BlitCursor: blit lpCursor to destSurf at (x, y) with colour-key.
// -----------------------------------------------------------------------
void DialogBase::BlitCursor(LPDIRECTDRAWSURFACE destSurf, int x, int y)
{
    if (lpCursor == NULL || lpCursor->IsLost() != DD_OK)
        RestoreCursor();
    if (lpCursor == NULL)
        return;

    DDSURFACEDESC ddsc;
    DDRAW_INIT_STRUCT(ddsc);
    DDBLTFX ddbltfx;
    DDRAW_INIT_STRUCT(ddbltfx);

    lpCursor->GetSurfaceDesc(&ddsc);
    ddbltfx.ddckSrcColorkey.dwColorSpaceLowValue  = CursorBackground & 0xffff;
    ddbltfx.ddckSrcColorkey.dwColorSpaceHighValue = CursorBackground >> 16;

    RECT Dest = { x, y, (LONG)(x + ddsc.dwWidth), (LONG)(y + ddsc.dwHeight) };
    RECT Src  = { 0, 0, (LONG)ddsc.dwWidth,        (LONG)ddsc.dwHeight       };

    if (destSurf->Blt(&Dest, lpCursor, &Src, DDBLT_ASYNC | DDBLT_KEYSRCOVERRIDE, &ddbltfx) != DD_OK)
        destSurf->Blt(&Dest, lpCursor, &Src, DDBLT_WAIT  | DDBLT_KEYSRCOVERRIDE, &ddbltfx);
}

// -----------------------------------------------------------------------
// RestoreCursor: restore lpCursor from GAF (or PCX 4 as fallback).
// -----------------------------------------------------------------------
void DialogBase::RestoreCursor()
{
    if (lpCursor != NULL)
    {
        if (lpCursor->IsLost() != DD_OK)
            lpCursor->Restore();

        PGAFSequence CursorSequence = (*TAmainStruct_PtrPtr)->cursor_ary[cursornormal];
        if (CursorSequence != NULL)
        {
            PGAFFrame GafFrame = CursorSequence->PtrFrameAry[0].PtrFrame;

            DDSURFACEDESC ddsd;
            DDRAW_INIT_STRUCT(ddsd);
            lpCursor->Lock(NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL);

            unsigned char* SurfPTR = (unsigned char*)ddsd.lpSurface;
            CursorBackground = GafFrame->Background;
            POINT Aspect = { ddsd.lPitch, (LONG)ddsd.dwHeight };
            memset(SurfPTR, CursorBackground, (size_t)ddsd.lPitch * ddsd.dwHeight);
            CopyGafToBits(SurfPTR, &Aspect, 0, 0, GafFrame);
            lpCursor->Unlock(NULL);
        }
        else
        {
            RestoreFromPCX(4, lpCursor);
        }
    }
    else
    {
        PGAFSequence CursorSequence = (*TAmainStruct_PtrPtr)->cursor_ary[cursornormal];
        if (CursorSequence != NULL)
        {
            PGAFFrame GafFrame = CursorSequence->PtrFrameAry[0].PtrFrame;
            lpCursor = CreateSurfByGafFrame((LPDIRECTDRAW)LocalShare->TADirectDraw, GafFrame, VidMem);
            CursorBackground = GafFrame->Background;
        }
    }
}

// -----------------------------------------------------------------------
// DrawText: blit proportional UC/LC font glyphs onto dest.
// -----------------------------------------------------------------------
void DialogBase::DrawText(LPDIRECTDRAWSURFACE dest, int x, int y, const char* text)
{
    RECT Dest;
    Dest.left  = x;
    Dest.top   = y;
    Dest.bottom = Dest.top + 14;
    RECT Source;
    Source.left   = 0;
    Source.top    = 0;
    Source.bottom = 14;
    DDBLTFX ddbltfx;
    DDRAW_INIT_STRUCT(ddbltfx);
    ddbltfx.ddckSrcColorkey.dwColorSpaceLowValue  = 102;
    ddbltfx.ddckSrcColorkey.dwColorSpaceHighValue = 102;

    for (size_t i = 0; i < strlen(text); i++)
    {
        if (text[i] < 91 && text[i] >= 33)
        {
            Dest.right   = Dest.left + FontOffsetUC[text[i] - 33][0];
            Source.left  = FontOffsetUC[text[i] - 33][1];
            Source.right = Source.left + FontOffsetUC[text[i] - 33][0];
            if (dest->Blt(&Dest, lpUCFont, &Source, DDBLT_ASYNC | DDBLT_KEYSRCOVERRIDE, &ddbltfx) != DD_OK)
                dest->Blt(&Dest, lpUCFont, &Source, DDBLT_WAIT | DDBLT_KEYSRCOVERRIDE, &ddbltfx);
            Dest.left += FontOffsetUC[text[i] - 33][0];
        }
        else if (text[i] < 123 && text[i] >= 97)
        {
            Dest.right   = Dest.left + FontOffsetLC[text[i] - 97][0];
            Source.left  = FontOffsetLC[text[i] - 97][1];
            Source.right = Source.left + FontOffsetLC[text[i] - 97][0];
            if (dest->Blt(&Dest, lpLCFont, &Source, DDBLT_ASYNC | DDBLT_KEYSRCOVERRIDE, &ddbltfx) != DD_OK)
                dest->Blt(&Dest, lpLCFont, &Source, DDBLT_WAIT | DDBLT_KEYSRCOVERRIDE, &ddbltfx);
            Dest.left += FontOffsetLC[text[i] - 97][0];
        }
        if (text[i] == ' ')
            Dest.left += 7;
    }
}

// -----------------------------------------------------------------------
// DrawSmallText: blit proportional small UC/LC font glyphs onto dest.
// -----------------------------------------------------------------------
void DialogBase::DrawSmallText(LPDIRECTDRAWSURFACE dest, int x, int y, const char* text)
{
    RECT Dest;
    Dest.left   = x;
    Dest.top    = y;
    Dest.bottom = Dest.top + 12;
    RECT Source;
    Source.left   = 0;
    Source.top    = 0;
    Source.bottom = 12;
    DDBLTFX ddbltfx;
    DDRAW_INIT_STRUCT(ddbltfx);
    ddbltfx.ddckSrcColorkey.dwColorSpaceLowValue  = 102;
    ddbltfx.ddckSrcColorkey.dwColorSpaceHighValue = 102;

    for (size_t i = 0; i < strlen(text); i++)
    {
        if (text[i] < 91 && text[i] >= 33)
        {
            Dest.right   = Dest.left + SmallFontOffsetUC[text[i] - 33][0];
            Source.left  = SmallFontOffsetUC[text[i] - 33][1];
            Source.right = Source.left + SmallFontOffsetUC[text[i] - 33][0];
            if (dest->Blt(&Dest, lpSmallUCFont, &Source, DDBLT_ASYNC | DDBLT_KEYSRCOVERRIDE, &ddbltfx) != DD_OK)
                dest->Blt(&Dest, lpSmallUCFont, &Source, DDBLT_WAIT | DDBLT_KEYSRCOVERRIDE, &ddbltfx);
            Dest.left += SmallFontOffsetUC[text[i] - 33][0];
        }
        else if (text[i] < 123 && text[i] >= 97)
        {
            Dest.right   = Dest.left + SmallFontOffsetLC[text[i] - 97][0];
            Source.left  = SmallFontOffsetLC[text[i] - 97][1];
            Source.right = Source.left + SmallFontOffsetLC[text[i] - 97][0];
            if (dest->Blt(&Dest, lpSmallLCFont, &Source, DDBLT_ASYNC | DDBLT_KEYSRCOVERRIDE, &ddbltfx) != DD_OK)
                dest->Blt(&Dest, lpSmallLCFont, &Source, DDBLT_WAIT | DDBLT_KEYSRCOVERRIDE, &ddbltfx);
            Dest.left += SmallFontOffsetLC[text[i] - 97][0];
        }
        if (text[i] == ' ')
            Dest.left += 6;
    }
}

// -----------------------------------------------------------------------
// DrawTexture: blit a region of texture onto lpDialogSurf.
// -----------------------------------------------------------------------
void DialogBase::DrawTexture(int x, int y, int width, int height,
                             LPDIRECTDRAWSURFACE texture, int texturePosX, int texturePosY)
{
    RECT Dest   = { x,           y,            x + width,          y + height          };
    RECT Source = { texturePosX, texturePosY,  texturePosX + width, texturePosY + height };
    if (lpDialogSurf->Blt(&Dest, texture, &Source, DDBLT_ASYNC, NULL) != DD_OK)
        lpDialogSurf->Blt(&Dest, texture, &Source, DDBLT_WAIT, NULL);
}

// -----------------------------------------------------------------------
// DrawTextField: render a text string into a locked region of lpDialogSurf.
// -----------------------------------------------------------------------
int DialogBase::DrawTextField(int pX, int pY, int width, int height,
                              const std::string& text, char color)
{
    DDSURFACEDESC ddsd;
    DDRAW_INIT_STRUCT(ddsd);
    if (lpDialogSurf->Lock(NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL) == DD_OK)
    {
        SurfaceMemory = ddsd.lpSurface;
        lPitch        = ddsd.lPitch;

        FillRect(pX, pY, pX + width, pY + height, 0);
        pY += 7;

        int  CharsPerLine = (width - 4) / 8;
        char Line[100];
        Line[0] = '\0';
        char LineNum  = 0;
        char LinePos  = 0;
        bool WasLineBreak = false;

        for (size_t i = 0; i < text.size(); i++)
        {
            if (LinePos > CharsPerLine)
            {
                DrawTinyText(Line, pX + 2, pY + LineNum * 9, color);
                LineNum++;
                LinePos  = 0;
                Line[0]  = ' ';
                Line[1]  = '\0';
                LinePos++;
                i--;
                WasLineBreak = true;
            }
            else if (text[i] != 13)
            {
                Line[LinePos]     = text[i];
                Line[LinePos + 1] = '\0';
                LinePos++;
                WasLineBreak = false;
            }
            else
            {
                DrawTinyText(Line, pX + 2, pY + LineNum * 9, color);
                if (!WasLineBreak)
                    LineNum++;
                LinePos  = 0;
                Line[0]  = '\0';
                WasLineBreak = false;
            }
        }
        DrawTinyText(Line, pX + 2, pY + LineNum * 9, color);
        lpDialogSurf->Unlock(NULL);
        return LineNum + (LinePos == CharsPerLine);
    }
    return 0;
}

// -----------------------------------------------------------------------
// DrawTinyText: 8×8 bitmap font, writes directly into SurfaceMemory.
// -----------------------------------------------------------------------
void DialogBase::DrawTinyText(char* str, int posx, int posy, char color)
{
    if (!SurfaceMemory)
        return;

    char* SurfMem = (char*)SurfaceMemory;
    for (size_t i = 0; i < strlen(str); i++)
        for (int j = 0; j < 8; j++)
            for (int k = 0; k < 8; k++)
                if (ThinFont[str[i] * 8 + j] & (1 << k))
                    SurfMem[(posx + (i * 8) + (7 - k)) + (posy + j) * lPitch] = color;
}

// -----------------------------------------------------------------------
// FillRect: fill a rectangle in SurfaceMemory with a colour byte.
// -----------------------------------------------------------------------
void DialogBase::FillRect(int x, int y, int x2, int y2, char color)
{
    if (!SurfaceMemory)
        return;

    char* SurfMem = (char*)SurfaceMemory;
    for (int i = y; i < y2; i++)
        memset(&SurfMem[x + i * lPitch], color, x2 - x);
}
