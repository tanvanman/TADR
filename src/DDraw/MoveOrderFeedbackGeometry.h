#pragma once

#include "SmallMapViewport.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

// Presentation-only raster helpers. All rectangles are half-open, and every
// line is clipped before rasterization so distant units cannot create long
// off-screen loops. Shared with the portable regression tests.
namespace MoveOrderFeedbackGeometry
{
    using SmallMapViewport::Rect;
    struct Color { int red, green, blue; };
    using Palette = std::array<Color, 256>;
    struct BlendTable
    {
        std::array<unsigned char, 256> green, black;
    };
    inline BlendTable MakeBlendTable(const Palette& palette)
    {
        BlendTable table = {};
        for (int layer = 0; layer < 2; ++layer)
        {
            const Color target = palette[layer == 0 ? 234 : 0];
            auto& row = layer == 0 ? table.green : table.black;
            for (int background = 0; background < 256; ++background)
            {
                const Color base = palette[background];
                const int red = (base.red + 4 * target.red + 2) / 5;
                const int green = (base.green + 4 * target.green + 2) / 5;
                const int blue = (base.blue + 4 * target.blue + 2) / 5;
                int best = (std::numeric_limits<int>::max)(), chosen = 0;
                for (int index = 0; index < 256; ++index)
                {
                    const int r = palette[index].red - red, g = palette[index].green - green,
                        b = palette[index].blue - blue;
                    const int distance = r * r + g * g + b * b;
                    if (distance < best) { best = distance; chosen = index; }
                }
                row[background] = static_cast<unsigned char>(chosen);
            }
        }
        return table;
    }
    struct Surface
    {
        unsigned char* pixels;
        int width, height, pitch;
        Rect clip, excluded;
        const BlendTable* blend = nullptr;
    };
    inline bool Valid(const Surface& s)
    {
        return s.pixels && s.width > 0 && s.height > 0 && s.width <= 65536 && s.height <= 65536
            && s.pitch >= s.width && static_cast<std::uint64_t>(s.pitch) * s.height
                <= (std::numeric_limits<std::size_t>::max)();
    }
    inline bool Contains(Rect r, int x, int y)
    {
        return x >= r.left && x < r.right && y >= r.top && y < r.bottom;
    }
    inline void Pixel(const Surface& s, int x, int y, unsigned char color)
    {
        if (x >= 0 && y >= 0 && x < s.width && y < s.height
            && Contains(s.clip, x, y) && !Contains(s.excluded, x, y))
        {
            auto& pixel = s.pixels[static_cast<std::size_t>(y) * s.pitch + x];
            if (s.blend && color == 234) pixel = s.blend->green[pixel];
            else if (s.blend && color == 0) pixel = s.blend->black[pixel];
            else pixel = color;
        }
    }
    inline bool Boundary(double p, double q, double& first, double& last)
    {
        if (p == 0.0) return q >= 0.0;
        const double amount = q / p;
        if (p < 0.0)
        {
            if (amount > last) return false;
            if (amount > first) first = amount;
        }
        else
        {
            if (amount < first) return false;
            if (amount < last) last = amount;
        }
        return true;
    }
    inline int Rounded(double value, int low, int high)
    {
        if (value <= low) return low;
        if (value >= high) return high;
        return static_cast<int>(std::floor(value + 0.5));
    }
    inline bool ClipLine(Rect clip, int& x0, int& y0, int& x1, int& y1)
    {
        if (SmallMapViewport::Empty(clip)) return false;
        const double startX = x0, startY = y0;
        const double dx = static_cast<double>(x1) - startX;
        const double dy = static_cast<double>(y1) - startY;
        double first = 0.0, last = 1.0;
        if (!Boundary(-dx, startX - clip.left, first, last)
            || !Boundary(dx, (clip.right - 1.0) - startX, first, last)
            || !Boundary(-dy, startY - clip.top, first, last)
            || !Boundary(dy, (clip.bottom - 1.0) - startY, first, last)) return false;
        x0 = Rounded(startX + first * dx, clip.left, clip.right - 1);
        y0 = Rounded(startY + first * dy, clip.top, clip.bottom - 1);
        x1 = Rounded(startX + last * dx, clip.left, clip.right - 1);
        y1 = Rounded(startY + last * dy, clip.top, clip.bottom - 1);
        return true;
    }
    struct Point { double x, y; };
    struct TriangleMask { Point a, b, c; };
    inline double Edge(Point a, Point b, double x, double y)
    {
        return (b.x - a.x) * (y - a.y) - (b.y - a.y) * (x - a.x);
    }
    inline bool Covers(const TriangleMask& triangle, int x, int y)
    {
        const double area = Edge(triangle.a, triangle.b, triangle.c.x, triangle.c.y);
        if (area == 0.0) return false;
        const double ab = Edge(triangle.a, triangle.b, x, y);
        const double bc = Edge(triangle.b, triangle.c, x, y);
        const double ca = Edge(triangle.c, triangle.a, x, y);
        return area > 0.0 ? (ab >= 0.0 && bc >= 0.0 && ca >= 0.0)
            : (ab <= 0.0 && bc <= 0.0 && ca <= 0.0);
    }
    inline void Line(const Surface& s, int x0, int y0, int x1, int y1, unsigned char color,
        const TriangleMask* skip = nullptr)
    {
        if (!Valid(s)) return;
        const Rect clip = SmallMapViewport::Intersect(s.clip, {0, 0, s.width, s.height});
        if (!ClipLine(clip, x0, y0, x1, y1)) return;
        // Only the few shaft pixels near the arrow need edge tests. Keep the
        // bounds in double so an extreme off-screen endpoint cannot overflow.
        const double skipLeft = skip ? (std::min)(skip->a.x, (std::min)(skip->b.x, skip->c.x)) : 0.0;
        const double skipRight = skip ? (std::max)(skip->a.x, (std::max)(skip->b.x, skip->c.x)) : 0.0;
        const double skipTop = skip ? (std::min)(skip->a.y, (std::min)(skip->b.y, skip->c.y)) : 0.0;
        const double skipBottom = skip ? (std::max)(skip->a.y, (std::max)(skip->b.y, skip->c.y)) : 0.0;
        const std::int64_t dx = x1 >= x0 ? x1 - x0 : x0 - x1;
        const std::int64_t dy = -(y1 >= y0 ? y1 - y0 : y0 - y1);
        const int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
        std::int64_t error = dx + dy;
        for (;;)
        {
            if (!skip || x0 < skipLeft || x0 > skipRight || y0 < skipTop || y0 > skipBottom
                || !Covers(*skip, x0, y0)) Pixel(s, x0, y0, color);
            if (x0 == x1 && y0 == y1) break;
            const std::int64_t twice = error * 2;
            if (twice >= dy) { error += dy; x0 += sx; }
            if (twice <= dx) { error += dx; y0 += sy; }
        }
    }
    inline void Triangle(const Surface& s, Point a, Point b, Point c, unsigned char color,
        const TriangleMask* fill = nullptr)
    {
        if (!Valid(s)) return;
        const Rect clip = SmallMapViewport::Intersect(s.clip, {0, 0, s.width, s.height});
        if (SmallMapViewport::Empty(clip)) return;
        const double minX = (std::min)(a.x, (std::min)(b.x, c.x));
        const double maxX = (std::max)(a.x, (std::max)(b.x, c.x));
        const double minY = (std::min)(a.y, (std::min)(b.y, c.y));
        const double maxY = (std::max)(a.y, (std::max)(b.y, c.y));
        if (maxX < clip.left || minX >= clip.right || maxY < clip.top || minY >= clip.bottom) return;
        const int left = Rounded(std::floor(minX), clip.left, clip.right - 1);
        const int right = Rounded(std::ceil(maxX), clip.left, clip.right - 1);
        const int top = Rounded(std::floor(minY), clip.top, clip.bottom - 1);
        const int bottom = Rounded(std::ceil(maxY), clip.top, clip.bottom - 1);
        const TriangleMask outer = {a, b, c};
        if (Edge(a, b, c.x, c.y) == 0.0) return;
        for (int y = top; y <= bottom; ++y)
            for (int x = left; x <= right; ++x)
            {
                if (Covers(outer, x, y))
                    Pixel(s, x, y, fill && Covers(*fill, x, y) ? 234 : color);
            }
    }
    inline void Draw(const Surface& s, int x0, int y0, int x1, int y1, bool megamap)
    {
        if (!Valid(s)) return;
        constexpr unsigned char green = 234;
        double dx = static_cast<double>(x1) - x0, dy = static_cast<double>(y1) - y0;
        const double length = std::sqrt(dx * dx + dy * dy);
        if (length > 0.0) { dx /= length; dy /= length; }
        else { dx = 0.0; dy = -1.0; }
        const double size = megamap ? 4.0 : 8.0, width = megamap ? 2.5 : 4.5;
        // A black outline keeps the green destination readable on light terrain.
        const Point outerTip = {x1 + dx, y1 + dy};
        const Point outerLeft = {x1 - dx * (size + 1) - dy * (width + 1),
            y1 - dy * (size + 1) + dx * (width + 1)};
        const Point outerRight = {x1 - dx * (size + 1) + dy * (width + 1),
            y1 - dy * (size + 1) - dx * (width + 1)};
        const Point tip = {static_cast<double>(x1), static_cast<double>(y1)};
        const Point left = {x1 - dx * size - dy * width, y1 - dy * size + dx * width};
        const Point right = {x1 - dx * size + dy * width, y1 - dy * size - dx * width};
        const TriangleMask outer = {outerTip, outerLeft, outerRight};
        const TriangleMask inner = {tip, left, right};
        // Every pixel belongs to the shaft or the arrow, never both. The
        // outline/fill share one raster pass so 80% opacity is not compounded.
        Line(s, x0, y0, x1, y1, green, &outer);
        Triangle(s, outerTip, outerLeft, outerRight, 0, &inner);
    }
}
