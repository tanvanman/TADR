#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string>
#include <vector>

// Values are copied from the loaded mod, not from a separate unit database.
// This layer formats snapshots only and never retains native unit pointers.
namespace UnitStatsCore
{
    struct Weapon
    {
        std::uint32_t damage = 0, reloadTicks = 0, burst = 0, burstRateTicks = 0, flags = 0;
        int range = 0, aoe = 0;
    };

    struct Stats
    {
        std::string name;
        unsigned count = 1, maxHealth = 0, currentHealth = 0;
        bool liveHealth = false;
        double metalCost = 0, energyCost = 0, speed = 0;
        unsigned los = 0, radar = 0, sonar = 0, radarJam = 0, sonarJam = 0;
        unsigned buildPower = 0, buildRange = 0, buildWork = 0;
        double energyMake = 0, energyUse = 0, metalMake = 0;
        double energyStorage = 0, metalStorage = 0;
        std::array<Weapon, 3> weapons = {};
    };

    constexpr std::uint32_t Paralyzer = 1u << 7;
    constexpr std::uint32_t Water = 1u << 16;
    constexpr std::uint32_t ToAir = 1u << 17;
    constexpr std::uint32_t NoExplode = 1u << 22;
    constexpr std::uint32_t CommandFire = 1u << 26;
    constexpr std::uint32_t Stockpile = 1u << 28;
    constexpr std::uint32_t Interceptor = 1u << 30;

    inline double Speed(std::uint32_t raw)
    {
        // Match TA's native unit-help conversion: 30 ticks/s, 0.4 metres/pixel.
        return static_cast<double>(raw) * 30.0 * 0.4 / 65536.0;
    }

    // The burst projectile is a carrier that spawns exactly burst children.
    // A zero reload is not an infinite firing rate: its nominal DPS is unknown.
    // Health, veterancy, target damage overrides, splash and script/aim delays
    // are deliberately excluded from this theoretical full-health base rate.
    inline double NominalDps(const Weapon& weapon)
    {
        if (!weapon.reloadTicks) return -1.0;
        return static_cast<double>(weapon.damage)
            * std::max<std::uint32_t>(1, weapon.burst) * 30.0 / weapon.reloadTicks;
    }

    namespace Detail
    {
        inline std::string Number(double value)
        {
            if (!std::isfinite(value) || value < 0) return "?";
            if (value == 0) return "0";
            std::ostringstream stream;
            stream.imbue(std::locale::classic());
            if (value >= 1000000000.0 || (value > 0 && value < 0.01))
                stream << std::scientific << std::setprecision(2) << value;
            else
            {
                const unsigned digits = value < 10 && value > 0 ? 2 : 1;
                stream << std::fixed << std::setprecision(digits) << value;
            }
            std::string result = stream.str();
            if (result.find('e') == std::string::npos && result.find('.') != std::string::npos)
            {
                while (!result.empty() && result.back() == '0') result.pop_back();
                if (!result.empty() && result.back() == '.') result.pop_back();
            }
            return result;
        }

        inline std::string SpeedNumber(double value)
        {
            if (!std::isfinite(value) || value < 0) return "?";
            // Native MakePropList uses %.1f even for speeds below 10 m/s.
            // Retain bounded output for values outside the native raw-speed range.
            if (value >= 1000000000.0) return Number(value);
            std::ostringstream stream;
            stream.imbue(std::locale::classic());
            stream << std::fixed << std::setprecision(1) << value;
            return stream.str();
        }

        inline std::string Name(const std::string& source)
        {
            std::string result;
            bool space = false;
            for (unsigned char character : source)
            {
                if (result.size() >= 80) break;
                if (character <= 32 || character == 127)
                {
                    space = !result.empty();
                    continue;
                }
                if (space)
                {
                    if (result.size() >= 79) break;
                    result.push_back(' ');
                }
                result.push_back(static_cast<char>(character));
                space = false;
            }
            return result.empty() ? "Unit" : result;
        }

        inline bool Present(const Weapon& weapon)
        {
            return weapon.damage || weapon.reloadTicks || weapon.flags || weapon.range > 0;
        }

        struct Rate
        {
            bool present = false, unknown = false;
            double value = 0;
            void Add(const Weapon& weapon)
            {
                present = true;
                const double rate = NominalDps(weapon);
                if (!std::isfinite(rate) || rate < 0) unknown = true;
                else value += rate;
            }
            std::string Format() const { return unknown ? "?" : Number(value); }
        };

        inline void Nonzero(std::vector<std::string>& fields, const char* label, double value)
        {
            if (value != 0) fields.push_back(std::string(label) + Number(value));
        }
    }

    inline std::string Title(const Stats& stats)
    {
        std::string result = Detail::Name(stats.name);
        if (stats.count > 1) result += " x" + std::to_string(stats.count) + " (each)";
        result += " | HP ";
        if (stats.liveHealth && stats.count == 1)
            result += std::to_string(stats.currentHealth) + "/";
        result += std::to_string(stats.maxHealth);
        result += " | M " + Detail::Number(stats.metalCost);
        result += "  E " + Detail::Number(stats.energyCost);
        return result;
    }

    inline std::vector<std::string> Fields(const Stats& stats)
    {
        Detail::Rate ground, air, water, paralysis;
        std::vector<std::string> special;
        int range = 0, firstRange = 0, aoe = 0;
        bool mixedRanges = false;
        std::uint32_t minReload = 0, maxReload = 0;
        unsigned weaponCount = 0;
        for (const Weapon& weapon : stats.weapons)
        {
            if (!Detail::Present(weapon)) continue;
            ++weaponCount;
            if (weapon.range > 0 && !(weapon.flags & Interceptor))
            {
                range = std::max(range, weapon.range);
                if (firstRange && firstRange != weapon.range) mixedRanges = true;
                firstRange = weapon.range;
            }
            aoe = std::max(aoe, weapon.aoe);
            if (weapon.flags & Interceptor)
                special.push_back("Interceptor");
            else if (weapon.flags & Stockpile)
                special.push_back("Stock shot " + std::to_string(weapon.damage));
            else if (weapon.flags & CommandFire)
                special.push_back(std::string(weapon.flags & Paralyzer ? "Manual stun " : "Manual shot ")
                    + std::to_string(weapon.damage));
            else if (weapon.flags & NoExplode)
                special.push_back("Special shot " + std::to_string(weapon.damage));
            else
            {
                if (weapon.flags & Paralyzer) paralysis.Add(weapon);
                else if (weapon.flags & ToAir) air.Add(weapon);
                else if (weapon.flags & Water) water.Add(weapon);
                else ground.Add(weapon);
                if (weapon.reloadTicks)
                {
                    if (!minReload || weapon.reloadTicks < minReload) minReload = weapon.reloadTicks;
                    maxReload = std::max(maxReload, weapon.reloadTicks);
                }
            }
        }

        std::vector<std::string> fields;
        if (ground.present) fields.push_back("DPS~ " + ground.Format());
        else if (!weaponCount) fields.push_back("DPS 0");
        if (air.present) fields.push_back("AA DPS~ " + air.Format());
        if (water.present) fields.push_back("Water DPS~ " + water.Format());
        if (range > 0) fields.push_back("Range " + std::to_string(range) + (mixedRanges ? " max" : ""));
        if (stats.speed != 0) fields.push_back("Speed " + Detail::SpeedNumber(stats.speed) + " m/s");
        fields.push_back("LOS " + std::to_string(stats.los));
        if (paralysis.present) fields.push_back("Para/s~ " + paralysis.Format());
        for (const std::string& entry : special)
            if (std::find(fields.begin(), fields.end(), entry) == fields.end()) fields.push_back(entry);
        Detail::Nonzero(fields, "Radar ", stats.radar);
        Detail::Nonzero(fields, "Sonar ", stats.sonar);
        Detail::Nonzero(fields, "Radar jam ", stats.radarJam);
        Detail::Nonzero(fields, "Sonar jam ", stats.sonarJam);
        Detail::Nonzero(fields, "BP ", stats.buildPower);
        Detail::Nonzero(fields, "Build range ", stats.buildRange);
        Detail::Nonzero(fields, "+E/s ", stats.energyMake);
        Detail::Nonzero(fields, "E use/s ", stats.energyUse);
        Detail::Nonzero(fields, "+M/s ", stats.metalMake);
        Detail::Nonzero(fields, "E storage ", stats.energyStorage);
        Detail::Nonzero(fields, "M storage ", stats.metalStorage);
        if (aoe > 0) fields.push_back("AOE " + std::to_string(aoe));
        if (minReload)
        {
            std::string reload = "Reload " + Detail::Number(minReload / 30.0);
            if (maxReload != minReload) reload += "-" + Detail::Number(maxReload / 30.0);
            fields.push_back(reload + "s");
        }
        return fields;
    }
}
