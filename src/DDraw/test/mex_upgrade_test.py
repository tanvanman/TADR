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
#include "OptionalUnitControls.h"
#include "MexUpgradeMenu.h"
#if !MEX_UPGRADE_ENABLE
#error "The behavior harness requires compiled-in extractor upgrades"
#endif
#define _strnicmp strncasecmp
#define __stdcall
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
struct _OFFSCREEN { void* lpSurface = nullptr; };
struct UnitDefStruct {
    char UnitName[32] = {};
    short* CANBUILD_ptr = nullptr;
    int canbuildCount = 0, bmcode = 0, FootX = 1, FootY = 1, buildLimit = -1, GUINum = 2;
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
    unsigned GameTime = 0;
    void* CameraToUnit = nullptr;
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
static unsigned queuedGestureLogs, cancelledGestureLogs;
struct IDDrawSurface {
    static void OutptTxt(const char*) {}
    template<class... Args> static void OutptFmtTxt(const char* format, Args...) {
        if (std::strstr(format,"[MexUpgrade] queued ") == format) ++queuedGestureLogs;
        if (std::strstr(format,"[MexUpgrade] cancelled ") == format) ++cancelledGestureLogs;
    }
};
static bool controlDown, altDown;
static int cursorChanges, feedbackDrawCalls;
static int MockFeedbackKeyDown(int key) { assert(key == 0xF9); return 0; }
static void MockDrawBuildSpotQueue(_OFFSCREEN* surface, const void*, UnitOrdersStruct* order, Position_Dword*, int) {
    assert(surface && surface->lpSurface && order && order->BuildUnitID);
    ++feedbackDrawCalls;
}
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

static std::vector<std::string> buildMenuPages;
static int buildMenuCalls;
bool HasBuildMenuEntry(const TAdynmemStruct&, const UnitDefStruct& constructor, const char* name) {
    ++buildMenuCalls;
    for (int page=0; page < constructor.GUINum && page < static_cast<int>(buildMenuPages.size()); ++page) {
        const auto& data = buildMenuPages[page];
        if (MexUpgradeMenu::ContainsBuildButton(data.data(),data.size(),name)) return true;
    }
    return false;
}
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
    void DrawFeedback(TAdynmemStruct*, _OFFSCREEN*);
}

// PRODUCTION_FUNCTIONS

static void Name(UnitDefStruct& def, const char* name) {
    std::memset(def.UnitName, 0, sizeof(def.UnitName));
    std::strncpy(def.UnitName, name, sizeof(def.UnitName)-1);
}
static std::string BuildButton(const char* name) {
    return std::string("[GADGET8]{[COMMON]{id=1;assoc=0;name=") + name
        + ";xpos=0;ypos=128;width=64;height=64;attribs=32;colorf=15;colorb=0;"
          "texturenumber=0;fontnumber=0;active=1;commonattribs=4;}quickkey=113;}";
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
        buildMenuCalls = 0; buildMenuPages = {"",BuildButton("ARMMAS") + BuildButton("ARMMOHO")};
        controlDown = altDown = false; cursorChanges = feedbackDrawCalls = 0; feedback = {};
        shared = SharedData{};
        ta.UnitDef = defs; ta.FeatureMap = map.data();
        ta.BeginUnitsArray_p = units; ta.EndOfUnitsArray_p = units+7; // Native range is inclusive.
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

// Complete build-button blocks copied from the installed Escalation GUIe pages.
// Includes vehicle, hover, submarine, ship and seaplane menus omitted by AI plans.
struct RealGuiCase { const char* constructor; const char* replacement; int page; const char* gui; };
static const RealGuiCase kRealMenus[] = {
    // TAESC.gp3/guie/armaca1.gui
    {"ARMACA","ARMMOHO",1,"[GADGET8]\r\n{\r\n\t[COMMON]\r\n\t{\r\n\t\tid=1;\r\n\t\tassoc=0;\r\n\t\tname=ARMMOHO;\r\n\t\txpos=0;\r\n\t\typos=128;\r\n\t\twidth=64;\r\n\t\theight=64;\r\n\t\tattribs=32;\r\n\t\tcolorf=15;\r\n\t\tcolorb=0;\r\n\t\ttexturenumber=0;\r\n\t\tfontnumber=0;\r\n\t\tactive=1;\r\n\t\tcommonattribs=4;\r\n\t}\r\n\tquickkey=113;\r\n}"},
    // TAESC.gp3/guie/armack1.gui
    {"ARMACK","ARMMOHO",1,"[GADGET8]\r\n{\r\n\t[COMMON]\r\n\t{\r\n\t\tid=1;\r\n\t\tassoc=0;\r\n\t\tname=ARMMOHO;\r\n\t\txpos=0;\r\n\t\typos=128;\r\n\t\twidth=64;\r\n\t\theight=64;\r\n\t\tattribs=32;\r\n\t\tcolorf=15;\r\n\t\tcolorb=0;\r\n\t\ttexturenumber=0;\r\n\t\tfontnumber=0;\r\n\t\tactive=1;\r\n\t\tcommonattribs=4;\r\n\t}\r\n\tquickkey=113;\r\n}"},
    // TAESC.gp3/guie/armacsub1.gui
    {"ARMACSUB","ARMUWMOHO",1,"[GADGET8]\r\n{\r\n\t[COMMON]\r\n\t{\r\n\t\tid=1;\r\n\t\tassoc=0;\r\n\t\tname=ARMUWMOHO;\r\n\t\txpos=0;\r\n\t\typos=128;\r\n\t\twidth=64;\r\n\t\theight=64;\r\n\t\tattribs=32;\r\n\t\tcolorf=15;\r\n\t\tcolorb=0;\r\n\t\ttexturenumber=0;\r\n\t\tfontnumber=0;\r\n\t\tactive=1;\r\n\t\tcommonattribs=4;\r\n\t}\r\n\tquickkey=113;\r\n}"},
    // TAESC.gp3/guie/armacv1.gui
    {"ARMACV","ARMMOHO",1,"[GADGET8]\r\n{\r\n\t[COMMON]\r\n\t{\r\n\t\tid=1;\r\n\t\tassoc=0;\r\n\t\tname=ARMMOHO;\r\n\t\txpos=0;\r\n\t\typos=128;\r\n\t\twidth=64;\r\n\t\theight=64;\r\n\t\tattribs=32;\r\n\t\tcolorf=15;\r\n\t\tcolorb=0;\r\n\t\ttexturenumber=0;\r\n\t\tfontnumber=0;\r\n\t\tactive=1;\r\n\t\tcommonattribs=4;\r\n\t}\r\n\tquickkey=113;\r\n}"},
    // TAESC.gp3/guie/coraca1.gui
    {"CORACA","CORMOHO",1,"[GADGET8]\r\n{\r\n\t[COMMON]\r\n\t{\r\n\t\tid=1;\r\n\t\tassoc=0;\r\n\t\tname=CORMOHO;\r\n\t\txpos=0;\r\n\t\typos=128;\r\n\t\twidth=64;\r\n\t\theight=64;\r\n\t\tattribs=32;\r\n\t\tcolorf=15;\r\n\t\tcolorb=0;\r\n\t\ttexturenumber=0;\r\n\t\tfontnumber=0;\r\n\t\tactive=1;\r\n\t\tcommonattribs=4;\r\n\t}\r\n\tquickkey=113;\r\n}"},
    // TAESC.gp3/guie/corack1.gui
    {"CORACK","CORMOHO",1,"[GADGET8]\r\n{\r\n\t[COMMON]\r\n\t{\r\n\t\tid=1;\r\n\t\tassoc=0;\r\n\t\tname=CORMOHO;\r\n\t\txpos=0;\r\n\t\typos=128;\r\n\t\twidth=64;\r\n\t\theight=64;\r\n\t\tattribs=32;\r\n\t\tcolorf=15;\r\n\t\tcolorb=0;\r\n\t\ttexturenumber=0;\r\n\t\tfontnumber=0;\r\n\t\tactive=1;\r\n\t\tcommonattribs=4;\r\n\t}\r\n\tquickkey=113;\r\n}"},
    // TAESC.gp3/guie/coracsub1.gui
    {"CORACSUB","CORUWMOHO",1,"[GADGET8]\r\n{\r\n\t[COMMON]\r\n\t{\r\n\t\tid=1;\r\n\t\tassoc=0;\r\n\t\tname=CORUWMOHO;\r\n\t\txpos=0;\r\n\t\typos=128;\r\n\t\twidth=64;\r\n\t\theight=64;\r\n\t\tattribs=32;\r\n\t\tcolorf=15;\r\n\t\tcolorb=0;\r\n\t\ttexturenumber=0;\r\n\t\tfontnumber=0;\r\n\t\tactive=1;\r\n\t\tcommonattribs=4;\r\n\t}\r\n\tquickkey=113;\r\n}"},
    // TAESC.gp3/guie/coracv1.gui
    {"CORACV","CORMOHO",1,"[GADGET8]\r\n{\r\n\t[COMMON]\r\n\t{\r\n\t\tid=1;\r\n\t\tassoc=0;\r\n\t\tname=CORMOHO;\r\n\t\txpos=0;\r\n\t\typos=128;\r\n\t\twidth=64;\r\n\t\theight=64;\r\n\t\tattribs=32;\r\n\t\tcolorf=15;\r\n\t\tcolorb=0;\r\n\t\ttexturenumber=0;\r\n\t\tfontnumber=0;\r\n\t\tactive=1;\r\n\t\tcommonattribs=4;\r\n\t}\r\n\tquickkey=113;\r\n}"},
    // T2ESC.ufo/guie/armach1.gui
    {"ARMACH","ARMMOHO",1,"[GADGET8]\r\n{\r\n\t[COMMON]\r\n\t{\r\n\t\tid=1;\r\n\t\tassoc=0;\r\n\t\tname=ARMMOHO;\r\n\t\txpos=0;\r\n\t\typos=128;\r\n\t\twidth=64;\r\n\t\theight=64;\r\n\t\tattribs=32;\r\n\t\tcolorf=15;\r\n\t\tcolorb=0;\r\n\t\ttexturenumber=0;\r\n\t\tfontnumber=0;\r\n\t\tactive=1;\r\n\t\tcommonattribs=4;\r\n\t}\r\n\tquickkey=113;\r\n}"},
    // T2ESC.ufo/guie/armach3.gui
    {"ARMACH","ARMUWMOHO",3,"[GADGET8]\r\n{\r\n\t[COMMON]\r\n\t{\r\n\t\tid=1;\r\n\t\tassoc=0;\r\n\t\tname=ARMUWMOHO;\r\n\t\txpos=0;\r\n\t\typos=128;\r\n\t\twidth=64;\r\n\t\theight=64;\r\n\t\tattribs=32;\r\n\t\tcolorf=15;\r\n\t\tcolorb=0;\r\n\t\ttexturenumber=0;\r\n\t\tfontnumber=0;\r\n\t\tactive=1;\r\n\t\tcommonattribs=4;\r\n\t}\r\n\tquickkey=113;\r\n}"},
    // T2ESC.ufo/guie/armacsa1.gui
    {"ARMACSA","ARMUWMOHO",1,"[GADGET8]\r\n{\r\n\t[COMMON]\r\n\t{\r\n\t\tid=1;\r\n\t\tassoc=0;\r\n\t\tname=ARMUWMOHO;\r\n\t\txpos=0;\r\n\t\typos=128;\r\n\t\twidth=64;\r\n\t\theight=64;\r\n\t\tattribs=32;\r\n\t\tcolorf=15;\r\n\t\tcolorb=0;\r\n\t\ttexturenumber=0;\r\n\t\tfontnumber=0;\r\n\t\tactive=1;\r\n\t\tcommonattribs=4;\r\n\t}\r\n\tquickkey=113;\r\n}"},
    // T2ESC.ufo/guie/armacsa2.gui
    {"ARMACSA","ARMMOHO",2,"[GADGET8]\r\n{\r\n\t[COMMON]\r\n\t{\r\n\t\tid=1;\r\n\t\tassoc=0;\r\n\t\tname=ARMMOHO;\r\n\t\txpos=0;\r\n\t\typos=128;\r\n\t\twidth=64;\r\n\t\theight=64;\r\n\t\tattribs=32;\r\n\t\tcolorf=15;\r\n\t\tcolorb=0;\r\n\t\ttexturenumber=0;\r\n\t\tfontnumber=0;\r\n\t\tactive=1;\r\n\t\tcommonattribs=4;\r\n\t}\r\n\tquickkey=113;\r\n}"},
    // T2ESC.ufo/guie/corach1.gui
    {"CORACH","CORMOHO",1,"[GADGET8]\r\n{\r\n\t[COMMON]\r\n\t{\r\n\t\tid=1;\r\n\t\tassoc=0;\r\n\t\tname=CORMOHO;\r\n\t\txpos=0;\r\n\t\typos=128;\r\n\t\twidth=64;\r\n\t\theight=64;\r\n\t\tattribs=32;\r\n\t\tcolorf=15;\r\n\t\tcolorb=0;\r\n\t\ttexturenumber=0;\r\n\t\tfontnumber=0;\r\n\t\tactive=1;\r\n\t\tcommonattribs=4;\r\n\t}\r\n\tquickkey=113;\r\n}"},
    // T2ESC.ufo/guie/corach3.gui
    {"CORACH","CORUWMOHO",3,"[GADGET8]\r\n{\r\n\t[COMMON]\r\n\t{\r\n\t\tid=1;\r\n\t\tassoc=0;\r\n\t\tname=CORUWMOHO;\r\n\t\txpos=0;\r\n\t\typos=128;\r\n\t\twidth=64;\r\n\t\theight=64;\r\n\t\tattribs=32;\r\n\t\tcolorf=15;\r\n\t\tcolorb=0;\r\n\t\ttexturenumber=0;\r\n\t\tfontnumber=0;\r\n\t\tactive=1;\r\n\t\tcommonattribs=4;\r\n\t}\r\n\tquickkey=113;\r\n}"},
    // T2ESC.ufo/guie/coracsa1.gui
    {"CORACSA","CORUWMOHO",1,"[GADGET8]\r\n{\r\n\t[COMMON]\r\n\t{\r\n\t\tid=1;\r\n\t\tassoc=0;\r\n\t\tname=CORUWMOHO;\r\n\t\txpos=0;\r\n\t\typos=128;\r\n\t\twidth=64;\r\n\t\theight=64;\r\n\t\tattribs=32;\r\n\t\tcolorf=15;\r\n\t\tcolorb=0;\r\n\t\ttexturenumber=0;\r\n\t\tfontnumber=0;\r\n\t\tactive=1;\r\n\t\tcommonattribs=4;\r\n\t}\r\n\tquickkey=113;\r\n}"},
    // T2ESC.ufo/guie/coracsa2.gui
    {"CORACSA","CORMOHO",2,"[GADGET8]\r\n{\r\n\t[COMMON]\r\n\t{\r\n\t\tid=1;\r\n\t\tassoc=0;\r\n\t\tname=CORMOHO;\r\n\t\txpos=0;\r\n\t\typos=128;\r\n\t\twidth=64;\r\n\t\theight=64;\r\n\t\tattribs=32;\r\n\t\tcolorf=15;\r\n\t\tcolorb=0;\r\n\t\ttexturenumber=0;\r\n\t\tfontnumber=0;\r\n\t\tactive=1;\r\n\t\tcommonattribs=4;\r\n\t}\r\n\tquickkey=113;\r\n}"},
    // T3ESC.ufo/guie/armeca1.gui
    {"ARMECA","ARMMAS",1,"[GADGET8]\r\n{\r\n\t[COMMON]\r\n\t{\r\n\t\tid=1;\r\n\t\tassoc=0;\r\n\t\tname=ARMMAS;\r\n\t\txpos=0;\r\n\t\typos=128;\r\n\t\twidth=64;\r\n\t\theight=64;\r\n\t\tattribs=32;\r\n\t\tcolorf=15;\r\n\t\tcolorb=0;\r\n\t\ttexturenumber=0;\r\n\t\tfontnumber=0;\r\n\t\tactive=1;\r\n\t\tcommonattribs=4;\r\n\t}\r\n\tquickkey=113;\r\n}"},
    // T3ESC.ufo/guie/armech1.gui
    {"ARMECH","ARMMAS",1,"[GADGET8]\r\n{\r\n\t[COMMON]\r\n\t{\r\n\t\tid=1;\r\n\t\tassoc=0;\r\n\t\tname=ARMMAS;\r\n\t\txpos=0;\r\n\t\typos=128;\r\n\t\twidth=64;\r\n\t\theight=64;\r\n\t\tattribs=32;\r\n\t\tcolorf=15;\r\n\t\tcolorb=0;\r\n\t\ttexturenumber=0;\r\n\t\tfontnumber=0;\r\n\t\tactive=1;\r\n\t\tcommonattribs=4;\r\n\t}\r\n\tquickkey=113;\r\n}"},
    // T3ESC.ufo/guie/armech2.gui
    {"ARMECH","ARMUWMAS",2,"[GADGET8]\r\n{\r\n\t[COMMON]\r\n\t{\r\n\t\tid=1;\r\n\t\tassoc=0;\r\n\t\tname=ARMUWMAS;\r\n\t\txpos=0;\r\n\t\typos=128;\r\n\t\twidth=64;\r\n\t\theight=64;\r\n\t\tattribs=32;\r\n\t\tcolorf=15;\r\n\t\tcolorb=0;\r\n\t\ttexturenumber=0;\r\n\t\tfontnumber=0;\r\n\t\tactive=1;\r\n\t\tcommonattribs=4;\r\n\t}\r\n\tquickkey=113;\r\n}"},
    // T3ESC.ufo/guie/armeck1.gui
    {"ARMECK","ARMMAS",1,"[GADGET8]\r\n{\r\n\t[COMMON]\r\n\t{\r\n\t\tid=1;\r\n\t\tassoc=0;\r\n\t\tname=ARMMAS;\r\n\t\txpos=0;\r\n\t\typos=128;\r\n\t\twidth=64;\r\n\t\theight=64;\r\n\t\tattribs=32;\r\n\t\tcolorf=15;\r\n\t\tcolorb=0;\r\n\t\ttexturenumber=0;\r\n\t\tfontnumber=0;\r\n\t\tactive=1;\r\n\t\tcommonattribs=4;\r\n\t}\r\n\tquickkey=113;\r\n}"},
    // T3ESC.ufo/guie/armecs1.gui
    {"ARMECS","ARMUWMAS",1,"[GADGET8]\r\n{\r\n\t[COMMON]\r\n\t{\r\n\t\tid=1;\r\n\t\tassoc=0;\r\n\t\tname=ARMUWMAS;\r\n\t\txpos=0;\r\n\t\typos=128;\r\n\t\twidth=64;\r\n\t\theight=64;\r\n\t\tattribs=32;\r\n\t\tcolorf=15;\r\n\t\tcolorb=0;\r\n\t\ttexturenumber=0;\r\n\t\tfontnumber=0;\r\n\t\tactive=1;\r\n\t\tcommonattribs=4;\r\n\t}\r\n\tquickkey=113;\r\n}"},
    // T3ESC.ufo/guie/armecsa1.gui
    {"ARMECSA","ARMMAS",1,"[GADGET8]\r\n{\r\n\t[COMMON]\r\n\t{\r\n\t\tid=1;\r\n\t\tassoc=0;\r\n\t\tname=ARMMAS;\r\n\t\txpos=0;\r\n\t\typos=128;\r\n\t\twidth=64;\r\n\t\theight=64;\r\n\t\tattribs=32;\r\n\t\tcolorf=15;\r\n\t\tcolorb=0;\r\n\t\ttexturenumber=0;\r\n\t\tfontnumber=0;\r\n\t\tactive=1;\r\n\t\tcommonattribs=4;\r\n\t}\r\n\tquickkey=113;\r\n}"},
    // T3ESC.ufo/guie/armecsa2.gui
    {"ARMECSA","ARMUWMAS",2,"[GADGET8]\r\n{\r\n\t[COMMON]\r\n\t{\r\n\t\tid=1;\r\n\t\tassoc=0;\r\n\t\tname=ARMUWMAS;\r\n\t\txpos=0;\r\n\t\typos=128;\r\n\t\twidth=64;\r\n\t\theight=64;\r\n\t\tattribs=32;\r\n\t\tcolorf=15;\r\n\t\tcolorb=0;\r\n\t\ttexturenumber=0;\r\n\t\tfontnumber=0;\r\n\t\tactive=1;\r\n\t\tcommonattribs=4;\r\n\t}\r\n\tquickkey=113;\r\n}"},
    // T3ESC.ufo/guie/armecsub1.gui
    {"ARMECSUB","ARMUWMAS",1,"[GADGET8]\r\n{\r\n\t[COMMON]\r\n\t{\r\n\t\tid=1;\r\n\t\tassoc=0;\r\n\t\tname=ARMUWMAS;\r\n\t\txpos=0;\r\n\t\typos=128;\r\n\t\twidth=64;\r\n\t\theight=64;\r\n\t\tattribs=32;\r\n\t\tcolorf=15;\r\n\t\tcolorb=0;\r\n\t\ttexturenumber=0;\r\n\t\tfontnumber=0;\r\n\t\tactive=1;\r\n\t\tcommonattribs=4;\r\n\t}\r\n\tquickkey=113;\r\n}"},
    // T3ESC.ufo/guie/armecv1.gui
    {"ARMECV","ARMMAS",1,"[GADGET8]\r\n{\r\n\t[COMMON]\r\n\t{\r\n\t\tid=1;\r\n\t\tassoc=0;\r\n\t\tname=ARMMAS;\r\n\t\txpos=0;\r\n\t\typos=128;\r\n\t\twidth=64;\r\n\t\theight=64;\r\n\t\tattribs=32;\r\n\t\tcolorf=15;\r\n\t\tcolorb=0;\r\n\t\ttexturenumber=0;\r\n\t\tfontnumber=0;\r\n\t\tactive=1;\r\n\t\tcommonattribs=4;\r\n\t}\r\n\tquickkey=113;\r\n}"},
    // T3ESC.ufo/guie/coreca1.gui
    {"CORECA","CORMAS",1,"[GADGET8]\r\n{\r\n\t[COMMON]\r\n\t{\r\n\t\tid=1;\r\n\t\tassoc=0;\r\n\t\tname=CORMAS;\r\n\t\txpos=0;\r\n\t\typos=128;\r\n\t\twidth=64;\r\n\t\theight=64;\r\n\t\tattribs=32;\r\n\t\tcolorf=15;\r\n\t\tcolorb=0;\r\n\t\ttexturenumber=0;\r\n\t\tfontnumber=0;\r\n\t\tactive=1;\r\n\t\tcommonattribs=4;\r\n\t}\r\n\tquickkey=113;\r\n}"},
    // T3ESC.ufo/guie/corech1.gui
    {"CORECH","CORMAS",1,"[GADGET8]\r\n{\r\n\t[COMMON]\r\n\t{\r\n\t\tid=1;\r\n\t\tassoc=0;\r\n\t\tname=CORMAS;\r\n\t\txpos=0;\r\n\t\typos=128;\r\n\t\twidth=64;\r\n\t\theight=64;\r\n\t\tattribs=32;\r\n\t\tcolorf=15;\r\n\t\tcolorb=0;\r\n\t\ttexturenumber=0;\r\n\t\tfontnumber=0;\r\n\t\tactive=1;\r\n\t\tcommonattribs=4;\r\n\t}\r\n\tquickkey=113;\r\n}"},
    // T3ESC.ufo/guie/corech2.gui
    {"CORECH","CORUWMAS",2,"[GADGET8]\r\n{\r\n\t[COMMON]\r\n\t{\r\n\t\tid=1;\r\n\t\tassoc=0;\r\n\t\tname=CORUWMAS;\r\n\t\txpos=0;\r\n\t\typos=128;\r\n\t\twidth=64;\r\n\t\theight=64;\r\n\t\tattribs=32;\r\n\t\tcolorf=15;\r\n\t\tcolorb=0;\r\n\t\ttexturenumber=0;\r\n\t\tfontnumber=0;\r\n\t\tactive=1;\r\n\t\tcommonattribs=4;\r\n\t}\r\n\tquickkey=113;\r\n}"},
    // T3ESC.ufo/guie/coreck1.gui
    {"CORECK","CORMAS",1,"[GADGET8]\r\n{\r\n\t[COMMON]\r\n\t{\r\n\t\tid=1;\r\n\t\tassoc=0;\r\n\t\tname=CORMAS;\r\n\t\txpos=0;\r\n\t\typos=128;\r\n\t\twidth=64;\r\n\t\theight=64;\r\n\t\tattribs=32;\r\n\t\tcolorf=15;\r\n\t\tcolorb=0;\r\n\t\ttexturenumber=0;\r\n\t\tfontnumber=0;\r\n\t\tactive=1;\r\n\t\tcommonattribs=4;\r\n\t}\r\n\tquickkey=113;\r\n}"},
    // T3ESC.ufo/guie/corecs1.gui
    {"CORECS","CORUWMAS",1,"[GADGET8]\r\n{\r\n\t[COMMON]\r\n\t{\r\n\t\tid=1;\r\n\t\tassoc=0;\r\n\t\tname=CORUWMAS;\r\n\t\txpos=0;\r\n\t\typos=128;\r\n\t\twidth=64;\r\n\t\theight=64;\r\n\t\tattribs=32;\r\n\t\tcolorf=15;\r\n\t\tcolorb=0;\r\n\t\ttexturenumber=0;\r\n\t\tfontnumber=0;\r\n\t\tactive=1;\r\n\t\tcommonattribs=4;\r\n\t}\r\n\tquickkey=113;\r\n}"},
    // T3ESC.ufo/guie/corecsa1.gui
    {"CORECSA","CORMAS",1,"[GADGET8]\r\n{\r\n\t[COMMON]\r\n\t{\r\n\t\tid=1;\r\n\t\tassoc=0;\r\n\t\tname=CORMAS;\r\n\t\txpos=0;\r\n\t\typos=128;\r\n\t\twidth=64;\r\n\t\theight=64;\r\n\t\tattribs=32;\r\n\t\tcolorf=15;\r\n\t\tcolorb=0;\r\n\t\ttexturenumber=0;\r\n\t\tfontnumber=0;\r\n\t\tactive=1;\r\n\t\tcommonattribs=4;\r\n\t}\r\n\tquickkey=113;\r\n}"},
    // T3ESC.ufo/guie/corecsa2.gui
    {"CORECSA","CORUWMAS",2,"[GADGET8]\r\n{\r\n\t[COMMON]\r\n\t{\r\n\t\tid=1;\r\n\t\tassoc=0;\r\n\t\tname=CORUWMAS;\r\n\t\txpos=0;\r\n\t\typos=128;\r\n\t\twidth=64;\r\n\t\theight=64;\r\n\t\tattribs=32;\r\n\t\tcolorf=15;\r\n\t\tcolorb=0;\r\n\t\ttexturenumber=0;\r\n\t\tfontnumber=0;\r\n\t\tactive=1;\r\n\t\tcommonattribs=4;\r\n\t}\r\n\tquickkey=113;\r\n}"},
    // T3ESC.ufo/guie/corecsub1.gui
    {"CORECSUB","CORUWMAS",1,"[GADGET8]\r\n{\r\n\t[COMMON]\r\n\t{\r\n\t\tid=1;\r\n\t\tassoc=0;\r\n\t\tname=CORUWMAS;\r\n\t\txpos=0;\r\n\t\typos=128;\r\n\t\twidth=64;\r\n\t\theight=64;\r\n\t\tattribs=32;\r\n\t\tcolorf=15;\r\n\t\tcolorb=0;\r\n\t\ttexturenumber=0;\r\n\t\tfontnumber=0;\r\n\t\tactive=1;\r\n\t\tcommonattribs=4;\r\n\t}\r\n\tquickkey=113;\r\n}"},
    // T3ESC.ufo/guie/corecv1.gui
    {"CORECV","CORMAS",1,"[GADGET8]\r\n{\r\n\t[COMMON]\r\n\t{\r\n\t\tid=1;\r\n\t\tassoc=0;\r\n\t\tname=CORMAS;\r\n\t\txpos=0;\r\n\t\typos=128;\r\n\t\twidth=64;\r\n\t\theight=64;\r\n\t\tattribs=32;\r\n\t\tcolorf=15;\r\n\t\tcolorb=0;\r\n\t\ttexturenumber=0;\r\n\t\tfontnumber=0;\r\n\t\tactive=1;\r\n\t\tcommonattribs=4;\r\n\t}\r\n\tquickkey=113;\r\n}"},
};

static void TestMenuParser() {
    const auto valid = BuildButton("ARMMOHO");
    assert(MexUpgradeMenu::ContainsBuildButton(valid.data(),valid.size(),"armmoho"));
    for (std::size_t length=0; length<valid.size(); ++length)
        assert(!MexUpgradeMenu::ContainsBuildButton(valid.data(),length,"ARMMOHO"));
    const char* good[] = {
        "[gAdGeT12] { [cOmMoN] { ID=\"1\"; name=\"armmoho\"; ACTIVE=+1; ATTRIBS=32; } }",
        "[GADGET8]{/* name=FAKE; */[COMMON]{id=1;name=ARMMOHO;active=1;attribs=96;}}",
        "[GADGET8]{[COMMON]{id=1;name=ARMMOHO;attribs=32;active=-1;}[OTHER]{name=WRONG;}}",
        "// [GADGET8]{[COMMON]{name=WRONG;}}\n[GADGET8]{[COMMON]{id=1;name/* gap */=ARMMOHO;active=1;attribs=32;}}",
    };
    for (const char* data : good)
        assert(MexUpgradeMenu::ContainsBuildButton(data,std::strlen(data),"ARMMOHO"));
    const char* bad[] = {
        "// [GADGET8]{[COMMON]{id=1;name=ARMMOHO;active=1;attribs=32;}}",
        "/* [GADGET8]{[COMMON]{id=1;name=ARMMOHO;active=1;attribs=32;}} */",
        "[OTHER]{[COMMON]{id=1;name=ARMMOHO;active=1;attribs=32;}}",
        "[OTHER]{[GADGET8]{[COMMON]{id=1;name=ARMMOHO;active=1;attribs=32;}}}",
        "[GADGET8]{name=ARMMOHO;id=1;active=1;attribs=32;}",
        "[GADGET8]{[OTHER]{[COMMON]{id=1;name=ARMMOHO;active=1;attribs=32;}}}",
        "[GADGET8]{[COMMON]{id=7;name=ARMMOHO;active=1;attribs=32;}}",
        "[GADGET8]{[COMMON]{id=1;name=ARMMOHO;active=0;attribs=32;}}",
        "[GADGET8]{[COMMON]{id=1;name=ARMMOHO;attribs=32;}}", // Native active defaults to zero.
        "[GADGET8]{[COMMON]{id=1;name=ARMMOHO;active=1;attribs=0;}}",
        "[GADGET8]{[COMMON]{id=1;name=ARMMOHO2;active=1;attribs=32;}}",
        "[GADGET8]{[COMMON]{id=1;name=PRE_ARMMOHO;active=1;attribs=32;}}",
        "[GADGET8]{[COMMON]{id=1;name=ARMMOHO;active=1;attribs=32;}",
        "[GADGET8]{[COMMON]{id=1;name=\"ARMMOHO;active=1;attribs=32;}}",
        "[GADGET8]{[COMMON]{id=1;name=ARMMOHO;active=1;attribs=32;}} /* truncated",
        "[GADGET8]{[COMMON]{id=1;name=ARMMOHO;active=1;attribs=32;}} [GADGET9]{",
        "[GADGET8]{[COMMON]{id=1;name=OTHER;caption=\"name=ARMMOHO; active=1;\";attribs=32;}}",
        "[GADGET8]{[COMMON]{id=1;name=ARMMOHO;id=7;attribs=32;}}",
        "[GADGET8]{[COMMON]{id=1;name=ARMMOHO;attribs=2147483648;}}",
        "[GADGET8]{[COMMON]{id=1;name=ARMMOHO;attribs=32x;}}",
        "[GADGET8]{[COMMON]{id=1;name=ARMMOHO;active=1;attribs=3/*split*/2;}}",
        "[GADGET8]{[COMMON]{id=1;name=ARMMOHO;attribs=32;}[COMMON]{id=1;name=OTHER;attribs=32;}}",
        "[GADGET8suffix]{[COMMON]{id=1;name=ARMMOHO;active=1;attribs=32;}}",
    };
    for (const char* data : bad)
        assert(!MexUpgradeMenu::ContainsBuildButton(data,std::strlen(data),"ARMMOHO"));
    std::string deep;
    for (int i=0; i<40; ++i) deep += "[OTHER]{";
    deep += valid;
    for (int i=0; i<40; ++i) deep += "}";
    assert(!MexUpgradeMenu::ContainsBuildButton(deep.data(),deep.size(),"ARMMOHO"));
    const auto nul = std::string("\0",1) + valid;
    assert(!MexUpgradeMenu::ContainsBuildButton(nul.data(),nul.size(),"ARMMOHO"));
    assert(!MexUpgradeMenu::ContainsBuildButton(nullptr,10,"ARMMOHO"));
    assert(!MexUpgradeMenu::ContainsBuildButton(valid.data(),valid.size(),nullptr));
    assert(!MexUpgradeMenu::ContainsBuildButton(valid.data(),valid.size(),""));
    assert(!MexUpgradeMenu::ContainsBuildButton(valid.data(),1024*1024+1,"ARMMOHO"));
    // The length is authoritative: valid, non-NUL-terminated bytes are accepted.
    std::vector<char> bounded(valid.begin(),valid.end());
    assert(MexUpgradeMenu::ContainsBuildButton(bounded.data(),bounded.size(),"ARMMOHO"));
}

static const char* PreviousTier(const char* replacement) {
    for (const auto& pair : kUpgrades)
        if (_strnicmp(pair.to,replacement,32) == 0) return pair.from;
    assert(false); return "";
}

static void TestHumanMenusIndependentOfAiPlans() {
    assert(sizeof(kRealMenus)/sizeof(kRealMenus[0]) == 34);
    for (const auto& menu : kRealMenus) for (int aiPlan : {0,1,2}) {
        const bool moho = std::string(menu.replacement).find("MOHO") != std::string::npos;
        World world(moho ? 5 : 7,moho ? 5 : 7);
        Name(world.defs[1],menu.constructor);
        Name(world.defs[2],PreviousTier(menu.replacement));
        Name(world.defs[3],menu.replacement); Name(world.defs[4],"OTHER");
        world.defs[2].extractsmetal = moho ? .001f : .003f;
        world.defs[3].extractsmetal = moho ? .003f : .009f;
        world.defs[1].GUINum = menu.page+1; // Native field is the first missing page.
        buildMenuPages.assign(menu.page+1,""); buildMenuPages[menu.page] = menu.gui;
        if (aiPlan == 0) world.defs[1].CANBUILD_ptr = nullptr;
        if (aiPlan == 1) world.defs[1].canbuildCount = 0;
        if (aiPlan == 2) { world.canbuild[0] = 4; world.defs[1].canbuildCount = 1; }
        assert(FindReplacement(world.ta,world.defs[2],world.defs[1],menu.replacement) == 3);
        auto upgrade = world.candidate();
        assert(PrepareUpgrade(world.ta,upgrade,false) && upgrade.type == 3);
        assert(buildMenuCalls > 0);
        // A unit in the AI plan does not authorize a human menu that lacks it.
        buildMenuPages.clear(); world.canbuild[0] = 3;
        world.defs[1].CANBUILD_ptr = world.canbuild; world.defs[1].canbuildCount = 1;
        assert(FindReplacement(world.ta,world.defs[2],world.defs[1],menu.replacement) == 0);
    }
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

static void MoveUnitSlot(World& world, int from, int to) {
    assert(from > 0 && from < 8 && to > 0 && to < 8 && from != to);
    world.units[to] = world.units[from];
    world.units[to].UnitInGameIndex = static_cast<short>(to);
    world.units[from] = UnitStruct{};
    for (auto& tile : world.map)
        if (tile.occupyingUnitNumber == static_cast<unsigned>(from)) tile.occupyingUnitNumber = to;
    if (world.ta.MouseOverUnit == static_cast<unsigned>(from)) world.ta.MouseOverUnit = to;
}

static void UsePlayer(World& world, int player) {
    assert(player == 0 || player == 9);
    if (player == 0) return;
    world.ta.Players[player] = world.ta.Players[0];
    for (auto& unit : world.units)
        if (unit.Owner_PlayerPtr0 == &world.ta.Players[0]) unit.Owner_PlayerPtr0 = &world.ta.Players[player];
    world.ta.LocalHumanPlayer_PlayerID = player;
}

static void TestInclusiveUnitRanges() {
    for (int player : {0,9}) for (int finalSlot : {0,1,2}) {
        World world; UsePlayer(world,player);
        if (finalSlot == 1) MoveUnitSlot(world,1,7); // Last global slot is the constructor.
        if (finalSlot == 2) MoveUnitSlot(world,2,7); // Last global slot is the clicked extractor.
        auto& owner = world.ta.Players[player];
        assert(owner.UnitsAry_End == world.ta.EndOfUnitsArray_p);
        Upgrade upgrade = {};
        assert(FindUpgradeTarget(&world.ta,upgrade));
        assert(upgrade.constructor == &world.units[finalSlot == 1 ? 7 : 1]);
        assert(upgrade.target == &world.units[finalSlot == 2 ? 7 : 2]);
        assert(PrepareUpgrade(world.ta,upgrade,false));
    }
    for (int invalid : {0,1,2,3}) {
        World world;
        if (invalid == 0) world.ta.BeginUnitsArray_p = world.units+2; // Owner block starts before global range.
        if (invalid == 1) world.ta.EndOfUnitsArray_p = world.units+6; // Owner block ends after global range.
        if (invalid == 2) world.owner().Units = world.units+7, world.owner().UnitsAry_End = world.units+6;
        if (invalid == 3) world.ta.MouseOverUnit = 8; // One past the inclusive last slot.
        Upgrade upgrade = {};
        assert(!FindUpgradeTarget(&world.ta,upgrade) && nativeSends == 0);
    }
}

static void TestInclusiveUnitQueueAndFeedback() {
    for (int player : {0,9}) for (int finalSlot : {1,2}) for (int interface : {0,1}) {
        World world; UsePlayer(world,player);
        MoveUnitSlot(world,finalSlot,7);
        world.ta.InterfaceType = interface;
        if (interface) world.ta.BuildSpotState |= 4;
        auto* constructor = &world.units[finalSlot == 1 ? 7 : 1];
        _MOUSEEVENT click; click.Msg = interface ? WM_RBUTTONDOWN : WM_LBUTTONUP;
        assert(MexUpgrade::TryCommand(&world.ta,&click));
        auto* reclaim = constructor->UnitOrders;
        assert(nativeSends == 2 && reclaim && reclaim->NextOrder);
        assert(MexUpgrade::IsQueuedBuild(&world.ta,constructor,reclaim));
        int pixel = 0; _OFFSCREEN surface; surface.lpSurface = &pixel;
        MexUpgrade::DrawFeedback(&world.ta,&surface);
        assert(feedbackDrawCalls == 1); // A last-slot constructor still draws the native footprint.
        click.fwKeys = MK_SHIFT;
        assert(MexUpgrade::TryCommand(&world.ta,&click));
        assert(!constructor->UnitOrders && removedOrders == 2);
    }
    World outside;
    UnitOrdersStruct reclaim = {}, build = {};
    reclaim.Unit_ptr = build.Unit_ptr = &outside.units[1];
    reclaim.AttackTargat = outside.units+8; // One-past pointer must be rejected before dereferencing.
    reclaim.COBHandler_index = 11; reclaim.NextOrder = &build;
    build.COBHandler_index = 12; build.BuildUnitID = 3;
    assert(!MexUpgrade::IsQueuedBuild(&outside.ta,&outside.units[1],&reclaim));
    auto upgrade = outside.candidate();
    assert(PrepareUpgrade(outside.ta,upgrade,false));
    assert(QueueUpgrade(outside.ta,upgrade,UpgradeActions(outside.units[1]),false));
    feedback.constructorSlot = 8;
    int pixel = 0; _OFFSCREEN surface; surface.lpSurface = &pixel;
    MexUpgrade::DrawFeedback(&outside.ta,&surface);
    assert(feedbackDrawCalls == 0); // One past the inclusive end cannot reach a native draw call.
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
        if (reason == 0) buildMenuPages.clear();
        if (reason == 1) buildMenuPages = {"",BuildButton("ARMMOHO")};
        if (reason == 2) Name(rejected.defs[3],"UNRELATED");
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
        buildMenuPages = {"",BuildButton(pairs[pair][1])};
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

static void TestGestureLoggingIsBounded() {
    World world; _MOUSEEVENT click;
    assert(queuedGestureLogs == 0 && cancelledGestureLogs == 0);
    for (unsigned cycle = 0; cycle < 40; ++cycle) {
        click.fwKeys = 0;
        assert(MexUpgrade::TryCommand(&world.ta,&click));
        auto* reclaim = world.units[1].UnitOrders;
        assert(reclaim && reclaim->COBHandler_index == 11 && reclaim->NextOrder);
        assert(reclaim->NextOrder->BuildUnitID == 3);
        click.fwKeys = MK_SHIFT;
        assert(MexUpgrade::TryCommand(&world.ta,&click));
        assert(!world.units[1].UnitOrders);
        assert(nativeSends == static_cast<int>((cycle+1)*2));
        assert(removedOrders == static_cast<int>((cycle+1)*2));
        const unsigned logged = std::min(cycle+1,32u);
        assert(queuedGestureLogs == logged && cancelledGestureLogs == logged);
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
        if (reason == 2) buildMenuPages.clear();
        if (reason == 3) buildMenuPages = {"",BuildButton("ARMMOHO")};
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
    const bool enabled = config.active;
    assert(MexUpgrade::Enabled() == enabled && config.reads == 1);
    TestMenuParser();
    TestHumanMenusIndependentOfAiPlans();
    TestInclusiveUnitRanges();
    TestMappingsAndNoTierSkipping();
    TestBuilderAndDefinitionEligibility();
    TestExpandedSiteAndPrivateYard();
    TestOffsetSites();
    TestAllPairsOffsetFootprints();
    TestAlignmentAndEdges();
    TestOffsetOrderRecognition();
    if (enabled) {
        TestGestureLoggingIsBounded();
        TestInclusiveUnitQueueAndFeedback();
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



DISABLED_HARNESS = r'''
#include <cassert>
#include <iostream>
#include "OptionalUnitControls.h"
#include "MexUpgrade.h"
#if MEX_UPGRADE_ENABLE
#error "The disabled harness requires extractor upgrades to be excluded"
#endif
// This compiles the full production translation unit with its platform includes
// removed. No TA/config/native mocks are provided: an off build must need none.
// PRODUCTION_SOURCE
int main(int argc, char** argv) {
    assert(argc == 2); (void)argv;
    int sentinel = 0x1234;
    auto* ta = reinterpret_cast<TAdynmemStruct*>(&sentinel);
    auto* mouse = reinterpret_cast<_MOUSEEVENT*>(&sentinel);
    auto* surface = reinterpret_cast<_OFFSCREEN*>(&sentinel);
    auto* unit = reinterpret_cast<UnitStruct*>(&sentinel);
    auto* order = reinterpret_cast<UnitOrdersStruct*>(&sentinel);
    assert(!MexUpgrade::Enabled() && !MexUpgrade::CheckingSite());
    assert(!MexUpgrade::TryCommand(ta, mouse));
    assert(!MexUpgrade::IsQueuedBuild(ta, unit, order));
    MexUpgrade::UpdateCursor(ta);
    MexUpgrade::DrawFeedback(ta, surface);
    assert(!MexUpgrade::TryCommand(nullptr, nullptr));
    assert(!MexUpgrade::IsQueuedBuild(nullptr, nullptr, nullptr));
    MexUpgrade::UpdateCursor(nullptr);
    MexUpgrade::DrawFeedback(nullptr, nullptr);
    assert(sentinel == 0x1234);
    std::cout << "Extractor upgrade compiled-out API tests passed\n";
}
'''


def strip_includes(source):
    return re.sub(r'^\s*#include[^\n]*$', "", source.lstrip("\ufeff"), flags=re.M)


def test_hook_guards():
    """Preprocess the actual hooks to ensure off means native input/cursors."""
    for enabled in (0, 1):
        compiler = shlex.split(os.environ.get("CXX", "g++"))
        flags = ["-E", "-P", "-x", "c++", "-DMEX_UPGRADE_ENABLE=" + str(enabled), "-"]
        result = subprocess.run(compiler + flags, input=strip_includes((DDRAW / "TABugFix.cpp").read_text()),
            text=True, capture_output=True, check=True).stdout
        for symbol in ("MexUpgrade::TryCommand", "DispatchRightCommand", "DispatchUpgradeCursor",
            "MexUpgradeRightClick", "MexUpgradeCursor", "MexUpgrade::Enabled"):
            assert (symbol in result) == bool(enabled), (symbol, enabled)
        assert "DispatchSmallMapCommand" in result and "0x00498F70u" in result
        kickout = subprocess.run(compiler + flags,
            input=strip_includes((DDRAW / "ConstructionKickout.cpp").read_text()),
            text=True, capture_output=True, check=True).stdout
        assert ("MexUpgrade::CheckingSite" in kickout) == bool(enabled)
    print("Extractor upgrade native-hook compile guards passed")


def main():
    source = (DDRAW / "MexUpgrade.cpp").read_text()
    constants = "\n".join(re.findall(r'^\s*constexpr (?:unsigned|int) k(?:DefinitionEnabled|MaxOrders|MaxUpgradeFootprint|FeedbackTicks)\s*=.*?;', source, re.M))
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
        "void MexUpgrade::DrawFeedback(",
    )
    methods = "\n\n".join(extract(source, signature) for signature in signatures)
    bridge = "reinterpret_cast<int (__cdecl*)()>(0x0047C780u)"
    if methods.count(bridge) != 1:
        raise RuntimeError("Expected one native grid-height bridge to mock")
    methods = methods.replace(bridge, "MockGridHeight")
    for bridge, mock in (
        ("reinterpret_cast<int (__stdcall*)(int)>(0x004C1B80u)", "MockFeedbackKeyDown"),
        ("reinterpret_cast<DrawBuildSpotQueue>(0x00438C00u)", "static_cast<DrawBuildSpotQueue>(MockDrawBuildSpotQueue)"),
    ):
        if methods.count(bridge) != 1:
            raise RuntimeError("Expected one native feedback bridge to mock: " + bridge)
        methods = methods.replace(bridge, mock)
    harness = HARNESS.replace("// PRODUCTION_CONSTANTS_AND_TABLE", constants + "\n" + table).replace(
        "// PRODUCTION_FUNCTIONS", methods)
    with tempfile.TemporaryDirectory(prefix="mex-upgrade-test-") as temp:
        cpp = Path(temp) / "mex_upgrade_test.cpp"
        cpp.write_text(harness)
        env = os.environ.copy()
        env["ASAN_OPTIONS"] = env.get("ASAN_OPTIONS", "") + ":detect_leaks=0"
        disabled = Path(temp) / "mex_upgrade_disabled.cpp"
        disabled.write_text(DISABLED_HARNESS.replace("// PRODUCTION_SOURCE", strip_includes(source)))
        for mod in ("ESCALATION", "PROTA", "OTA"):
            for override in (None, 0, 1):
                compiled = mod == "ESCALATION" if override is None else bool(override)
                binary = Path(temp) / ("mex_upgrade_" + mod.lower() + str(override))
                flags = [] if override is None else ["-DMEX_UPGRADE_ENABLE=" + str(override)]
                subprocess.run(shlex.split(os.environ.get("CXX", "g++")) + [
                    "-std=c++11", "-Wall", "-Wextra", "-Werror", "-g",
                    "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                    "-I", str(DDRAW), "-DTDRAW_CONFIG_" + mod,
                ] + flags + [str(cpp if compiled else disabled), "-o", str(binary)], check=True)
                for configured in ("1", "0"):
                    subprocess.run([str(binary), configured], check=True, env=env)
        test_hook_guards()


if __name__ == "__main__":
    main()
