#include "../UnitStatsCore.h"

#include <cassert>
#include <iostream>
#include <limits>

using namespace UnitStatsCore;

namespace
{
    bool Has(const std::vector<std::string>& fields, const std::string& text)
    {
        return std::find(fields.begin(), fields.end(), text) != fields.end();
    }
    bool Starts(const std::vector<std::string>& fields, const std::string& text)
    {
        for (const std::string& field : fields)
            if (field.compare(0, text.size(), text) == 0) return true;
        return false;
    }
    Weapon Gun(unsigned damage, unsigned reload, unsigned burst = 0, unsigned flags = 0)
    {
        Weapon gun;
        gun.damage = damage; gun.reloadTicks = reload;
        gun.burst = burst; gun.flags = flags;
        return gun;
    }
}

int main()
{
    assert(Speed(0) == 0);
    assert(Speed(65536) == 12);
    assert(Speed(32768) == 6);
    // Native F1 help reports ESC Flash's MaxVelocity=2.15 as 25.8 m/s.
    assert(Detail::SpeedNumber(Speed(140902)) == "25.8");
    assert(Detail::SpeedNumber(Speed(35389)) == "6.5"); // native %.1f, not 6.48
    assert(Detail::SpeedNumber(Speed(32768)) == "6.0");
    assert(std::isfinite(Speed(std::numeric_limits<std::uint32_t>::max())));
    assert(NominalDps(Gun(100, 30)) == 100);
    assert(NominalDps(Gun(100, 60, 3)) == 150); // exactly N shots, not N+1
    assert(NominalDps(Gun(100, 30, 1)) == 100);
    assert(NominalDps(Gun(40000, 30)) == 40000); // unsigned 16-bit damage
    assert(NominalDps(Gun(100, 0)) < 0);
    assert(std::isfinite(NominalDps(Gun(0xffffffffu, 1, 0xffffffffu))));

    Stats stats;
    stats.name = "Flash"; stats.maxHealth = 70000;
    stats.currentHealth = 150; stats.liveHealth = true;
    stats.metalCost = 112.5; stats.energyCost = 1000;
    stats.speed = Speed(65536); stats.los = 320;
    stats.weapons[0] = Gun(200, 30);
    stats.weapons[0].range = 70000; // range is a full dword, not a signed short
    auto fields = Fields(stats);
    assert(Has(fields, "DPS~ 200"));
    assert(Has(fields, "Range 70000"));
    assert(Has(fields, "Speed 12.0 m/s"));
    assert(Has(fields, "LOS 320"));
    assert(Title(stats) == "Flash | HP 150/70000 | M 112.5  E 1000");
    stats.count = 3;
    assert(Title(stats) == "Flash x3 (each) | HP 70000 | M 112.5  E 1000");
    assert(Fields(stats) == fields); // group values are per unit
    stats.count = 1; stats.liveHealth = false;
    assert(Title(stats) == "Flash | HP 70000 | M 112.5  E 1000");

    stats.weapons[1] = Gun(100, 60, 4, ToAir);
    stats.weapons[1].range = 800;
    stats.weapons[2] = Gun(150, 30, 0, Paralyzer);
    fields = Fields(stats);
    assert(Has(fields, "DPS~ 200"));
    assert(Has(fields, "AA DPS~ 200"));
    assert(Has(fields, "Para/s~ 150"));
    assert(Has(fields, "Range 70000 max"));
    assert(fields[0] == "DPS~ 200" && fields[1] == "AA DPS~ 200");
    stats.weapons[0].reloadTicks = 0;
    assert(Has(Fields(stats), "DPS~ ?"));
    stats.weapons[0] = Gun(50, 30);
    stats.weapons[1] = Gun(60, 30);
    stats.weapons[2] = Gun(70, 30);
    assert(Has(Fields(stats), "DPS~ 180"));
    stats.weapons[2].reloadTicks = 0;
    assert(Has(Fields(stats), "DPS~ ?")); // do not silently undercount unknown slot

    stats.weapons[0] = Gun(30000, 30, 0, CommandFire);
    stats.weapons[1] = Gun(40000, 30, 0, Stockpile);
    stats.weapons[2] = Gun(0, 30, 0, Interceptor | Stockpile);
    stats.weapons[2].range = 32767; // default weapon range is not interception coverage
    fields = Fields(stats);
    assert(!Starts(fields, "DPS"));
    assert(Has(fields, "Manual shot 30000"));
    assert(Has(fields, "Stock shot 40000"));
    assert(Has(fields, "Interceptor"));
    assert(!Starts(fields, "Range "));
    stats.weapons[0].flags |= Paralyzer;
    assert(Has(Fields(stats), "Manual stun 30000"));
    stats.weapons[0].flags = NoExplode;
    assert(Has(Fields(stats), "Special shot 30000"));

    stats.weapons = {};
    stats.weapons[0] = Gun(100, 30);
    stats.weapons[1] = Gun(200, 30);
    stats.weapons[1].flags = Water;
    fields = Fields(stats);
    assert(Has(fields, "DPS~ 100") && Has(fields, "Water DPS~ 200"));

    stats.weapons = {};
    stats.speed = 0; stats.radar = 1200; stats.sonar = 400;
    stats.radarJam = 300; stats.sonarJam = 200;
    stats.buildPower = 180; stats.buildRange = 120; stats.buildWork = 9000;
    stats.energyMake = 30; stats.energyUse = 2; stats.metalMake = 1.25;
    stats.energyStorage = 1000; stats.metalStorage = 500;
    fields = Fields(stats);
    assert(Has(fields, "DPS 0") && !Starts(fields, "Speed "));
    assert(Has(fields, "Radar 1200") && Has(fields, "Sonar 400"));
    assert(Has(fields, "Radar jam 300") && Has(fields, "Sonar jam 200"));
    assert(Has(fields, "BP 180") && Has(fields, "Build range 120"));
    assert(!Starts(fields, "Build work ") && Has(fields, "+E/s 30"));
    assert(Has(fields, "E use/s 2") && Has(fields, "+M/s 1.25"));
    assert(Has(fields, "E storage 1000") && Has(fields, "M storage 500"));
    stats.weapons[0] = Gun(10, 15); stats.weapons[0].aoe = 60;
    stats.weapons[1] = Gun(10, 30);
    fields = Fields(stats);
    assert(Has(fields, "AOE 60") && Has(fields, "Reload 0.5-1s"));

    const double nan = std::numeric_limits<double>::quiet_NaN();
    stats.metalCost = nan; stats.energyCost = std::numeric_limits<double>::infinity();
    stats.speed = nan; stats.energyUse = -100;
    assert(Title(stats).find("M ?  E ?") != std::string::npos);
    fields = Fields(stats);
    assert(Has(fields, "Speed ? m/s") && Has(fields, "E use/s ?"));
    stats.energyMake = 1e300;
    fields = Fields(stats);
    assert(Has(fields, "+E/s 1.00e+300"));
    for (const std::string& field : fields)
    {
        assert(field.size() < 80);
        assert(field.find("nan") == std::string::npos);
        assert(field.find("inf") == std::string::npos);
    }
    stats.name = "\n\t\001";
    assert(Title(stats).compare(0, 4, "Unit") == 0);
    stats.name = std::string(5000, 'a');
    assert(Title(stats).size() < 200);
    stats.name = " \tArm\r\nFlash\177\001  ";
    assert(Title(stats).compare(0, 9, "Arm Flash") == 0);
    stats.weapons[0].range = -100; stats.weapons[1].range = -200;
    assert(!Starts(Fields(stats), "Range "));

    std::cout << "Unit stats core calculations and formatting passed\n";
}
