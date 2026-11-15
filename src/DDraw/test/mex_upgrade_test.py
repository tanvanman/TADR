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
constexpr unsigned WM_LBUTTONUP = 0x202, WM_RBUTTONDOWN = 0x204, VK_CONTROL = 0x11, VK_MENU = 0x12;
namespace ordertype { constexpr int STOP = 1; }
struct Position_Dword { std::uint16_t x_, X, z_, Z, y_, Y; };
struct GAFFrame {
    unsigned short Width, Height, xPosition, yPosition;
    unsigned char Background, Compressed;
    unsigned short FramePointers;
    unsigned Unknown0;
    unsigned char* PtrFrameBits;
    unsigned FPS;
};
struct GAFENTRY { GAFFrame* PtrFrame; int Animated; };
struct GAFSequence {
    unsigned short Frames;
    unsigned Signature;
    unsigned short Padding;
    char Name[32];
    GAFENTRY PtrFrameAry[1];
};
struct DesktopGUI { void* Cursor_3 = nullptr; };
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
    unsigned CurrentCursora_Index = 0;
    GAFSequence* cursor_ary[4] = {};
    DesktopGUI desktopGUI;
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
static bool controlDown, altDown;
static int cursorChanges;
static short GetAsyncKeyState(unsigned key) {
    return ((key == VK_CONTROL && controlDown) || (key == VK_MENU && altDown)) ? -32768 : 0;
}
static void SetUICursor(DesktopGUI* desktop, GAFSequence* cursor) {
    desktop->Cursor_3 = cursor; ++cursorChanges;
}
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
static std::vector<unsigned> allowedGridSites, probedSites;
struct SiteProbe {
    bool previous = checkingSite;
    SiteProbe() { checkingSite = true; }
    ~SiteProbe() { checkingSite = previous; }
};
static int MockGridHeight() { return 77; }
static int TestGridSpot(UnitDefStruct* preview, unsigned packed, unsigned slot, PlayerStruct*) {
    assert(checkingSite && preview->YardMap != originalYard);
    ++gridCalls; probedPacked = packed; probedSlot = slot;
    probedSites.push_back(packed);
    probedYard.assign(preview->YardMap, preview->YardMap + preview->FootX * preview->FootY);
    return gridAllowed && (allowedGridSites.empty()
        || std::find(allowedGridSites.begin(), allowedGridSites.end(), packed) != allowedGridSites.end());
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
    bool IsQueuedBuild(const TAdynmemStruct*, const UnitStruct*, const UnitOrdersStruct*);
    void UpdateCursor(TAdynmemStruct*);
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
    GAFSequence stockCursor = {}, otherCursor = {};
    World(int footX = 7, int footY = 7) : yard(footX * footY, 3) {
        gridAllowed = reclaimAllowed = true; checkingSite = false;
        gridCalls = nativeSends = removedOrders = 0; probedYard.clear(); allocatedOrders.clear();
        allowedGridSites.clear(); probedSites.clear();
        controlDown = altDown = false; cursorChanges = 0;
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
        ta.cursor_ary[0] = &stockCursor; ta.desktopGUI.Cursor_3 = &stockCursor;
    }
    UnitStruct& old() { return units[2]; }
    UnitDefStruct& replacement() { return defs[3]; }
    PlayerStruct& owner() { return ta.Players[0]; }
    Upgrade candidate() { Upgrade out = {}; assert(FindUpgradeTarget(&ta, out)); return out; }
};

static unsigned Packed(int left, int top) {
    return (static_cast<unsigned>(top) << 16) | static_cast<unsigned>(left);
}

static void WindmillCorner(World& world) {
    // Adjacent windmills block the centered footprint, leaving only the
    // upper-right corner placement while the source footprint stays covered.
    const auto& old = world.old();
    for (int y = old.YGridPos; y < old.YGridPos+old.SizeFootZ; ++y)
        world.map[y*32+old.XGridPos-1].occupyingUnitNumber = 99;
    for (int x = old.XGridPos; x < old.XGridPos+old.SizeFootX; ++x)
        world.map[(old.YGridPos+old.SizeFootZ)*32+x].occupyingUnitNumber = 99;
}

static void BlockAllLargerSites(World& world) {
    for (int y = 0; y < 32; ++y) for (int x = 0; x < 32; ++x)
        world.map[y*32+x].occupyingUnitNumber = (x >= 18 && x < 23 && y >= 18 && y < 23) ? 2 : 99;
}

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
    assert(CheckReplacementTile(ownOutside.ta, ownOutside.owner(), ownOutside.old(), 2,
        ownOutside.replacement(), left, top, pos, blocker));
    assert(probedYard[0] == 3); // Same slot outside its real footprint is not masked.

    World blocked; const auto original = blocked.yard;
    assert(ReplacementTile(blocked.old(), blocked.replacement(), left, top));
    blocked.map[top*32+left].occupyingUnitNumber = 99;
    assert(!CheckReplacementTile(blocked.ta, blocked.owner(), blocked.old(), 2,
        blocked.replacement(), left, top, pos, blocker));
    assert(blocker == 99 && gridCalls == 0 && blocked.yard == original);
    World terrain; gridAllowed = false;
    assert(!CheckReplacementSite(terrain.ta, terrain.owner(), terrain.old(), 2, terrain.replacement(), pos, blocker));
    assert(gridCalls >= 1 && !checkingSite);
}

static void TestOffsetSites() {
    // Clear space keeps the existing centered position, even though every
    // corner could also cover the source metal extractor.
    World centered; Position_Dword pos = {}; unsigned blocker = 0;
    assert(CheckReplacementSite(centered.ta, centered.owner(), centered.old(), 2,
        centered.replacement(), pos, blocker));
    assert(pos.X == 328 && pos.Y == 328 && gridCalls == 1);

    World corner; WindmillCorner(corner); const auto original = corner.yard;
    assert(CheckReplacementSite(corner.ta, corner.owner(), corner.old(), 2,
        corner.replacement(), pos, blocker));
    assert(pos.X == 344 && pos.Y == 312 && pos.Z == 77);
    assert(probedPacked == Packed(18,16) && gridCalls == 1 && corner.yard == original && !checkingSite);
    assert(ReplacementContainsTarget(corner.old(), corner.replacement(), 18,16));
    for (int y = 0; y < 7; ++y) for (int x = 0; x < 7; ++x) {
        const bool source = x < 5 && y >= 2;
        assert(probedYard[y*7+x] == (source ? 2 : 3));
    }

    // Native terrain/depth checks can reject an otherwise empty center.
    // Prefer the nearest valid candidate over a diagonal candidate that
    // would be encountered first by a simple row-major search.
    World nearest; allowedGridSites = {Packed(18,17),Packed(16,16)};
    assert(CheckReplacementSite(nearest.ta, nearest.owner(), nearest.old(), 2,
        nearest.replacement(), pos, blocker));
    assert(pos.X == 344 && pos.Y == 328 && probedSites.front() == Packed(17,17));

    // Equal distances resolve in a fixed order (top, then left), independent
    // of the build/menu/occupancy iteration order.
    World tie; allowedGridSites = {Packed(18,17),Packed(17,18),Packed(16,17),Packed(17,16)};
    assert(CheckReplacementSite(tie.ta, tie.owner(), tie.old(), 2,
        tie.replacement(), pos, blocker));
    assert(pos.X == 328 && pos.Y == 312);

    World impossible; BlockAllLargerSites(impossible); const auto impossibleYard = impossible.yard;
    assert(!CheckReplacementSite(impossible.ta, impossible.owner(), impossible.old(), 2,
        impossible.replacement(), pos, blocker));
    assert(gridCalls == 0 && blocker == 99 && impossible.yard == impossibleYard);

    // A free footprint beside the metal spot is not a fallback. Every
    // fallback must still contain the complete footprint of the old mex.
    World uncovered; BlockAllLargerSites(uncovered);
    for (int y = 18; y < 25; ++y) for (int x = 19; x < 26; ++x)
        uncovered.map[y*32+x].occupyingUnitNumber = 0;
    assert(!CheckReplacementSite(uncovered.ta, uncovered.owner(), uncovered.old(), 2,
        uncovered.replacement(), pos, blocker));
    assert(!ReplacementContainsTarget(uncovered.old(), uncovered.replacement(), 19,18));
}

static void TestAllPairsOffsetFootprints() {
    const char* pairs[][2] = {
        {"ARMMEX","ARMMOHO"}, {"CORMEX","CORMOHO"},
        {"ARMUWMEX","ARMUWMOHO"}, {"CORUWMEX","CORUWMOHO"},
        {"ARMMOHO","ARMMAS"}, {"CORMOHO","CORMAS"},
        {"ARMUWMOHO","ARMUWMAS"}, {"CORUWMOHO","CORUWMAS"}
    };
    for (unsigned pair = 0; pair < 8; ++pair) {
        const int sourceFoot = pair < 4 ? 3 : 5, replacementFoot = sourceFoot+2;
        World world(replacementFoot,replacementFoot);
        Name(world.defs[2],pairs[pair][0]); Name(world.defs[3],pairs[pair][1]);
        world.old().SizeFootX = world.old().SizeFootZ = sourceFoot;
        world.old().XPos = world.old().YPos = 18*16+sourceFoot*8;
        world.map.assign(32*32,FeatureStruct{}); world.ta.FeatureMap = world.map.data();
        for (int y = 18; y < 18+sourceFoot; ++y) for (int x = 18; x < 18+sourceFoot; ++x)
            world.map[y*32+x].occupyingUnitNumber = 2;
        WindmillCorner(world); auto upgrade = world.candidate(); const auto original = world.yard;
        assert(PrepareUpgrade(world.ta,upgrade,false));
        assert(upgrade.type == 3 && upgrade.buildPos.X == 18*16+replacementFoot*8
            && upgrade.buildPos.Y == 16*16+replacementFoot*8);
        assert(world.yard == original && gridCalls == 1);
    }
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
    for (int edge : {0,1,2,3}) {
        World world; Position_Dword pos = {}; unsigned blocker = 0;
        world.map.assign(32*32, FeatureStruct{}); world.ta.FeatureMap = world.map.data();
        world.old().XGridPos = (edge == 0 || edge == 2) ? 0 : 27;
        world.old().YGridPos = (edge == 0 || edge == 1) ? 0 : 27;
        world.old().XPos = world.old().XGridPos*16+40;
        world.old().YPos = world.old().YGridPos*16+40;
        for (int y = world.old().YGridPos; y < world.old().YGridPos+5; ++y)
            for (int x = world.old().XGridPos; x < world.old().XGridPos+5; ++x)
                world.map[y*32+x].occupyingUnitNumber = 2;
        assert(CheckReplacementSite(world.ta, world.owner(), world.old(), 2, world.replacement(), pos, blocker));
        assert(pos.X == ((edge == 0 || edge == 2) ? 56 : 456));
        assert(pos.Y == ((edge == 0 || edge == 1) ? 56 : 456));
        assert(gridCalls == 1);
    }
    World tooSmall; tooSmall.ta.FeatureMapSizeX = 6;
    tooSmall.old().XGridPos = 0; tooSmall.old().XPos = 40;
    Position_Dword pos = {}; unsigned blocker = 0;
    assert(!CheckReplacementSite(tooSmall.ta, tooSmall.owner(), tooSmall.old(), 2,
        tooSmall.replacement(), pos, blocker) && gridCalls == 0);
}

static void TestOffsetOrderRecognition() {
    World world; auto upgrade = world.candidate(); WindmillCorner(world);
    assert(PrepareUpgrade(world.ta, upgrade, false));
    UnitOrdersStruct build; build.BuildUnitID = 3; build.Pos = upgrade.buildPos;
    assert(IsReplacementBuild(world.ta, build, world.old(), upgrade.replacement));
    for (int reason = 0; reason < 8; ++reason) {
        auto unrelated = build;
        if (reason == 0) unrelated.Pos.X += 16; // Loses the source's left edge.
        if (reason == 1) unrelated.Pos.Y -= 16; // Loses the source's bottom edge.
        if (reason == 2) unrelated.Pos.X += 1; // Not aligned to TA's build grid.
        if (reason == 3) unrelated.Pos.x_ = 1;
        if (reason == 4) unrelated.Pos.y_ = 1;
        if (reason == 5) unrelated.BuildUnitID = 4; // Different extractor tier.
        if (reason == 6) unrelated.State = 1;
        if (reason == 7) unrelated.AttackTargat = &world.old();
        assert(!IsReplacementBuild(world.ta, unrelated, world.old(), upgrade.replacement));
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

static void TestOffsetQueueAndCancellation() {
    for (int interface : {0,1}) for (bool queued : {false,true}) {
        World world; WindmillCorner(world);
        world.ta.InterfaceType = interface;
        if (interface) world.ta.BuildSpotState |= 4;
        _MOUSEEVENT click; click.Msg = interface ? WM_RBUTTONDOWN : WM_LBUTTONUP;
        UnitOrdersStruct* previous = nullptr;
        if (queued) {
            Position_Dword pos = {}; Send(99,0,&world.units[1],nullptr,&pos,0,0);
            previous = world.units[1].UnitOrders; click.fwKeys = MK_SHIFT;
        }
        assert(MexUpgrade::TryCommand(&world.ta, &click));
        auto* reclaim = previous ? previous->NextOrder : world.units[1].UnitOrders;
        assert(reclaim && reclaim->COBHandler_index == 11);
        auto* build = reclaim->NextOrder;
        assert(build && build->BuildUnitID == 3 && build->Pos.X == 344 && build->Pos.Y == 312);
        assert(MexUpgrade::IsQueuedBuild(&world.ta,&world.units[1],reclaim));
        const int probes = gridCalls, sends = nativeSends;
        // Cancellation recognizes the saved shifted order, without finding
        // a new site or changing an unrelated preceding order.
        BlockAllLargerSites(world); world.defs[3].buildLimit = 0; click.fwKeys = MK_SHIFT;
        assert(MexUpgrade::TryCommand(&world.ta, &click));
        assert(removedOrders == 2 && gridCalls == probes && nativeSends == sends);
        assert(world.units[1].UnitOrders == previous && (!previous || !previous->NextOrder));
    }

    World world; auto upgrade = world.candidate();
    Position_Dword pos = {}; pos.X = 360; pos.Y = 328;
    Send(11,0,&world.units[1],&world.old(),&pos,0,0);
    Send(12,1,&world.units[1],nullptr,&pos,3,0);
    auto* reclaim = world.units[1].UnitOrders;
    // An adjacent build that does not fully cover the old metal spot is not
    // an upgrade pair and cannot be removed or trigger upgrade protection.
    assert(!CancelUpgrade(world.ta,upgrade,UpgradeActions(world.units[1])));
    assert(!MexUpgrade::IsQueuedBuild(&world.ta,&world.units[1],reclaim));
    assert(removedOrders == 0 && world.units[1].UnitOrders == reclaim && reclaim->NextOrder);
}

static GAFSequence* DisplayedCursor(World& world) {
    return reinterpret_cast<GAFSequence*>(world.ta.desktopGUI.Cursor_3);
}

static int CursorPixels(GAFSequence* cursor, unsigned char color) {
    assert(cursor && cursor->Frames == 1 && cursor->Signature == 1);
    const auto* frame = cursor->PtrFrameAry[0].PtrFrame;
    assert(frame && frame->Width == 13 && frame->Height == 14 && frame->Background == 9);
    assert(!frame->Compressed && frame->PtrFrameBits);
    return std::count(frame->PtrFrameBits,frame->PtrFrameBits+13*14,color);
}

static void AssertBlockedCursor(GAFSequence* cursor) {
    assert(CursorPixels(cursor,211) > 0 && CursorPixels(cursor,232) == 0);
    const auto* frame = cursor->PtrFrameAry[0].PtrFrame;
    assert(frame->xPosition == 6 && frame->yPosition == 6);
    // A red X has branches at both upper corners and its center, rather
    // than the up-arrow's colored tip at the top center.
    assert(frame->PtrFrameBits[0] != 9 && frame->PtrFrameBits[12] != 9);
    assert(frame->PtrFrameBits[6] == 9 && frame->PtrFrameBits[6*13+6] == 211);
}

static void TestUpgradeCursorTransitions() {
    for (int interface : {0,1}) {
        World world; world.ta.InterfaceType = interface;
        if (interface) world.ta.BuildSpotState |= 4;
        MexUpgrade::UpdateCursor(&world.ta);
        auto* green = DisplayedCursor(world);
        assert(green != &world.stockCursor && CursorPixels(green,232) > 0);
        assert(green->PtrFrameAry[0].PtrFrame->xPosition == 6
            && green->PtrFrameAry[0].PtrFrame->yPosition == 0);
        assert(nativeSends == 0 && removedOrders == 0 && !world.units[1].UnitOrders);
        const int changes = cursorChanges;
        MexUpgrade::UpdateCursor(&world.ta);
        assert(DisplayedCursor(world) == green && cursorChanges == changes);

        WindmillCorner(world); MexUpgrade::UpdateCursor(&world.ta);
        assert(DisplayedCursor(world) == green && nativeSends == 0);
        BlockAllLargerSites(world); MexUpgrade::UpdateCursor(&world.ta);
        auto* red = DisplayedCursor(world);
        assert(red != green && red != &world.stockCursor);
        AssertBlockedCursor(red);
        assert(nativeSends == 0 && removedOrders == 0 && !world.units[1].UnitOrders);
        const int blockedChanges = cursorChanges;
        MexUpgrade::UpdateCursor(&world.ta);
        assert(DisplayedCursor(world) == red && cursorChanges == blockedChanges);

        // Modifier keys restore the native cursor and never probe or issue
        // orders. Releasing them restores the blocked-upgrade cursor.
        const int probes = gridCalls;
        controlDown = true; MexUpgrade::UpdateCursor(&world.ta);
        assert(DisplayedCursor(world) == &world.stockCursor && gridCalls == probes);
        controlDown = false; altDown = true; MexUpgrade::UpdateCursor(&world.ta);
        assert(DisplayedCursor(world) == &world.stockCursor && gridCalls == probes);
        altDown = false; MexUpgrade::UpdateCursor(&world.ta);
        assert(DisplayedCursor(world) == red);

        std::fill(world.map.begin(),world.map.end(),FeatureStruct{});
        MexUpgrade::UpdateCursor(&world.ta);
        assert(DisplayedCursor(world) == green);
        gridAllowed = false; MexUpgrade::UpdateCursor(&world.ta);
        assert(DisplayedCursor(world) == red && nativeSends == 0);
        gridAllowed = true; MexUpgrade::UpdateCursor(&world.ta);
        assert(DisplayedCursor(world) == green);

        // Wreck snapping and other patches own their custom cursors.
        world.ta.desktopGUI.Cursor_3 = &world.otherCursor;
        const int beforeProbes = gridCalls, beforeChanges = cursorChanges;
        MexUpgrade::UpdateCursor(&world.ta);
        assert(DisplayedCursor(world) == &world.otherCursor && gridCalls == beforeProbes
            && cursorChanges == beforeChanges);
        world.ta.desktopGUI.Cursor_3 = &world.stockCursor;
        world.ta.CurrentCursora_Index = 4;
        MexUpgrade::UpdateCursor(&world.ta);
        assert(DisplayedCursor(world) == &world.stockCursor && cursorChanges == beforeChanges);
        world.ta.CurrentCursora_Index = 0; world.ta.cursor_ary[0] = nullptr;
        MexUpgrade::UpdateCursor(&world.ta);
        assert(DisplayedCursor(world) == &world.stockCursor && cursorChanges == beforeChanges);
    }
}

static void TestUpgradeCursorContextAndCapability() {
    for (int reason = 0; reason < 15; ++reason) {
        World world; MexUpgrade::UpdateCursor(&world.ta);
        assert(DisplayedCursor(world) != &world.stockCursor);
        const int probes = gridCalls;
        if (reason == 0) Name(world.defs[2],"ARMMAS"); // No further upgrade tier.
        if (reason == 1) world.defs[2].extractsmetal = 0;
        if (reason == 2) world.defs[1].CANBUILD_ptr = nullptr;
        if (reason == 3) world.canbuild[0] = 0;
        if (reason == 4) world.info.PropertyMask |= WATCH;
        if (reason == 5) world.old().Owner_PlayerPtr0 = &world.ta.Players[1];
        if (reason == 6) world.old().Nanoframe = .5f;
        if (reason == 7) world.old().UnitSelected |= UnitSelected_State;
        if (reason == 8) controlDown = true;
        if (reason == 9) altDown = true;
        if (reason == 10) shared.PlayingDemo = true;
        if (reason == 11) world.ta.PrepareOrder_Type = 99;
        if (reason == 12) world.ta.InterfaceType = 1; // No command mode bit.
        if (reason == 13) world.defs[3].UnitTypeMask_0 = 0;
        if (reason == 14) world.defs[1].UnitTypeMask_0 &= ~builder;
        MexUpgrade::UpdateCursor(&world.ta);
        assert(DisplayedCursor(world) == &world.stockCursor && gridCalls == probes);
        assert(nativeSends == 0 && removedOrders == 0 && !world.units[1].UnitOrders);
    }
    for (int reason : {0,1}) {
        World world; MexUpgrade::UpdateCursor(&world.ta);
        auto* green = DisplayedCursor(world);
        if (reason == 0) reclaimAllowed = false;
        if (reason == 1) world.defs[3].buildLimit = 0;
        MexUpgrade::UpdateCursor(&world.ta);
        auto* red = DisplayedCursor(world);
        assert(red != green && red != &world.stockCursor);
        AssertBlockedCursor(red);
        assert(nativeSends == 0 && removedOrders == 0);
    }
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
    TestOffsetSites();
    TestAllPairsOffsetFootprints();
    TestAlignmentAndEdges();
    TestOffsetOrderRecognition();
    if (enabled) {
        TestShiftCancellationBeforeBlockedPreparation();
        TestOffsetQueueAndCancellation();
        TestUpgradeCursorTransitions();
        TestUpgradeCursorContextAndCapability();
    }
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
    table += "\n\n" + extract(source,"struct UpgradeCursor") + ";"
    signatures = (
        "bool LiveOwned(", "UnitStruct* SingleSelectedUnit(", "const char* ReplacementName(",
        "int FindReplacement(", "bool UnderBuildLimit(", "bool ReplacementTile(",
        "bool ReplacementContainsTarget(", "bool CheckReplacementTile(", "bool CheckReplacementSite(",
        "bool FindUpgradeTarget(", "bool PrepareUpgrade(",
        "UnitOrdersStruct* FindReclaim(", "bool IsBuildAt(", "bool IsReplacementBuild(",
        "bool CancelUpgrade(", "bool QueueUpgrade(", "bool MexUpgrade::Enabled()",
        "bool MexUpgrade::IsQueuedBuild(", "bool MexUpgrade::TryCommand(", "void MexUpgrade::UpdateCursor(",
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
