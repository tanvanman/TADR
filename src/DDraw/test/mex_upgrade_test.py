#!/usr/bin/env python3
"""Compile real extractor-upgrade helpers with native TA calls mocked.

Mapping, builder eligibility, footprint validation, occupancy, alignment and
Shift cancellation run verbatim from MexUpgrade.cpp. Only fixed-address native
bridges and their process scratch guard are mocked. Windows ABI/build validation
remains a separate check. Requires g++ (or $CXX).
"""

import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile


DDRAW = Path(__file__).resolve().parent.parent


def extract(source, signature):
    start = source.index(signature)
    opening = source.index("{", start)
    tokens = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[{}]', re.S)
    depth = 0
    for token in tokens.finditer(source, opening):
        if token.group() == "{":
            depth += 1
        elif token.group() == "}":
            depth -= 1
            if depth == 0:
                return source[start:token.end()]
    raise RuntimeError(f"Unclosed production definition: {signature}")


HARNESS = r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <strings.h>
#include <vector>
#define _strnicmp strncasecmp
constexpr int TRUE = 1, FALSE = 0;
constexpr unsigned builder = 1, canfly = 2, WATCH = 4;
constexpr int UnitSelected_State = 0x10, Player_LocalHuman = 1, TAInGame = 1;
constexpr unsigned MK_CONTROL = 8, MK_SHIFT = 4, MK_RBUTTON = 2, MK_LBUTTON = 1;
constexpr unsigned WM_LBUTTONUP = 0x202, WM_RBUTTONDOWN = 0x204, VK_MENU = 0x12;
namespace ordertype { constexpr int STOP = 1; }
struct Position_Dword { std::uint16_t x_, X, z_, Z, y_, Y; };
struct UnitDefStruct {
    char UnitName[32] = {};
    short* CANBUILD_ptr = nullptr;
    int canbuildCount = 0, bmcode = 0, FootX = 1, FootY = 1, buildLimit = -1;
    float extractsmetal = 0;
    unsigned UnitTypeMask_0 = 0;
    char* YardMap = nullptr;
};
struct PlayerStruct;
struct UnitOrdersStruct;
struct UnitStruct {
    short UnitID = 0;
    UnitDefStruct* UnitType = nullptr;
    PlayerStruct* Owner_PlayerPtr0 = nullptr;
    unsigned UnitSelected = 0;
    short UnitInGameIndex = 0;
    float Nanoframe = 0;
    void* IsUnit = nullptr;
    unsigned short XPos = 0, YPos = 0, ZPos = 0;
    int XGridPos = 0, YGridPos = 0, SizeFootX = 0, SizeFootZ = 0;
    UnitOrdersStruct* UnitOrders = nullptr;
};
struct UnitOrdersStruct {
    UnitOrdersStruct* NextOrder = nullptr;
    unsigned COBHandler_index = 0, BuildUnitID = 0, State = 0, Order_State = 0, StartTime = 0;
    UnitStruct* AttackTargat = nullptr;
    UnitStruct* Unit_ptr = nullptr;
    Position_Dword Pos = {};
};
struct PlayerInfoStruct { unsigned PropertyMask = 0; };
struct PlayerStruct {
    UnitStruct* Units = nullptr;
    UnitStruct* UnitsAry_End = nullptr;
    PlayerInfoStruct* PlayerInfo = nullptr;
    int My_PlayerType = Player_LocalHuman;
};
struct FeatureStruct { unsigned occupyingUnitNumber = 0; };
struct TAdynmemStruct {
    int PrepareOrder_Type = ordertype::STOP, BuildSpotState = 2, InterfaceType = 0;
    int LocalHumanPlayer_PlayerID = 0, FeatureMapSizeX = 32, FeatureMapSizeY = 32;
    unsigned MouseOverUnit = 2, UNITINFOCount = 12;
    UnitDefStruct* UnitDef = nullptr;
    FeatureStruct* FeatureMap = nullptr;
    UnitStruct* BeginUnitsArray_p = nullptr;
    UnitStruct* EndOfUnitsArray_p = nullptr;
    PlayerStruct Players[10];
};
struct _MOUSEEVENT { unsigned fwKeys = 0, Msg = WM_LBUTTONUP; };
struct SharedData { int TAProgress = TAInGame; bool PlayingDemo = false; } shared;
static SharedData* DataShare = &shared;
struct Config {
    bool active = true;
    int reads = 0;
    int GetIniBool(const char* key, int defaultValue) {
        assert(std::string(key) == "MexUpgrade" && defaultValue == TRUE);
        ++reads; return active;
    }
};
static Config config;
static Config* MyConfig = nullptr;
struct IDDrawSurface {
    static void OutptTxt(const char*) {}
    template<class... Args> static void OutptFmtTxt(const char*, Args...) {}
};
static short GetAsyncKeyState(unsigned) { return 0; }
struct Upgrade { UnitStruct* constructor; UnitStruct* target; const char* replacement; int type; Position_Dword buildPos; };
struct UpgradePair { const char* from; const char* to; };
struct Actions { unsigned reclaim, build; };
struct Feedback {
    const TAdynmemStruct* world; unsigned constructorSlot; short constructorType;
    unsigned action, start; int type; Position_Dword position;
} feedback = {};

// PRODUCTION_CONSTANTS_AND_TABLE

static bool checkingSite, gridAllowed, reclaimAllowed;
static int gridCalls, nativeSends, removedOrders;
static const char* originalYard;
static std::vector<char> probedYard;
static unsigned probedPacked, probedSlot;
struct SiteProbe {
    bool previous = checkingSite;
    SiteProbe() { checkingSite = true; }
    ~SiteProbe() { checkingSite = previous; }
};
static int MockGridHeight() { return 77; }
static int TestGridSpot(UnitDefStruct* preview, unsigned packed, unsigned slot, PlayerStruct*) {
    assert(checkingSite && preview->YardMap != originalYard);
    ++gridCalls; probedPacked = packed; probedSlot = slot;
    probedYard.assign(preview->YardMap, preview->YardMap + preview->FootX * preview->FootY);
    return gridAllowed;
}
static bool CanReclaim(UnitStruct*, UnitStruct*) { return reclaimAllowed; }
static Actions UpgradeActions(const UnitStruct&) { return {11,12}; }
static std::vector<std::unique_ptr<UnitOrdersStruct>> allocatedOrders;
static void Send(unsigned action, int shift, UnitStruct* unit, UnitStruct* target,
    const Position_Dword* pos, int type, int) {
    ++nativeSends;
    if (!shift) unit->UnitOrders = nullptr;
    allocatedOrders.emplace_back(new UnitOrdersStruct);
    auto* order = allocatedOrders.back().get();
    order->COBHandler_index = action; order->AttackTargat = target; order->Unit_ptr = unit;
    order->BuildUnitID = type; order->Pos = *pos;
    auto** tail = &unit->UnitOrders;
    while (*tail) tail = &(*tail)->NextOrder;
    *tail = order;
}
static void Remove(UnitStruct* unit, UnitOrdersStruct* victim) {
    auto** next = &unit->UnitOrders;
    while (*next && *next != victim) next = &(*next)->NextOrder;
    assert(*next == victim); *next = victim->NextOrder; ++removedOrders;
}
namespace MexUpgrade {
    bool Enabled();
    bool TryCommand(TAdynmemStruct*, const _MOUSEEVENT*);
}

// PRODUCTION_FUNCTIONS

static void Name(UnitDefStruct& def, const char* name) {
    std::memset(def.UnitName, 0, sizeof(def.UnitName));
    std::strncpy(def.UnitName, name, sizeof(def.UnitName)-1);
}
struct World {
    TAdynmemStruct ta;
    UnitDefStruct defs[12];
    UnitStruct units[8];
    PlayerInfoStruct info;
    short canbuild[3] = {3,4,0};
    std::vector<FeatureStruct> map = std::vector<FeatureStruct>(32*32);
    std::vector<char> yard;
    World(int footX = 7, int footY = 7) : yard(footX * footY, 3) {
        gridAllowed = reclaimAllowed = true; checkingSite = false;
        gridCalls = nativeSends = removedOrders = 0; probedYard.clear(); allocatedOrders.clear();
        shared = SharedData{};
        ta.UnitDef = defs; ta.FeatureMap = map.data();
        ta.BeginUnitsArray_p = units; ta.EndOfUnitsArray_p = units+8;
        auto& owner = ta.Players[0]; owner.Units = units+1; owner.UnitsAry_End = units+7; owner.PlayerInfo = &info;
        Name(defs[1], "ARMACON"); defs[1].bmcode = 1; defs[1].UnitTypeMask_0 = builder;
        defs[1].CANBUILD_ptr = canbuild; defs[1].canbuildCount = 2;
        Name(defs[2], "ARMMOHO"); defs[2].extractsmetal = .003f;
        Name(defs[3], "ARMMAS"); defs[3].extractsmetal = .009f; defs[3].UnitTypeMask_0 = kDefinitionEnabled;
        defs[3].FootX = footX; defs[3].FootY = footY; defs[3].YardMap = yard.data();
        Name(defs[4], "ARMMOHO"); defs[4].extractsmetal = .003f; defs[4].UnitTypeMask_0 = kDefinitionEnabled;
        for (int i : {1,2}) {
            units[i].UnitID = i; units[i].UnitInGameIndex = i; units[i].UnitType = &defs[i];
            units[i].Owner_PlayerPtr0 = &owner; units[i].UnitSelected = 0x10000000;
        }
        units[1].UnitSelected |= UnitSelected_State; units[1].IsUnit = reinterpret_cast<void*>(1);
        units[2].XPos = units[2].YPos = 328; units[2].XGridPos = units[2].YGridPos = 18;
        units[2].SizeFootX = units[2].SizeFootZ = 5;
        for (int y = 18; y < 23; ++y) for (int x = 18; x < 23; ++x) map[y*32+x].occupyingUnitNumber = 2;
        originalYard = yard.data();
    }
    UnitStruct& old() { return units[2]; }
    UnitDefStruct& replacement() { return defs[3]; }
    PlayerStruct& owner() { return ta.Players[0]; }
    Upgrade candidate() { Upgrade out = {}; assert(FindUpgradeTarget(&ta, out)); return out; }
};

static void TestMappingsAndNoTierSkipping() {
    const char* pairs[][2] = {
        {"ARMMEX","ARMMOHO"}, {"CORMEX","CORMOHO"},
        {"ARMUWMEX","ARMUWMOHO"}, {"CORUWMEX","CORUWMOHO"},
        {"ARMMOHO","ARMMAS"}, {"CORMOHO","CORMAS"},
        {"ARMUWMOHO","ARMUWMAS"}, {"CORUWMOHO","CORUWMAS"}
    };
    for (const auto& pair : pairs) {
        UnitDefStruct old; Name(old, pair[0]);
        assert(ReplacementName(old) && std::string(ReplacementName(old)) == pair[1]);
    }
    UnitDefStruct old; Name(old, "armuwmoho"); assert(std::string(ReplacementName(old)) == "ARMUWMAS");
    for (const char* finalName : {"ARMMAS","CORMAS","ARMUWMAS","CORUWMAS","OTHER"}) {
        Name(old, finalName); assert(!ReplacementName(old));
    }
    World world; Name(world.defs[2], "ARMMEX"); world.defs[2].extractsmetal = .001f;
    assert(FindReplacement(world.ta, world.defs[2], world.defs[1], ReplacementName(world.defs[2])) == 4);
}

static void TestBuilderAndDefinitionEligibility() {
    World world; assert(world.candidate().constructor == &world.units[1]);
    assert(FindReplacement(world.ta, world.defs[2], world.defs[1], "ARMMAS") == 3);
    for (int reason = 0; reason < 4; ++reason) {
        World rejected;
        if (reason == 0) rejected.units[1].IsUnit = nullptr;
        if (reason == 1) rejected.defs[1].bmcode = 0;
        if (reason == 2) rejected.defs[1].UnitTypeMask_0 &= ~builder;
        if (reason == 3) rejected.units[1].Nanoframe = .5f;
        Upgrade upgrade = {}; assert(!FindUpgradeTarget(&rejected.ta, upgrade));
    }
    for (int reason = 0; reason < 7; ++reason) {
        World rejected; auto upgrade = rejected.candidate();
        if (reason == 0) rejected.defs[1].CANBUILD_ptr = nullptr;
        if (reason == 1) rejected.defs[1].canbuildCount = 0;
        if (reason == 2) rejected.canbuild[0] = 0;
        if (reason == 3) rejected.defs[3].UnitTypeMask_0 = 0;
        if (reason == 4) rejected.defs[3].extractsmetal = rejected.defs[2].extractsmetal;
        if (reason == 5) rejected.defs[3].bmcode = 1;
        if (reason == 6) rejected.defs[3].buildLimit = 0;
        assert(!PrepareUpgrade(rejected.ta, upgrade, false) && gridCalls == 0);
    }
    World limited; limited.units[5].UnitID = 3; limited.units[5].Nanoframe = .5f;
    assert(!UnderBuildLimit(limited.owner(), 3, 1));
    assert(UnderBuildLimit(limited.owner(), 3, 2) && UnderBuildLimit(limited.owner(), 3, -1));
    assert(!UnderBuildLimit(limited.owner(), 3, 0));
    auto upgrade = limited.candidate(); limited.defs[3].buildLimit = 1;
    assert(!PrepareUpgrade(limited.ta, upgrade, false));
}

static void TestExpandedSiteAndPrivateYard() {
    for (const auto& size : std::vector<std::pair<int,int>>{{7,7},{7,9},{8,6}}) {
        World world(size.first, size.second);
        const auto original = world.yard;
        Position_Dword pos = {}; unsigned blocker = 0;
        assert(CheckReplacementSite(world.ta, world.owner(), world.old(), 2, world.replacement(), pos, blocker));
        assert(gridCalls == 1 && probedSlot == 2 && !checkingSite && world.yard == original);
        int left, top; assert(ReplacementTile(world.old(), world.replacement(), left, top));
        assert(probedPacked == (static_cast<unsigned>(top)<<16 | static_cast<unsigned>(left)));
        assert(pos.X == left*16 + size.first*8 && pos.Y == top*16 + size.second*8 && pos.Z == 77);
        for (int y = 0; y < size.second; ++y) for (int x = 0; x < size.first; ++x) {
            const bool oldFoot = left+x >= 18 && left+x < 23 && top+y >= 18 && top+y < 23;
            assert(probedYard[y*size.first+x] == (oldFoot ? 2 : 3));
        }
    }
    World ownOutside; int left, top;
    assert(ReplacementTile(ownOutside.old(), ownOutside.replacement(), left, top));
    ownOutside.map[top*32+left].occupyingUnitNumber = 2;
    Position_Dword pos = {}; unsigned blocker = 0;
    assert(CheckReplacementSite(ownOutside.ta, ownOutside.owner(), ownOutside.old(), 2, ownOutside.replacement(), pos, blocker));
    assert(probedYard[0] == 3); // Same slot outside its real footprint is not masked.

    World blocked; const auto original = blocked.yard;
    assert(ReplacementTile(blocked.old(), blocked.replacement(), left, top));
    blocked.map[top*32+left].occupyingUnitNumber = 99;
    assert(!CheckReplacementSite(blocked.ta, blocked.owner(), blocked.old(), 2, blocked.replacement(), pos, blocker));
    assert(blocker == 99 && gridCalls == 0 && blocked.yard == original);
    World terrain; gridAllowed = false;
    assert(!CheckReplacementSite(terrain.ta, terrain.owner(), terrain.old(), 2, terrain.replacement(), pos, blocker));
    assert(gridCalls == 1 && !checkingSite);
}

static void TestAlignmentAndEdges() {
    for (int center : {320,328}) for (int width : {6,7}) for (int height : {8,9}) {
        World world(width,height); world.old().XPos = world.old().YPos = center;
        int left = -1, top = -1;
        assert(ReplacementTile(world.old(), world.replacement(), left, top));
        assert(left == (center+8-width*8)/16 && top == (center+8-height*8)/16);
    }
    for (const auto& size : std::vector<std::pair<int,int>>{{0,7},{7,0},{-1,7},{7,-1},
        {kMaxUpgradeFootprint+1,7},{7,kMaxUpgradeFootprint+1}}) {
        World world; world.defs[3].FootX = size.first; world.defs[3].FootY = size.second;
        int left,top; assert(!ReplacementTile(world.old(), world.replacement(), left, top));
    }
    World maximum; maximum.defs[3].FootX = maximum.defs[3].FootY = kMaxUpgradeFootprint;
    int left, top; assert(ReplacementTile(maximum.old(), maximum.replacement(), left, top));
    for (int edge : {0,1,2}) {
        World world; Position_Dword pos = {}; unsigned blocker = 0;
        if (edge == 0) world.old().XPos = 8;
        if (edge == 1) world.old().XPos = 510;
        if (edge == 2) world.old().YPos = 510;
        assert(!CheckReplacementSite(world.ta, world.owner(), world.old(), 2, world.replacement(), pos, blocker));
        assert(gridCalls == 0);
    }
}

static void TestShiftCancellationBeforeBlockedPreparation() {
    World world; _MOUSEEVENT click;
    assert(MexUpgrade::TryCommand(&world.ta, &click));
    assert(nativeSends == 2 && world.units[1].UnitOrders && world.units[1].UnitOrders->NextOrder);
    assert(world.units[1].UnitOrders->COBHandler_index == 11);
    assert(world.units[1].UnitOrders->NextOrder->BuildUnitID == 3);
    const int before = gridCalls;
    int left, top; assert(ReplacementTile(world.old(), world.replacement(), left, top));
    world.map[top*32+left].occupyingUnitNumber = 99;
    world.defs[3].buildLimit = 0; // Even an unavailable replacement may be cancelled.
    click.fwKeys = MK_SHIFT;
    assert(MexUpgrade::TryCommand(&world.ta, &click));
    assert(removedOrders == 2 && !world.units[1].UnitOrders && nativeSends == 2 && gridCalls == before);
    assert(!MexUpgrade::TryCommand(&world.ta, &click));
    assert(nativeSends == 2 && removedOrders == 2);

    World right; right.ta.InterfaceType = 1; right.ta.BuildSpotState |= 4;
    click.Msg = WM_RBUTTONDOWN; click.fwKeys = 0;
    assert(MexUpgrade::TryCommand(&right.ta, &click) && nativeSends == 2);
}

int main(int argc, char** argv) {
    assert(argc == 2); config.active = std::string(argv[1]) == "1";
    assert(!MexUpgrade::Enabled()); // Missing config does not initialize the static cache.
    MyConfig = &config;
#if defined(TDRAW_CONFIG_ESCALATION)
    const bool enabled = config.active;
    assert(MexUpgrade::Enabled() == enabled && config.reads == 1);
#else
    const bool enabled = false;
    assert(!MexUpgrade::Enabled() && config.reads == 0);
#endif
    TestMappingsAndNoTierSkipping();
    TestBuilderAndDefinitionEligibility();
    TestExpandedSiteAndPrivateYard();
    TestAlignmentAndEdges();
    if (enabled) TestShiftCancellationBeforeBlockedPreparation();
    else {
        World world; _MOUSEEVENT click;
        assert(!MexUpgrade::TryCommand(&world.ta, &click) && nativeSends == 0 && gridCalls == 0);
    }
    std::cout << "Extractor upgrade regression tests passed (enabled=" << enabled << ")\n";
}
'''


def main():
    source = (DDRAW / "MexUpgrade.cpp").read_text()
    constants = "\n".join(re.findall(r'^\s*constexpr (?:unsigned|int) k(?:DefinitionEnabled|MaxOrders|MaxUpgradeFootprint)\s*=.*?;', source, re.M))
    table = extract(source, "constexpr UpgradePair kUpgrades[]") + ";"
    signatures = (
        "bool LiveOwned(", "UnitStruct* SingleSelectedUnit(", "const char* ReplacementName(",
        "int FindReplacement(", "bool UnderBuildLimit(", "bool ReplacementTile(",
        "bool CheckReplacementSite(", "bool FindUpgradeTarget(", "bool PrepareUpgrade(",
        "UnitOrdersStruct* FindReclaim(", "bool IsBuildAt(", "bool IsReplacementBuild(",
        "bool CancelUpgrade(", "bool QueueUpgrade(", "bool MexUpgrade::Enabled()", "bool MexUpgrade::TryCommand(",
    )
    methods = "\n\n".join(extract(source, signature) for signature in signatures)
    bridge = "reinterpret_cast<int (__cdecl*)()>(0x0047C780u)"
    if methods.count(bridge) != 1:
        raise RuntimeError("Expected one native grid-height bridge to mock")
    methods = methods.replace(bridge, "MockGridHeight")
    harness = HARNESS.replace("// PRODUCTION_CONSTANTS_AND_TABLE", constants + "\n" + table).replace(
        "// PRODUCTION_FUNCTIONS", methods)
    with tempfile.TemporaryDirectory(prefix="mex-upgrade-test-") as temp:
        cpp = Path(temp) / "mex_upgrade_test.cpp"
        cpp.write_text(harness)
        env = os.environ.copy()
        env["ASAN_OPTIONS"] = env.get("ASAN_OPTIONS", "") + ":detect_leaks=0"
        for mod in ("ESCALATION", "PROTA", "OTA"):
            binary = Path(temp) / ("mex_upgrade_" + mod.lower())
            subprocess.run(shlex.split(os.environ.get("CXX", "g++")) + [
                "-std=c++11", "-Wall", "-Wextra", "-Werror", "-g",
                "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                "-DTDRAW_CONFIG_" + mod, str(cpp), "-o", str(binary),
            ], check=True)
            for configured in ("1", "0"):
                subprocess.run([str(binary), configured], check=True, env=env)


if __name__ == "__main__":
    main()
