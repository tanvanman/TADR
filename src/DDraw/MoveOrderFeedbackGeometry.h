#pragma once

#include "SmallMapViewport.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

// Presentation-only raster helpers. All rectangles are half-open, and every
// line is clipped before rasterization so distant units cannot create long
// off-screen loops. Shared with the portable regression tests.
namespace MoveOrderFeedbackGeometry
{
    using SmallMapViewport::Rect;
    struct Surface
    {
        unsigned char* pixels;
        int width, height, pitch;
        Rect clip, excluded;
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
            s.pixels[static_cast<std::size_t>(y) * s.pitch + x] = color;
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
    inline void Line(const Surface& s, int x0, int y0, int x1, int y1, unsigned char color)
    {
        if (!Valid(s)) return;
        const Rect clip = SmallMapViewport::Intersect(s.clip, {0, 0, s.width, s.height});
        if (!ClipLine(clip, x0, y0, x1, y1)) return;
        const std::int64_t dx = x1 >= x0 ? x1 - x0 : x0 - x1;
        const std::int64_t dy = -(y1 >= y0 ? y1 - y0 : y0 - y1);
        const int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
        std::int64_t error = dx + dy;
        for (;;)
        {
            Pixel(s, x0, y0, color);
            if (x0 == x1 && y0 == y1) break;
            const std::int64_t twice = error * 2;
            if (twice >= dy) { error += dy; x0 += sx; }
            if (twice <= dx) { error += dx; y0 += sy; }
        }
    }
    struct Point { double x, y; };
    inline double Edge(Point a, Point b, double x, double y)
    {
        return (b.x - a.x) * (y - a.y) - (b.y - a.y) * (x - a.x);
    }
    inline void Triangle(const Surface& s, Point a, Point b, Point c, unsigned char color)
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
        const double area = Edge(a, b, c.x, c.y);
        if (area == 0.0) return;
        for (int y = top; y <= bottom; ++y)
            for (int x = left; x <= right; ++x)
            {
                const double ab = Edge(a, b, x, y), bc = Edge(b, c, x, y), ca = Edge(c, a, x, y);
                if (area > 0.0 ? (ab >= 0.0 && bc >= 0.0 && ca >= 0.0)
                    : (ab <= 0.0 && bc <= 0.0 && ca <= 0.0)) Pixel(s, x, y, color);
            }
    }
    inline void Draw(const Surface& s, int x0, int y0, int x1, int y1, bool megamap)
    {
        if (!Valid(s)) return;
        constexpr unsigned char green = 234;
        Line(s, x0, y0, x1, y1, green);
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
        Triangle(s, outerTip, outerLeft, outerRight, 0);
        const Point tip = {static_cast<double>(x1), static_cast<double>(y1)};
        const Point left = {x1 - dx * size - dy * width, y1 - dy * size + dx * width};
        const Point right = {x1 - dx * size + dy * width, y1 - dy * size - dx * width};
        Triangle(s, tip, left, right, green);
        Pixel(s, x1, y1, green);
    }
}
