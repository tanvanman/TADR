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

static G::Palette TestPalette()
{
    G::Palette palette;
    palette.fill({255, 0, 255});
    palette[0] = {0, 0, 0};
    palette[234] = {0, 200, 0};
    palette[42] = {200, 40, 80};
    palette[43] = {80, 80, 80};
    palette[44] = {20, 20, 20};
    palette[45] = {1, 3, 5};
    palette[1] = {40, 168, 16}; // green over background 42
    palette[2] = {40, 8, 16};  // black over background 42
    palette[3] = {16, 176, 16};  // green over background 43
    palette[4] = {16, 16, 16};   // black over background 43
    palette[5] = {4, 164, 4};  // green over background 44
    palette[6] = {4, 4, 4};   // black over background 44
    palette[7] = {0, 160, 0};   // 80% green over black
    palette[8] = {0, 161, 1};   // rounded 80% RGB blend, not averaged palette indices
    palette[9] = {0, 1, 1};
    palette[12] = {0, 40, 0};  // 80% black over green
    return palette;
}

static void TestPaletteBlending()
{
    const auto palette = TestPalette();
    const auto blend = G::MakeBlendTable(palette);
    for (int background : {42,43,44,45})
    {
        const int green = background==42 ? 1 : background==43 ? 3 : background==44 ? 5 : 8;
        const int black = background==42 ? 2 : background==43 ? 4 : background==44 ? 6 : 9;
        assert(blend.green[background]==green && blend.black[background]==black);
        for (bool megamap : {false,true})
        {
            Canvas opaque, mixed;
            mixed.surface.blend=&blend;
            mixed.surface.excluded=opaque.surface.excluded={24,12,32,20};
            for (int y=8;y<40;++y) for (int x=8;x<56;++x)
                if (!G::Contains(mixed.surface.excluded,x,y))
                    mixed.memory[Canvas::guard+y*Canvas::pitch+x]=static_cast<unsigned char>(background);
            G::Draw(opaque.surface,10,24,48,24,megamap);
            G::Draw(mixed.surface,10,24,48,24,megamap);
            // The endpoint and shaft/arrow junction blend against the original
            // terrain once, without a black underlay or a second green pass.
            assert(mixed.At(48,24)==green);
            assert(mixed.At(megamap ? 44 : 40,24)==green);
            for (int y=0;y<Canvas::height;++y) for (int x=0;x<Canvas::pitch;++x)
            {
                const bool inside=G::Contains(mixed.surface.clip,x,y)
                    && !G::Contains(mixed.surface.excluded,x,y) && x<Canvas::width;
                const int expected=opaque.At(x,y)==234 ? green
                    : opaque.At(x,y)==0 ? black : inside ? background : 42;
                assert(mixed.At(x,y)==expected);
            }
            mixed.AssertClip();
        }
    }
    assert(blend.green[0]==7 && blend.black[234]==12);
    G::Palette nearest;
    nearest.fill({255,0,255}); nearest[0]={0,0,0};
    nearest[234]={0,200,0}; nearest[42]={200,40,80};
    nearest[10]={39,168,16}; nearest[11]={41,168,16};
    assert(G::MakeBlendTable(nearest).green[42]==10); // deterministic equal-distance tie
    nearest[10]={38,168,16};
    assert(G::MakeBlendTable(nearest).green[42]==11); // nearest color without an exact RGB blend
    // A changed live palette must produce a changed RGB lookup, never reuse
    // a numerically averaged palette index.
    auto changed=palette;
    changed[234]={200,0,200}; changed[10]={200,8,176};
    const auto replacement=G::MakeBlendTable(changed);
    assert(replacement.green[42]==10 && replacement.green[42]!=blend.green[42]);
}

static void TestOneCompositePerCommand()
{
    // Every write advances the background index once. This exposes repeated
    // outline/fill/endpoint/shaft writes regardless of the chosen color.
    G::BlendTable counter = {};
    for (int i=0;i<256;++i) counter.green[i]=counter.black[i]=static_cast<unsigned char>(i+1);
    for (bool megamap : {false,true})
        for (int dx : {-16,0,16}) for (int dy : {-12,0,12})
        {
            Canvas c;
            c.surface.blend=&counter;
            G::Draw(c.surface,32,24,32+dx,24+dy,megamap);
            assert(c.At(32+dx,24+dy)==43);
            for (unsigned char pixel : c.memory) assert(pixel==42 || pixel==43);
            c.AssertClip();
        }
    for (const auto& coordinates : std::vector<std::vector<int>>{
        {INT_MIN,24,INT_MAX,24}, {INT_MIN,INT_MIN,INT_MAX,INT_MAX},
        {INT_MAX,INT_MIN,INT_MIN,INT_MAX}, {32,24,INT_MAX,INT_MIN}})
    {
        Canvas c;
        c.surface.blend=&counter;
        c.surface.excluded={24,12,32,20};
        G::Draw(c.surface,coordinates[0],coordinates[1],coordinates[2],coordinates[3],false);
        for (unsigned char pixel : c.memory) assert(pixel==42 || pixel==43);
        c.AssertClip();
    }
}

int main()
{
    TestPaletteBlending();
    TestOneCompositePerCommand();
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
    std::cout << "Move feedback palette blending, single composites, clipping and directional arrows passed\n";
}
