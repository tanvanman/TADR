#pragma once

#include "SmallMapViewport.h"
#include <array>
#include <cstdint>
#include <limits>

// Indexed-color raster operations shared by production and the portable test.
namespace AlliedCursorGeometry
{
    using SmallMapViewport::Rect;
    struct Color { int red, green, blue; };
    using Palette = std::array<Color, 256>;
    using BlendTable = std::array<std::array<std::uint8_t, 256>, 9>;
    struct Surface
    {
        std::uint8_t* pixels;
        int width, height, pitch;
        Rect clip, excluded;
    };
    inline bool Contains(Rect r, int x, int y)
    {
        return x >= r.left && x < r.right && y >= r.top && y < r.bottom;
    }
    inline bool Valid(const Surface& s)
    {
        return s.pixels && s.width > 0 && s.height > 0 && s.pitch >= s.width
            && static_cast<std::uint64_t>(s.pitch) * s.height <= (std::numeric_limits<std::size_t>::max)();
    }
    inline bool Writable(const Surface& s, int x, int y)
    {
        return x >= 0 && y >= 0 && x < s.width && y < s.height
            && Contains(s.clip, x, y) && !Contains(s.excluded, x, y);
    }
    inline void Pixel(const Surface& s, int x, int y, std::uint8_t color)
    {
        if (Writable(s, x, y)) s.pixels[static_cast<std::size_t>(y) * s.pitch + x] = color;
    }
    inline BlendTable MakeBlendTable(const Palette& palette, unsigned char color)
    {
        BlendTable table = {};
        const auto target = palette[color];
        for (int alpha = 0; alpha <= 8; ++alpha)
            for (int background = 0; background < 256; ++background)
            {
                if (alpha == 0) { table[alpha][background] = static_cast<unsigned char>(background); continue; }
                const auto base = palette[background];
                const int red = (base.red * (16 - alpha) + target.red * alpha + 8) / 16;
                const int green = (base.green * (16 - alpha) + target.green * alpha + 8) / 16;
                const int blue = (base.blue * (16 - alpha) + target.blue * alpha + 8) / 16;
                int best = (std::numeric_limits<int>::max)(), chosen = 0;
                for (int index = 0; index < 256; ++index)
                {
                    const int r = palette[index].red - red, g = palette[index].green - green,
                        b = palette[index].blue - blue;
                    const int distance = r * r + g * g + b * b;
                    if (distance < best) { best = distance; chosen = index; }
                }
                table[alpha][background] = static_cast<unsigned char>(chosen);
            }
        return table;
    }
    inline void Halo(const Surface& s, int x, int y, int radius,
        unsigned char color, const BlendTable& table)
    {
        if (!Valid(s) || radius <= 0 || radius > 64) return;
        // Intersect first, so even wildly off-screen network coordinates cannot
        // overflow x +/- radius or cause unbounded raster loops.
        const auto bounds = SmallMapViewport::Intersect(s.clip, {0, 0, s.width, s.height});
        if (SmallMapViewport::Empty(bounds) || static_cast<std::int64_t>(x) + radius < bounds.left
            || static_cast<std::int64_t>(y) + radius < bounds.top
            || static_cast<std::int64_t>(x) - radius >= bounds.right
            || static_cast<std::int64_t>(y) - radius >= bounds.bottom) return;
        const int left = SmallMapViewport::Bounded(static_cast<std::int64_t>(x) - radius, bounds.left, bounds.right);
        const int top = SmallMapViewport::Bounded(static_cast<std::int64_t>(y) - radius, bounds.top, bounds.bottom);
        const int right = SmallMapViewport::Bounded(static_cast<std::int64_t>(x) + radius + 1, bounds.left, bounds.right);
        const int bottom = SmallMapViewport::Bounded(static_cast<std::int64_t>(y) + radius + 1, bounds.top, bounds.bottom);
        const int radiusSquared = radius * radius;
        for (int row = top; row < bottom; ++row)
            for (int col = left; col < right; ++col)
            {
                if (!Writable(s, col, row)) continue;
                const int dx = col - x, dy = row - y, distance = dx * dx + dy * dy;
                if (distance >= radiusSquared) continue;
                auto& pixel = s.pixels[static_cast<std::size_t>(row) * s.pitch + col];
                const int alpha = (radiusSquared - distance) * 8 / radiusSquared;
                pixel = distance <= (radius >= 8 ? 2 : 0) ? color : table[alpha][pixel];
            }
    }
    inline void Glyph(const Surface& s, int x, int y, const unsigned char* bits,
        int width, int height, unsigned char color, bool outline)
    {
        if (!Valid(s) || !bits || width <= 0 || width > 64 || height <= 0 || height > 32) return;
        for (int row = 0; row < height; ++row)
            for (int col = 0; col < width; ++col)
            {
                const int bit = row * width + col;
                if (!(bits[bit / 8] & (128 >> (bit % 8)))) continue;
                if (outline)
                    for (int dy = -1; dy <= 1; ++dy)
                        for (int dx = -1; dx <= 1; ++dx) Pixel(s, x + col + dx, y + row + dy, 0);
                else Pixel(s, x + col, y + row, color);
            }
    }
}
