#include "../AlliedCursorGeometry.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>
#include <vector>

using namespace AlliedCursorGeometry;

int main()
{
    Palette gray = {};
    for (int i = 0; i < 256; ++i) gray[i] = {i, i, i};
    const auto blend = MakeBlendTable(gray, 224);
    for (int i = 0; i < 256; ++i)
    {
        assert(blend[0][i] == i);
        assert(blend[8][i] == (i + 224 + 1) / 2);
    }

    constexpr int width = 48, height = 32, pitch = 64, guard = 128;
    std::vector<unsigned char> bytes(guard + height * pitch + guard, 64);
    auto* pixels = bytes.data() + guard;
    Surface surface = {pixels, width, height, pitch, {0, 0, width, height}, {}};
    Halo(surface, 16, 16, 4, 224, blend);
    assert(pixels[16 * pitch + 16] == 224);
    assert(pixels[16 * pitch + 17] == blend[7][64]);
    assert(pixels[16 * pitch + 19] == blend[3][64]);
    assert(pixels[16 * pitch + 20] == 64);
    assert(pixels[12 * pitch + 16] == 64);

    std::fill(bytes.begin(), bytes.end(), 64);
    surface.clip = {5, 3, 38, 26};
    surface.excluded = {7, 6, 18, 19};
    Halo(surface, 6, 7, 12, 224, blend);
    Halo(surface, 37, 25, 12, 224, blend);
    Halo(surface, -2, -2, 12, 224, blend);
    Halo(surface, (std::numeric_limits<int>::min)(), 0, 12, 224, blend);
    Halo(surface, (std::numeric_limits<int>::max)(), 0, 12, 224, blend);
    Halo(surface, 0, (std::numeric_limits<int>::min)(), 12, 224, blend);
    Halo(surface, 0, (std::numeric_limits<int>::max)(), 12, 224, blend);
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < pitch; ++x)
            if (x >= width || !Contains(surface.clip, x, y) || Contains(surface.excluded, x, y))
                assert(pixels[y * pitch + x] == 64);
    for (int i = 0; i < guard; ++i)
    {
        assert(bytes[i] == 64);
        assert(bytes[guard + height * pitch + i] == 64);
    }

    std::fill(bytes.begin(), bytes.end(), 64);
    surface.clip = {0, 0, width, height};
    surface.excluded = {};
    const unsigned char glyph[] = {0xA0, 0xA0}; // 101 / 010 / 100 (MSB first)
    Glyph(surface, 12, 9, glyph, 3, 3, 224, true);
    Glyph(surface, 12, 9, glyph, 3, 3, 224, false);
    assert(pixels[9 * pitch + 12] == 224);
    assert(pixels[9 * pitch + 13] == 0);
    assert(pixels[8 * pitch + 11] == 0);
    assert(pixels[7 * pitch + 11] == 64);
    Glyph(surface, -2, -2, glyph, 3, 3, 224, true);
    Glyph(surface, width - 1, height - 1, glyph, 3, 3, 224, false);
    surface.excluded = {0, 0, width, height};
    const auto before = bytes;
    Halo(surface, 16, 16, 12, 224, blend);
    Glyph(surface, 12, 9, glyph, 3, 3, 224, false);
    assert(bytes == before);
    surface.excluded = {};
    surface.pitch = -pitch;
    assert(!Valid(surface));
    Halo(surface, 16, 16, 12, 224, blend);
    Glyph(surface, 12, 9, glyph, 3, 3, 224, false);
    assert(bytes == before);
    std::cout << "Allied cursor palette blending, halo, glyph, clipping and stride tests passed\n";
}
