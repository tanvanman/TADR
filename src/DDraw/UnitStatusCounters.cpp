#include "UnitStatusCounters.h"

#include "iddrawsurface.h"
#include "hook/hook.h"
#include "TAConfig.h"
#include "tafunctions.h"

#include <cstdio>
#include <cstring>
#include <memory>

namespace
{
    const DWORD kGroupNumberDrawCallAddr = 0x00469CF9u;
    const size_t kUnitWeaponOffset = 0x04;
    const size_t kUnitWeaponStride = 0x1C;
    const size_t kWeaponDefTypeMaskOffset = 0x111;
    const size_t kOrderQueueAmountOffset = 0x3A;
    const size_t kTransporterUnitOffset = 0x86;
    const size_t kFirstTransportedUnitOffset = 0x8A;
    const size_t kPreviousTransportedUnitOffset = 0x8E;
    const unsigned int kMaxTransportWalk = 4096;

    const unsigned char kExpectedGroupNumberCall[5] = { 0xE8, 0xF2, 0x77, 0x05, 0x00 };

    bool g_installed = false;
    bool g_showStockpileCounter = true;
    bool g_showTransportCounter = true;
    unsigned char g_stockpileCounterColor = 255;
    unsigned char g_transportCounterColor = 255;
    unsigned char g_groupNumberColor = 255;
    unsigned char g_groupNumberCallPatch[5] = {};
    std::unique_ptr<SingleHook> g_groupNumberHook;
    void* g_measuredFontHandle = nullptr;
    bool g_glyphInkMetricsValid[256] = {};
    signed char g_glyphInkLeft[256] = {};
    signed char g_glyphInkRight[256] = {};

    struct UnitWeaponView
    {
        unsigned short nTargetID;
        unsigned short nUsedSpot;
        void* pAutoAimCallback;
        unsigned int lState;
        WeaponStruct* pWeapon;
        int ZAngle;
        unsigned short nReloadTime;
        unsigned short nAngle;
        unsigned short nTrajectoryResult;
        unsigned char cStock;
        unsigned char cStateMask;
    };

    static_assert(sizeof(UnitWeaponView) == 0x1C, "UnitWeaponView size mismatch");

    int ClampPaletteIndex(int value)
    {
        if (value < 0) return 0;
        if (value > 255) return 255;
        return value;
    }

    UnitWeaponView* GetUnitWeapon(UnitStruct* unit, int slot)
    {
        if (!unit || slot < 0 || slot > 2) return nullptr;
        return reinterpret_cast<UnitWeaponView*>(
            reinterpret_cast<unsigned char*>(unit)
            + kUnitWeaponOffset
            + static_cast<size_t>(slot) * kUnitWeaponStride);
    }

    WeaponStruct* GetUnitTypeWeapon(const UnitStruct* unit, int slot)
    {
        if (!unit || !unit->UnitType) return nullptr;
        switch (slot)
        {
        case 0: return unit->UnitType->weapon1;
        case 1: return unit->UnitType->weapon2;
        case 2: return unit->UnitType->weapon3;
        default: return nullptr;
        }
    }

    unsigned int GetWeaponTypeMask(const WeaponStruct* weapon)
    {
        if (!weapon) return 0;
        return *reinterpret_cast<const unsigned int*>(
            reinterpret_cast<const unsigned char*>(weapon) + kWeaponDefTypeMaskOffset);
    }

    struct StockpileStatus
    {
        unsigned int stocked;
        unsigned int queued;
        bool hasStockpileWeapon;
    };

    StockpileStatus GetStockpileStatus(UnitStruct* unit)
    {
        StockpileStatus status = {};
        for (int slot = 0; slot < 3; ++slot)
        {
            UnitWeaponView* weaponView = GetUnitWeapon(unit, slot);
            if (!weaponView) continue;

            WeaponStruct* weaponDef = GetUnitTypeWeapon(unit, slot);
            if (!weaponDef) weaponDef = weaponView->pWeapon;
            if ((GetWeaponTypeMask(weaponDef) & WTM_Stockpile) == 0) continue;

            status.hasStockpileWeapon = true;
            status.stocked += weaponView->cStock;
        }

        if (status.hasStockpileWeapon && unit && unit->BackgroundOrder)
        {
            int queued = 0;
            std::memcpy(
                &queued,
                reinterpret_cast<const unsigned char*>(unit->BackgroundOrder)
                    + kOrderQueueAmountOffset,
                sizeof(queued));
            if (queued > 0)
                status.queued = static_cast<unsigned int>(queued);
        }

        return status;
    }

    UnitStruct* ReadUnitPointer(UnitStruct* unit, size_t offset)
    {
        if (!unit) return nullptr;
        return *reinterpret_cast<UnitStruct**>(reinterpret_cast<unsigned char*>(unit) + offset);
    }

    unsigned int GetTransportedUnitCount(UnitStruct* transport)
    {
        if (!transport) return 0;

        unsigned int count = 0;
        UnitStruct* first = ReadUnitPointer(transport, kFirstTransportedUnitOffset);
        UnitStruct* current = first;

        for (unsigned int walked = 0; current && walked < kMaxTransportWalk; ++walked)
        {
            if (ReadUnitPointer(current, kTransporterUnitOffset) == transport)
                ++count;

            UnitStruct* next = ReadUnitPointer(current, kPreviousTransportedUnitOffset);
            if (!next || next == current || next == first) break;
            current = next;
        }

        return count;
    }

    int GetTextWidth(void* fontHandle, const char* text)
    {
        if (!fontHandle || !text) return 0;
        return GetTextExtent(fontHandle, text);
    }

    int GetTransparentColor(const TAProgramStruct* program, unsigned char foregroundColor)
    {
        const unsigned char currentAlpha = static_cast<unsigned char>(program->fontAlpha);
        if (currentAlpha != foregroundColor) return currentAlpha;
        return foregroundColor == 0 ? 1 : 0;
    }

    int DrawTextWithColor(
        OFFSCREEN* offscreen,
        TAProgramStruct* program,
        char* text,
        int x,
        int y,
        int length,
        unsigned char colorIndex)
    {
        const int transparentColor = GetTransparentColor(program, colorIndex);
        program->fontFrontColour = colorIndex;
        program->fontBackColour = transparentColor;
        program->fontAlpha = transparentColor;
        return DrawTextInScreen(offscreen, text, x, y, length);
    }

    int DrawReadableText(
        OFFSCREEN* offscreen,
        TAProgramStruct* program,
        char* text,
        int x,
        int y,
        int length,
        unsigned char colorIndex)
    {
        for (int dx = -1; dx <= 1; ++dx)
        {
            for (int dy = -1; dy <= 1; ++dy)
            {
                if (dx != 0 || dy != 0)
                    DrawTextWithColor(offscreen, program, text, x + dx, y + dy, length, 0);
            }
        }

        return DrawTextWithColor(offscreen, program, text, x, y, length, colorIndex);
    }

    int GetVisibleGlyphCenterOffset(TAProgramStruct* program, const char* text)
    {
        if (!program || !program->fontHandle || !text || !text[0] || text[1])
            return GetTextWidth(program ? program->fontHandle : nullptr, text) / 2;

        if (g_measuredFontHandle != program->fontHandle)
        {
            g_measuredFontHandle = program->fontHandle;
            std::memset(g_glyphInkMetricsValid, 0, sizeof(g_glyphInkMetricsValid));
        }

        const unsigned char glyph = static_cast<unsigned char>(text[0]);
        if (!g_glyphInkMetricsValid[glyph])
        {
            const int probeWidth = 64;
            const int probeHeight = 64;
            const int probeOrigin = 8;
            unsigned char pixels[probeWidth * probeHeight] = {};
            OFFSCREEN probe = {};
            probe.Width = probeWidth;
            probe.Height = probeHeight;
            probe.lPitch = probeWidth;
            probe.lpSurface = pixels;
            probe.ScreenRect.left = 0;
            probe.ScreenRect.top = 0;
            probe.ScreenRect.right = probeWidth;
            probe.ScreenRect.bottom = probeHeight;

            const int oldFrontColor = program->fontFrontColour;
            const int oldBackColor = program->fontBackColour;
            const int oldAlphaColor = program->fontAlpha;
            program->fontFrontColour = 1;
            program->fontBackColour = 0;
            program->fontAlpha = 0;
            DrawTextInScreen(&probe, const_cast<char*>(text), probeOrigin, probeOrigin, 1);
            program->fontFrontColour = oldFrontColor;
            program->fontBackColour = oldBackColor;
            program->fontAlpha = oldAlphaColor;

            int left = probeWidth;
            int right = -1;
            for (int y = 0; y < probeHeight; ++y)
            {
                for (int x = 0; x < probeWidth; ++x)
                {
                    if (pixels[y * probeWidth + x] != 0)
                    {
                        if (x < left) left = x;
                        if (x > right) right = x;
                    }
                }
            }

            if (right >= left)
            {
                g_glyphInkLeft[glyph] = static_cast<signed char>(left - probeOrigin);
                g_glyphInkRight[glyph] = static_cast<signed char>(right - probeOrigin);
            }
            else
            {
                g_glyphInkLeft[glyph] = 0;
                g_glyphInkRight[glyph] = static_cast<signed char>(
                    GetTextWidth(program->fontHandle, text) - 1);
            }
            g_glyphInkMetricsValid[glyph] = true;
        }

        const int doubledCenter =
            static_cast<int>(g_glyphInkLeft[glyph]) +
            static_cast<int>(g_glyphInkRight[glyph]);
        return doubledCenter >= 0 ? (doubledCenter + 1) / 2 : doubledCenter / 2;
    }

    void DrawCounterText(
        OFFSCREEN* offscreen,
        TAProgramStruct* program,
        char* text,
        int x,
        int y,
        unsigned char colorIndex)
    {
        if (!offscreen || !program || !text || !text[0]) return;

        const int oldFrontColor = program->fontFrontColour;
        const int oldBackColor = program->fontBackColour;
        const int oldAlphaColor = program->fontAlpha;
        DrawReadableText(offscreen, program, text, x, y, -1, colorIndex);

        program->fontFrontColour = oldFrontColor;
        program->fontBackColour = oldBackColor;
        program->fontAlpha = oldAlphaColor;
    }

    int __stdcall DrawGroupNumberWithConfiguredColor(
        OFFSCREEN* offscreen,
        char* text,
        int x,
        int y,
        int length)
    {
        TAProgramStruct* program = TAProgramStruct_PtrPtr ? *TAProgramStruct_PtrPtr : nullptr;
        if (!program)
            return DrawTextInScreen(offscreen, text, x, y, length);

        const int oldFrontColor = program->fontFrontColour;
        const int oldBackColor = program->fontBackColour;
        const int oldAlphaColor = program->fontAlpha;
        const int visibleCenterOffset = GetVisibleGlyphCenterOffset(program, text);
        const int result = DrawReadableText(
            offscreen,
            program,
            text,
            x - visibleCenterOffset,
            y,
            length,
            g_groupNumberColor);

        program->fontFrontColour = oldFrontColor;
        program->fontBackColour = oldBackColor;
        program->fontAlpha = oldAlphaColor;
        return result;
    }

    bool InstallGroupNumberColorHook()
    {
        if (std::memcmp(
                reinterpret_cast<const void*>(kGroupNumberDrawCallAddr),
                kExpectedGroupNumberCall,
                sizeof(kExpectedGroupNumberCall)) != 0)
        {
            IDDrawSurface::OutptTxt("[UnitStatusCounters] Group-number call site did not match; color hook skipped");
            return false;
        }

        g_groupNumberCallPatch[0] = 0xE8;
        const DWORD target = reinterpret_cast<DWORD>(&DrawGroupNumberWithConfiguredColor);
        const DWORD relative = target - (kGroupNumberDrawCallAddr + 5u);
        std::memcpy(&g_groupNumberCallPatch[1], &relative, sizeof(relative));

        g_groupNumberHook.reset(new SingleHook(
            kGroupNumberDrawCallAddr,
            sizeof(g_groupNumberCallPatch),
            INLINE_UNPROTECTEVINMENT,
            g_groupNumberCallPatch));
        return true;
    }
}

namespace UnitStatusCounters
{
    void Install()
    {
        if (g_installed) return;

        if (MyConfig)
        {
            g_showStockpileCounter = MyConfig->GetIniBool("ShowStockpileCounter", TRUE) != FALSE;
            g_showTransportCounter = MyConfig->GetIniBool("ShowTransportCounter", TRUE) != FALSE;
            g_stockpileCounterColor = static_cast<unsigned char>(ClampPaletteIndex(
                MyConfig->GetIniInt("StockpileCounterColor", 255)));
            g_transportCounterColor = static_cast<unsigned char>(ClampPaletteIndex(
                MyConfig->GetIniInt("TransportCounterColor", 255)));
            g_groupNumberColor = static_cast<unsigned char>(ClampPaletteIndex(
                MyConfig->GetIniInt("GroupNumberColor", 255)));

        }

        InstallGroupNumberColorHook();
        g_installed = true;
    }

    void Shutdown()
    {
        g_groupNumberHook.reset();
        g_installed = false;
    }

    void DrawForUnit(OFFSCREEN* offscreen, UnitStruct* unit, int centerX, int centerY)
    {
        if (!offscreen || !unit || !unit->UnitType) return;

        TAdynmemStruct* ta = TAmainStruct_PtrPtr ? *TAmainStruct_PtrPtr : nullptr;
        TAProgramStruct* program = TAProgramStruct_PtrPtr ? *TAProgramStruct_PtrPtr : nullptr;
        if (!ta || !program || (ta->GameOptionMask & 1) == 0) return;
        if (centerX == 0 || centerY == 0 || unit->Health <= 0) return;

        PlayerStruct* owner = unit->Owner_PlayerPtr0 ? unit->Owner_PlayerPtr0 : unit->Owner_PlayerPtr1;
        if (!owner || owner->PlayerAryIndex != ta->LocalHumanPlayer_PlayerID) return;

        char stockText[32] = {};
        char transportText[16] = {};

        if (g_showStockpileCounter)
        {
            const StockpileStatus stockpile = GetStockpileStatus(unit);
            if (stockpile.stocked > 0 || stockpile.queued > 0)
            {
                std::snprintf(
                    stockText,
                    sizeof(stockText),
                    "%u +%u",
                    stockpile.stocked,
                    stockpile.queued);
            }
        }

        const unsigned int capacity = unit->UnitType->transportcapacity;
        const bool isSingleUnitFlyingTransport =
            (unit->UnitType->UnitTypeMask_0 & canfly) != 0 && capacity == 1;
        if (g_showTransportCounter &&
            capacity > 0 &&
            !isSingleUnitFlyingTransport)
        {
            const unsigned int loaded = GetTransportedUnitCount(unit);
            if (loaded > 0)
                std::snprintf(transportText, sizeof(transportText), "%u/%u", loaded, capacity);
        }

        if (!stockText[0] && !transportText[0]) return;

        unsigned char* counterFont = program->fontHandle;
        const int fontHeight = counterFont ? counterFont[0] : 8;
        const int textY = centerY - fontHeight - 3;
        const int stockWidth = GetTextWidth(counterFont, stockText);
        const int transportWidth = GetTextWidth(counterFont, transportText);
        const int stockX = centerX - stockWidth / 2;
        const int transportX = centerX - transportWidth / 2;
        int transportY = textY;

        if (stockText[0] && transportText[0])
            transportY -= fontHeight + 1;

        if (stockText[0])
            DrawCounterText(offscreen, program, stockText, stockX, textY, g_stockpileCounterColor);
        if (transportText[0])
            DrawCounterText(offscreen, program, transportText, transportX, transportY, g_transportCounterColor);
    }
}
