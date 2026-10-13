#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>

// Portable pixel/geometry helpers shared by the production hooks and tests.
// Rectangles here are half-open; TA's OFFSCREEN rectangles are inclusive.
namespace SmallMapViewport
{
    struct Rect { int left, top, right, bottom; };
    struct Surface { std::uint8_t* bits; int width, height, pitch; Rect clip; };
    struct Terrain {
        const std::uint16_t* ids;
        const std::uint8_t* pixels;
        int columns, rows;
        std::uint32_t count;
        int mapWidth, mapHeight;
    };
    inline int Low(int a, int b) { return a < b ? a : b; }
    inline int High(int a, int b) { return a > b ? a : b; }
    inline Rect Intersect(Rect a, Rect b) {
        return {High(a.left,b.left), High(a.top,b.top), Low(a.right,b.right), Low(a.bottom,b.bottom)};
    }
    inline bool Empty(Rect r) { return r.left >= r.right || r.top >= r.bottom; }
    inline bool Small(int mapWidth, int mapHeight, int viewWidth, int viewHeight) {
        return mapWidth > 0 && mapHeight > 0 && (mapWidth < viewWidth || mapHeight < viewHeight);
    }
    // Match the retail fog builder's signed division/remainder exactly.
    inline int FogOrigin(int camera) { return camera / 32 - (camera % 32 < 16 ? 1 : 0); }
    inline std::uint8_t ExtendFogMask(std::uint8_t mask, bool left, bool right, bool top, bool bottom) {
        // Bits 1/2/4/8 describe top-left/top-right/bottom-left/bottom-right.
        // Extend the existing edge sample into the nonexistent corners only.
        if (left) mask = static_cast<std::uint8_t>((mask & ~5u) | ((mask & 10u) >> 1));
        if (right) mask = static_cast<std::uint8_t>((mask & ~10u) | ((mask & 5u) << 1));
        if (top) mask = static_cast<std::uint8_t>((mask & ~3u) | ((mask & 12u) >> 2));
        if (bottom) mask = static_cast<std::uint8_t>((mask & ~12u) | ((mask & 3u) << 2));
        return mask;
    }
    inline void ExtendFogEdges(std::uint8_t* cells, int columns, int rows, int count,
        int cameraX, int cameraY, int mapColumns, int mapRows) {
        if (!cells || columns <= 0 || rows <= 0 || mapColumns <= 0 || mapRows <= 0
            || count < 0 || static_cast<std::int64_t>(columns)*rows > count) return;
        const int baseX = FogOrigin(cameraX), baseY = FogOrigin(cameraY);
        for (int y=0; y<rows; ++y) {
            const bool top = static_cast<std::int64_t>(baseY)+y == -1;
            const bool bottom = static_cast<std::int64_t>(baseY)+y == mapRows-1;
            for (int x=0; x<columns; ++x) {
                const bool left = static_cast<std::int64_t>(baseX)+x == -1;
                const bool right = static_cast<std::int64_t>(baseX)+x == mapColumns-1;
                if (!left && !right && !top && !bottom) continue;
                auto* cell = cells + (static_cast<std::size_t>(y)*columns+x)*2;
                cell[0] = ExtendFogMask(cell[0],left,right,top,bottom);
                cell[1] = ExtendFogMask(cell[1],left,right,top,bottom);
            }
        }
    }
    inline bool ContainsWorld(int x, int y, int mapWidth, int mapHeight) {
        return x >= 0 && y >= 0 && x < mapWidth && y < mapHeight;
    }
    inline std::uint8_t PointerFlags(std::uint8_t flags, int x, int y, int mapWidth, int mapHeight) {
        return (flags & 2) && !ContainsWorld(x,y,mapWidth,mapHeight)
            ? static_cast<std::uint8_t>(flags & ~6u) : flags;
    }
    inline bool ContainsScreen(Rect view, int cameraX, int cameraY,
        int mapWidth, int mapHeight, int x, int y) {
        const std::int64_t wx = static_cast<std::int64_t>(x) - view.left + cameraX;
        const std::int64_t wy = static_cast<std::int64_t>(y) - view.top + cameraY;
        return x >= view.left && x < view.right && y >= view.top && y < view.bottom
            && wx >= 0 && wy >= 0 && wx < mapWidth && wy < mapHeight;
    }
    inline bool Valid(Surface s) { return s.bits && s.width > 0 && s.height > 0 && s.pitch >= s.width; }
    inline Rect Clip(Surface s, Rect view) {
        return Intersect(Intersect(view,s.clip),{0,0,s.width,s.height});
    }
    inline int Bounded(std::int64_t value, int low, int high) {
        return value < low ? low : (value > high ? high : static_cast<int>(value));
    }
    inline Rect MapClip(Rect view, int cameraX, int cameraY, int mapWidth, int mapHeight) {
        const int left=Bounded(static_cast<std::int64_t>(view.left)-cameraX,view.left,view.right);
        const int top=Bounded(static_cast<std::int64_t>(view.top)-cameraY,view.top,view.bottom);
        return {left,top,Bounded(static_cast<std::int64_t>(view.left)-cameraX+mapWidth,left,view.right),
            Bounded(static_cast<std::int64_t>(view.top)-cameraY+mapHeight,top,view.bottom)};
    }
    inline void ClearMargins(Surface s, Rect view, int cameraX, int cameraY, int mapWidth, int mapHeight) {
        if (!Valid(s)) return;
        Rect clip = Clip(s,view);
        if (Empty(clip)) return;
        const int left = Bounded(static_cast<std::int64_t>(view.left)-cameraX,clip.left,clip.right);
        const int right = Bounded(static_cast<std::int64_t>(view.left)-cameraX+mapWidth,left,clip.right);
        for (int y=clip.top; y<clip.bottom; ++y) {
            auto* row = s.bits + static_cast<std::size_t>(y)*s.pitch;
            const std::int64_t worldY = static_cast<std::int64_t>(y)-view.top+cameraY;
            if (worldY < 0 || worldY >= mapHeight) std::memset(row+clip.left,0,clip.right-clip.left);
            else {
                std::memset(row+clip.left,0,left-clip.left);
                std::memset(row+right,0,clip.right-right);
            }
        }
    }
    inline void DrawTerrain(Surface s, Rect view, int cameraX, int cameraY, Terrain terrain) {
        if (!Valid(s)) return;
        const Rect clip = Clip(s,view);
        if (Empty(clip)) return;
        for (int y=clip.top; y<clip.bottom; ++y)
            std::memset(s.bits+static_cast<std::size_t>(y)*s.pitch+clip.left,0,clip.right-clip.left);
        if (!terrain.ids || !terrain.pixels || terrain.columns <= 0 || terrain.rows <= 0) return;
        const int firstX = Bounded(static_cast<std::int64_t>(cameraX)+clip.left-view.left,0,terrain.mapWidth);
        const int lastX = Bounded(static_cast<std::int64_t>(cameraX)+clip.right-view.left,0,terrain.mapWidth);
        if (firstX >= lastX) return;
        for (int y=clip.top; y<clip.bottom; ++y) {
            const std::int64_t worldY = static_cast<std::int64_t>(y)+cameraY-view.top;
            if (worldY < 0 || worldY >= terrain.mapHeight || worldY/32 >= terrain.rows) continue;
            const auto rowIndex = static_cast<std::size_t>(worldY/32)*terrain.columns;
            for (int x=firstX; x<lastX;) {
                const int length = Low(32-x%32,lastX-x);
                if (x/32 < terrain.columns) {
                    const auto id = terrain.ids[rowIndex+x/32];
                    if (id < terrain.count) {
                        const auto* src = terrain.pixels+static_cast<std::size_t>(id)*1024
                            +static_cast<std::size_t>(worldY%32)*32+x%32;
                        const int screenX = static_cast<int>(static_cast<std::int64_t>(view.left)+x-cameraX);
                        std::memcpy(s.bits+static_cast<std::size_t>(y)*s.pitch+screenX,src,length);
                    }
                }
                x += length;
            }
        }
    }
    inline void MinimapAxis(int& first, int& last, int origin, int pictureSpan,
        int camera, int viewportSpan, int mapSpan) {
        if(pictureSpan<=0 || mapSpan<=0) return;
        const int begin=Bounded(camera,0,mapSpan);
        const int end=Bounded(static_cast<std::int64_t>(camera)+viewportSpan,begin,mapSpan);
        first=origin+static_cast<int>(static_cast<std::int64_t>(begin)*pictureSpan/mapSpan);
        last=origin+static_cast<int>((static_cast<std::int64_t>(end)*pictureSpan+mapSpan-1)/mapSpan)-1;
        first=Bounded(first,origin,origin+pictureSpan-1);
        last=Bounded(last,first,origin+pictureSpan-1);
    }
}
