#!/usr/bin/env python3
"""Exercise the production stats reader against packed native-shaped records.

Only native I/O and SEH syntax are substituted. Four-byte pointer tokens keep
the Win32 layouts exact while running the reader with ASan/UBSan on a 64-bit
host. Bad pointer faults themselves remain a Windows/runtime check.
"""

import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile


DDRAW = Path(__file__).resolve().parent.parent


HARNESS = r'''
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <string>
#include <utility>
#include <vector>
#include "UnitStatsData.h"

std::vector<void*> pointers(1, nullptr);

#pragma pack(push, 1)
template<class T> struct NativePtr {
    std::uint32_t token = 0;
    NativePtr& operator=(T* pointer) {
        if (!pointer) { token = 0; return *this; }
        auto found = std::find(pointers.begin(), pointers.end(), pointer);
        if (found == pointers.end()) {
            token = static_cast<std::uint32_t>(pointers.size());
            pointers.push_back(pointer);
        } else token = static_cast<std::uint32_t>(found - pointers.begin());
        return *this;
    }
    operator T*() const { assert(token < pointers.size()); return static_cast<T*>(pointers[token]); }
};

struct WeaponStruct {
    char WeaponName[32] = {};
    unsigned char toDamage[0xD4 - 32] = {};
    union { std::int16_t Damage = 0; std::uint16_t defaultDamage; };
    union { std::int16_t AOE = 0; std::uint16_t areaOfEffect; };
    unsigned char toRange[4] = {};
    union { std::int32_t FullRange = 0; std::int32_t range; };
    unsigned char toReload[4] = {};
    union { std::uint16_t ReloadTicks = 0; std::uint16_t reloadTicks; };
    unsigned char toBurst[4] = {};
    union { std::uint16_t BurstCount = 0; std::uint16_t burst; };
    union { std::uint16_t BurstRateTicks = 0; std::uint16_t burstRateTicks; };
    unsigned char toFlags[0x111 - 0xEE] = {};
    union { std::uint32_t WeaponTypeMask = 0; std::uint32_t flags; };
};
struct UnitDefStruct {
    char Name[32] = {};
    unsigned char toEnergy[0x186 - 32] = {};
    float buildcostenergy = 0, buildcostmetal = 0;
    unsigned char toSpeed[0x192 - 0x18E] = {};
    std::uint32_t lRawSpeed_maxvelocity = 0;
    unsigned char toEconomy[0x1C2 - 0x196] = {};
    float energymake = 0, energyuse = 0, metalmake = 0, extractsmetal = 0;
    float windgenerator = 0, tidalgenerator = 0, cloakcost = 0, cloakcostmoving = 0;
    float energystorage = 0, metalstorage = 0;
    std::uint32_t buildtime = 0;
    NativePtr<WeaponStruct> weapon1, weapon2, weapon3;
    union { std::uint32_t MaxHealth = 0; std::uint32_t health; };
    std::uint16_t nWorkerTime = 0, nHealTime = 0;
    std::uint16_t nSightDistance = 0, nRadarDistance = 0, nSonarDistance = 0;
    std::uint16_t mincloakdistance = 0, radardistancejam = 0, sonardistancejam = 0;
    std::uint16_t soundClass = 0, nBuildDistance = 0, builddistance = 0;
    unsigned char remainder[0x249 - 0x214] = {};
};
struct PlayerStruct { std::uint32_t cookie = 0; };
struct UnitStruct {
    unsigned char prefix[0x92] = {};
    NativePtr<UnitDefStruct> UnitType;
    NativePtr<PlayerStruct> Owner_PlayerPtr0;
    unsigned char toType[0xA6 - 0x9A] = {};
    std::uint16_t UnitID = 0;
    unsigned char toEnergy[0xCC - 0xA8] = {};
    float EnergyMakePrev = 0, EnergyUsePrev = 0;
    unsigned char toMetal[0xE4 - 0xD4] = {};
    float MetalMakePrev = 0;
    unsigned char toHealth[0x108 - 0xE8] = {};
    std::int16_t Health = 0;
    unsigned char toFlags[0x110 - 0x10A] = {};
    std::uint32_t UnitSelected = 0;
    unsigned char tail[4] = {};
};
#pragma pack(pop)

static_assert(sizeof(NativePtr<WeaponStruct>) == 4, "Win32 pointer width");
static_assert(sizeof(UnitDefStruct) == 0x249, "Win32 unit definition stride");
static_assert(offsetof(UnitDefStruct, buildcostenergy) == 0x186, "Energy cost offset");
static_assert(offsetof(UnitDefStruct, lRawSpeed_maxvelocity) == 0x192, "Speed offset");
static_assert(offsetof(UnitDefStruct, weapon1) == 0x1EE, "Weapon pointer offset");
static_assert(offsetof(UnitDefStruct, health) == 0x1FA, "Full-width health offset");
static_assert(offsetof(UnitDefStruct, nWorkerTime) == 0x1FE, "Buildpower offset");
static_assert(offsetof(UnitDefStruct, builddistance) == 0x212, "Build range offset");
static_assert(sizeof(UnitStruct) == 0x118, "Win32 live-unit stride");
static_assert(offsetof(UnitStruct, UnitType) == 0x92, "Unit type pointer offset");
static_assert(offsetof(UnitStruct, UnitSelected) == 0x110, "Live/selection flags offset");
static_assert(offsetof(UnitStruct, Health) == 0x108, "Native live health offset");
static_assert(offsetof(UnitStruct, EnergyMakePrev) == 0xCC, "Native previous energy production");
static_assert(offsetof(UnitStruct, EnergyUsePrev) == 0xD0, "Native previous energy use");
static_assert(offsetof(UnitStruct, MetalMakePrev) == 0xE4, "Native previous metal production");
static_assert(sizeof(WeaponStruct) == 0x115, "Native weapon stride");
static_assert(offsetof(WeaponStruct, defaultDamage) == 0xD4, "Native damage offset");
static_assert(offsetof(WeaponStruct, range) == 0xDC, "Native range offset");
static_assert(offsetof(WeaponStruct, reloadTicks) == 0xE4, "Native reload offset");
static_assert(offsetof(WeaponStruct, burst) == 0xEA, "Native burst offset");
static_assert(offsetof(WeaponStruct, burstRateTicks) == 0xEC, "Native burst-rate offset");
static_assert(offsetof(WeaponStruct, flags) == 0x111, "Native weapon flags offset");

struct TAdynmemStruct {
    UnitDefStruct* UnitDef = nullptr;
    unsigned UNITINFOCount = 0;
    UnitStruct* BeginUnitsArray_p = nullptr;
    UnitStruct* EndOfUnitsArray_p = nullptr;
    int LocalHumanPlayer_PlayerID = 0;
    PlayerStruct Players[10] = {};
    WeaponStruct Weapons[256] = {};
};
struct Share { int TAProgress = 7; } share;
Share* DataShare = &share;
const int TAInGame = 7;
const unsigned UnitSelected_State = 0x10u;

// PRODUCTION_READER

template<class T, class O> void Put(O& object, std::size_t offset, T value) {
    assert(offset + sizeof(value) <= sizeof(object));
    std::memcpy(reinterpret_cast<unsigned char*>(&object) + offset, &value, sizeof(value));
}

struct World {
    TAdynmemStruct ta;
    UnitDefStruct defs[4] = {};
    WeaponStruct weapons[3] = {};
    UnitStruct units[3] = {};
    World() {
        DataShare = &share; share.TAProgress = TAInGame;
        ta.UnitDef = defs; ta.UNITINFOCount = 4;
        ta.BeginUnitsArray_p = units; ta.EndOfUnitsArray_p = units + 2;
        ta.LocalHumanPlayer_PlayerID = 1;
        auto& def = defs[2];
        std::memcpy(def.Name, "Loaded mod unit", 16);
        def.health = 100000;
        def.buildcostmetal = 42.5f; def.buildcostenergy = 123.25f;
        def.lRawSpeed_maxvelocity = 98304;
        def.nSightDistance = 375; def.nRadarDistance = 1200; def.nSonarDistance = 600;
        def.radardistancejam = 180; def.sonardistancejam = 160;
        def.nWorkerTime = 91; def.builddistance = 222; def.buildtime = 12000;
        def.energymake = 12.5f; def.energyuse = 2.5f; def.metalmake = 1.25f;
        def.energystorage = 300; def.metalstorage = 100;
        auto& weapon = weapons[0];
        std::memcpy(weapon.WeaponName, "TEST_GUN", 9);
        Put<std::uint16_t>(weapon, 0xD4, 40000);
        Put<std::uint16_t>(weapon, 0xD6, 125);
        Put<std::int32_t>(weapon, 0xDC, 50000);
        Put<std::uint16_t>(weapon, 0xE4, 45);
        Put<std::uint16_t>(weapon, 0xEA, 3);
        Put<std::uint16_t>(weapon, 0xEC, 2);
        Put<std::uint32_t>(weapon, 0x111, (1u << 17));
        def.weapon1 = &weapon; def.weapon2 = nullptr; def.weapon3 = nullptr;
        auto& unit = units[1];
        unit.UnitID = 2; unit.UnitType = &def; unit.Owner_PlayerPtr0 = &ta.Players[1];
        unit.UnitSelected = 0x10000000u | UnitSelected_State;
        Put<std::uint16_t>(unit, 0x108, 40000);
        Put<float>(unit, 0xCC, 47.5f); Put<float>(unit, 0xD0, 8.0f); Put<float>(unit, 0xE4, 3.5f);
    }
    bool Read(UnitStatsCore::Stats& out, unsigned count = 1, const UnitStruct* selected = nullptr) {
        return UnitStatsData::Read(&ta, 2, selected, count, out);
    }
    std::vector<unsigned char> Image() const {
        std::vector<unsigned char> result;
        auto append = [&result](const void* pointer, std::size_t length) {
            auto first = static_cast<const unsigned char*>(pointer);
            result.insert(result.end(), first, first + length);
        };
        append(&ta, sizeof(ta)); append(defs, sizeof(defs));
        append(weapons, sizeof(weapons)); append(units, sizeof(units));
        append(&share, sizeof(share)); return result;
    }
};

void LoadedDefinitionsAndNativeWidths() {
    World world; UnitStatsCore::Stats stats;
    const auto before = world.Image();
    assert(world.Read(stats));
    assert(stats.name == "Loaded mod unit" && stats.count == 1);
    assert(stats.maxHealth == 100000 && stats.currentHealth == 100000 && !stats.liveHealth);
    assert(stats.metalCost == 42.5 && stats.energyCost == 123.25);
    assert(stats.speed == 18 && stats.los == 375);
    assert(stats.radar == 1200 && stats.sonar == 600);
    assert(stats.radarJam == 180 && stats.sonarJam == 160);
    assert(stats.buildPower == 91 && stats.buildRange == 222 && stats.buildWork == 12000);
    assert(stats.energyMake == 12.5 && stats.energyUse == 2.5 && stats.metalMake == 1.25);
    assert(stats.energyStorage == 300 && stats.metalStorage == 100);
    const auto& weapon = stats.weapons[0];
    assert(weapon.damage == 40000 && weapon.range == 50000 && weapon.aoe == 125);
    assert(weapon.reloadTicks == 45 && weapon.burst == 3 && weapon.burstRateTicks == 2);
    assert(weapon.flags == (1u << 17));
    assert(stats.weapons[1].damage == 0 && stats.weapons[2].damage == 0);
    assert(world.Image() == before);
}

void OwnedSelectionHealthAndEconomy() {
    World world; UnitStatsCore::Stats stats;
    const auto before = world.Image();
    assert(world.Read(stats, 1, &world.units[1]));
    assert(stats.liveHealth && stats.currentHealth == 40000 && stats.maxHealth == 100000);
    assert(stats.energyMake == 47.5 && stats.energyUse == 8 && stats.metalMake == 3.5);
    assert(world.Image() == before);
    world.defs[2].health = 100;
    assert(world.Read(stats, 1, &world.units[1]));
    assert(stats.currentHealth == 100);
    world.defs[2].health = 0;
    assert(world.Read(stats, 1, &world.units[1]) && stats.currentHealth == 0);
}

void MatchingGroupsRemainPerUnit() {
    World world; UnitStatsCore::Stats stats;
    assert(world.Read(stats, 12, &world.units[1]));
    assert(stats.count == 12 && !stats.liveHealth && stats.currentHealth == stats.maxHealth);
    assert(stats.metalCost == 42.5 && stats.energyCost == 123.25);
    assert(stats.weapons[0].damage == 40000 && stats.buildPower == 91);
    assert(stats.energyMake == 12.5 && stats.metalMake == 1.25);
    assert(world.Read(stats, 0, &world.units[1]) && stats.count == 1 && !stats.liveHealth);
}

void InvalidSelectedUnitsUseOnlyDefinition() {
    World world; UnitStatsCore::Stats stats;
    auto check = [&]() {
        assert(world.Read(stats, 1, &world.units[1]));
        assert(!stats.liveHealth && stats.currentHealth == 100000 && stats.energyMake == 12.5);
    };
    world.units[1].Owner_PlayerPtr0 = &world.ta.Players[2]; check();
    world.units[1].Owner_PlayerPtr0 = &world.ta.Players[1];
    world.units[1].UnitID = 3; check(); world.units[1].UnitID = 2;
    world.units[1].UnitType = &world.defs[1]; check(); world.units[1].UnitType = &world.defs[2];
    world.units[1].UnitSelected = UnitSelected_State; check();
    world.units[1].UnitSelected = 0x10000000u | UnitSelected_State | 0x4000u; check();
    world.units[1].UnitSelected = 0x10000000u; check();
    world.units[1].UnitSelected = 0x10000000u | UnitSelected_State;
    world.ta.LocalHumanPlayer_PlayerID = -1; check();
    world.ta.LocalHumanPlayer_PlayerID = 10; check();
    world.ta.LocalHumanPlayer_PlayerID = 1;
    world.ta.BeginUnitsArray_p = nullptr; check();
    world.ta.BeginUnitsArray_p = world.units + 2; check();
    world.ta.BeginUnitsArray_p = world.units; world.ta.EndOfUnitsArray_p = world.units; check();
    world.ta.EndOfUnitsArray_p = world.units + 2;
    UnitStruct outside = {};
    assert(world.Read(stats, 1, &outside) && !stats.liveHealth);
    auto misaligned = reinterpret_cast<const UnitStruct*>(reinterpret_cast<const char*>(world.units) + 1);
    assert(world.Read(stats, 1, misaligned) && !stats.liveHealth);
}

void InvalidContextsDoNotPublishPartialStats() {
    World world; UnitStatsCore::Stats stats; stats.name = "unchanged"; stats.maxHealth = 73;
    assert(!UnitStatsData::Read(nullptr, 0, nullptr, 1, stats));
    assert(!UnitStatsData::Read(&world.ta, 4, nullptr, 1, stats));
    assert(!world.Read(stats, 65536));
    world.ta.UNITINFOCount = 0; assert(!world.Read(stats)); world.ta.UNITINFOCount = 4;
    DataShare = nullptr; assert(!world.Read(stats)); DataShare = &share;
    share.TAProgress = 0; assert(!world.Read(stats)); share.TAProgress = TAInGame;
    world.ta.UnitDef = nullptr; assert(!world.Read(stats));
    assert(stats.name == "unchanged" && stats.maxHealth == 73);
}

void SanitizedNumbersAndBoundedNames() {
    World world; UnitStatsCore::Stats stats;
    auto& def = world.defs[2];
    std::memset(def.Name, 'X', sizeof(def.Name));
    def.buildcostenergy = std::numeric_limits<float>::infinity();
    def.buildcostmetal = std::numeric_limits<float>::quiet_NaN();
    def.energymake = -1; def.energyuse = std::numeric_limits<float>::quiet_NaN();
    def.metalmake = -std::numeric_limits<float>::infinity();
    def.energystorage = -10; def.metalstorage = 0;
    assert(world.Read(stats));
    assert(stats.name == std::string(32, 'X'));
    assert(stats.energyCost == 0 && stats.metalCost == 0 && stats.energyMake == 0);
    assert(stats.energyUse == 0 && stats.metalMake == 0 && stats.energyStorage == 0 && stats.metalStorage == 0);
    Put<float>(world.units[1], 0xCC, std::numeric_limits<float>::quiet_NaN());
    Put<float>(world.units[1], 0xD0, -10);
    Put<float>(world.units[1], 0xE4, std::numeric_limits<float>::infinity());
    assert(world.Read(stats, 1, &world.units[1]));
    assert(stats.energyMake == 0 && stats.energyUse == 0 && stats.metalMake == 0);
}

void FreshModsGamesAndWeaponSlots() {
    World world; UnitStatsCore::Stats first, next;
    assert(world.Read(first));
    std::memset(world.defs[2].Name, 0, sizeof(world.defs[2].Name));
    std::memcpy(world.defs[2].Name, "Second mod", 11);
    world.defs[2].health = 100; world.defs[2].nSightDistance = 777;
    world.defs[2].weapon2 = &world.weapons[1];
    std::memcpy(world.weapons[1].WeaponName, "SECOND", 7);
    Put<std::uint16_t>(world.weapons[1], 0xD4, 123);
    Put<std::uint16_t>(world.weapons[1], 0xE4, 0);
    Put<std::int32_t>(world.weapons[1], 0xDC, 80000);
    Put<std::uint32_t>(world.weapons[1], 0x111, (1u << 7) | (1u << 28));
    // A non-null zero-name sentinel must never become a weapon, even with
    // nonzero garbage in otherwise meaningful fields.
    world.defs[2].weapon3 = &world.weapons[2];
    Put<std::uint16_t>(world.weapons[2], 0xD4, 65000);
    assert(world.Read(next));
    assert(next.name == "Second mod" && next.maxHealth == 100 && next.los == 777);
    assert(next.weapons[1].damage == 123 && next.weapons[1].range == 80000);
    assert(next.weapons[1].reloadTicks == 0 && next.weapons[1].flags == ((1u << 7) | (1u << 28)));
    assert(next.weapons[2].damage == 0);
    assert(first.name == "Loaded mod unit" && first.maxHealth == 100000 && first.los == 375);
    World fresh;
    fresh.defs[2].health = 12345;
    assert(fresh.Read(next) && next.maxHealth == 12345);
    assert(world.Read(next) && next.maxHealth == 100);
}

void ArchiveFlashAndNamedNoWeaponSlots() {
    World world; UnitStatsCore::Stats stats;
    auto& def = world.defs[2];
    // Actual ESC TAESC.gp3: unitsE/ARMFLASH.fbi, weaponE/TAESC.tdf.
    // ProTA.gp3: units/ARMFLASH.FBI, weaponP/WEAPONS.TDF use the same
    // EMG damage=8, burst=3, reloadtime=.4 and named NOWEAPON ID=0.
    // Populate named fields in the independently declared native weapon layout;
    // missing slots use table entry zero, as TA's unit loader does.
    auto& noWeapon = world.ta.Weapons[0];
    std::memcpy(noWeapon.WeaponName, "NOWEAPON", 9);
    noWeapon.range = 16;
    auto& emg = world.ta.Weapons[43];
    std::memcpy(emg.WeaponName, "EMG", 4);
    emg.defaultDamage = 8; emg.areaOfEffect = 16; emg.range = 180;
    emg.reloadTicks = 12; emg.burst = 3; emg.burstRateTicks = 3;
    def.lRawSpeed_maxvelocity = 140902; // trunc(2.15 * 65536)
    def.weapon1 = &emg; def.weapon2 = &noWeapon; def.weapon3 = &noWeapon;
    const auto before = world.Image();
    assert(world.Read(stats));
    auto fields = UnitStatsCore::Fields(stats);
    auto has = [&fields](const std::string& field) {
        return std::find(fields.begin(), fields.end(), field) != fields.end();
    };
    assert(has("DPS~ 60") && has("Speed 25.8 m/s") && has("Reload 0.4s"));
    assert(stats.weapons[1].range == 0 && stats.weapons[2].range == 0);
    assert(world.Image() == before);

    // ProTA's Flash is MaxVelocity=2.0 and uses EMG table slot 16.
    world.ta.Weapons[16] = emg;
    def.weapon1 = &world.ta.Weapons[16]; def.lRawSpeed_maxvelocity = 131072;
    assert(world.Read(stats)); fields = UnitStatsCore::Fields(stats);
    assert(has("DPS~ 60") && has("Speed 24.0 m/s"));

    def.weapon1 = &noWeapon;
    assert(world.Read(stats)); fields = UnitStatsCore::Fields(stats);
    assert(has("DPS 0") && !has("Range 16"));

    // A distinct definition with the same values is not the sentinel. Do not
    // turn a legitimate zero-reload weapon's unknown rate into a guessed zero.
    world.weapons[1] = noWeapon;
    def.weapon1 = &world.weapons[1];
    assert(world.Read(stats)); fields = UnitStatsCore::Fields(stats);
    assert(has("DPS~ ?") && has("Range 16"));
}

int main() {
#if TEST_ENABLED
    LoadedDefinitionsAndNativeWidths(); OwnedSelectionHealthAndEconomy();
    MatchingGroupsRemainPerUnit(); InvalidSelectedUnitsUseOnlyDefinition();
    InvalidContextsDoNotPublishPartialStats(); SanitizedNumbersAndBoundedNames();
    FreshModsGamesAndWeaponSlots(); ArchiveFlashAndNamedNoWeaponSlots();
    std::cout << "Unit stats production adapter: native widths, owned/live selection, groups, mod reload, readonly data and malformed values passed\n";
#else
    UnitStatsCore::Stats stats; stats.name = "unchanged";
    assert(!UnitStatsData::Read(reinterpret_cast<TAdynmemStruct*>(1), 999999,
        reinterpret_cast<UnitStruct*>(1), 999999, stats));
    assert(stats.name == "unchanged");
    std::cout << "Compiled-out unit stats reader never touches native memory\n";
#endif
}
'''


def production_reader(enabled):
    source = (DDRAW / "UnitStatsData.cpp").read_text()
    if enabled:
        body = source.split("#if UNIT_STATS_PANEL_ENABLE", 1)[1].split("#else", 1)[0]
        body = re.sub(r"^#include[^\n]*\n", "", body, flags=re.M)
        body = body.replace("__try", "try").replace(
            "__except (EXCEPTION_EXECUTE_HANDLER)", "catch (...)"
        )
        return body
    return source.split("#else", 1)[1].rsplit("#endif", 1)[0]


def main():
    compiler = shlex.split(os.environ.get("CXX", "g++"))
    with tempfile.TemporaryDirectory(prefix="unit-stats-data-") as directory:
        temp = Path(directory)
        for enabled in (1, 0):
            source = temp / ("enabled.cpp" if enabled else "disabled.cpp")
            binary = temp / ("enabled" if enabled else "disabled")
            # Disabled production exports only Read; do not compile the
            # enabled-only helpers/fixtures that deliberately call it.
            harness = HARNESS
            if not enabled:
                start = harness.index("template<class T, class O> void Put")
                end = harness.index("int main()", start)
                harness = harness[:start] + harness[end:]
            source.write_text(harness.replace("// PRODUCTION_READER", production_reader(enabled)))
            subprocess.run(
                compiler + ["-std=c++17", "-Wall", "-Wextra", "-Werror", "-g",
                            "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                            f"-DTEST_ENABLED={enabled}", "-I", str(DDRAW), str(source), "-o", str(binary)],
                check=True,
            )
            environment = dict(os.environ)
            environment["ASAN_OPTIONS"] = "detect_leaks=0"
            subprocess.run([str(binary)], check=True, env=environment)


if __name__ == "__main__":
    main()
