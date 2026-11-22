#pragma once

#include <cstdint>
#include <cstring>

bool SmallMapCameraEnabled();

// Small-map camera fix by TAG_Venom. An undersized axis has one centred
// origin; source clipping in the companion terrain pass makes it safe.
namespace CameraBounds
{
    inline int Minimum(int mapExtent, int viewportExtent)
    {
        return mapExtent < viewportExtent ? -(viewportExtent - mapExtent) / 2 : 0;
    }
    inline int Maximum(int mapExtent, int viewportExtent)
    {
        return mapExtent > viewportExtent ? mapExtent - viewportExtent : Minimum(mapExtent, viewportExtent);
    }
    inline int Clamp(int position, int mapExtent, int viewportExtent)
    {
        const int low = Minimum(mapExtent, viewportExtent), high = Maximum(mapExtent, viewportExtent);
        return position < low ? low : (position > high ? high : position);
    }
    inline void ClampPair(int& x, int& y, int mapWidth, int mapHeight, int viewWidth, int viewHeight)
    {
        x = Clamp(x,mapWidth,viewWidth);
        y = Clamp(y,mapHeight,viewHeight);
    }

    static const std::uint8_t CurrentExpected[] = {
        0x8B,0x88,0x2B,0x42,0x01,0x00, 0x8B,0x90,0x37,0x7E,0x03,0x00,
        0x8B,0xB0,0x3B,0x7E,0x03,0x00, 0x2B,0xCA,
        0x8B,0x90,0x2F,0x42,0x01,0x00, 0x2B,0xD6,
        0x8B,0xB0,0x1F,0x43,0x01,0x00
    };
    static const std::uint8_t DesiredExpected[] = {
        0x8B,0x88,0x2B,0x42,0x01,0x00, 0x8B,0x90,0x37,0x7E,0x03,0x00,
        0x8B,0xB0,0x3B,0x7E,0x03,0x00, 0x2B,0xCA,
        0x8B,0x90,0x2F,0x42,0x01,0x00, 0x2B,0xD6,
        0x8B,0xB0,0x27,0x43,0x01,0x00
    };
    static const std::uint8_t LateXExpected[] = {
        0x8B,0x90,0x2F,0x42,0x01,0x00, 0x8B,0xB0,0x3B,0x7E,0x03,0x00,
        0x8B,0x88,0x2B,0x42,0x01,0x00, 0x8B,0xB8,0x37,0x7E,0x03,0x00,
        0x2B,0xD6, 0x8B,0xB0,0x27,0x43,0x01,0x00, 0x2B,0xCF
    };
    static_assert(sizeof(CurrentExpected) == 34 && sizeof(DesiredExpected) == 34
        && sizeof(LateXExpected) == 34, "Retail camera arithmetic signatures");

    static const std::uint8_t MinimapContinuation[] = {0x05,0xCB,0x42,0x01,0x00};
    static const std::uint8_t ReturnContinuation[] = {0x5E,0xC3,0x90,0x90,0x90};
    static const std::uint8_t ReloadContinuation[] = {0xA1,0xE8,0x1D,0x51,0x00};
    static const std::uint8_t CacheContinuation[] = {0x66,0x81,0xA0,0x81,0x42,0x01,0x00,0xF7,0xFF};

    struct Site
    {
        std::uint32_t calculation;
        std::uint32_t continuation;
        bool current;
        const std::uint8_t* expected;
        const std::uint8_t* resumeExpected;
        unsigned resumeLength;
    };

    // Replace each paired clamp, preserving its original continuation and
    // side effects. The first overwritten instruction is exactly six bytes.
    static const Site Sites[] = {
        {0x0041C3C6u, 0x0041C43Cu, true,  CurrentExpected, MinimapContinuation,5},
        {0x0041C456u, 0x0041C4B7u, false, DesiredExpected, ReturnContinuation,5},
        {0x0041C4F1u, 0x0041C5C6u, false, DesiredExpected, ReloadContinuation,5},
        {0x0041C80Du, 0x0041C8C4u, false, DesiredExpected, CacheContinuation,9},
        {0x0041C940u, 0x0041C9F8u, false, LateXExpected,   CacheContinuation,9},
        {0x0041CADDu, 0x0041CB3Bu, false, LateXExpected,   CacheContinuation,9}
    };

    inline bool Matches(const Site& site, const void* bytes)
    {
        return std::memcmp(bytes, site.expected, sizeof(CurrentExpected)) == 0;
    }
    inline bool MatchesContinuation(const Site& site, const void* bytes)
    {
        return std::memcmp(bytes,site.resumeExpected,site.resumeLength)==0;
    }

    // Save all integer registers/flags, call the paired clamp, then jump to
    // the untouched minimap/cache/return path. No original branch is relocated.
    inline void BuildTrampoline(std::uint8_t* code, std::uint32_t helper,
        std::uint32_t continuation, const std::uint8_t* replay=nullptr, unsigned replayLength=0)
    {
        const std::uint8_t prologue[] = {0x9C,0x60,0xE8}; // pushfd; pushad; call
        std::memcpy(code, prologue, sizeof(prologue));
        const auto base = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(code));
        const std::uint32_t call = helper - (base + 7), jump = continuation - (base + 14 + replayLength);
        std::memcpy(code + 3, &call, 4);
        code[7] = 0x61; code[8] = 0x9D; // popad; popfd
        if(replayLength) std::memcpy(code+9,replay,replayLength);
        code[9+replayLength] = 0xE9;
        std::memcpy(code + 10 + replayLength, &jump, 4);
    }
}
