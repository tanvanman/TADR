#include "ReloadBars.h"
#include "UnitStatusCounters.h"

#include "iddrawsurface.h"
#include "hook/hook.h"
#include "tamem.h"
#include "tafunctions.h"
#include "WeaponTdfHook.h"

#include <cstring>
#include <memory>
#include <string>
#include <unordered_set>

namespace
{
    const char kReloadBarKey[] = "reloadbar";

    const DWORD kDrawUnitBarsHookAddr = 0x00469CB1u;
    const DWORD kDrawUnitBarsHookLen = 6u;
    const DWORD kDrawBarAddr = 0x004BF6F0u;
    const size_t kColorsPaletteOffset = 0x0DCB;
    const size_t kOffscreenStackOffset = 0x34;

    const size_t kUnitWeaponOffset = 0x04;
    const size_t kUnitWeaponStride = 0x1C;
    const size_t kWeaponDefReloadTimeOffset = 0xE4;
    const size_t kWeaponDefTypeMaskOffset = 0x111;

    bool g_installed = false;
    std::unique_ptr<InlineSingleHook> g_drawHook;
    std::unordered_set<DWORD> g_reloadBarWeapons;
    std::unordered_set<std::string> g_reloadBarWeaponNames;

    typedef void (__stdcall *DrawBarFn)(OFFSCREEN*, RECT*, unsigned char);
    DrawBarFn const kDrawBar = reinterpret_cast<DrawBarFn>(kDrawBarAddr);

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

    UnitWeaponView* GetUnitWeapon(UnitStruct* unit, int slot)
    {
        if (!unit || slot < 0 || slot > 2) return nullptr;
        return reinterpret_cast<UnitWeaponView*>(
            reinterpret_cast<unsigned char*>(unit)
            + kUnitWeaponOffset
            + static_cast<size_t>(slot) * kUnitWeaponStride);
    }

    unsigned short GetWeaponReloadTime(const WeaponStruct* weapon)
    {
        if (!weapon) return 0;
        return *reinterpret_cast<const unsigned short*>(
            reinterpret_cast<const unsigned char*>(weapon) + kWeaponDefReloadTimeOffset);
    }

    unsigned int GetWeaponTypeMask(const WeaponStruct* weapon)
    {
        if (!weapon) return 0;
        return *reinterpret_cast<const unsigned int*>(
            reinterpret_cast<const unsigned char*>(weapon) + kWeaponDefTypeMaskOffset);
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

    unsigned short GetBestWeaponReloadTime(const UnitWeaponView* weaponView, const WeaponStruct* weaponDef)
    {
        const unsigned short defReload = GetWeaponReloadTime(weaponDef);
        if (defReload != 0) return defReload;
        return weaponView && weaponView->pWeapon ? GetWeaponReloadTime(weaponView->pWeapon) : 0;
    }

    unsigned int GetBestWeaponTypeMask(const UnitWeaponView* weaponView, const WeaponStruct* weaponDef)
    {
        const unsigned int defMask = GetWeaponTypeMask(weaponDef);
        if (defMask != 0) return defMask;
        return weaponView && weaponView->pWeapon ? GetWeaponTypeMask(weaponView->pWeapon) : 0;
    }

    bool IsTaggedWeapon(const WeaponStruct* weapon)
    {
        if (!weapon) return false;
        if (g_reloadBarWeapons.count((DWORD)weapon) != 0) return true;
        return g_reloadBarWeaponNames.count(weapon->WeaponName) != 0;
    }

    bool SelectTrackedWeapon(UnitStruct* unit, UnitWeaponView*& outWeapon, unsigned short& outMaxReload)
    {
        outWeapon = nullptr;
        outMaxReload = 0;

        for (int slot = 0; slot < 3; ++slot)
        {
            UnitWeaponView* weaponView = GetUnitWeapon(unit, slot);
            WeaponStruct* weaponDef = GetUnitTypeWeapon(unit, slot);
            if (!weaponView) continue;
            if (!IsTaggedWeapon(weaponDef) && !IsTaggedWeapon(weaponView->pWeapon)) continue;

            const unsigned short maxReload = GetBestWeaponReloadTime(weaponView, weaponDef);
            if (maxReload == 0) continue;
            if ((GetBestWeaponTypeMask(weaponView, weaponDef) & WTM_Stockpile) != 0) continue;

            if (!outWeapon || maxReload > outMaxReload)
            {
                outWeapon = weaponView;
                outMaxReload = maxReload;
            }
        }

        return outWeapon != nullptr;
    }

    void DrawBarRect(OFFSCREEN* offscreen, int left, int top, int right, int bottom, unsigned char color)
    {
        RECT rect = { left, top, right, bottom };
        kDrawBar(offscreen, &rect, color);
    }

    unsigned char GetGuiPaletteColor(const TAdynmemStruct* ta, unsigned char colorIndex)
    {
        if (!ta) return colorIndex;
        const unsigned char* colorsPalette = reinterpret_cast<const unsigned char*>(
            reinterpret_cast<const unsigned char*>(ta) + kColorsPaletteOffset);
        return colorsPalette ? colorsPalette[colorIndex] : colorIndex;
    }

    void DrawReloadBar(OFFSCREEN* offscreen, int centerX, int topY, unsigned short curReload, unsigned short maxReload)
    {
        if (maxReload == 0) return;

        if (curReload > maxReload) curReload = maxReload;

        TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
        const unsigned char backgroundColor = GetGuiPaletteColor(ta, 0);
        unsigned char fillColor = GetGuiPaletteColor(ta, 144);
        if (maxReload > 0)
        {
            const unsigned barProgress = (static_cast<unsigned>(curReload) * 100u) / maxReload;
            const unsigned shadeStep = barProgress / 15u;
            const unsigned char shadeIndex = static_cast<unsigned char>(144u - (shadeStep > 6u ? 6u : shadeStep));
            fillColor = GetGuiPaletteColor(ta, shadeIndex);
        }

        const int outerHalfWidth = 17;
        const int outerHeight = 4;
        const int innerWidth = 32;

        const int left = centerX - outerHalfWidth;
        const int right = centerX + outerHalfWidth;
        const int bottom = topY + outerHeight;

        DrawBarRect(offscreen, left, topY, right, bottom, backgroundColor);
        DrawBarRect(offscreen, left + 1, topY + 1, right - 1, bottom - 1, backgroundColor);

        const int fillWidth = static_cast<int>((static_cast<unsigned>(innerWidth) * curReload) / maxReload);
        if (fillWidth > 0)
        {
            DrawBarRect(offscreen, left + 1, topY + 1, left + 1 + fillWidth, bottom - 1, fillColor);
        }
    }

    void DrawReloadBarForUnit(OFFSCREEN* offscreen, UnitStruct* unit, int centerX, int centerY)
    {
        if (!unit) return;
        TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
        if (!ta)
            return;
        if ((ta->GameOptionMask & 1) == 0)
            return;
        if (centerX == 0 || centerY == 0)
            return;
        if (!unit->UnitType)
            return;

        if (unit->Nanoframe != 0.0f)
            return;

        PlayerStruct* owner = unit->Owner_PlayerPtr0 ? unit->Owner_PlayerPtr0 : unit->Owner_PlayerPtr1;
        if (!owner || owner->PlayerAryIndex != ta->LocalHumanPlayer_PlayerID)
            return;

        UnitWeaponView* weaponView = nullptr;
        unsigned short maxReload = 0;
        if (!SelectTrackedWeapon(unit, weaponView, maxReload))
            return;

        unsigned short curReload = 0;
        if (weaponView->nReloadTime <= maxReload)
            curReload = static_cast<unsigned short>(maxReload - weaponView->nReloadTime);

        DrawReloadBar(offscreen, centerX, centerY + 3, curReload, maxReload);

    }

    int __stdcall DrawUnitBarsHookProc(PInlineX86StackBuffer pBuf)
    {
        __try
        {
            UnitStruct* unit = reinterpret_cast<UnitStruct*>(pBuf->Edi);
            const int centerX = static_cast<int>(pBuf->Ebp);
            const int centerY = static_cast<int>(pBuf->Edx);
            OFFSCREEN* offscreen = reinterpret_cast<OFFSCREEN*>(pBuf->Esp + kOffscreenStackOffset);

            UnitStatusCounters::DrawForUnit(offscreen, unit, centerX, centerY);
            DrawReloadBarForUnit(offscreen, unit, centerX, centerY);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
        }

        return 0;
    }
}

namespace ReloadBars
{
    void Install()
    {
        if (g_installed)
            return;

        WeaponTdfHook::Register([](const WeaponTdfHook::Context& ctx) {
            if (ctx.getInt(kReloadBarKey) & 1)
            {
                WeaponStruct* weaponDef = reinterpret_cast<WeaponStruct*>(ctx.pWeaponDef);
                g_reloadBarWeapons.insert((DWORD)weaponDef);
                g_reloadBarWeaponNames.insert(weaponDef->WeaponName);
            }
        });

        g_drawHook.reset(new InlineSingleHook(
            kDrawUnitBarsHookAddr,
            kDrawUnitBarsHookLen,
            INLINE_5BYTESLAGGERJMP,
            DrawUnitBarsHookProc));

        g_installed = true;
    }

    void Shutdown()
    {
        g_drawHook.reset();
        g_reloadBarWeapons.clear();
        g_reloadBarWeaponNames.clear();
        g_installed = false;
    }

    void DrawUnitReloadBars()
    {
    }
}
