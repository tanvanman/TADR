#!/usr/bin/env python3
"""Compile the production retreat helper with native TA structures/calls mocked.

This runs the complete BuildRectangleOrders.cpp on every invocation. The only
platform substitutions are Windows headers, the fixed-address world lookup, and
SEH syntax: a C++ scope guard executes the real production Restore method for
both normal returns and exceptions. It checks queue preflight, native MOVE toggle
avoidance, per-constructor selection isolation, and UI restoration. It does not
simulate pathfinding or verify live multiplayer traffic.

Requires g++ (or $CXX). Set BUILD_RECTANGLE_SANITIZERS=1 for ASan/UBSan.
"""

import os
from pathlib import Path
import shlex
import subprocess
import tempfile

DDRAW = Path(__file__).resolve().parent.parent

PRELUDE = r'''

#include <cassert>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <vector>
#include <stdexcept>
#include <iostream>
struct UnitDefStruct { unsigned UnitTypeMask_0=0,UnitTypeMask_1=0; unsigned char bmcode=1,maxslope=20; unsigned short builddistance=128,minwaterdepth=0,maxwaterdepth=255; };
struct PlayerStruct;
struct Position_Dword { unsigned short x_=0,X=0,z_=0,Z=0,y_=0,Y=0; };
static_assert(sizeof(Position_Dword) == 12, "Match the native position layout");
struct UnitOrdersStruct { unsigned char COBHandler_index=0; Position_Dword Pos; UnitOrdersStruct* NextOrder=nullptr; };
struct UnitStruct { int IsUnit=1; unsigned UnitSelected=0; short UnitID=0,UnitInGameIndex=0,SizeFootX=2,SizeFootZ=2; UnitDefStruct* UnitType=nullptr; PlayerStruct* Owner_PlayerPtr0=nullptr; float Nanoframe=0; unsigned short XPos=200,YPos=200; UnitOrdersStruct* UnitOrders=nullptr; };
struct PlayerInfoStruct { unsigned PropertyMask=0; };
struct PlayerStruct { UnitStruct* Units=nullptr; UnitStruct* UnitsAry_End=nullptr; int My_PlayerType=1; PlayerInfoStruct* PlayerInfo=nullptr; };
struct FeatureStruct { unsigned short occupyingUnitNumber=0; unsigned char height=0,maxHeight2x2=0,minHeight2x2=0; unsigned short FeatureDefIndex=65535; };
struct FeatureDefStruct { char Height=0; };
struct TAdynmemStruct { PlayerStruct Players[10]; int LocalHumanPlayer_PlayerID=0; UnitStruct* BeginUnitsArray_p=nullptr; UnitStruct* EndOfUnitsArray_p=nullptr; int FeatureMapSizeX=64,FeatureMapSizeY=72,MapSizeX=1024,MapSizeY=1024,NumFeatureDefs=0,SeaLevel=0; FeatureStruct* FeatureMap=nullptr; FeatureDefStruct* FeatureDef=nullptr; Position_Dword MouseMapPos; unsigned char PrepareOrder_Type=14,CurrentCursora_Index=18; short BuildUnitID=7; char BuildSpotState=70; };
struct COBHandle { unsigned char cursorIndex=0; };
struct MOUSEEVENT { unsigned X=0,Y=0,fwKeys=0,PressTime_sec=0,Msg=0,DblClick=0; };
enum { builder=0x40,canmove=1,canfly=0x800,canhover=0x1000,cursormove=14,Player_LocalHuman=1,WATCH=0x40 };
namespace ordertype { enum { MOVE=2 }; }
TAdynmemStruct* mockWorld;
COBHandle handlers[3]; COBHandle* handlersBegin=handlers; COBHandle* handlersEnd=handlers+3;
COBHandle** COBSciptHandler_Begin=&handlersBegin; COBHandle** COBSciptHandler_End=&handlersEnd;
unsigned short Feature(FeatureStruct* f) { return f->FeatureDefIndex; }
int Height(Position_Dword*) { return 23; }
auto GetGridPosFeature=&Feature; auto GetPosHeight=&Height;
void UpdateSelectUnitEffect() {}
struct Issued { int slot,x,y; };
std::vector<Issued> issued;
UnitOrdersStruct newOrders[64]; int newOrderCount=0;
bool throwDispatch=false,mutateUI=false;
int Move(MOUSEEVENT* event,int action,unsigned char,Position_Dword* pos,int,int) {
    assert(event->fwKeys==4 && action==2 && pos->x_==0 && pos->y_==0 && pos->z_==0 && pos->Z==23);
    UnitStruct* selected=nullptr;
    for (UnitStruct* u=mockWorld->Players[0].Units;u<=mockWorld->Players[0].UnitsAry_End;++u)
        if(u->UnitSelected&0x10) { assert(!selected);selected=u; }
    assert(selected && selected->UnitID);
    issued.push_back({selected->UnitInGameIndex,pos->X,pos->Y});
    // Match native Shift cancellation: the helper must keep every old MOVE.
    for (auto* old=selected->UnitOrders;old;old=old->NextOrder)
        assert(old->COBHandler_index!=1 || std::abs(int(pos->X)-old->Pos.X)>16 || std::abs(int(pos->Y)-old->Pos.Y)>16);
    UnitOrdersStruct* tail=selected->UnitOrders;
    for (;tail && tail->NextOrder;tail=tail->NextOrder) {}
    newOrders[newOrderCount]={}; newOrders[newOrderCount].COBHandler_index=1; newOrders[newOrderCount].Pos=*pos;
    if(tail) tail->NextOrder=&newOrders[newOrderCount]; else selected->UnitOrders=&newOrders[newOrderCount];
    ++newOrderCount;
    if(mutateUI) { mockWorld->PrepareOrder_Type=1;mockWorld->CurrentCursora_Index=7;mockWorld->BuildUnitID=42;mockWorld->BuildSpotState=9;mockWorld->MouseMapPos.X=999; }
    if(throwDispatch) throw std::runtime_error("native mock failure");
    return 1;
}
auto MOUSE_EVENT_2UnitOrder_=&Move;
'''

TESTS = r'''

TAdynmemStruct world; UnitStruct units[5]; UnitDefStruct defs[5]; PlayerInfoStruct info; FeatureStruct tiles[64*72];
void reset() {
    world={};for(auto& u:units)u={};for(auto& d:defs)d={};for(auto& t:tiles)t={};
    mockWorld=&world;world.FeatureMap=tiles;world.BeginUnitsArray_p=units;world.EndOfUnitsArray_p=units+5;
    world.Players[0].Units=units+1;world.Players[0].UnitsAry_End=units+4;world.Players[0].PlayerInfo=&info;info={};
    for(int i=1;i<5;++i){ units[i].UnitType=&defs[i];units[i].Owner_PlayerPtr0=&world.Players[0];units[i].UnitInGameIndex=i;units[i].UnitID=i;defs[i].UnitTypeMask_0=builder|canmove;defs[i].UnitTypeMask_1=0x80; }
    units[1].UnitSelected=0x10000010;handlers[1].cursorIndex=14;handlersBegin=handlers;handlersEnd=handlers+3;
    world.MouseMapPos.X=17;world.MouseMapPos.Y=29;world.MouseMapPos.x_=123;
    issued.clear();newOrderCount=0;throwDispatch=false;mutateUI=false;
}
bool queue(bool finishing=false,int x=320,int y=320,int ox=1,int oy=0) { return BuildRectangleOrders::QueueRetreat(x,y,ox,oy,2,2,finishing); }
void restored() { assert(world.PrepareOrder_Type==14 && world.CurrentCursora_Index==18 && world.BuildUnitID==7 && world.BuildSpotState==70);assert(world.MouseMapPos.X==17 && world.MouseMapPos.Y==29 && world.MouseMapPos.x_==123); }
int main() {
    reset();mutateUI=true;assert(queue());assert(issued.size()==1 && issued[0].x==384);assert(units[1].UnitSelected&0x10);restored();
    reset();units[2].UnitSelected=0x10000010;defs[2].UnitTypeMask_0=canmove;units[3].UnitSelected=0x10000010;defs[3].UnitTypeMask_0|=canfly;units[4].UnitSelected=0x10;units[4].UnitID=0;assert(queue());assert(issued.size()==1);assert(units[2].UnitSelected&0x10);assert(units[3].UnitSelected&0x10);assert(!(units[4].UnitSelected&0x10));restored();
    reset();units[2].UnitSelected=0x10000010;assert(queue());assert(issued.size()==2);assert(std::abs(issued[0].y-issued[1].y)>=32);restored();
    reset();units[2].UnitSelected=0x10000010;defs[2].builddistance=10;assert(!queue());assert(issued.empty());assert(units[1].UnitSelected&0x10);assert(units[2].UnitSelected&0x10);restored();
    reset();defs[1].builddistance=10;assert(queue(true));assert(issued.size()==1 && issued[0].y==320);restored();
    reset();assert(!queue(false,16,320,-1,0));assert(issued.empty());restored();
    reset();for(auto& t:tiles)t.occupyingUnitNumber=4;assert(!queue());assert(issued.empty());restored();
    reset();for(auto& t:tiles)t.occupyingUnitNumber=1;assert(queue());assert(issued.size()==1);restored();
    reset();UnitOrdersStruct old;old.COBHandler_index=1;old.Pos.X=400;old.Pos.Y=304;units[1].UnitOrders=&old;assert(queue());assert(issued.size()==1);assert(std::abs(issued[0].x-400)>16 || std::abs(issued[0].y-304)>16);assert(units[1].UnitOrders==&old);restored();
    reset();UnitOrdersStruct cycle;cycle.COBHandler_index=1;cycle.NextOrder=&cycle;units[1].UnitOrders=&cycle;assert(!queue());assert(issued.empty());restored();
    reset();units[1].IsUnit=0;assert(queue());assert(issued.empty());restored();
    reset();defs[1].UnitTypeMask_1=0;assert(!queue());assert(issued.empty());restored();
    reset();mutateUI=true;throwDispatch=true;try{queue();assert(false);}catch(const std::runtime_error&){}assert(units[1].UnitSelected&0x10);restored();
    reset();for(auto& t:tiles)t.maxHeight2x2=30;assert(!queue());assert(issued.empty());
    reset();world.SeaLevel=30;defs[1].maxwaterdepth=10;assert(!queue());assert(issued.empty());
    reset();assert(!queue(false,320,320,2,0));assert(!queue(false,320,320,1,1));assert(issued.empty());
    std::cout<<"16 retreat queue mock scenarios passed\n";
}
'''


def replace_once(source, before, after):
    count = source.count(before)
    if count != 1:
        raise RuntimeError(f"Expected one production fragment, found {count}: {before!r}")
    return source.replace(before, after)


def production_source():
    source = (DDRAW / "BuildRectangleOrders.cpp").read_text()
    source = replace_once(source, '#include "tamem.h"', "")
    source = replace_once(source, '#include "tafunctions.h"', "")
    source = replace_once(source,
        "return *reinterpret_cast<TAdynmemStruct**>(0x00511DE8u);", "return mockWorld;")
    source = replace_once(source, "__try\n        {",
        "struct OnExit { Context& context; ~OnExit() { Restore(context); } } "
        "onExit{context};\n        {")
    source = replace_once(source, "__finally\n        {\n            Restore(context);\n        }", "")
    return source


def main():
    compiler = shlex.split(os.environ.get("CXX", "g++"))
    flags = ["-std=c++14", "-Wall", "-Wextra", "-Werror"]
    sanitizers = os.environ.get("BUILD_RECTANGLE_SANITIZERS") == "1"
    if sanitizers:
        flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-g"]
    with tempfile.TemporaryDirectory(prefix="build-rectangle-orders-") as directory:
        source = Path(directory) / "orders_test.cpp"
        binary = Path(directory) / "orders_test"
        source.write_text(PRELUDE + production_source() + TESTS)
        subprocess.run(compiler + flags + ["-I", str(DDRAW), str(source), "-o", str(binary)], check=True)
        environment = os.environ.copy()
        if sanitizers:
            # LeakSanitizer cannot operate inside the desktop runner's ptrace.
            environment.setdefault("ASAN_OPTIONS", "detect_leaks=0")
        subprocess.run([str(binary)], check=True, env=environment)


if __name__ == "__main__":
    main()
