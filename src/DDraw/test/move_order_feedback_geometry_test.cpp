#include "../MoveOrderFeedbackGeometry.h"
#include <algorithm>
#include <cassert>
#include <climits>
#include <iostream>
#include <vector>

namespace G = MoveOrderFeedbackGeometry;

struct Canvas
{
    static constexpr int width = 64, height = 48, pitch = 80, guard = 17;
    std::vector<unsigned char> memory;
    G::Surface surface;
    Canvas() : memory(guard + height * pitch + guard, 42),
        surface{memory.data() + guard, width, height, pitch, {8, 8, 56, 40}, {}}
    {}
    unsigned char At(int x, int y) const { return memory[guard + y * pitch + x]; }
    int Count(unsigned char color) const
    {
        return static_cast<int>(std::count(memory.begin(), memory.end(), color));
    }
    void AssertClip() const
    {
        for (int i = 0; i < guard; ++i)
        {
            assert(memory[i] == 42);
            assert(memory[memory.size() - 1 - i] == 42);
        }
        for (int y = 0; y < height; ++y)
            for (int x = 0; x < pitch; ++x)
                if (x < surface.clip.left || x >= surface.clip.right
                    || y < surface.clip.top || y >= surface.clip.bottom
                    || G::Contains(surface.excluded, x, y)) assert(At(x, y) == 42);
    }
};

int main()
{
    {
        Canvas c;
        G::Line(c.surface, INT_MIN, 24, INT_MAX, 24, 234);
        for (int x = 8; x < 56; ++x) assert(c.At(x, 24) == 234);
        assert(c.Count(234) == 48);
        c.AssertClip();
    }
    {
        Canvas c;
        G::Line(c.surface, INT_MIN, INT_MIN, INT_MAX, INT_MAX, 234);
        for (int i = 8; i < 40; ++i) assert(c.At(i, i) == 234);
        c.AssertClip();
    }
    {
        Canvas c;
        for (const auto& points : std::vector<std::vector<int>>{{-10,-10,-1,-1},
            {10,-100,20,-1}, {80,20,100,30}, {0,45,100,45}})
            G::Line(c.surface, points[0], points[1], points[2], points[3], 234);
        assert(c.Count(234) == 0);
        c.AssertClip();
    }
    for (int x : {12, 32, 52})
        for (int y : {12, 24, 36})
        {
            Canvas c;
            c.surface.excluded = {24, 12, 32, 20};
            G::Draw(c.surface, 32, 24, x, y, false);
            if (!G::Contains(c.surface.excluded, x, y)) assert(c.At(x, y) == 234);
            c.AssertClip();
        }
    {
        Canvas main, mega;
        G::Draw(main.surface, 10, 24, 48, 24, false);
        G::Draw(mega.surface, 10, 24, 48, 24, true);
        assert(main.At(48, 24) == 234 && mega.At(48, 24) == 234);
        assert(main.Count(234) > mega.Count(234));
        assert(main.Count(0) > 0 && mega.Count(0) > 0);
        main.AssertClip(); mega.AssertClip();
    }
    {
        Canvas c;
        G::Draw(c.surface, 32, 24, 32, 24, false);
        assert(c.At(32, 24) == 234 && c.Count(234) > 1);
        c.AssertClip();
    }
    {
        Canvas c;
        G::Draw(c.surface, -200, 24, 1000, 24, false);
        // The real off-screen endpoint gets no false marker at the viewport edge.
        assert(c.Count(234) == 48 && c.Count(0) == 0);
        c.AssertClip();
    }
    {
        Canvas c;
        c.surface.clip = SmallMapViewport::MapClip({8,8,56,40}, -16, 0, 1024, 896);
        G::Draw(c.surface, 0, 24, 48, 24, false);
        for (int x = 0; x < 24; ++x) assert(c.At(x, 24) == 42);
        assert(c.At(24, 24) == 234);
        c.AssertClip();
    }
    for (int invalid : {0, -1, 65537, INT_MAX})
    {
        Canvas c;
        c.surface.width = invalid;
        G::Draw(c.surface, INT_MIN, INT_MAX, INT_MAX, INT_MIN, false);
        assert(c.Count(234) == 0 && c.Count(0) == 0);
    }
    {
        Canvas c;
        c.surface.pitch = -1;
        G::Draw(c.surface, 10, 24, 48, 24, false);
        assert(c.Count(234) == 0 && c.Count(0) == 0);
    }
    std::cout << "Move feedback clipping and directional arrows passed\n";
}
