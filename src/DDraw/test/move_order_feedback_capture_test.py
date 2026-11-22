#!/usr/bin/env python3
"""Exercise real move-feedback capture and hook ownership with native bridges mocked.

Linux checks production branching and queue observation under ASan/UBSan. Fixed
x86 addresses and actual SEH fault handling are covered by the native build.
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
    depth = 0
    tokens = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[{}]', re.S)
    for token in tokens.finditer(source, opening):
        if token.group() == "{":
            depth += 1
        elif token.group() == "}":
            depth -= 1
            if depth == 0:
                return source[start:token.end()]
    raise AssertionError("Unclosed production function: " + signature)


HARNESS = r'''
#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#define __stdcall
using LPDIRECTDRAWSURFACE = void*;
constexpr int TRUE=1, FALSE=0, TAInGame=1, Player_LocalHuman=1;
constexpr unsigned WATCH=4, canmove=1, canfly=2;
constexpr int INLINE_UNPROTECTEVINMENT=0x1003;
namespace ordertype { enum { MOVE=2, ATTACK=3 }; }
struct _Position_Dword { std::uint16_t x_=0,X=0,z_=0,Z=0,y_=0,Y=0; };
using Position_Dword = _Position_Dword;
static_assert(sizeof(Position_Dword)==12,"Native fixed-point position");
struct PlayerStruct;
struct UnitOrdersStruct;
struct UnitDefStruct { unsigned UnitTypeMask_0=canmove,UnitTypeMask_1=0x80u; int bmcode=1; };
struct UnitStruct {
    short UnitID=1;
    UnitDefStruct* UnitType=nullptr;
    PlayerStruct* Owner_PlayerPtr0=nullptr;
    void* UnkPTR2=reinterpret_cast<void*>(0x1000u);
    void* Object3do=reinterpret_cast<void*>(0x2000u);
    char data9[14]={0x34,0x12};
    unsigned UnitSelected=0x10000000u;
    float Nanoframe=0.0f;
    UnitOrdersStruct* UnitOrders=nullptr;
    unsigned short XPos__=0,XPos=100,ZPos__=0,ZPos=40,YPos__=0,YPos=200;
};
struct UnitOrdersStruct {
    unsigned char COBHandler_index=0;
    UnitStruct* Unit_ptr=nullptr;
    UnitStruct* AttackTargat=nullptr;
    Position_Dword Pos;
    UnitOrdersStruct* NextOrder=nullptr;
};
struct PlayerInfoStruct { unsigned PropertyMask=0; };
struct PlayerStruct {
    PlayerInfoStruct* PlayerInfo=nullptr;
    int My_PlayerType=Player_LocalHuman;
    unsigned DirectPlayID=0;
    UnitStruct* Units=nullptr;
    UnitStruct* UnitsAry_End=nullptr;
};
struct TAdynmemStruct {
    int LocalHumanPlayer_PlayerID=0;
    int GameTime=100;
    PlayerStruct Players[10];
    UnitStruct* BeginUnitsArray_p=nullptr;
    UnitStruct* EndOfUnitsArray_p=nullptr;
};
TAdynmemStruct* world=nullptr;
TAdynmemStruct** TAmainStruct_PtrPtr=&world;
struct Shared { int TAProgress=TAInGame,PlayingDemo=0,ehaOff=0; } shared;
Shared* DataShare=&shared;
struct COBHandle { unsigned unused=0; } actionTable[128];
COBHandle* beginActions=actionTable;
COBHandle* endActions=actionTable+128;
COBHandle** COBSciptHandler_Begin=&beginActions;
COBHandle** COBSciptHandler_End=&endActions;

static unsigned char MoveAction(UnitStruct* unit) {
    return unit && unit->UnitType && (unit->UnitType->UnitTypeMask_0&canfly) ? 26 : 25;
}
static bool throwAction=false, mutateProbe=false;
static unsigned actionQueries=0;
static unsigned char* MockAction(unsigned* out,unsigned kind,UnitStruct* unit,UnitStruct*,const Position_Dword* pos) {
    ++actionQueries; assert(kind==ordertype::MOVE);
    if (throwAction) throw std::runtime_error("observer lookup failed");
    *out=MoveAction(unit);
    if (mutateProbe) const_cast<Position_Dword*>(pos)->X=17;
    return reinterpret_cast<unsigned char*>(out);
}
struct Sent {
    unsigned action;
    int shift;
    UnitStruct* unit;
    UnitStruct* target;
    const Position_Dword* position;
    int p1,p2;
};
static std::vector<Sent> sent;
static std::function<void()> onSend;
static unsigned prependCalls=0;
static bool activePrepend=false, rejectPrepend=false;
static void MockSend(unsigned action,int shift,UnitStruct* unit,UnitStruct* target,
    const Position_Dword* pos,int p1,int p2) {
    sent.push_back({action,shift,unit,target,pos,p1,p2});
    if (onSend) onSend();
}
namespace PrependOrders {
    bool Active() { return activePrepend; }
    void Dispatch(unsigned action,int shift,UnitStruct* unit,UnitStruct* target,const Position_Dword* pos,int p1,int p2) {
        ++prependCalls;
        if (activePrepend && rejectPrepend) return;
        MockSend(action,shift,unit,target,pos,p1,p2);
    }
}
// PRODUCTION_HEADER
struct Mark { TAdynmemStruct* world; unsigned index; MoveOrderFeedback::ActorIdentity identity; Position_Dword origin,destination; };
static std::vector<Mark> recorded;
static std::vector<Mark> cancelled;
static bool throwRecord=false;
namespace MoveOrderFeedback {
    void Record(TAdynmemStruct* world,unsigned index,const ActorIdentity& identity,const Position_Dword& origin,const Position_Dword& dest) {
        if (throwRecord) throw std::bad_alloc();
        recorded.push_back({world,index,identity,origin,dest});
    }
    void Cancel(TAdynmemStruct* world,unsigned index,const Position_Dword& dest) {
        if (throwRecord) throw std::bad_alloc();
        cancelled.push_back({world,index,{},{},dest});
    }
}

static const std::array<unsigned char,5> originalCode={{0xE8,0xC7,0xDD,0xFA,0xFF}};
static std::array<unsigned char,5> code=originalCode;
static bool failWrite=false,throwHook=false,failRead=false;
static unsigned hooksCreated=0,hooksDestroyed=0;
struct SingleHook {
    std::array<unsigned char,5> previous;
    SingleHook(unsigned address,unsigned length,int mode,unsigned char* bytes):previous(code) {
        assert(address==0x0048D1F4u && length==5 && mode==INLINE_UNPROTECTEVINMENT);
        if (throwHook) throw std::bad_alloc();
        ++hooksCreated;
        if (!failWrite) std::copy(bytes,bytes+5,code.begin());
    }
    ~SingleHook() { ++hooksDestroyed; code=previous; }
};
static bool MockCodeMatches(const unsigned char* bytes) { return !failRead && std::memcmp(code.data(),bytes,5)==0; }
static std::vector<std::string> diagnostics;
static bool throwCountRead=false;
struct IDDrawSurface {
    static void OutptTxt(const char*) {}
    static void OutptFmtTxt(const char* format,...) {
        char line[512];va_list args;va_start(args,format);vsnprintf(line,sizeof(line),format,args);va_end(args);
        diagnostics.emplace_back(line);
    }
};
// PRODUCTION_MODULE

static std::vector<std::unique_ptr<UnitOrdersStruct>> nodes;
static Position_Dword Position(unsigned x=500,unsigned y=600,unsigned z=50) {
    Position_Dword p; p.X=x;p.Y=y;p.Z=z;return p;
}
static UnitOrdersStruct* Add(UnitStruct& actor,const Position_Dword& p,unsigned action=25,UnitStruct* target=nullptr) {
    std::unique_ptr<UnitOrdersStruct> n(new UnitOrdersStruct);
    n->Unit_ptr=&actor;n->COBHandler_index=action;n->Pos=p;n->AttackTargat=target;
    auto* out=n.get(); nodes.push_back(std::move(n));
    if (!actor.UnitOrders) actor.UnitOrders=out;
    else { auto* last=actor.UnitOrders; while(last->NextOrder)last=last->NextOrder; last->NextOrder=out; }
    return out;
}
struct Scenario {
    TAdynmemStruct state;
    UnitStruct units[8];
    UnitDefStruct defs[8];
    PlayerInfoStruct infos[2];
    Scenario() {
        world=&state;shared={};DataShare=&shared;
        state.BeginUnitsArray_p=units;state.EndOfUnitsArray_p=units+7;
        state.Players[0].PlayerInfo=&infos[0];state.Players[0].Units=units;state.Players[0].UnitsAry_End=units+3;
        state.Players[1].PlayerInfo=&infos[1];state.Players[1].Units=units+4;state.Players[1].UnitsAry_End=units+7;
        for(unsigned i=0;i<8;++i) {units[i].UnitType=&defs[i];units[i].Owner_PlayerPtr0=&state.Players[i<4 ? 0 : 1];}
    }
    UnitStruct& actor() {return units[1];}
};
static void Reset() {
#if DRAWN_MOVE_FORMATIONS_ENABLE
    callHook.reset();installed=false;diagnosticCount.store(0);
#endif
    world=nullptr;nodes.clear();sent.clear();recorded.clear();cancelled.clear();onSend={};
    activePrepend=rejectPrepend=throwAction=mutateProbe=throwRecord=false;
    prependCalls=actionQueries=hooksCreated=hooksDestroyed=0;
    failWrite=throwHook=failRead=false;code=originalCode;
    diagnostics.clear();throwCountRead=false;
    beginActions=actionTable;endActions=actionTable+128;
}
static void Issue(UnitStruct* actor,const Position_Dword& pos,int shift=0,unsigned action=25,UnitStruct* target=nullptr) {
    MoveOrderFeedback::Dispatch(action,shift,actor,target,&pos,71,72);
}
static void TestForwarding() {
    Reset();Scenario s;Position_Dword p=Position();
    MoveOrderFeedback::Install();
    Issue(&s.actor(),p,7,0xcafe0019u,&s.units[4]);
    assert(sent.size()==1);
    const auto& call=sent.front();
    assert(call.action==0xcafe0019u && call.shift==7 && call.unit==&s.actor() &&call.target==&s.units[4]);
    assert(call.position==&p &&call.p1==71 &&call.p2==72);
    assert(prependCalls==unsigned(PREPEND_ORDERS_ENABLE));
}
#if DRAWN_MOVE_FORMATIONS_ENABLE
static void TestNativeAcceptedMovesAndImmutableOrigins() {
    for(bool air:{false,true}) for(int shift:{0,1}) {
        Reset();Scenario s;Position_Dword p=Position();
        MoveOrderFeedback::Install();
        assert(installed &&hooksCreated==1); // Also covers prepend runtime-off: the stock CALL is still observed.
        if(air)s.actor().UnitType->UnitTypeMask_0|=canfly;
        auto* actor=&s.actor();const unsigned action=MoveAction(actor);
        mutateProbe=true;
        onSend=[&]{Add(*actor,p,action);actor->XPos=333;};
        Issue(actor,p,shift,0xbabe0000u|action);
        assert(sent.size()==1 &&recorded.size()==1 &&cancelled.empty());
        const auto& mark=recorded.front();
        assert(mark.world==&s.state &&mark.index==1 &&mark.origin.X==100 &&mark.origin.Y==200 &&mark.origin.Z==40);
        assert(mark.destination.X==500 &&mark.destination.Y==600 &&p.X==500);
        assert(mark.identity.type==s.actor().UnitID &&mark.identity.birthSalt==0x1234u);
        assert(mark.identity.definition==reinterpret_cast<std::uintptr_t>(s.actor().UnitType));
        assert(mark.identity.script==0x1000u &&mark.identity.model==0x2000u);
    }
    Reset();Scenario s;Position_Dword p=Position();Add(s.actor(),p);
    onSend=[&]{s.actor().UnitOrders=nullptr;Add(s.actor(),p);};
    Issue(&s.actor(),p); // Replacing an identical destination is still an issued move.
    assert(recorded.size()==1 &&cancelled.empty());
}
static void TestShiftCancellationAndPrependRefusal() {
    Reset();Scenario s;Position_Dword p=Position();
    auto* a=Add(s.actor(),p);auto* b=Add(s.actor(),p);auto* c=Add(s.actor(),p);
    onSend=[&]{s.actor().UnitOrders=b;};
    Issue(&s.actor(),p,1);
    assert(recorded.empty() &&cancelled.size()==1 &&b->NextOrder==c &&a!=b);
    Reset();Scenario s2;
    onSend=[]{};Issue(&s2.actor(),p,1);
    assert(recorded.empty() &&cancelled.empty());
#if PREPEND_ORDERS_ENABLE
    Reset();Scenario s3;Add(s3.actor(),p);activePrepend=true;rejectPrepend=true;
    Issue(&s3.actor(),p,1);
    assert(prependCalls==1 &&sent.empty() &&recorded.empty() &&cancelled.empty());
    rejectPrepend=false;onSend=[&]{Add(s3.actor(),p);};
    Issue(&s3.actor(),p,1);
    assert(sent.size()==1 &&recorded.size()==1 &&cancelled.empty());
    // Capture the scope before forwarding even if it ends during the call.
    activePrepend=true;onSend=[&]{activePrepend=false;};
    Issue(&s3.actor(),p,0);
    assert(recorded.size()==1);
#endif
}
static void TestNativeAcceptanceOverridesLegacyCapabilities() {
    for(unsigned fields=0;fields<4;++fields) for(bool accepted:{false,true}) {
        Reset();Scenario s;Position_Dword p=Position();
        // Starting/completed commanders can retain capability/construction
        // metadata that disagrees with the native accepted movement command.
        if(fields==0 ||fields==3)s.actor().Nanoframe=0.5f;
        if(fields==1 ||fields==3)s.actor().UnitType->UnitTypeMask_0=0;
        if(fields==2 ||fields==3)s.actor().UnitType->bmcode=0;
        if(accepted)onSend=[&]{Add(s.actor(),p);};
        Issue(&s.actor(),p);
        assert(sent.size()==1 &&recorded.size()==unsigned(accepted) &&cancelled.empty());
        assert(diagnostics.size()==1 &&diagnostics.front().find("reason=ok bounded=1")!=std::string::npos);
        assert(diagnostics.front().find(accepted ? "stage=record" : "stage=no-match")!=std::string::npos);
    }
}
static void TestLiveRejectionDiagnostics() {
    Reset();Scenario s;Position_Dword p=Position();s.actor().UnitSelected|=0x4000u;
    onSend=[&]{Add(s.actor(),p);};Issue(&s.actor(),p);
    assert(sent.size()==1 &&recorded.empty());
    assert(diagnostics.front().find("stage=live unit=1")!=std::string::npos);
    assert(diagnostics.front().find("reason=pending-death bounded=1 flags=0x10004000")!=std::string::npos);
    Reset();Scenario s2;
    MoveOrderFeedback::Dispatch(25,0,&s2.actor(),nullptr,nullptr,71,72);
    assert(sent.size()==1 &&recorded.empty() &&diagnostics.front().find("reason=position-null")!=std::string::npos);
}
static void TestExactFixedPointCancellationTolerance() {
    Position_Dword a=Position(500,600),b=a;
    b.X=484;assert(Nearby(a,b));
    a.x_=1;assert(!Nearby(a,b)); // Sixteen plus one fractional bit is outside native tolerance.
    a=Position(500,600);b=a;b.X=516;assert(Nearby(a,b));
    b.x_=1;assert(!Nearby(a,b));
    a=Position(500,600);b=a;b.Y=584;assert(Nearby(a,b));
    a.y_=1;assert(!Nearby(a,b));
}
static void TestActivePrependNativeAppendCancellation() {
#if PREPEND_ORDERS_ENABLE
    Reset();Scenario s;Position_Dword p=Position();activePrepend=true;
    // An idle actor latches the batch's native Shift-append fallback while
    // Active() remains true. A repeated destination then cancels that move.
    onSend=[&]{Add(s.actor(),p);};
    Issue(&s.actor(),p,1);
    assert(sent.size()==1 &&recorded.size()==1 &&cancelled.empty());
    onSend=[&]{s.actor().UnitOrders=nullptr;};
    Issue(&s.actor(),p,1);
    assert(activePrepend &&sent.size()==2 &&recorded.size()==1 &&cancelled.size()==1);
    assert(cancelled.front().world==&s.state &&cancelled.front().index==1);
    assert(cancelled.front().destination.X==p.X &&cancelled.front().destination.Y==p.Y);
#endif
}
static void TestFiltersNeverSuppressNativeDelivery() {
    for(unsigned failure=0;failure<17;++failure) {
        Reset();Scenario s;Position_Dword p=Position();UnitStruct* actor=&s.actor();unsigned action=25;
        switch(failure) {
        case 0:shared.TAProgress=0;break;case 1:shared.PlayingDemo=1;break;case 2:shared.ehaOff=1;break;
        case 3:s.infos[0].PropertyMask=WATCH;break;case 4:s.state.Players[0].My_PlayerType=2;break;
        case 5:actor=&s.units[4];break;case 6:s.actor().UnitID=0;break;case 7:s.actor().UnitSelected|=0x4000;break;
        case 8:s.actor().Nanoframe=0.5f;break;case 9:s.actor().UnitType->UnitTypeMask_0=0;break;
        case 10:s.actor().UnitType->bmcode=0;break;case 11:action=41;break;
        case 12:endActions=beginActions;break;case 13:throwAction=true;break;
        case 14:actor=reinterpret_cast<UnitStruct*>(reinterpret_cast<unsigned char*>(actor)+1);break;
        case 15:actor=nullptr;break;case 16:actor=s.units+8;break;
        }
        Issue(actor,p,0,action);
        assert(sent.size()==1 &&recorded.empty() &&cancelled.empty());
    }
}
static void TestPostDispatchContextAndBadQueues() {
    for(unsigned failure=0;failure<13;++failure) {
        Reset();Scenario s;Position_Dword p=Position();
        onSend=[&]{
            Add(s.actor(),p);
            switch(failure) {
            case 0:world=nullptr;break;case 1:s.state.LocalHumanPlayer_PlayerID=1;break;
            case 2:s.state.BeginUnitsArray_p=s.units+1;break;case 3:s.state.GameTime=0;break;
            case 4:s.state.Players[0].UnitsAry_End=s.units+2;break;case 5:s.state.Players[0].DirectPlayID=99;break;
            case 6:++s.actor().UnitID;break;case 7:s.actor().UnitType=&s.defs[2];break;
            case 8:s.actor().Owner_PlayerPtr0=&s.state.Players[1];break;case 9:shared.PlayingDemo=1;break;
            case 10:++s.actor().data9[0];break;case 11:s.actor().UnkPTR2=nullptr;break;
            case 12:s.actor().Object3do=nullptr;break;
            }
        };
        Issue(&s.actor(),p);
        assert(sent.size()==1 &&recorded.empty() &&cancelled.empty());
    }
    Reset();Scenario s;Position_Dword p=Position();auto* loop=Add(s.actor(),p);loop->NextOrder=loop;
    Issue(&s.actor(),p);assert(sent.size()==1 &&recorded.empty());loop->NextOrder=nullptr;
    Reset();Scenario s2;
    // The real observer must remain bounded for a long but acyclic list too.
    UnitOrdersStruct* tail=nullptr;
    for(unsigned i=0;i<8193;++i) {
        std::unique_ptr<UnitOrdersStruct> n(new UnitOrdersStruct);n->Unit_ptr=&s2.actor();
        if(tail)tail->NextOrder=n.get();else s2.actor().UnitOrders=n.get();
        tail=n.get();nodes.push_back(std::move(n));
    }
    Issue(&s2.actor(),p);assert(sent.size()==1 &&recorded.empty());
}
static void TestUnreadablePriorQueueDoesNotSuppressReplacement() {
    for(unsigned failure=0;failure<3;++failure) for(int shift:{0,1}) for(bool prepend:{false,true}) {
#if !PREPEND_ORDERS_ENABLE
        if(prepend)continue;
#endif
        Reset();Scenario s;Position_Dword p=Position();activePrepend=prepend;
        auto* old=Add(s.actor(),Position(100,200),0);
        if(failure==0)old->Unit_ptr=nullptr;
        if(failure==1)old->NextOrder=old;
        if(failure==2)throwCountRead=true;
        onSend=[&]{throwCountRead=false;old->NextOrder=nullptr;s.actor().UnitOrders=nullptr;Add(s.actor(),p);};
        Issue(&s.actor(),p,shift);
        assert(sent.size()==1 &&cancelled.empty());
        assert(recorded.size()==unsigned(!shift &&!prepend));
        assert(diagnostics.size()==1 &&diagnostics.front().find("beforeValid=0")!=std::string::npos);
        assert(diagnostics.front().find(!shift &&!prepend ? "stage=record" : "stage=before-invalid")!=std::string::npos);
    }
}
static void TestDiagnosticsAreBoundedAndPassive() {
    Reset();Scenario s;Position_Dword p=Position();
    onSend=[&]{Add(s.actor(),p);};
    for(unsigned i=0;i<20;++i)Issue(&s.actor(),p);
    assert(sent.size()==20 &&recorded.size()==20 &&diagnostics.size()==12);
    assert(diagnostics.front().find("stage=record unit=1 action=25 type=1 prepend=0 shift=0 beforeValid=1 before=0 after=1")!=std::string::npos);
}
static void TestObserverFailureAndGroupedUnitDestinations() {
    Reset();Scenario s;Position_Dword a=Position(500,600),b=Position(700,800);
    throwRecord=true;onSend=[&]{Add(s.actor(),a);};Issue(&s.actor(),a);
    assert(sent.size()==1 &&recorded.empty());
    throwRecord=false;onSend=[&]{Add(s.units[3],b);};Issue(&s.units[3],b,1);
    assert(sent.size()==2 &&recorded.size()==1 &&recorded.front().index==3 &&recorded.front().destination.X==700);
    onSend=[&]{Add(s.actor(),a);};Issue(&s.actor(),a,1);
    assert(recorded.size()==2 &&recorded.back().index==1 &&recorded.back().destination.X==500);
}
static void TestSingleHookOwnershipAndRollback() {
    Reset();MoveOrderFeedback::Install();
    assert(installed &&hooksCreated==1 &&callHook &&code!=originalCode);
    MoveOrderFeedback::Install();assert(hooksCreated==1);
    auto selfPatch=code;
    Reset();code=selfPatch;MoveOrderFeedback::Install();
    assert(installed &&hooksCreated==0 &&!callHook); // Existing prepend hook already targets this exact wrapper.
    Reset();code[0]=0x90;auto foreign=code;MoveOrderFeedback::Install();
    assert(!installed &&hooksCreated==0 &&code==foreign);
    Reset();failWrite=true;MoveOrderFeedback::Install();
    assert(!installed &&!callHook &&hooksCreated==1 &&hooksDestroyed==1 &&code==originalCode);
    Reset();throwHook=true;MoveOrderFeedback::Install();
    assert(!installed &&!callHook &&code==originalCode);
    Reset();failRead=true;MoveOrderFeedback::Install();
    assert(!installed &&hooksCreated==0 &&code==originalCode);
}
#endif
int main() {
    TestForwarding();
#if DRAWN_MOVE_FORMATIONS_ENABLE
    TestNativeAcceptedMovesAndImmutableOrigins();TestShiftCancellationAndPrependRefusal();
    TestNativeAcceptanceOverridesLegacyCapabilities();TestLiveRejectionDiagnostics();
    TestExactFixedPointCancellationTolerance();TestActivePrependNativeAppendCancellation();
    TestFiltersNeverSuppressNativeDelivery();
    TestPostDispatchContextAndBadQueues();TestUnreadablePriorQueueDoesNotSuppressReplacement();
    TestDiagnosticsAreBoundedAndPassive();TestObserverFailureAndGroupedUnitDestinations();
    TestSingleHookOwnershipAndRollback();
    std::cout << "Move feedback capture, native forwarding, cancellation and shared hook ownership passed\n";
#else
    assert(code==originalCode &&hooksCreated==0 &&recorded.empty() &&cancelled.empty());
    std::cout << "Disabled move feedback leaves native orders and hooks untouched\n";
#endif
}
'''


def prepare_source():
    source = (DDRAW / 'MoveOrderFeedbackCapture.cpp').read_text()
    source = re.sub(r'^\s*#include[^\n]*$', '', source, flags=re.M)
    for native, mock in (('NativeSend', 'MockSend'), ('NativeAction', 'MockAction')):
        source, count = re.subn(r'const auto ' + native + r'\s*=\s*reinterpret_cast<.*?;',
                                'const auto ' + native + ' = ' + mock + ';', source, flags=re.S)
        assert count == 1
    helper = extract(source, 'bool CodeMatches(')
    source = source.replace(helper, 'bool CodeMatches(const unsigned char* bytes) {return MockCodeMatches(bytes);}')
    source = source.replace('reinterpret_cast<Send>(0x0043AFC0u)', 'static_cast<Send>(&MockSend)')
    source = source.replace('__except (GetExceptionCode() == 0xE06D7363u ? EXCEPTION_CONTINUE_SEARCH : EXCEPTION_EXECUTE_HANDLER)', 'catch (...)')
    source = source.replace('__try', 'try').replace('__except (EXCEPTION_EXECUTE_HANDLER)', 'catch (...)')
    source = source.replace('count = 0;', 'if (throwCountRead) throw std::runtime_error("queue read fault");\n        count = 0;')
    header = (DDRAW / 'MoveOrderFeedback.h').read_text()
    header = re.sub(r'^\s*(?:#include[^\n]*|#pragma once)\s*$', '', header, flags=re.M)
    return HARNESS.replace('// PRODUCTION_HEADER', header).replace('// PRODUCTION_MODULE', source)


def hook_choice_tests(compiler):
    prepend_source = DDRAW / 'PrependOrders.cpp'
    if not prepend_source.is_file():
        # This standalone feature owns the stock CALL itself. Once the prepend
        # PR is merged, inspect its real routing below as an integration check.
        # The production feedback wrapper is still built and exercised in all
        # four formation/prepend combinations by main(), even without this file.
        print('Standalone feedback: native hook ownership and all wrapper combinations remain covered')
        return
    source = prepend_source.read_text()
    source = re.sub(r'^\s*#include[^\n]*$', '', source, flags=re.M)
    for formations in (0, 1):
        for prepend in (0, 1):
            output = subprocess.run(compiler + ['-E', '-P', '-x', 'c++',
                '-DDRAWN_MOVE_FORMATIONS_ENABLE='+str(formations), '-DPREPEND_ORDERS_ENABLE='+str(prepend), '-'],
                input=source, text=True, capture_output=True, check=True).stdout
            if prepend:
                install = extract(output, 'void Install(')
                assert ('&MoveOrderFeedback::Dispatch' in install) == bool(formations)
                assert '{0x0048D1F4u,' in output
                assert '{0x004197A9u,' in output
            else:
                assert 'CallSite sites[]' not in output
    print('All four formation/prepend choices retain one guarded native move CALL owner')


def main():
    compiler = shlex.split(os.environ.get('CXX', 'g++'))
    hook_choice_tests(compiler)
    with tempfile.TemporaryDirectory(prefix='move-feedback-capture-') as tmp:
        root = Path(tmp)
        cpp = root / 'capture.cpp'
        cpp.write_text(prepare_source())
        for formations in (0, 1):
            for prepend in (0, 1):
                binary = root / ('capture_'+str(formations)+'_'+str(prepend))
                subprocess.run(compiler + ['-std=c++17','-Wall','-Wextra','-Werror',
                    '-Wno-unused-function','-Wno-unused-variable','-fsanitize=address,undefined',
                    '-fno-omit-frame-pointer','-g','-DDRAWN_MOVE_FORMATIONS_ENABLE='+str(formations),
                    '-DPREPEND_ORDERS_ENABLE='+str(prepend),str(cpp),'-o',str(binary)],check=True)
                subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})


if __name__ == '__main__':
    main()
