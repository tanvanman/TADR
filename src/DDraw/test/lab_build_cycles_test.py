#!/usr/bin/env python3
"""Compile the actual lab-cycle controller with mocked Windows/native bridges.

Win32 pointer tokens retain the production unit/order offsets on a 64-bit host.
The native append/count/completion behavior is a test fixture, not replacement
cycle code. Retail hook addresses, SEH faults and gameplay remain Windows/Wine
checks; this test never attaches to or launches a game.
"""
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent


def adapted_source():
    source = (ROOT / 'LabBuildCycles.cpp').read_text()
    source = '\n'.join(line for line in source.splitlines() if not line.startswith('#include'))
    bridge = 'NativeAdjust const AdjustBuildCount = reinterpret_cast<NativeAdjust>(0x0043B0B0u);'
    assert source.count(bridge) == 1
    source = source.replace(bridge, 'NativeAdjust const AdjustBuildCount = &FakeAdjustBuildCount;')
    source = source.replace('__try', 'try').replace('__except (EXCEPTION_EXECUTE_HANDLER)', 'catch (...)')
    assert '__except' not in source and '__finally' not in source
    return source


HARNESS = r'''
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "LabBuildCycles.h"
#include "LabBuildCycleMenu.h"
#define __stdcall
using SIZE_T=std::size_t;
constexpr int TAInGame=6, Player_LocalHuman=1;
constexpr unsigned WATCH=0x40, UnitSelected_State=0x10, builder=0x40;
constexpr int INLINE_5BYTESLAGGERJMP=0x1002;
std::vector<void*> pointerTokens(1,nullptr);
#pragma pack(push,1)
template<class T> struct NativePtr {
    std::uint32_t token=0;
    NativePtr& operator=(T* pointer) {
        auto it=std::find(pointerTokens.begin(),pointerTokens.end(),pointer);
        if(it==pointerTokens.end()){token=static_cast<std::uint32_t>(pointerTokens.size());pointerTokens.push_back(pointer);}
        else token=static_cast<std::uint32_t>(it-pointerTokens.begin());
        return *this;
    }
    operator T*() const {assert(token<pointerTokens.size());return static_cast<T*>(pointerTokens[token]);}
    T* operator->() const {return static_cast<T*>(*this);}
};
struct PlayerStruct;
struct Object3doStruct {unsigned cookie=0;};
struct UnitDefStruct {
    int bmcode=0;unsigned UnitTypeMask_0=builder;unsigned char GUINum=1;
    int canbuildCount=1;short* CANBUILD_ptr=nullptr;
};
struct UnitOrdersStruct {
    unsigned vtable=0;unsigned char COBHandler_index=7,State=0;
    unsigned char toUnit[8]={};NativePtr<UnitStruct> Unit_ptr;
    unsigned field_12=0;NativePtr<UnitStruct> AttackTargat;
    unsigned char toBuild[0x36-0x1a]={};unsigned BuildUnitID=0;
    int BuildCount=0;unsigned char toNext[0x4a-0x3e]={};
    NativePtr<UnitOrdersStruct> NextOrder;unsigned char tail[8]={};
};
struct UnitStruct {
    int IsUnit=0;unsigned char toOrders[0x5c-4]={};
    NativePtr<UnitOrdersStruct> UnitOrders;unsigned char toType[0x92-0x60]={};
    NativePtr<UnitDefStruct> UnitType;NativePtr<PlayerStruct> Owner_PlayerPtr0;
    NativePtr<void> UnkPTR2;NativePtr<Object3doStruct> Object3do;
    unsigned Order_Unknow=0;short UnitID=0,UnitInGameIndex=0;
    unsigned char toOwner[0xff-0xaa]={};unsigned char cOwnerID=0;
    unsigned char toFrame[4]={};float Nanoframe=0;short Health=100;
    unsigned char toFlags[6]={};unsigned UnitSelected=0;unsigned char tail[4]={};
};
#pragma pack(pop)
static_assert(sizeof(UnitOrdersStruct)==0x56,"native order fixture");
struct PlayerInfo {unsigned PropertyMask=0;};
struct PlayerStruct {
    int PlayerActive=1,My_PlayerType=Player_LocalHuman;
    ::PlayerInfo* PlayerInfo=nullptr;UnitStruct* Units=nullptr;UnitStruct* UnitsAry_End=nullptr;
};
struct TAdynmemStruct {
    UnitStruct* BeginUnitsArray_p=nullptr;UnitStruct* EndOfUnitsArray_p=nullptr;
    UnitDefStruct* UnitDef=nullptr;unsigned UNITINFOCount=0;
    int GameTime=0;char LocalHumanPlayer_PlayerID=1;unsigned GameStateMask=0;
    PlayerStruct Players[10];
};
struct Shared {int TAProgress=TAInGame;bool PlayingDemo=false;int ehaOff=0;} shared;
Shared* DataShare=&shared;
TAdynmemStruct* currentWorld=nullptr;
TAdynmemStruct** TAmainStruct_PtrPtr=&currentWorld;
struct InlineX86StackBuffer {std::uintptr_t Edi=0,Esi=0,Esp=0;};
using PInlineX86StackBuffer=InlineX86StackBuffer*;
unsigned hookCreates=0,hookDeletes=0,throwHookNumber=0;
class InlineSingleHook {
public:
    InlineSingleHook(unsigned,unsigned,int,int(*)(PInlineX86StackBuffer)) {
        ++hookCreates;if(hookCreates==throwHookNumber)throw std::runtime_error("hook failure");
    }
    ~InlineSingleHook(){++hookDeletes;}
};
struct GameTickHook {
    std::vector<void(*)(int)> callbacks;
    static GameTickHook* GetInstance(){static GameTickHook hook;return &hook;}
    void addCallback(void(*callback)(int)){callbacks.push_back(callback);}
};
std::vector<std::string> logs;
struct IDDrawSurface {static void OutptTxt(const char* text){logs.emplace_back(text);}};
bool menuCompatible=true;unsigned menuInstalls=0,menuShutdowns=0,menuUpdates=0;
namespace LabBuildCycleMenu {
bool Install(){++menuInstalls;return menuCompatible;}
void Shutdown(){++menuShutdowns;}
void Update(){++menuUpdates;}
}
bool ReadProcessMemory(void*,const void*,void*,SIZE_T,SIZE_T*);
void* GetCurrentProcess(){return nullptr;}
void FakeAdjustBuildCount(unsigned,UnitStruct*,int,int);

// PRODUCTION

unsigned badAddress=0,shortReadAddress=0,readCalls=0,adjustCalls=0;
bool throwAdjust=false;
bool ReadProcessMemory(void*,const void* pointer,void* output,SIZE_T length,SIZE_T* read) {
    ++readCalls;
    const auto address=reinterpret_cast<std::uintptr_t>(pointer);
    const unsigned char* expected=nullptr;std::size_t size=0;
#if LAB_BUILD_CYCLES_ENABLE
    if(address==kCompletedCall){expected=kCompletedBytes;size=sizeof(kCompletedBytes);}
    if(address==0x00402640u){expected=kBuildingEntryBytes;size=sizeof(kBuildingEntryBytes);}
    if(address==0x0043B0B0u){expected=kAdjustBytes;size=sizeof(kAdjustBytes);}
    if(address==kInitUnit){expected=kInitBytes;size=sizeof(kInitBytes);}
#endif
    if(!expected||length!=size){*read=0;return false;}
    std::memcpy(output,expected,length);*read=length;
    if(address==badAddress)static_cast<unsigned char*>(output)[0]^=1;
    if(address==shortReadAddress)--*read;
    return true;
}

// Native fixture: AdjustBuildCount merges only a matching tail, otherwise
// appends; negative clicks remove the last matching type, as 0x43B0B0 does.
std::vector<std::unique_ptr<UnitOrdersStruct>> nodes;
UnitOrdersStruct* MakeOrder(UnitStruct* lab,unsigned type,int amount) {
    nodes.emplace_back(new UnitOrdersStruct());auto* order=nodes.back().get();
    order->Unit_ptr=lab;order->BuildUnitID=type;order->BuildCount=amount;return order;
}
void FakeAdjustBuildCount(unsigned kind,UnitStruct* lab,int type,int amount) {
    if(throwAdjust)throw std::bad_alloc();
    ++adjustCalls;assert(kind==7&&type>0);
    UnitOrdersStruct* tail=lab->UnitOrders;
    if(amount>0){
        if(tail)while(tail->NextOrder)tail=tail->NextOrder;
        if(tail&&tail->COBHandler_index==kind&&tail->BuildUnitID==static_cast<unsigned>(type))tail->BuildCount+=amount;
        else {auto* fresh=MakeOrder(lab,static_cast<unsigned>(type),amount);if(tail)tail->NextOrder=fresh;else lab->UnitOrders=fresh;}
        return;
    }
    while(amount<0){
        UnitOrdersStruct* previous=nullptr;UnitOrdersStruct* found=nullptr;UnitOrdersStruct* foundPrevious=nullptr;
        for(UnitOrdersStruct* item=lab->UnitOrders;item;item=item->NextOrder){
            if(item->BuildUnitID==static_cast<unsigned>(type)){found=item;foundPrevious=previous;}previous=item;
        }
        if(!found)return;
        if(found->BuildCount > -amount){found->BuildCount+=amount;return;}
        amount+=found->BuildCount;
        if(foundPrevious)foundPrevious->NextOrder=static_cast<UnitOrdersStruct*>(found->NextOrder);
        else lab->UnitOrders=static_cast<UnitOrdersStruct*>(found->NextOrder);
    }
}

struct Fixture {
    TAdynmemStruct ta;UnitStruct units[6]={};UnitDefStruct definitions[5];
    PlayerInfo info;short canBuild[2]={2,3};Object3doStruct object;int script=0;
    Fixture(){
        LabBuildCycles::Shutdown();nodes.clear();pointerTokens.resize(1);
        shared={};DataShare=&shared;currentWorld=&ta;
        ta.BeginUnitsArray_p=units;ta.EndOfUnitsArray_p=units+5;
        ta.UnitDef=definitions;ta.UNITINFOCount=5;ta.GameTime=100;
        ta.Players[1].PlayerInfo=&info;ta.Players[1].Units=units+1;ta.Players[1].UnitsAry_End=units+4;
        for(unsigned i=1;i<=4;++i){auto& unit=units[i];unit.UnitID=1;unit.UnitInGameIndex=static_cast<short>(i);
            unit.cOwnerID=1;unit.UnitType=definitions+1;unit.Owner_PlayerPtr0=&ta.Players[1];
            unit.UnitSelected=0x30000000u;unit.UnkPTR2=&script;unit.Object3do=&object;}
        units[1].UnitSelected|=UnitSelected_State;definitions[1].CANBUILD_ptr=canBuild;
        badAddress=shortReadAddress=0;throwHookNumber=0;menuCompatible=true;throwAdjust=false;
        LabBuildCycles::Install();
    }
    ~Fixture(){LabBuildCycles::Shutdown();currentWorld=nullptr;nodes.clear();}
    UnitStruct* Lab(){return units+1;}
    void Enable(){assert(LabBuildCycles::ToggleSelection());assert(LabBuildCycles::IsEnabled(Lab()));}
    int Count(){int total=0;for(UnitOrdersStruct* n=Lab()->UnitOrders;n;n=n->NextOrder)total+=n->BuildCount;return total;}
    void Add(unsigned type,int count){FakeAdjustBuildCount(7,Lab(),static_cast<int>(type),count);}
    unsigned Complete(bool success=true){
        auto* lab=Lab();auto* order=static_cast<UnitOrdersStruct*>(lab->UnitOrders);assert(order&&order->BuildCount>0);
        const unsigned type=order->BuildUnitID;--order->BuildCount;order->State=4;order->AttackTargat=nullptr;
#if LAB_BUILD_CYCLES_ENABLE
        if(success){InlineX86StackBuffer buffer;buffer.Edi=reinterpret_cast<std::uintptr_t>(lab);buffer.Esi=reinterpret_cast<std::uintptr_t>(order);assert(CompletionProc(&buffer)==0);}
#else
        (void)success;
#endif
        if(order->BuildCount==0)lab->UnitOrders=static_cast<UnitOrdersStruct*>(order->NextOrder);
        else order->State=0;
        return type;
    }
};

#if LAB_BUILD_CYCLES_ENABLE
void RotationAndLiveEditing(){
    Fixture f;f.Add(2,5);f.Add(3,1);f.Enable();
    for(unsigned i=0;i<30;++i){assert(f.Complete()==(i%6==5?3u:2u));assert(f.Count()==6);}
    assert(LabBuildCycles::ToggleSelection());assert(!LabBuildCycles::IsEnabled(f.Lab()));
    std::vector<unsigned> drain;while(f.Lab()->UnitOrders)drain.push_back(f.Complete());
    assert((drain==std::vector<unsigned>{2,2,2,2,2,3}));assert(f.Count()==0);
    f.Enable();f.Add(2,1);for(int i=0;i<100;++i){assert(f.Complete()==2);assert(f.Count()==1);}
    f.Add(3,1);f.Add(2,1);
    for(unsigned i=0;i<15;++i){assert(f.Complete()==(i%3==1?3u:2u));assert(f.Count()==3);}
    f.Add(3,5);assert(f.Count()==8);f.Add(3,-5);assert(f.Count()==3);
    f.Add(3,-1);assert(f.Count()==2);for(int i=0;i<8;++i)assert(f.Complete()==2);
    // Cancelling the sole queued type and pressing STOP both clear native
    // work without manufacturing a fresh order. Cycle stays On for new work.
    f.Add(2,-5);assert(!f.Lab()->UnitOrders);assert(LabBuildCycles::IsEnabled(f.Lab()));
    const auto before=adjustCalls;OnGameTick(0);assert(adjustCalls==before);
    f.Add(3,1);assert(f.Complete(false)==3);assert(!f.Lab()->UnitOrders);
    assert(adjustCalls==before+1); // failed construction consumed, never recycled
    f.Add(2,1);f.Lab()->UnitOrders=nullptr;OnGameTick(0);assert(!f.Lab()->UnitOrders);
}

void MidroundAndPerLabState(){
    Fixture f;f.Add(2,5);f.Add(3,1);assert(f.Complete()==2);assert(f.Complete()==2);f.Enable();
    for(unsigned i=0;i<12;++i)assert(f.Complete()==(i%4==3?3u:2u));
    // Selection changes do not transfer the per-lab toggle.
    f.units[1].UnitSelected&=~UnitSelected_State;f.units[2].UnitSelected|=UnitSelected_State;
    bool enabled=true;assert(LabBuildCycles::SelectionState(enabled)&&!enabled);
    assert(LabBuildCycles::ToggleSelection());assert(LabBuildCycles::IsEnabled(f.units+2));
    assert(LabBuildCycles::IsEnabled(f.units+1));
    f.units[1].UnitSelected|=UnitSelected_State;enabled=true;
    assert(!LabBuildCycles::SelectionState(enabled)&&!enabled);assert(!LabBuildCycles::ToggleSelection());
}

void InvalidCompletionNeverAppends(){
    Fixture f;f.Add(2,1);f.Enable();auto* order=static_cast<UnitOrdersStruct*>(f.Lab()->UnitOrders);
    order->State=4;order->BuildCount=0;const auto baseline=adjustCalls;
    auto rejected=[&](){CompletedSafe(f.Lab(),order);assert(adjustCalls==baseline);};
    order->State=3;rejected();order->State=4;
    order->AttackTargat=f.units+3;rejected();order->AttackTargat=nullptr;
    order->Unit_ptr=f.units+2;rejected();order->Unit_ptr=f.Lab();
    order->BuildUnitID=0;rejected();order->BuildUnitID=5;rejected();order->BuildUnitID=2;
    order->BuildCount=-1;rejected();order->BuildCount=0;
    order->NextOrder=order;rejected();order->NextOrder=nullptr;
    order->BuildCount=(std::numeric_limits<int>::max)();rejected();order->BuildCount=0;
    order->COBHandler_index=0;rejected();order->COBHandler_index=7;
    f.Lab()->UnitOrders=nullptr;rejected();f.Lab()->UnitOrders=order;
    CompletedSafe(f.Lab(),nullptr);assert(adjustCalls==baseline);
    CompletedSafe(f.Lab(),order);assert(adjustCalls==baseline+1&&order->BuildCount==1);
}

void OwnershipAndLifecycle(){
    Fixture f;f.Enable();bool enabled=true;
    auto reject=[&](){assert(!LabBuildCycles::SelectionState(enabled)&&!enabled);assert(!LabBuildCycles::ToggleSelection());};
    f.ta.GameStateMask=3;reject();f.ta.GameStateMask=0;
    // Human factory menus do not require the separate AI CANBUILD list.
    f.definitions[1].canbuildCount=0;f.definitions[1].CANBUILD_ptr=nullptr;
    assert(LabBuildCycles::SelectionState(enabled)&&enabled);
    f.Lab()->Health=static_cast<short>(50000);assert(LabBuildCycles::SelectionState(enabled)&&enabled);
    f.Lab()->IsUnit=1;reject();f.Lab()->IsUnit=0;assert(!LabBuildCycles::IsEnabled(f.Lab()));f.Enable();
    f.Lab()->Owner_PlayerPtr0=&f.ta.Players[2];reject();f.Lab()->Owner_PlayerPtr0=&f.ta.Players[1];assert(!LabBuildCycles::IsEnabled(f.Lab()));f.Enable();
    f.Lab()->cOwnerID=2;reject();f.Lab()->cOwnerID=1;f.Enable();
    f.Lab()->UnitSelected|=0x4000;reject();f.Lab()->UnitSelected&=~0x4000u;f.Enable();
    f.Lab()->Nanoframe=0.5f;reject();f.Lab()->Nanoframe=0;f.Enable();
    f.Lab()->Health=0;reject();f.Lab()->Health=100;f.Enable();
    f.Lab()->UnitInGameIndex=2;reject();f.Lab()->UnitInGameIndex=1;f.Enable();
    f.ta.Players[1].My_PlayerType=3;reject();f.ta.Players[1].My_PlayerType=1;f.Enable();
    f.info.PropertyMask=WATCH;reject();f.info.PropertyMask=0;f.Enable();
    shared.PlayingDemo=true;reject();shared.PlayingDemo=false;f.Enable();
    shared.ehaOff=1;reject();shared.ehaOff=2;f.Enable();shared.ehaOff=0;
    shared.TAProgress=0;reject();shared.TAProgress=TAInGame;f.Enable();
    auto* saved=DataShare;DataShare=nullptr;reject();DataShare=saved;f.Enable();
    f.ta.GameTime=101;OnGameTick(101);f.ta.GameTime=0;assert(!LabBuildCycles::IsEnabled(f.Lab()));f.Enable();
    int replacementScript=0;f.Lab()->UnkPTR2=&replacementScript;assert(!LabBuildCycles::IsEnabled(f.Lab()));f.Enable();
    Object3doStruct replacementObject;f.Lab()->Object3do=&replacementObject;assert(!LabBuildCycles::IsEnabled(f.Lab()));f.Enable();
    // Actual slot initializer callback clears even an identical type/owner/
    // script/object identity before a recycled slot can complete production.
    alignas(16) unsigned char stack[32]={};auto* lab=f.Lab();std::memcpy(stack+8,&lab,sizeof(lab));
    InlineX86StackBuffer buffer;buffer.Esp=reinterpret_cast<std::uintptr_t>(stack+4);
    assert(InitUnitProc(&buffer)==0);assert(!LabBuildCycles::IsEnabled(f.Lab()));
    f.Enable();UnitDefStruct nextDefinitions[5];nextDefinitions[1]=f.definitions[1];
    f.ta.UnitDef=nextDefinitions;f.Lab()->UnitType=nextDefinitions+1;assert(!LabBuildCycles::IsEnabled(f.Lab()));
    f.Enable();f.ta.LocalHumanPlayer_PlayerID=10;reject();
}

void HooksAndCompiledGuards(){
    Fixture f;LabBuildCycles::Shutdown();
    const auto before=hookCreates;
    for(unsigned address:{kCompletedCall,0x00402640u,0x0043B0B0u,kInitUnit}){
        badAddress=address;LabBuildCycles::Install();assert(hookCreates==before);assert(!g_installed);
    }
    badAddress=0;shortReadAddress=kCompletedCall;LabBuildCycles::Install();assert(hookCreates==before);shortReadAddress=0;
    throwHookNumber=hookCreates+2;LabBuildCycles::Install();assert(!g_installed&&!g_initHook&&!g_completedHook);throwHookNumber=0;
    menuCompatible=false;LabBuildCycles::Install();assert(!g_installed&&!g_initHook&&!g_completedHook);menuCompatible=true;
    LabBuildCycles::Install();assert(g_installed);const auto installed=hookCreates;LabBuildCycles::Install();assert(hookCreates==installed);
    assert(GameTickHook::GetInstance()->callbacks.size()==1);
    f.Enable();LabBuildCycles::Shutdown();assert(g_enabled.empty()&&!g_world&&!g_installed);
    const auto calls=menuUpdates;OnGameTick(0);assert(menuUpdates==calls);
}
void RefillAllocationFailure(){
    Fixture f;f.Add(2,1);f.Enable();auto* order=static_cast<UnitOrdersStruct*>(f.Lab()->UnitOrders);
    order->State=4;order->BuildCount=0;const auto before=adjustCalls;throwAdjust=true;
    CompletedSafe(f.Lab(),order);throwAdjust=false;
    assert(adjustCalls==before&&order->BuildCount==0&&!LabBuildCycles::IsEnabled(f.Lab()));
}
int main(){RotationAndLiveEditing();MidroundAndPerLabState();InvalidCompletionNeverAppends();OwnershipAndLifecycle();HooksAndCompiledGuards();RefillAllocationFailure();std::cout<<"Native lab-cycle adapter: batches, edits, off/drain, ownership, lifecycle and guarded hooks passed\n";}
#else
int main(){
    const auto hooks=hookCreates,reads=readCalls,adjusts=adjustCalls,menus=menuInstalls;
    LabBuildCycles::Install();LabBuildCycles::Shutdown();bool enabled=true;
    assert(!LabBuildCycles::SelectionState(enabled)&&!enabled);assert(!LabBuildCycles::ToggleSelection());
    assert(!LabBuildCycles::IsEnabled(reinterpret_cast<UnitStruct*>(std::uintptr_t(1))));
    assert(hookCreates==hooks&&readCalls==reads&&adjustCalls==adjusts&&menuInstalls==menus);
    std::cout<<"Compiled-out lab cycles leave native state untouched\n";
}
#endif
'''


def main():
    source = HARNESS.replace('// PRODUCTION', adapted_source())
    compiler = shlex.split(os.environ.get('CXX', 'g++'))
    environment = dict(os.environ, ASAN_OPTIONS='detect_leaks=0')
    with tempfile.TemporaryDirectory(prefix='lab-cycles-test-') as directory:
        path = Path(directory)
        (path / 'test.cpp').write_text(source)
        for enabled in (1, 0):
            executable = path / f'lab-cycles-{enabled}'
            subprocess.run(compiler + ['-std=c++17', '-Wall', '-Wextra', '-Werror',
                '-fsanitize=address,undefined', '-fno-pie', '-no-pie', '-g',
                f'-DLAB_BUILD_CYCLES_ENABLE={enabled}', '-I', str(ROOT),
                str(path / 'test.cpp'), '-o', str(executable)], check=True)
            subprocess.run([str(executable)], check=True, env=environment)


if __name__ == '__main__':
    main()
