#!/usr/bin/env python3
"""Exercise the production prepend adapter with native TA bridges mocked.

The portable queue transaction has its own tests. This suite compiles the real
adapter and dispatch code, replacing only fixed-address engine/patch calls.
Native addresses, x86 layout and SEH remain the
responsibility of the Windows build and in-game tests.
"""

import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile
import sys

sys.dont_write_bytecode = True
from build_rectangle_gesture_test import normalize_finally


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
    raise RuntimeError("Unclosed production definition: " + signature)


HARNESS = r'''
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <strings.h>
#include <iostream>
#include <functional>
#include <mutex>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "PrependQueue.h"
#define __stdcall
#define __cdecl
#define __thiscall
#define __fastcall
#define _stricmp strcasecmp
using UINT = unsigned;
using WPARAM = std::uintptr_t;
using BYTE = unsigned char;
using DWORD = unsigned;
constexpr int TRUE = 1, FALSE = 0, TAInGame = 1, Player_LocalHuman = 1;
constexpr unsigned WATCH = 4, VK_SPACE = 0x20, VK_SHIFT = 0x10, VK_CONTROL = 0x11, VK_MENU = 0x12;
constexpr unsigned MK_CONTROL = 8, MK_SHIFT = 4;
constexpr UINT WM_KEYDOWN = 0x100, WM_KEYUP = 0x101, WM_CHAR = 0x102;
constexpr UINT WM_SYSKEYDOWN = 0x104, WM_SYSKEYUP = 0x105, WM_MOUSEMOVE = 0x200;
constexpr UINT WM_LBUTTONDOWN=0x201, WM_LBUTTONUP=0x202, WM_LBUTTONDBLCLK=0x203;
constexpr UINT WM_RBUTTONDOWN=0x204, WM_RBUTTONUP=0x205, WM_RBUTTONDBLCLK=0x206;
constexpr int INLINE_UNPROTECTEVINMENT = 0x1003;
namespace ordertype {
    enum { STOP=1, MOVE=2, ATTACK=3, BLAST=4, LOAD=5, UNLOAD=6, DEFEND=7,
        REPAIR=8, PATROL=9, RECLAIM=12, CAPTURE=13, BUILD=14 };
}
struct Position_Dword { std::uint16_t x_=0, X=0, z_=0, Z=0, y_=0, Y=0; };
struct MOUSEEVENT { unsigned X=0,Y=0,fwKeys=0,PressTime_sec=0,Msg=0,DblClick=0; };
static_assert(sizeof(MOUSEEVENT)==24,"Native saved click size");
struct PlayerStruct;
struct UnitOrdersStruct;
struct UnitDefStruct { unsigned marker = 1; int bmcode=1; bool flying=false; };
struct UnitStruct {
    short UnitID=0, UnitInGameIndex=0;
    UnitDefStruct* UnitType=nullptr;
    PlayerStruct* Owner_PlayerPtr0=nullptr;
    unsigned UnitSelected=0;
    float Nanoframe=0;
    UnitOrdersStruct* UnitOrders=nullptr;
};
struct UnitOrdersStruct {
    unsigned char COBHandler_index=0, State=0;
    UnitStruct* Unit_ptr=nullptr;
    UnitStruct* AttackTargat=nullptr;
    Position_Dword Pos;
    int BuildUnitID=0;
    union { int parameter2=0; int BuildCount; };
    union { int parameter3=0; int OrderParameter3; };
    unsigned Order_State=0;
    UnitOrdersStruct* NextOrder=nullptr;
};
struct PlayerInfoStruct { unsigned PropertyMask=0; };
struct PlayerStruct {
    PlayerInfoStruct* PlayerInfo=nullptr;
    int My_PlayerType=Player_LocalHuman;
    UnitStruct* Units=nullptr;
    UnitStruct* UnitsAry_End=nullptr;
};
struct GUIMEMSTRUCT { GUIMEMSTRUCT* per_active=nullptr; char GUIName[16]={}; };
struct Desktop { GUIMEMSTRUCT* TheActive_GUIMEM=nullptr; };
struct TAdynmemStruct {
    int LocalHumanPlayer_PlayerID=0;
    unsigned BuildSpotState=3;
    PlayerStruct Players[10];
    UnitStruct* BeginUnitsArray_p=nullptr;
    UnitStruct* EndOfUnitsArray_p=nullptr;
    Desktop desktopGUI;
};
struct Shared { int TAProgress=TAInGame, PlayingDemo=0, ehaOff=0; } shared;
Shared* DataShare=&shared;
TAdynmemStruct* world=nullptr;
TAdynmemStruct** TAmainStruct_PtrPtr=&world;
struct Config {
    int active=TRUE, key=0, mask=-999;
    int GetIniBool(const char* name, int fallback) {
        assert(std::strcmp(name,"QueuePrepend")==0); (void)fallback; return active;
    }
    int GetIniInt(const char* name, int fallback) {
        if (std::strcmp(name,"QueuePrependKey")==0) return key;
        assert(std::strcmp(name,"QueuePrependModifiers")==0); return mask==-999 ? fallback : mask;
    }
} configuration;
Config* MyConfig=&configuration;
std::set<unsigned> pressedKeys={VK_CONTROL,VK_SHIFT};
unsigned warnings=0, insertedLogs=0, inputDisabledLogs=0;
short GetAsyncKeyState(unsigned key) { return pressedKeys.count(key) ? static_cast<short>(0x8000) : 0; }
void SendText(char*,int) { ++warnings; }
struct IDDrawSurface {
    static void OutptTxt(const char* text) {
        if (std::strstr(text,"[QueuePrepend] native input hooks incomplete; disabled")==text)
            ++inputDisabledLogs;
    }
    template<class... Args> static void OutptFmtTxt(const char* text,Args...) {
        if (std::strstr(text,"[QueuePrepend] inserted ")==text) ++insertedLogs;
    }
};
struct COBHandle { unsigned unused=0; const char* technicalName=nullptr; } actionTable[128];
COBHandle* actionBegin=actionTable;
COBHandle* actionEnd=actionTable+128;
COBHandle** COBSciptHandler_Begin=&actionBegin;
COBHandle** COBSciptHandler_End=&actionEnd;

static std::map<void*,int> rotations;
static int currentRotation=0;
static bool throwRotation=false;
class CUnitRotate {
public:
    static CUnitRotate* GetInstance() { static CUnitRotate rotation; return &rotation; }
    void TagOrderRotation(void* p,int r) { if (throwRotation) throw std::bad_alloc(); rotations[p]=r; }
    int TakeOrderRotation(void* p) { auto it=rotations.find(p); return it==rotations.end() ? 0 : it->second; }
    void ClearOrderRotation(void* p) { rotations.erase(p); }
    int GetRotation() { return currentRotation; }
};
static unsigned Action(unsigned kind) { return kind+32; }
static unsigned failAction=0;
static bool scribblePosition=false, helpBuildMapping=false;
static std::vector<Position_Dword> lookupPositions;
unsigned char* MockAction(unsigned* out,unsigned type,UnitStruct* unit,UnitStruct* target,const Position_Dword* position) {
    if (scribblePosition) {
        assert(position); lookupPositions.push_back(*position);
        auto* writable=const_cast<Position_Dword*>(position);
        writable->X=65530; writable->Y=65520; writable->Z=65510;
    }
    if (type==failAction) return nullptr;
    *out=(helpBuildMapping && type==ordertype::REPAIR && target && target->Nanoframe>0)
        ? (unit->UnitType->flying ? 91u : 90u) : Action(type);
    return reinterpret_cast<unsigned char*>(out);
}
int MockParameter(const UnitOrdersStruct* node,unsigned offset) {
    switch (offset) { case 0x36:return node->BuildUnitID; case 0x3a:return node->parameter2;
        case 0x3e:return node->parameter3; default: assert(false); return 0; }
}
struct Sent {
    unsigned action; int shift; UnitStruct* unit; UnitStruct* target;
    const Position_Dword* pointer; Position_Dword position; int p1,p2;
};
static std::vector<Sent> sent;
static std::function<void(const Sent&)> onNativeSend;
void MockSend(unsigned action,int shift,UnitStruct* unit,UnitStruct* target,
    const Position_Dword* pos,int p1,int p2) {
    const Sent value={action,shift,unit,target,pos,pos ? *pos : Position_Dword{},p1,p2};
    sent.push_back(value);
    if (onNativeSend) onNativeSend(value);
}
static std::set<void*> allocations;
static unsigned allocationCalls=0, failAllocation=0, constructorCalls=0, removed=0, destroyed=0;
static unsigned definitionFlags[128]={};
static bool requireRotationDuringRemove=false;
static int expectedCleanupRotation=0;
void* MockAllocate(std::size_t) {
    ++allocationCalls;
    if (failAllocation && allocationCalls==failAllocation) return nullptr;
    void* p=std::malloc(sizeof(UnitOrdersStruct)); assert(p); allocations.insert(p); return p;
}
void MockFree(void* p) { assert(allocations.erase(p)==1); std::free(p); }
UnitOrdersStruct* MockConstruct(UnitOrdersStruct* node,unsigned action,UnitStruct* target,
    const Position_Dword* pos,int p1,int p2,int p3) {
    ++constructorCalls;
    new(node) UnitOrdersStruct;
    assert(action<128); node->Order_State=definitionFlags[action];
    node->COBHandler_index=static_cast<unsigned char>(action); node->AttackTargat=target;
    if (pos) node->Pos=*pos;
    node->BuildUnitID=p1; node->parameter2=p2; node->parameter3=p3;
    return node;
}
void MockDestroy(UnitOrdersStruct* node) {
    ++destroyed; assert(node->Unit_ptr);
    assert(node->Unit_ptr->UnitOrders!=node || (node->Order_State&0x10000));
}
void MockRemove(UnitStruct* unit,UnitOrdersStruct* node) {
    if (requireRotationDuringRemove) {
        assert(rotations.count(node)==1 && rotations.at(node)==expectedCleanupRotation);
    }
    auto** link=&unit->UnitOrders;
    while (*link && *link!=node) link=&(*link)->NextOrder;
    assert(*link==node); *link=node->NextOrder; ++removed;
    MockDestroy(node); MockFree(node);
}
void MockInsert(UnitStruct* unit,UnitOrdersStruct* node,UnitOrdersStruct* before) {
    auto** link=&unit->UnitOrders;
    while (*link && *link!=before) link=&(*link)->NextOrder;
    assert(*link==before); node->NextOrder=before; node->Unit_ptr=unit; *link=node;
}
using Code=std::array<unsigned char,5>;
static std::map<unsigned,Code> code;
static unsigned failPatch=0;
static std::set<unsigned> patched;
bool MockCodeMatches(unsigned address,const unsigned char* expected) {
    auto found=code.find(address);
    if (found==code.end()) return false;
    if (address==failPatch && patched.count(address)) return false;
    return std::memcmp(found->second.data(),expected,5)==0;
}
class SingleHook {
    unsigned address; Code previous;
public:
    SingleHook(unsigned a,int n,int mode,const unsigned char* patch):address(a),previous(code.at(a)) {
        assert(n==5 && mode==INLINE_UNPROTECTEVINMENT); std::memcpy(code[a].data(),patch,5); patched.insert(a);
    }
    ~SingleHook() { code[address]=previous; patched.erase(address); }
};

struct Finally { std::function<void()> cleanup; ~Finally() { cleanup(); } };

// PRODUCTION_HEADER
// PRODUCTION_MODULE

namespace MexControlTest {
// PRODUCTION_MEX_HELPER
bool QueueAsAppend(unsigned keys) {
    struct { unsigned fwKeys; } event={keys};
    const auto* mouse=&event;
    // PRODUCTION_MEX_SHIFT
    return shift;
}
}

static UnitStruct* clickUnit=nullptr;
static UnitStruct* clickTarget=nullptr;
static std::vector<unsigned> clickKinds;
static unsigned leftClicks=0,rightClicks=0,mexClicks=0,mexHandledClicks=0;
static bool smallMap=false,mexHandles=false;
static void* lastReceiver=nullptr;
static bool HasSmallMap(TAdynmemStruct*) { return smallMap; }
static void IssueNativeClick(const MOUSEEVENT* mouse) {
    if (!mouse) return;
    for (std::size_t i=0;i<clickKinds.size();++i) {
        Position_Dword p; p.X=static_cast<unsigned short>(mouse->X+i*32); p.Y=static_cast<unsigned short>(mouse->Y);
        auto* target=clickKinds[i]==ordertype::BUILD ? nullptr : clickTarget;
        PrependOrders::Dispatch(Action(clickKinds[i]),(mouse->fwKeys&MK_SHIFT)!=0,
            clickUnit,target,&p,static_cast<int>(70+i),0);
    }
}
static void MockLeftClick(void* receiver,int* mouse) {
    ++leftClicks; lastReceiver=receiver; IssueNativeClick(reinterpret_cast<MOUSEEVENT*>(mouse));
}
static void MockRightClick(void* receiver,MOUSEEVENT* mouse) {
    ++rightClicks; lastReceiver=receiver; IssueNativeClick(mouse);
}
namespace MexUpgrade {
    static bool TryCommand(TAdynmemStruct*,const MOUSEEVENT* mouse) {
        ++mexClicks;
        if (!mexHandles || !mouse || MexControlTest::UpgradeControlHeld(mouse->fwKeys)) return false;
        ++mexHandledClicks;
        for (std::size_t i=0;i<clickKinds.size();++i) {
            Position_Dword p; p.X=static_cast<unsigned short>(mouse->X+i*32); p.Y=static_cast<unsigned short>(mouse->Y);
            auto* target=clickKinds[i]==ordertype::BUILD ? nullptr : clickTarget;
            PrependOrders::Dispatch(Action(clickKinds[i]),i ? 1 : MexControlTest::QueueAsAppend(mouse->fwKeys),
                clickUnit,target,&p,static_cast<int>(70+i),0);
        }
        return true;
    }
}
namespace MouseConsumerTest {
// PRODUCTION_CLICK_WRAPPERS
}

static bool SamePosition(const Position_Dword& a,const Position_Dword& b) {
    return std::memcmp(&a,&b,sizeof(a))==0;
}
static void CodeFixture() {
    code.clear(); patched.clear(); failPatch=0;
    code[0x0043A0C0u]={{0x83,0xEC,0x0C,0x8A,0x44}};
    code[0x00439F80u]={{0x8B,0x44,0x24,0x04,0x56}};
    code[0x0043AC60u]={{0x56,0x8B,0x74,0x24,0x0C}};
    code[0x0043A1F0u]={{0x53,0x56,0x8B,0xF1,0x57}};
    code[0x0048D1F4u]={{0xE8,0xC7,0xDD,0xFA,0xFF}};
    code[0x004197A9u]={{0xE8,0x12,0x18,0x02,0x00}};
}
static void Reset() {
    assert(allocations.empty());
#if PREPEND_ORDERS_ENABLE
    assert(context==nullptr && !requested && !mutating);
    hooks.clear(); installed=false; enabled=true; modifier=0; modifierMask=3; successLogs=failureLogs=0;
#endif
    CodeFixture(); shared=Shared{}; configuration=Config{}; MyConfig=&configuration;
    PrependOrders::SetInputHooksReady(true); // Model the guarded native input installation.
    pressedKeys={VK_CONTROL,VK_SHIFT}; warnings=insertedLogs=inputDisabledLogs=0; failAction=0;
    scribblePosition=helpBuildMapping=false; lookupPositions.clear();
    allocationCalls=failAllocation=constructorCalls=removed=destroyed=0;
    std::fill(definitionFlags,definitionFlags+128,0u);
    requireRotationDuringRemove=false; expectedCleanupRotation=0;
    rotations.clear(); currentRotation=0; throwRotation=false; sent.clear();
    onNativeSend=nullptr;
    actionBegin=actionTable; actionEnd=actionTable+128;
    for (auto& definition:actionTable) definition.technicalName=nullptr;
    clickUnit=clickTarget=nullptr; clickKinds={ordertype::MOVE};
    leftClicks=rightClicks=mexClicks=mexHandledClicks=0; smallMap=mexHandles=false; lastReceiver=nullptr;
}
struct Scenario {
    TAdynmemStruct state;
    UnitStruct units[8]; UnitDefStruct definitions[8]; PlayerInfoStruct information[10];
    Scenario() {
        world=&state; state.BeginUnitsArray_p=units; state.EndOfUnitsArray_p=units+7;
        for (int i=0;i<10;++i) state.Players[i].PlayerInfo=&information[i];
        state.Players[0].Units=units+1; state.Players[0].UnitsAry_End=units+3;
        for (int i=0;i<8;++i) {
            units[i].UnitID=static_cast<short>(i+1); units[i].UnitInGameIndex=static_cast<short>(i);
            units[i].UnitType=&definitions[i]; units[i].Owner_PlayerPtr0=&state.Players[i<=3 ? 0 : 1];
            units[i].UnitSelected=0x10000010u;
        }
    }
    ~Scenario() { for (void* raw:allocations) std::free(raw); allocations.clear(); rotations.clear(); world=nullptr; }
    UnitStruct& actor() { return units[1]; }
    UnitOrdersStruct* Order(unsigned kind,int value=0,unsigned flags=1u|0x1000u) {
        auto* node=static_cast<UnitOrdersStruct*>(MockAllocate(sizeof(UnitOrdersStruct)));
        Position_Dword position; position.X=static_cast<unsigned short>(value); position.Y=320;
        MockConstruct(node,Action(kind),nullptr,&position,value,value+1,value+2);
        node->Order_State=flags; node->Unit_ptr=&actor(); return node;
    }
    void ResetCounts() { allocationCalls=constructorCalls=removed=destroyed=0; sent.clear(); }
};
static std::vector<unsigned> Commands(UnitStruct& unit) {
    std::vector<unsigned> result; unsigned count=0;
    for (auto* p=unit.UnitOrders;p;p=p->NextOrder) { assert(++count<=8192); result.push_back(p->COBHandler_index); }
    return result;
}
static void Dispatch(UnitStruct* unit,unsigned type=ordertype::MOVE,int shift=0,UnitStruct* target=nullptr) {
    Position_Dword pos; pos.X=900; pos.Y=700;
    PrependOrders::Dispatch(Action(type),shift,unit,target,&pos,71,72);
}

static void Dispatch(UnitStruct& unit,unsigned type=ordertype::MOVE,int shift=0,UnitStruct* target=nullptr) { Dispatch(&unit,type,shift,target); }

#if PREPEND_ORDERS_ENABLE
static void TestNativeInputReadiness() {
    for (unsigned mask=0;mask<4;++mask) {
        Reset(); Scenario world;
        const bool left=(mask&1)!=0, right=(mask&2)!=0;
        PrependOrders::SetInputHooksReady(left && right);
        const auto originalCode=code;
        PrependOrders::Install();
        if (left && right) {
            assert(installed && hooks.size()==2 && !inputDisabledLogs);
            continue;
        }
        assert(!installed && hooks.empty() && patched.empty() && code==originalCode);
        assert(inputDisabledLogs==1 && !PrependOrders::ModifierDown());
        assert(!PrependOrders::ModifierForMouse(MK_CONTROL|MK_SHIFT));
        auto* current=world.Order(ordertype::MOVE,111);
        world.actor().UnitOrders=current; clickUnit=&world.actor(); world.ResetCounts();
        MOUSEEVENT event; event.X=500; event.Y=600; event.fwKeys=MK_CONTROL|MK_SHIFT;
        event.Msg=WM_LBUTTONDOWN;
        MouseConsumerTest::DispatchSmallMapCommand(nullptr,nullptr,reinterpret_cast<int*>(&event));
        event.Msg=WM_RBUTTONDOWN;
        MouseConsumerTest::DispatchRightCommand(nullptr,nullptr,&event);
        assert(sent.size()==2 && sent[0].shift==1 && sent[1].shift==1);
        assert(world.actor().UnitOrders==current && removed==0 && allocationCalls==0 && warnings==0);
#if MEX_UPGRADE_ENABLE
        // A prepend installation problem does not disable the separate mex route.
        sent.clear(); mexHandles=true; event.fwKeys=0;
        MouseConsumerTest::DispatchRightCommand(nullptr,nullptr,&event);
        assert(mexHandledClicks==1 && sent.size()==1 && sent.front().shift==0);
#endif
    }
    Reset(); Scenario world; configuration.active=FALSE;
    PrependOrders::SetInputHooksReady(false);
    PrependOrders::Install();
    assert(!installed && hooks.empty() && inputDisabledLogs==0); // Explicit opt-out is quiet.
}

static void TestInstallAndControls() {
    Reset(); Scenario world;
    assert(LiveOwned(&world.state,&world.units[3]));
    assert(LiveTarget(&world.state,&world.units[7]));
    assert(!LiveTarget(&world.state,world.units+8));
    PrependOrders::Install(); assert(installed && hooks.size()==2);
    assert(PrependOrders::ModifierDown());
    assert(PrependOrders::ModifierKey(VK_CONTROL) && PrependOrders::ModifierKey(VK_SHIFT));
    assert(!PrependOrders::ModifierKey(VK_SPACE) && !PrependOrders::ModifierKey(VK_MENU));
    for (unsigned first:{VK_CONTROL,VK_SHIFT}) {
        pressedKeys.clear(); assert(!PrependOrders::ModifierDown());
        pressedKeys.insert(first); assert(!PrependOrders::ModifierDown());
        pressedKeys.insert(first==VK_CONTROL ? VK_SHIFT : VK_CONTROL);
        assert(PrependOrders::ModifierDown());
    }
    for (UINT message:{WM_KEYDOWN,WM_KEYUP,WM_SYSKEYDOWN,WM_SYSKEYUP,WM_CHAR})
        for (unsigned key:{VK_CONTROL,VK_SHIFT,VK_SPACE,65u,32u})
            assert(!PrependOrders::KeyMessage(message,key));
    for (UINT message:{WM_LBUTTONDOWN,WM_LBUTTONUP,WM_LBUTTONDBLCLK,
        WM_RBUTTONDOWN,WM_RBUTTONUP,WM_RBUTTONDBLCLK}) assert(PrependOrders::CommandMessage(message));
    for (UINT message:{WM_MOUSEMOVE,WM_KEYDOWN,WM_KEYUP,WM_SYSKEYDOWN,WM_SYSKEYUP,WM_CHAR})
        assert(!PrependOrders::CommandMessage(message));
    pressedKeys={VK_SPACE}; assert(!PrependOrders::ModifierDown()); // Native Space remains untouched.
    pressedKeys={VK_CONTROL,VK_SHIFT};
    GUIMEMSTRUCT talk; std::strcpy(talk.GUIName,"TALK.GUI");
    world.state.desktopGUI.TheActive_GUIMEM=&talk;
    assert(!PrependOrders::ModifierDown() && !PrependOrders::KeyMessage(WM_CHAR,' '));
    GUIMEMSTRUCT a,b,c; a.per_active=&b; b.per_active=&c; c.per_active=&b;
    world.state.desktopGUI.TheActive_GUIMEM=&a;
    assert(!PrependOrders::ModifierDown()); // A cycle not including the first node stays bounded.
    world.state.desktopGUI.TheActive_GUIMEM=nullptr;
    for (int mode=0;mode<6;++mode) {
        switch(mode) { case 0:shared.TAProgress=0;break; case 1:shared.PlayingDemo=1;break;
        case 2:shared.ehaOff=1;break; case 3:world.information[0].PropertyMask=WATCH;break;
        case 4:world.state.Players[0].My_PlayerType=2;break; case 5:world.state.LocalHumanPlayer_PlayerID=10;break; }
        assert(!PrependOrders::ModifierDown() && !PrependOrders::KeyMessage(WM_KEYDOWN,VK_CONTROL));
        shared=Shared{}; world.information[0].PropertyMask=0;
        world.state.Players[0].My_PlayerType=Player_LocalHuman; world.state.LocalHumanPlayer_PlayerID=0;
    }
}
static void TestPreflightAndConfiguration() {
    for (unsigned address:{0x0043A0C0u,0x00439F80u,0x0043AC60u,0x0043A1F0u,0x0048D1F4u,0x004197A9u}) {
        Reset(); Scenario world; code[address][0]^=1; const auto before=code;
        PrependOrders::Install(); assert(!installed && hooks.empty() && code==before);
    }
    Reset(); { Scenario world; const auto before=code; failPatch=0x004197A9u;
        PrependOrders::Install(); assert(!installed && hooks.empty() && patched.empty() && code==before); }
    Reset(); { Scenario world; configuration.active=FALSE; PrependOrders::Install(); assert(!installed && hooks.empty()); }
    for (int bad:{-1,0,7,256,999}) {
        Reset(); Scenario world; configuration.key=bad; PrependOrders::Install();
        assert(installed && PrependOrders::ModifierDown());
        assert(PrependOrders::ModifierKey(VK_CONTROL) && !PrependOrders::ModifierKey(VK_SPACE));
    }
    for (int badMask:{-1,0,8,999}) {
        Reset(); Scenario world; configuration.mask=badMask; PrependOrders::Install();
        assert(installed && PrependOrders::ModifierDown());
        assert(PrependOrders::ModifierKey(VK_CONTROL) && PrependOrders::ModifierKey(VK_SHIFT));
    }
    Reset(); { Scenario world; configuration.key=0x2d; pressedKeys={0x2d}; PrependOrders::Install();
        assert(PrependOrders::ModifierDown() && PrependOrders::ModifierKey(0x2d));
        assert(PrependOrders::KeyMessage(WM_KEYDOWN,0x2d) && !PrependOrders::KeyMessage(WM_CHAR,' ')); }
    Reset(); { Scenario world; configuration.key=VK_SPACE; configuration.mask=0; pressedKeys={VK_SPACE};
        PrependOrders::Install(); assert(PrependOrders::ModifierDown());
        assert(PrependOrders::KeyMessage(WM_KEYDOWN,VK_SPACE) && PrependOrders::KeyMessage(WM_CHAR,' ')); }
    Reset(); { Scenario world; configuration.mask=7; pressedKeys={VK_CONTROL,VK_SHIFT};
        PrependOrders::Install(); assert(!PrependOrders::ModifierDown());
        pressedKeys.insert(VK_MENU); assert(PrependOrders::ModifierDown() && PrependOrders::ModifierKey(VK_MENU)); }
}
static void TestMexChordKeepsNativeShiftAppend() {
    Reset(); Scenario world; PrependOrders::Install();
    pressedKeys={VK_CONTROL};
    assert(!PrependOrders::ModifierDown() && MexControlTest::UpgradeControlHeld());
    assert(MexControlTest::UpgradeControlHeld(MK_CONTROL));
    pressedKeys={VK_SHIFT};
    assert(!PrependOrders::ModifierDown() && !MexControlTest::UpgradeControlHeld());
    assert(MexControlTest::QueueAsAppend(MK_SHIFT));
    assert(MexControlTest::UpgradeControlHeld(MK_CONTROL)); // Event Ctrl also bypasses without the chord.
    pressedKeys={VK_CONTROL,VK_SHIFT};
    assert(PrependOrders::ModifierDown());
    assert(!MexControlTest::UpgradeControlHeld() && !MexControlTest::UpgradeControlHeld(MK_CONTROL|MK_SHIFT));
    assert(!MexControlTest::QueueAsAppend(MK_CONTROL|MK_SHIFT));
    pressedKeys.clear();
    assert(!PrependOrders::ModifierDown() && PrependOrders::ModifierForMouse(MK_CONTROL|MK_SHIFT));
    assert(!MexControlTest::UpgradeControlHeld(MK_CONTROL|MK_SHIFT));
    assert(!MexControlTest::QueueAsAppend(MK_CONTROL|MK_SHIFT)); // The saved click outlives its keys.
    pressedKeys.clear();
    assert(!MexControlTest::UpgradeControlHeld() && !MexControlTest::QueueAsAppend(0));
}
static void ConsumeClick(MOUSEEVENT& mouse,bool right) {
    void* receiver=reinterpret_cast<void*>(0x1234);
    if (right) MouseConsumerTest::DispatchRightCommand(receiver,nullptr,&mouse);
    else MouseConsumerTest::DispatchSmallMapCommand(receiver,nullptr,reinterpret_cast<int*>(&mouse));
}
static void TestDeferredClickAfterInputAndKeyRelease() {
    for (bool right:{false,true}) for (unsigned phase:{0u,1u,2u}) {
        Reset(); Scenario world; PrependOrders::Install();
        auto* old=world.Order(ordertype::MOVE,111); auto* tail=world.Order(ordertype::PATROL,222,1);
        old->NextOrder=tail; world.actor().UnitOrders=old; clickUnit=&world.actor();
        MOUSEEVENT queued; queued.X=500; queued.Y=600; queued.fwKeys=MK_CONTROL|MK_SHIFT;
        queued.Msg=(right ? WM_RBUTTONDOWN : WM_LBUTTONDOWN)+phase; queued.PressTime_sec=123; queued.DblClick=9;
        MOUSEEVENT delivered;
        {
            PrependOrders::Batch input(PrependOrders::CommandMessage(queued.Msg)&&PrependOrders::ModifierDown());
            delivered=queued; // TA copies the 24-byte event into its native input queue.
        }
        pressedKeys.clear(); assert(!PrependOrders::ModifierDown());
        assert(PrependOrders::ModifierForMouse(delivered.fwKeys));
        world.ResetCounts(); ConsumeClick(delivered,right);
        auto* first=world.actor().UnitOrders;
        assert(first!=old && first->Pos.X==500 && first->NextOrder->Pos.X==111);
        assert(first->NextOrder->NextOrder==tail && removed==1 && sent.empty());
        assert(leftClicks+rightClicks==1 && lastReceiver==reinterpret_cast<void*>(0x1234));
        assert(std::memcmp(&queued,&delivered,sizeof(queued))==0);
        assert(context==nullptr && !requested);
    }
}
static void TestUnmarkedDeferredClickRetainsNativeBehavior() {
    for (unsigned keys:{0u,MK_SHIFT,MK_CONTROL}) for (bool right:{false,true}) {
        Reset(); Scenario world; PrependOrders::Install();
        auto* old=world.Order(ordertype::MOVE,111); world.actor().UnitOrders=old; clickUnit=&world.actor();
        MOUSEEVENT mouse; mouse.X=500; mouse.Y=600; mouse.Msg=right ? WM_RBUTTONDOWN : WM_LBUTTONUP;
        mouse.fwKeys=keys;
        // The current keys cannot turn an earlier plain click into a prepend.
        assert(PrependOrders::ModifierDown() && !PrependOrders::ModifierForMouse(keys));
        ConsumeClick(mouse,right);
        assert(world.actor().UnitOrders==old && sent.size()==1 && removed==0);
        assert(sent.back().shift==static_cast<int>((keys&MK_SHIFT)!=0));
    }
}
static void TestDeferredBlocksAndIndependentClicks() {
    Reset(); Scenario world; PrependOrders::Install();
    world.actor().UnitOrders=world.Order(ordertype::MOVE,111); clickUnit=&world.actor();
    pressedKeys.clear();
    MOUSEEVENT first; first.X=200; first.Y=600; first.Msg=WM_LBUTTONUP; first.fwKeys=MK_CONTROL|MK_SHIFT;
    MOUSEEVENT second=first; second.X=300;
    ConsumeClick(first,false); ConsumeClick(second,false);
    auto* head=world.actor().UnitOrders;
    assert(head->Pos.X==300 && head->NextOrder->Pos.X==200 && head->NextOrder->NextOrder->Pos.X==111);
    assert(removed==2 && sent.empty());

    // Repeated synthetic grid clicks inside one input event retain one anchor.
    {
        PrependOrders::Batch outer(true);
        MOUSEEVENT synthetic; synthetic.Msg=0; synthetic.fwKeys=MK_SHIFT; synthetic.X=700; synthetic.Y=800;
        clickKinds={ordertype::BUILD}; const unsigned removedBefore=removed;
        for (unsigned i=0;i<3;++i) { synthetic.X=700+i*32; ConsumeClick(synthetic,false); }
        head=world.actor().UnitOrders;
        assert(head->Pos.X==700 && head->NextOrder->Pos.X==732 && head->NextOrder->NextOrder->Pos.X==764);
        assert(head->NextOrder->NextOrder->NextOrder->Pos.X==300 && removed==removedBefore+1 && sent.empty());
    }
}
static void TestDeferredMexAndMarginInterceptorsRemainActive() {
    Reset(); Scenario world; PrependOrders::Install();
    auto* old=world.Order(ordertype::MOVE,111); world.actor().UnitOrders=old;
    clickUnit=&world.actor(); clickTarget=&world.units[7]; pressedKeys.clear();
    MOUSEEVENT mouse; mouse.X=500; mouse.Y=600; mouse.Msg=WM_LBUTTONUP; mouse.fwKeys=MK_CONTROL|MK_SHIFT;
    smallMap=true; world.state.BuildSpotState=0;
    ConsumeClick(mouse,false);
    assert(world.actor().UnitOrders==old && leftClicks==0 && mexClicks==0 && removed==0);
    smallMap=false;
#if MEX_UPGRADE_ENABLE
    mexHandles=true; clickKinds={ordertype::RECLAIM,ordertype::BUILD};
    ConsumeClick(mouse,false);
    assert(Commands(world.actor())==std::vector<unsigned>({Action(ordertype::RECLAIM),Action(ordertype::BUILD),Action(ordertype::MOVE)}));
    assert(leftClicks==0 && mexClicks==1 && removed==1 && sent.empty());
#endif
}
static void TestDeferredMexDoesNotBorrowCurrentPhysicalChord() {
#if MEX_UPGRADE_ENABLE
    for (bool right:{false,true}) {
        Reset(); Scenario world; PrependOrders::Install();
        auto* old=world.Order(ordertype::MOVE,111); world.actor().UnitOrders=old;
        clickUnit=&world.actor(); clickTarget=&world.units[7]; mexHandles=true;
        clickKinds={ordertype::RECLAIM,ordertype::BUILD};
        MOUSEEVENT saved; saved.Msg=right ? WM_RBUTTONDOWN : WM_LBUTTONUP;
        saved.X=500; saved.Y=600; saved.fwKeys=MK_SHIFT;
        assert(PrependOrders::ModifierDown() && !PrependOrders::ModifierForMouse(saved.fwKeys));
        assert(!MexControlTest::UpgradeControlHeld(saved.fwKeys));
        assert(MexControlTest::QueueAsAppend(saved.fwKeys));
        ConsumeClick(saved,right);
        assert(world.actor().UnitOrders==old && removed==0 && sent.size()==2);
        assert(sent[0].shift==1 && sent[1].shift==1 && mexHandledClicks==1);
        assert(leftClicks+rightClicks==0 && !PrependOrders::Active());

        sent.clear(); clickKinds={ordertype::MOVE}; saved.fwKeys=MK_CONTROL;
        assert(MexControlTest::UpgradeControlHeld(saved.fwKeys));
        ConsumeClick(saved,right);
        assert(mexHandledClicks==1 && leftClicks+rightClicks==1 && sent.size()==1);
        assert(world.actor().UnitOrders==old && removed==0);
    }
    Reset(); Scenario world; PrependOrders::Install();
    world.actor().UnitOrders=world.Order(ordertype::MOVE,111);
    clickUnit=&world.actor(); clickTarget=&world.units[7]; mexHandles=true;
    clickKinds={ordertype::RECLAIM,ordertype::BUILD}; pressedKeys.clear();
    MOUSEEVENT synthetic; synthetic.Msg=WM_LBUTTONUP; synthetic.fwKeys=MK_SHIFT;
    {
        PrependOrders::Batch outer(true); assert(PrependOrders::Active());
        ConsumeClick(synthetic,false);
        assert(Commands(world.actor())==std::vector<unsigned>({Action(ordertype::RECLAIM),Action(ordertype::BUILD),Action(ordertype::MOVE)}));
        assert(removed==1 && sent.empty() && PrependOrders::Active());
    }
    assert(!PrependOrders::Active());
#endif
}
static void TestNativePassthrough() {
    Reset(); Scenario world;
    Position_Dword p; p.X=123; p.z_=456; p.Y=789;
    PrependOrders::Dispatch(0x12340037u,1,&world.actor(),&world.units[7],&p,81,82);
    assert(sent.size()==1 && sent.back().action==0x12340037u && sent.back().shift==1);
    assert(sent.back().pointer==&p && SamePosition(sent.back().position,p));
    assert(sent.back().target==&world.units[7] && sent.back().p1==81 && sent.back().p2==82);
    PrependOrders::Install();
    { PrependOrders::Batch off(false); Dispatch(world.actor()); }
    assert(sent.size()==2 && allocations.empty());
}
static void TestRealIdleHandlersUseNativeAppend() {
    const char* names[]={"Standby","VTOL_Standby","Stop",nullptr};
    for (unsigned i=0;i<4;++i) {
        Reset(); Scenario world; PrependOrders::Install();
        UnitOrdersStruct* head=nullptr;
        if (names[i]) {
            head=world.Order(ordertype::STOP,0,0x1000u);
            head->COBHandler_index=static_cast<unsigned char>(i ? 90+i : 0);
            actionTable[head->COBHandler_index].technicalName=names[i];
            world.actor().UnitOrders=head;
        }
        failAction=ordertype::STOP; // An idle handler is not the contextual STOP mapping.
        world.ResetCounts();
        Position_Dword p; p.X=600; p.Y=700;
        {
            PrependOrders::Batch batch(true);
            PrependOrders::Dispatch(0x12340000u|Action(ordertype::LOAD),0,&world.actor(),&world.units[7],&p,51,52);
        }
        assert(sent.size()==1 && sent[0].shift==1 && sent[0].action==(0x12340000u|Action(ordertype::LOAD)));
        assert(sent[0].p1==51 && sent[0].p2==52 && sent[0].pointer==&p);
        assert(world.actor().UnitOrders==head && removed==0 && allocationCalls==0 && warnings==0);
    }
}
static void TestIdleFallbackStaysPerUnitForTheWholeBatch() {
    Reset(); Scenario world; PrependOrders::Install();
    auto* standby=world.Order(ordertype::STOP,0,0x1000u);
    standby->COBHandler_index=0; actionTable[0].technicalName="Standby";
    world.actor().UnitOrders=standby;
    UnitStruct& busy=world.units[3]; busy.UnitOrders=world.Order(ordertype::MOVE,333);
    busy.UnitOrders->Unit_ptr=&busy;
    onNativeSend=[&](const Sent& command) {
        assert(command.unit==&world.actor() && command.shift==1);
        auto* fresh=static_cast<UnitOrdersStruct*>(MockAllocate(sizeof(UnitOrdersStruct)));
        MockConstruct(fresh,command.action&0xffu,command.target,&command.position,command.p1,command.p2,0);
        fresh->Unit_ptr=command.unit; fresh->Order_State=1u|0x1000u;
        auto*& head=command.unit->UnitOrders;
        if (head==standby) { head=fresh; MockFree(standby); }
        else {
            auto* last=head;
            while (last->NextOrder) last=last->NextOrder;
            last->Order_State&=~0x1000u; last->NextOrder=fresh;
        }
    };
    {
        PrependOrders::Batch batch(true);
        Dispatch(world.actor(),ordertype::MOVE);
        assert(world.actor().UnitOrders!=standby);
        Dispatch(world.actor(),ordertype::BUILD);
        Dispatch(world.actor(),ordertype::RECLAIM,0,&world.units[7]);
        assert(sent.size()==3 && removed==0 && warnings==0);
        assert(Commands(world.actor())==std::vector<unsigned>({Action(ordertype::MOVE),Action(ordertype::BUILD),Action(ordertype::RECLAIM)}));
        Dispatch(busy,ordertype::ATTACK,0,&world.units[7]);
        assert(sent.size()==3 && removed==1);
        assert(Commands(busy)==std::vector<unsigned>({Action(ordertype::ATTACK),Action(ordertype::MOVE)}));
    }
    onNativeSend=nullptr;
    {
        PrependOrders::Batch nextInput(true);
        Dispatch(world.actor(),ordertype::ATTACK,0,&world.units[7]);
        assert(sent.size()==3 && removed==2);
        assert(Commands(world.actor())==std::vector<unsigned>({Action(ordertype::ATTACK),Action(ordertype::MOVE),Action(ordertype::BUILD),Action(ordertype::RECLAIM)}));
    }
}
static void TestIdleNameDoesNotAdmitActiveOrInvalidQueues() {
    for (unsigned failure=0;failure<6;++failure) {
        Reset(); Scenario world; PrependOrders::Install();
        auto* head=world.Order(ordertype::STOP,0,0x1000u); head->COBHandler_index=0;
        actionTable[0].technicalName="Standby"; world.actor().UnitOrders=head;
        switch (failure) {
        case 0:head->NextOrder=world.Order(ordertype::MOVE,111);break;
        case 1:head->AttackTargat=&world.units[7];break;
        case 2:head->Order_State|=0x40000u;break;
        case 3:head->Unit_ptr=&world.units[3];break;
        case 4:actionTable[0].technicalName="Standby_Attack";break;
        case 5:actionEnd=actionBegin;break;
        }
        world.ResetCounts();
        { PrependOrders::Batch batch(true); Dispatch(world.actor()); }
        assert(world.actor().UnitOrders==head && removed==0 && allocationCalls==0 && sent.empty() && warnings==1);
    }
}
static void TestNativeByteActionABI() {
    for (unsigned kind:{ordertype::MOVE,ordertype::BUILD}) {
        Reset(); Scenario world; PrependOrders::Install();
        world.actor().UnitOrders=world.Order(ordertype::MOVE);
        Position_Dword p; p.X=512; p.Y=768;
        {
            PrependOrders::Batch batch(true);
            PrependOrders::Dispatch(0x12340000u|Action(kind),0,&world.actor(),nullptr,&p,22,23);
            assert(world.actor().UnitOrders->COBHandler_index==Action(kind));
            assert(world.actor().UnitOrders->BuildUnitID==22 && removed==1 && warnings==0);
        }
    }
}
static void TestActionClassificationCannotEditDestinations() {
    Reset(); Scenario world; PrependOrders::Install();
    auto* old=world.Order(ordertype::MOVE,345); world.actor().UnitOrders=old;
    const Position_Dword original=old->Pos;
    Position_Dword urgent; urgent.X=700; urgent.Y=800; urgent.z_=123;
    scribblePosition=true;
    NativeEngine engine; NativeEngine::Request resume;
    assert(engine.SnapshotResume(world.actor(),old,resume)==PrependQueue::CurrentKind::Resume);
    assert(SamePosition(old->Pos,original) && SamePosition(resume.position,original));
    {
        PrependOrders::Batch batch(true);
        PrependOrders::Dispatch(Action(ordertype::ATTACK),0,&world.actor(),&world.units[7],&urgent,0,0);
        auto* first=world.actor().UnitOrders;
        assert(first->COBHandler_index==Action(ordertype::ATTACK));
        assert(SamePosition(first->Pos,urgent) && SamePosition(first->NextOrder->Pos,original));
    }
    assert(lookupPositions.size()>=4);
    for (const auto& lookedUp:lookupPositions)
        assert(SamePosition(lookedUp,original) || SamePosition(lookedUp,urgent));
}
static void TestMoveAndBatchOrder() {
    Reset(); Scenario world; PrependOrders::Install();
    auto* old=world.Order(ordertype::MOVE,111); old->State=6;
    auto* tail=world.Order(ordertype::PATROL,222,1); old->NextOrder=tail;
    world.actor().UnitOrders=old; const Position_Dword original=old->Pos;
    world.ResetCounts();
    {
        PrependOrders::Batch batch(true);
        Dispatch(world.actor(),ordertype::ATTACK,1,&world.units[7]); // Ctrl+Shift overrides the Shift append bit.
        auto* first=world.actor().UnitOrders; auto* resume=first->NextOrder;
        assert(Commands(world.actor())==std::vector<unsigned>({Action(ordertype::ATTACK),Action(ordertype::MOVE),Action(ordertype::PATROL)}));
        assert(resume!=old && resume->NextOrder==tail && resume->State==0 && SamePosition(resume->Pos,original));
        assert(resume->BuildUnitID==111 && resume->parameter2==112 && resume->parameter3==113);
        assert((resume->Order_State&0x1000) && !(first->Order_State&0x1000));
        assert(first->AttackTargat==&world.units[7] && first->BuildUnitID==71 && first->parameter2==72 && first->parameter3==0);
        { PrependOrders::Batch inner(false); Dispatch(world.actor()); }
        Dispatch(world.actor(),ordertype::REPAIR,0,&world.units[7]);
        assert(Commands(world.actor())==std::vector<unsigned>({Action(ordertype::ATTACK),Action(ordertype::REPAIR),Action(ordertype::MOVE),Action(ordertype::PATROL)}));
        assert(removed==1 && sent.size()==1 && allocations.size()==4);
    }
}
static void TestKnownUserBitFourCanResumeFresh() {
    for (unsigned kind:{ordertype::MOVE,ordertype::BUILD}) {
        Reset(); Scenario world; PrependOrders::Install();
        auto* old=world.Order(kind,111,1u|4u|0x1000u);
        auto* tail=world.Order(ordertype::PATROL,222,1u);
        old->NextOrder=tail; old->State=6; world.actor().UnitOrders=old;
        // The constructor owns static definition flags; the adapter must not
        // copy the running order's bit4 or other temporary handler state.
        definitionFlags[Action(kind)]=0x80u;
        {
            PrependOrders::Batch batch(true);
            Dispatch(world.actor(),ordertype::ATTACK,0,&world.units[7]);
            auto* urgent=world.actor().UnitOrders; auto* resumed=urgent->NextOrder;
            assert(urgent->COBHandler_index==Action(ordertype::ATTACK));
            assert(resumed->COBHandler_index==Action(kind) && resumed->NextOrder==tail);
            assert(resumed->State==0 && (resumed->Order_State&0x80u));
            assert(!(resumed->Order_State&4u) && (resumed->Order_State&0x1000u));
            assert(removed==1 && warnings==0);
        }
    }
}
static void TestBuildResumptionAndRotation() {
    for (bool flying:{false,true}) for (unsigned stage:{0u,1u,2u}) {
        const bool started=stage!=0;
        Reset(); Scenario world; PrependOrders::Install();
        world.actor().UnitType->flying=flying; helpBuildMapping=true;
        auto* old=world.Order(ordertype::BUILD,333); old->State=started ? 3 : 0;
        auto* tail=world.Order(ordertype::PATROL,444,1u); old->NextOrder=tail;
        if (started) { old->AttackTargat=&world.units[7]; world.units[7].Nanoframe=stage==2 ? 1.0f : 0.4f; }
        rotations[old]=3; world.actor().UnitOrders=old; world.ResetCounts(); currentRotation=1;
        requireRotationDuringRemove=true; expectedCleanupRotation=3;
        {
            PrependOrders::Batch batch(true); Dispatch(world.actor(),ordertype::BUILD);
            auto* urgent=world.actor().UnitOrders; auto* resume=urgent->NextOrder;
            assert(rotations.at(urgent)==1 && urgent->BuildUnitID==71);
            assert(resume->NextOrder==tail);
            assert(resume->COBHandler_index==(started ? (flying ? 91u : 90u) : Action(ordertype::BUILD)));
            if (started) {
                // The native REPAIR selector resolves a nanoframe to the
                // appropriate HELPBUILD handler, including zero progress (1.0).
                assert(resume->AttackTargat==&world.units[7] && resume->BuildUnitID==0 && resume->parameter2==0 && resume->parameter3==0);
            }
            else { assert(rotations.at(resume)==3 && resume->BuildUnitID==333 && resume->parameter2==334 && resume->parameter3==335); }
            assert(rotations.count(old)==0 && removed==1 && warnings==0);
        }
    }
}
static void TestRefusalAndStaleIdentity() {
    for (int failure=0;failure<14;++failure) {
        Reset(); Scenario world; PrependOrders::Install();
        auto* old=world.Order(ordertype::MOVE,10); world.actor().UnitOrders=old;
        UnitStruct* actor=&world.actor(); UnitStruct* target=nullptr; unsigned type=ordertype::MOVE;
        switch(failure) {
        case 0:actor=&world.units[4];break;
        case 1:world.actor().UnitID=0;break;
        case 2:world.actor().UnitSelected=0;break;
        case 3:world.actor().Nanoframe=0.5f;break;
        case 4:world.actor().Owner_PlayerPtr0=&world.state.Players[1];break;
        case 5:actor=reinterpret_cast<UnitStruct*>(reinterpret_cast<unsigned char*>(actor)+1);break;
        case 6:target=&world.units[7]; target->UnitID=0;break;
        case 7:type=ordertype::LOAD;break;
        case 8:old->Order_State|=0x40000;break;
        case 9:old->COBHandler_index=127;break;
        case 10:old->COBHandler_index=static_cast<unsigned char>(Action(ordertype::LOAD));break;
        case 11:old->COBHandler_index=static_cast<unsigned char>(Action(ordertype::BUILD));world.actor().UnitType->bmcode=0;break;
        case 12:target=world.units+8;break;
        case 13:world.state.EndOfUnitsArray_p=world.units;break;
        }
        world.ResetCounts();
        { PrependOrders::Batch batch(true); Dispatch(actor,type,0,target); }
        assert(world.actor().UnitOrders==old && removed==0 && constructorCalls==0 && sent.empty() && warnings==1);
    }
    Reset(); Scenario world; PrependOrders::Install(); world.actor().UnitOrders=world.Order(ordertype::MOVE);
    {
        PrependOrders::Batch batch(true); Dispatch(world.actor());
        auto* first=world.actor().UnitOrders; const auto before=Commands(world.actor()); const unsigned removedBefore=removed;
        ++world.actor().UnitID; Dispatch(world.actor());
        assert(world.actor().UnitOrders==first && Commands(world.actor())==before && removed==removedBefore);
    }
}
static void TestNativeFailurePreservesQueue() {
    for (unsigned fail:{1u,2u}) {
        Reset(); Scenario world; PrependOrders::Install(); auto* old=world.Order(ordertype::MOVE);
        world.actor().UnitOrders=old; world.ResetCounts(); failAllocation=fail;
        { PrependOrders::Batch batch(true); Dispatch(world.actor()); }
        assert(world.actor().UnitOrders==old && allocations.size()==1 && constructorCalls==0 && removed==0 && warnings==1);
    }
    Reset(); Scenario world; PrependOrders::Install(); auto* old=world.Order(ordertype::BUILD);
    world.actor().UnitOrders=old; rotations[old]=2; world.ResetCounts(); throwRotation=true;
    { PrependOrders::Batch batch(true); Dispatch(world.actor(),ordertype::BUILD); }
    assert(world.actor().UnitOrders==old && allocations.size()==1 && rotations.at(old)==2 && removed==0);
}
static void TestFailedBatchCannotQueueItsTail() {
    Reset(); Scenario world; PrependOrders::Install();
    auto* old=world.Order(ordertype::MOVE,515); world.actor().UnitOrders=old;
    world.ResetCounts(); failAllocation=1;
    {
        PrependOrders::Batch batch(true);
        Dispatch(world.actor(),ordertype::RECLAIM,0,&world.units[7]);
        assert(world.actor().UnitOrders==old && allocationCalls==1 && removed==0);
        failAllocation=0; // Resource pressure can recover before the next cell/order.
        Dispatch(world.actor(),ordertype::BUILD);
        assert(world.actor().UnitOrders==old && allocationCalls==1 && constructorCalls==0 && removed==0);
    }
    {
        PrependOrders::Batch retry(true);
        Dispatch(world.actor());
        assert(world.actor().UnitOrders!=old && removed==1 && warnings==1);
        assert(Commands(world.actor())==std::vector<unsigned>({Action(ordertype::MOVE),Action(ordertype::MOVE)}));
    }
}
#endif
int main() {
#if PREPEND_ORDERS_ENABLE
    assert(!inputHooksReady); // Never arm a partially installed feature by default.
    TestNativeInputReadiness(); TestInstallAndControls(); TestPreflightAndConfiguration(); TestMexChordKeepsNativeShiftAppend(); TestNativePassthrough();
    TestRealIdleHandlersUseNativeAppend(); TestIdleFallbackStaysPerUnitForTheWholeBatch();
    TestIdleNameDoesNotAdmitActiveOrInvalidQueues();
    TestDeferredClickAfterInputAndKeyRelease(); TestUnmarkedDeferredClickRetainsNativeBehavior();
    TestDeferredBlocksAndIndependentClicks(); TestDeferredMexAndMarginInterceptorsRemainActive();
    TestDeferredMexDoesNotBorrowCurrentPhysicalChord();
    TestNativeByteActionABI(); TestActionClassificationCannotEditDestinations();
    TestMoveAndBatchOrder(); TestKnownUserBitFourCanResumeFresh();
    TestBuildResumptionAndRotation(); TestRefusalAndStaleIdentity();
    TestNativeFailurePreservesQueue(); TestFailedBatchCannotQueueItsTail();
    std::cout << "Production prepend adapter, ownership, Ctrl+Shift and native bridges passed\n";
#else
    Reset(); Scenario world;
    PrependOrders::Install(); assert(!PrependOrders::ModifierDown() && !PrependOrders::ModifierKey(VK_SPACE));
    assert(!PrependOrders::KeyMessage(WM_KEYDOWN,VK_SPACE) && !PrependOrders::CommandMessage(WM_LBUTTONDOWN));
    assert(MexControlTest::UpgradeControlHeld(MK_CONTROL) && MexControlTest::QueueAsAppend(MK_SHIFT));
    { PrependOrders::Batch batch(true); Dispatch(world.actor()); }
    assert(sent.size()==1 && sent.front().shift==0 && allocations.empty());
    std::cout << "Compiled-out prepend adapter preserves native dispatch\n";
#endif
}
'''


def prepare_source():
    source = (DDRAW / "PrependOrders.cpp").read_text()
    source = re.sub(r'^\s*#include[^\n]*$', '', source, flags=re.M)
    source = re.sub(r'^\s*static_assert[^\n]*$', '', source, flags=re.M)
    for native, mock in (
        ("NativeSend", "MockSend"), ("NativeAllocate", "MockAllocate"),
        ("NativeFree", "MockFree"), ("NativeConstruct", "MockConstruct"),
        ("NativeDestroy", "MockDestroy"), ("NativeRemove", "MockRemove"),
        ("NativeInsert", "MockInsert"), ("NativeAction", "MockAction"),
    ):
        pattern = r'const auto ' + native + r'\s*=\s*reinterpret_cast<.*?;'
        source, count = re.subn(pattern, 'const auto ' + native + ' = ' + mock + ';', source, flags=re.S)
        assert count == 1, (native, count)
    envelope = extract(source, 'PrependQueue::Result InsertNative(')
    source = source.replace(envelope, normalize_finally(envelope))
    bridge = extract(source, 'bool CodeMatches(')
    source = source.replace(bridge, 'bool CodeMatches(unsigned address, const unsigned char* expected) { return MockCodeMatches(address,expected); }')
    source = source.replace('reinterpret_cast<Send>(0x0043AFC0u)', 'static_cast<Send>(&MockSend)')
    # C++ exceptions model only the read guard in this Linux harness. Windows
    # SEH and access-violation behavior are checked by native builds/runtime.
    source = source.replace('__try', 'try').replace('__except (EXCEPTION_EXECUTE_HANDLER)', 'catch (...)')
    header = (DDRAW / 'PrependOrders.h').read_text()
    header = re.sub(r'^\s*(?:#include[^\n]*|#pragma once)\s*$', '', header, flags=re.M)
    header = header.replace('typedef struct _Position_Dword Position_Dword;', '')
    mex = (DDRAW / 'MexUpgrade.cpp').read_text()
    control = extract(mex, 'bool UpgradeControlHeld(')
    shifts = re.findall(r'const bool shift =.*?;', mex, re.S)
    assert len(shifts) == 1
    hooks = (DDRAW / 'TABugFix.cpp').read_text().lstrip('\ufeff')
    wrappers = extract(hooks, 'void __fastcall DispatchSmallMapCommand(')
    wrappers += '\n#if MEX_UPGRADE_ENABLE || PREPEND_ORDERS_ENABLE\n'
    wrappers += extract(hooks, 'void __fastcall DispatchRightCommand(') + '\n#endif\n'
    for bridge, mock in (
        ('reinterpret_cast<void (__thiscall*)(void*, int*)>(0x00498F70u)', 'MockLeftClick'),
        ('reinterpret_cast<void (__thiscall*)(void*, MOUSEEVENT*)>(0x00499100u)', 'MockRightClick'),
    ):
        assert wrappers.count(bridge) == 1
        wrappers = wrappers.replace(bridge, mock)
    return (HARNESS.replace('// PRODUCTION_HEADER', header).replace('// PRODUCTION_MODULE', source)
        .replace('// PRODUCTION_MEX_HELPER', control).replace('// PRODUCTION_MEX_SHIFT', shifts[0])
        .replace('// PRODUCTION_CLICK_WRAPPERS', wrappers))


def main():
    compiler = shlex.split(os.environ.get('CXX', 'g++'))
    test_input_integration(compiler)
    with tempfile.TemporaryDirectory(prefix='prepend-adapter-test-') as temporary:
        root = Path(temporary)
        cpp = root / 'adapter.cpp'
        cpp.write_text(prepare_source())
        for enabled, mex in ((0,0),(1,0),(1,1)):
            binary = root / ('adapter_' + str(enabled) + '_' + str(mex))
            subprocess.run(compiler + ['-std=c++17', '-Wall', '-Wextra', '-Werror',
                '-Wno-unused-function', '-Wno-unused-variable', '-fsanitize=address,undefined', '-g',
                '-fno-omit-frame-pointer', '-DPREPEND_ORDERS_ENABLE=' + str(enabled), '-DMEX_UPGRADE_ENABLE=' + str(mex),
                '-I', str(DDRAW), str(cpp), '-o', str(binary)], check=True)
            subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})


def test_input_integration(compiler):
    surface = (DDRAW / 'iddrawsurface.cpp').read_text()
    outer = extract(surface, 'LRESULT CALLBACK WinProc(')
    inner = extract(surface, 'LRESULT CALLBACK _WinProc(')
    assert 'PrependOrders::Batch prependBatch(PrependOrders::CommandMessage(Msg) && PrependOrders::ModifierDown());' in outer
    assert outer.index('PrependOrders::Batch') < outer.index('return _WinProc(')
    assert '__try' not in outer and 'PrependOrders::Batch' not in inner
    # Native chat is checked by the module; custom text editors get first refusal.
    assert inner.index('NowSupportUnicode->Message') < inner.index('PrependOrders::KeyMessage')
    assert inner.index('Dialog*)LocalShare->Dialog)->Message') < inner.index('PrependOrders::KeyMessage')
    assert inner.index('AlliesWhiteboard*)LocalShare->Whiteboard)->Message') < inner.index('PrependOrders::KeyMessage')
    startup = (DDRAW / 'ddraw.cpp').read_text()
    assert startup.index('MyConfig = new TADRConfig') < startup.index('PrependOrders::Install()')
    assert '<ClCompile Include="PrependOrders.cpp" />' in (DDRAW / 'ddraw.vcxproj').read_text()
    mex = (DDRAW / 'MexUpgrade.cpp').read_text()
    mex = re.sub(r'^\s*#include[^\n]*$', '', mex, flags=re.M)
    for enabled in (0, 1):
        output = subprocess.run(compiler + ['-E', '-P', '-x', 'c++', '-DMEX_UPGRADE_ENABLE=1',
            '-DPREPEND_ORDERS_ENABLE=' + str(enabled), '-'], input=mex, text=True,
            capture_output=True, check=True).stdout
        assert ('const Dispatch Send = &PrependOrders::Dispatch;' in output) == bool(enabled)
        assert ('&& !PrependOrders::ModifierForMouse(mouse->fwKeys)' in output) == bool(enabled)
    hooks = (DDRAW / 'TABugFix.cpp').read_text().lstrip('\ufeff')
    hooks = re.sub(r'^\s*#include[^\n]*$', '', hooks, flags=re.M)
    for prepend, mex_enabled in ((0,0),(0,1),(1,0),(1,1)):
        output = subprocess.run(compiler + ['-E','-P','-x','c++',
            '-DMEX_UPGRADE_ENABLE='+str(mex_enabled), '-DPREPEND_ORDERS_ENABLE='+str(prepend), '-'],
            input=hooks, text=True, capture_output=True, check=True).stdout
        assert ('void __fastcall DispatchRightCommand(' in output) == bool(prepend or mex_enabled)
        assert ('void __cdecl DispatchUpgradeCursor(' in output) == bool(mex_enabled)
        assert ('PrependOrders::ModifierForMouse' in output) == bool(prepend)
        assert ('PrependOrders::SetInputHooksReady(SmallMapHooksInstalled && right);' in output) == bool(prepend)
        if prepend:
            left = extract(output, 'void __fastcall DispatchSmallMapCommand(')
            right = extract(output, 'void __fastcall DispatchRightCommand(')
            assert left.index('PrependOrders::Batch') < left.index('0x00498F70u')
            assert 'ModifierForMouse(mouse->fwKeys), true)' in left
            assert 'ModifierForMouse(message->fwKeys), true)' in right
    for profile in ('ESCALATION','PROTA','OTA','TAZERO','BTA','MAYHEM','TWILIGHT'):
        source = '#include "config.h"\nPREPEND_ORDERS_ENABLE\n'
        result = subprocess.run(compiler + ['-E','-P','-x','c++','-DTDRAW_CONFIG_'+profile,
            '-I',str(DDRAW),'-'], input=source, text=True, capture_output=True, check=True).stdout
        assert result.strip().splitlines()[-1] == ('1' if profile == 'ESCALATION' else '0')
    print('Prepend native input integration and all seven mod defaults passed')


if __name__ == '__main__':
    main()
