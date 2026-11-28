#include "config.h"
#include "AlliedCursorDraw.h"

#if ALLIED_CURSORS_ENABLE

#include "oddraw.h"
#include "iddrawsurface.h"
#include "tamem.h"
#include "tafunctions.h"
#include "dialog.h"
#include "cincome.h"
#include "font.h"
#include "AlliedCursorGeometry.h"
#if USEMEGAMAP
#include "GUIExpand.h"
#include "fullscreenminimap.h"
#include "MegamapControl.h"
#endif
#include <algorithm>
#include <array>
#include <mutex>

extern PALETTEENTRY TAPalette[];

namespace
{
    namespace Geometry = AlliedCursorGeometry;
    using Geometry::Rect;

#if USEMEGAMAP
    MegaMapControl* Megamap()
    {
        return GUIExpander && GUIExpander->myMinimap && GUIExpander->myMinimap->Controler
            && GUIExpander->myMinimap->Controler->IsBliting()
            ? GUIExpander->myMinimap->Controler : nullptr;
    }
#endif

    Rect PopupRect(const TAdynmemStruct& ta)
    {
        const auto* gui = ta.desktopGUI.TheActive_GUIMEM;
        if (!gui || !gui->per_active || !gui->ControlsAry) return {};
        const auto& panel = *gui->ControlsAry;
        return {panel.xpos, panel.ypos, panel.xpos + panel.width, panel.ypos + panel.height};
    }

    bool OverOverlay(const TAdynmemStruct& ta, int x, int y)
    {
        if (Geometry::Contains(PopupRect(ta), x, y)) return true;
        if (!LocalShare) return false;
        RECT rect = {};
        auto* dialog = static_cast<Dialog*>(LocalShare->Dialog);
        if (dialog && dialog->IsShow(&rect)
            && x >= rect.left && x < rect.right && y >= rect.top && y < rect.bottom) return true;
        auto* income = static_cast<CIncome*>(LocalShare->Income);
        return income && income->IsPointOver(x, y);
    }

    Geometry::BlendTable Blends(unsigned char color)
    {
        struct Cache
        {
            std::array<Geometry::BlendTable, 256> tables = {};
            std::array<bool, 256> ready = {};
            Geometry::Palette palette = {};
            std::mutex mutex;
        };
        static Cache cache;
        std::lock_guard<std::mutex> lock(cache.mutex);
        Geometry::Palette palette = {};
        bool changed = false;
        for (int i = 0; i < 256; ++i)
        {
            palette[i] = {TAPalette[i].peRed, TAPalette[i].peGreen, TAPalette[i].peBlue};
            changed = changed || palette[i].red != cache.palette[i].red
                || palette[i].green != cache.palette[i].green || palette[i].blue != cache.palette[i].blue;
        }
        if (changed) { cache.ready.fill(false); cache.palette = palette; }
        if (!cache.ready[color])
        {
            cache.tables[color] = Geometry::MakeBlendTable(palette, color);
            cache.ready[color] = true;
        }
        // Return a snapshot so a palette refresh on another presentation
        // thread cannot mutate a table while its caller rasterizes a halo.
        return cache.tables[color];
    }

    struct Font
    {
        const unsigned char* data = nullptr;
        int height = 8;
    };

    Font LabelFont(const TAdynmemStruct& ta, bool megamap)
    {
        const auto* font = static_cast<const unsigned char*>(megamap ? ta.smalFontHandle : ta.COMIXFontHandle);
        if (font && font[0] > 0 && font[0] <= 32) return {font, font[0]};
        return {};
    }

    const unsigned char* Glyph(const Font& font, unsigned char code, int& width)
    {
        width = 8;
        if (!font.data) return reinterpret_cast<const unsigned char*>(ThinFont) + code * 8;
        if (code < font.data[3]) { width = 0; return nullptr; }
        const unsigned index = 4 + (code - font.data[3]) * 2;
        const unsigned offset = font.data[index] | (font.data[index + 1] << 8);
        if (!offset || font.data[offset] > 64) { width = 0; return nullptr; }
        width = font.data[offset];
        return font.data + offset + 1;
    }

    void DrawNameLabel(const Geometry::Surface& surface, const Font& font, int centerX, int centerY,
        int radius, const char (&name)[32], unsigned char color)
    {
        int count = 0, width = 0;
        const int available = surface.clip.right - surface.clip.left - 4;
        for (; count < 31 && name[count]; ++count)
        {
            int advance = 0;
            Glyph(font, static_cast<unsigned char>(name[count]), advance);
            if (width + advance > available) break;
            width += advance;
        }
        if (!count || width <= 0 || surface.clip.bottom - surface.clip.top < font.height + 2) return;
        // Keep names readable at all map edges without painting the interface.
        const int x = (std::max)(surface.clip.left + 1,
            (std::min)(centerX - width / 2, surface.clip.right - width - 1));
        int y = centerY + radius + 3;
        if (y + font.height + 1 > surface.clip.bottom) y = centerY - radius - font.height - 3;
        y = (std::max)(surface.clip.top + 1, (std::min)(y, surface.clip.bottom - font.height - 1));
        for (int pass = 0; pass < 2; ++pass)
        {
            int pen = x;
            for (int i = 0; i < count; ++i)
            {
                int advance = 0;
                const auto* bits = Glyph(font, static_cast<unsigned char>(name[i]), advance);
                Geometry::Glyph(surface, pen, y, bits, advance, font.height, color, pass == 0);
                pen += advance;
            }
        }
    }
}

namespace AlliedCursorDraw
{
    bool Sample(TAdynmemStruct* ta, Position_Dword& position)
    {
        position = {};
        if (!ta || !DataShare || DataShare->TAProgress != TAInGame) return false;
        const auto* program = *TAProgramStruct_PtrPtr;
        const HWND window = program ? program->TAClass_Hwnd : nullptr;
        if (!window || !IsWindowVisible(window) || IsIconic(window) || GetForegroundWindow() != window) return false;
        POINT cursor = {};
        if (!GetCursorPos(&cursor) || !ScreenToClient(window, &cursor) || OverOverlay(*ta, cursor.x, cursor.y)) return false;
        if (ta->FeatureMapSizeX <= 2 || ta->FeatureMapSizeY <= 8) return false;
#if USEMEGAMAP
        if (auto* megamap = Megamap()) return megamap->CursorGroundPoint(&position, cursor.x, cursor.y);
#endif
        const RECT& rect = ta->GameSreen_Rect;
        if (!SmallMapViewport::ContainsScreen({rect.left, rect.top, rect.right + 1, rect.bottom + 1},
            ta->EyeBallMapXPos, ta->EyeBallMapYPos, ta->MapSizeX, ta->MapSizeY, cursor.x, cursor.y)) return false;
        // Resample against today's camera every frame, including when the
        // mouse remains still while keyboard scrolling moves the battlefield.
        GetPosition_Dword(cursor.x - rect.left + ta->EyeBallMapXPos,
            cursor.y - rect.top + ta->EyeBallMapYPos, &position);
        if (static_cast<short>(position.Z) < 0) position.Z = static_cast<unsigned short>(ta->SeaLevel);
        return position.X < ta->FeatureMapSizeX * 16 && position.Y < (ta->FeatureMapSizeY - 8) * 16;
    }

    void Draw(char* pixels, int width, int height, int pitch,
        const TAdynmemStruct& ta, const std::array<AlliedCursorView, 10>& views)
    {
        if (!DataShare || DataShare->TAProgress != TAInGame || ta.FeatureMapSizeX <= 2 || ta.FeatureMapSizeY <= 8) return;
        Geometry::Surface surface = {reinterpret_cast<unsigned char*>(pixels), width, height, pitch, {}, PopupRect(ta)};
        if (!Geometry::Valid(surface)) return;
#if USEMEGAMAP
        auto* megamap = Megamap();
        const bool mega = megamap != nullptr;
#else
        const bool mega = false;
#endif
        const auto font = LabelFont(ta, mega);
        for (const auto& view : views)
        {
            if (!view.visible || view.x < 0 || view.y < 0 || view.x >= ta.FeatureMapSizeX * 16
                || view.y >= (ta.FeatureMapSizeY - 8) * 16 || view.z < 0 || view.z > 65535
                || view.color >= 10) continue;
            const unsigned char color = static_cast<unsigned char>(DataShare->PlayerDotColors[view.color]);
            int x = 0, y = 0;
#if USEMEGAMAP
            if (megamap)
            {
                Position_Dword point = {};
                point.X = static_cast<unsigned short>(view.x);
                point.Y = static_cast<unsigned short>(view.y);
                point.Z = static_cast<unsigned short>(view.z);
                POINT screen = {};
                RECT clip = {};
                if (!megamap->CursorScreenPoint(&screen, &clip, point)) continue;
                x = screen.x; y = screen.y;
                surface.clip = {clip.left, clip.top, clip.right + 1, clip.bottom + 1};
            }
            else
#endif
            {
                const RECT& clip = ta.GameSreen_Rect;
                x = view.x - ta.EyeBallMapXPos + clip.left;
                y = view.y - ta.EyeBallMapYPos + clip.top - view.z / 2;
                surface.clip = SmallMapViewport::MapClip({clip.left, clip.top, clip.right + 1, clip.bottom + 1},
                    ta.EyeBallMapXPos, ta.EyeBallMapYPos, ta.MapSizeX, ta.MapSizeY);
            }
            surface.clip = SmallMapViewport::Intersect(surface.clip, {0, 0, width, height});
            if (!Geometry::Contains(surface.clip, x, y)) continue;
            const int radius = mega ? 4 : 12;
            Geometry::Halo(surface, x, y, radius, color, Blends(color));
            // White text with a black outline stays legible for dark player colours.
            DrawNameLabel(surface, font, x, y, radius, view.name, 255);
        }
    }
}

#else

namespace AlliedCursorDraw
{
    bool Sample(TAdynmemStruct*, _Position_Dword&) { return false; }
    void Draw(char*, int, int, int, const TAdynmemStruct&, const std::array<AlliedCursorView, 10>&) {}
}

#endif
