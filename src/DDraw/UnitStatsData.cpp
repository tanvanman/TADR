#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "config.h"
#include "UnitStatsData.h"

#if UNIT_STATS_PANEL_ENABLE
#include "oddraw.h"
#include "tamem.h"
#include "iddrawsurface.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <utility>

namespace
{
    struct Raw
    {
        char name[33];
        unsigned count, maxHealth, currentHealth, speed;
        bool liveHealth;
        float metalCost, energyCost;
        unsigned los, radar, sonar, radarJam, sonarJam, buildPower, buildRange, buildWork;
        float energyMake, energyUse, metalMake, energyStorage, metalStorage;
        UnitStatsCore::Weapon weapons[3];
    };

    bool OwnLiveUnit(TAdynmemStruct* ta, const UnitStruct* unit, unsigned typeIndex)
    {
        if (!unit || !ta->BeginUnitsArray_p || !ta->EndOfUnitsArray_p) return false;
        const std::uintptr_t address = reinterpret_cast<std::uintptr_t>(unit);
        const std::uintptr_t first = reinterpret_cast<std::uintptr_t>(ta->BeginUnitsArray_p);
        const std::uintptr_t last = reinterpret_cast<std::uintptr_t>(ta->EndOfUnitsArray_p);
        const int player = ta->LocalHumanPlayer_PlayerID;
        return player >= 0 && player < 10 && last >= first && address >= first && address <= last
            && (address - first) % sizeof(UnitStruct) == 0
            && static_cast<unsigned short>(unit->UnitID) == typeIndex
            && unit->UnitType == &ta->UnitDef[typeIndex]
            && unit->Owner_PlayerPtr0 == &ta->Players[player]
            && (unit->UnitSelected & 0x10000000u) && !(unit->UnitSelected & 0x4000u)
            && (unit->UnitSelected & UnitSelected_State);
    }

    // A separate SEH envelope protects reads during match teardown; it owns no
    // C++ containers and cannot strand a native reference or change game state.
    bool Capture(TAdynmemStruct* ta, unsigned typeIndex, const UnitStruct* selected,
        unsigned count, Raw& raw)
    {
        __try
        {
            if (!ta || !DataShare || DataShare->TAProgress != TAInGame || !ta->UnitDef
                || typeIndex >= static_cast<unsigned>(ta->UNITINFOCount) || count > 65535u)
                return false;
            const UnitDefStruct& def = ta->UnitDef[typeIndex];
            std::memcpy(raw.name, def.Name, sizeof(def.Name));
            raw.name[sizeof(def.Name)] = 0;
            raw.count = count ? count : 1;
            raw.maxHealth = def.MaxHealth;
            raw.currentHealth = raw.maxHealth;
            raw.liveHealth = count == 1 && OwnLiveUnit(ta, selected, typeIndex);
            if (raw.liveHealth) raw.currentHealth = static_cast<unsigned short>(selected->Health);
            raw.metalCost = def.buildcostmetal;
            raw.energyCost = def.buildcostenergy;
            raw.speed = def.lRawSpeed_maxvelocity;
            raw.los = def.nSightDistance;
            raw.radar = def.nRadarDistance;
            raw.sonar = def.nSonarDistance;
            raw.radarJam = def.radardistancejam;
            raw.sonarJam = def.sonardistancejam;
            raw.buildPower = def.nWorkerTime;
            raw.buildRange = def.builddistance;
            raw.buildWork = def.buildtime;
            raw.energyMake = def.energymake;
            raw.energyUse = def.energyuse;
            raw.metalMake = def.metalmake;
            raw.energyStorage = def.energystorage;
            raw.metalStorage = def.metalstorage;
            if (raw.liveHealth)
            {
                raw.energyMake = selected->EnergyMakePrev;
                raw.energyUse = selected->EnergyUsePrev;
                raw.metalMake = selected->MetalMakePrev;
            }
            const WeaponStruct* weapons[] = {def.weapon1, def.weapon2, def.weapon3};
            for (unsigned i = 0; i < 3; ++i)
            {
                const WeaponStruct* weapon = weapons[i];
                raw.weapons[i] = {};
                // Absent slots point to weapon zero, which mods can name and
                // give a range. Skip its identity, not legitimate zero-reload weapons.
                if (!weapon || weapon == &ta->Weapons[0] || !weapon->WeaponName[0]) continue;
                auto& out = raw.weapons[i];
                out.damage = static_cast<unsigned short>(weapon->Damage);
                out.aoe = static_cast<unsigned short>(weapon->AOE);
                out.range = weapon->FullRange;
                out.reloadTicks = weapon->ReloadTicks;
                out.burst = weapon->BurstCount;
                out.burstRateTicks = weapon->BurstRateTicks;
                out.flags = weapon->WeaponTypeMask;
            }
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }

    double Finite(float number)
    {
        return std::isfinite(number) && number >= 0.0f ? static_cast<double>(number) : 0.0;
    }
}

namespace UnitStatsData
{
    bool Read(TAdynmemStruct* ta, unsigned typeIndex, const UnitStruct* selected,
        unsigned count, UnitStatsCore::Stats& out)
    {
        Raw raw = {};
        if (!Capture(ta, typeIndex, selected, count, raw)) return false;
        UnitStatsCore::Stats snapshot = {};
        snapshot.name = raw.name;
        snapshot.count = raw.count;
        snapshot.maxHealth = raw.maxHealth;
        snapshot.currentHealth = std::min(raw.currentHealth, raw.maxHealth);
        snapshot.liveHealth = raw.liveHealth;
        snapshot.metalCost = Finite(raw.metalCost);
        snapshot.energyCost = Finite(raw.energyCost);
        snapshot.speed = UnitStatsCore::Speed(raw.speed);
        snapshot.los = raw.los;
        snapshot.radar = raw.radar;
        snapshot.sonar = raw.sonar;
        snapshot.radarJam = raw.radarJam;
        snapshot.sonarJam = raw.sonarJam;
        snapshot.buildPower = raw.buildPower;
        snapshot.buildRange = raw.buildRange;
        snapshot.buildWork = raw.buildWork;
        snapshot.energyMake = Finite(raw.energyMake);
        snapshot.energyUse = Finite(raw.energyUse);
        snapshot.metalMake = Finite(raw.metalMake);
        snapshot.energyStorage = Finite(raw.energyStorage);
        snapshot.metalStorage = Finite(raw.metalStorage);
        std::copy(raw.weapons, raw.weapons + 3, snapshot.weapons.begin());
        out = std::move(snapshot);
        return true;
    }
}
#else
namespace UnitStatsData
{
    bool Read(TAdynmemStruct*, unsigned, const UnitStruct*, unsigned, UnitStatsCore::Stats&) { return false; }
}
#endif
