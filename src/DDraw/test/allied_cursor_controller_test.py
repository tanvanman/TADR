#!/usr/bin/env python3
"""Compile the production cursor controller with native TA/network mocks.

The real controller, protocol, mutexes and packet generation run unchanged.
Only native game structures, clock, tick/router registration and HAPI transport
are replaced. No test opens sockets or touches a running game.
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
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>
#include "AlliedCursors.h"
#include "AlliedCursorProtocol.h"
#include "ChatHijackIds.h"
using namespace AlliedCursorProtocol;
constexpr unsigned WATCH = 0x40;
constexpr int TAInGame = 1;
enum PlayerType { Player_none=0,Player_LocalHuman=1,Player_LocalAI=2,Player_RemoteHuman=3,Player_RemoteAI=4 };
struct PlayerInfoStruct { unsigned PropertyMask = 0; int PlayerLogoColor = 0; int PlayerType = Player_LocalHuman; };
struct PlayerStruct {
    int PlayerActive = 0, DirectPlayID = 0;
    PlayerType My_PlayerType = Player_RemoteHuman;
    PlayerInfoStruct* PlayerInfo = nullptr;
    char Name[30] = {};
    char AllyFlagAry[10] = {};
};
PlayerType GetInferredPlayerType(PlayerStruct* p) {
    if(p->My_PlayerType==Player_LocalHuman)return Player_LocalHuman;
    if(p->My_PlayerType==Player_LocalAI)return Player_LocalAI;
    if(p->PlayerInfo->PlayerType==Player_LocalHuman)return Player_RemoteHuman;
    if(p->PlayerInfo->PlayerType==Player_LocalAI)return Player_RemoteAI;
    return Player_none;
}
struct TAdynmemStruct {
    PlayerStruct Players[10];
    int LocalHumanPlayer_PlayerID = 0, GameTime = 0;
    int FeatureMapSizeX = 64, FeatureMapSizeY = 72, SeaLevel = 15;
};
struct _Position_Dword { unsigned short x_=0,X=0,z_=0,Z=0,y_=0,Y=0; };
struct SharedData { int TAProgress = TAInGame; bool PlayingDemo = false; } shared;
SharedData* DataShare = &shared;
TAdynmemStruct* currentWorld = nullptr;
TAdynmemStruct** TAmainStruct_PtrPtr = &currentWorld;
std::uint32_t wallClock = 10000;
unsigned GetTickCount() { return wallClock; }
struct IDDrawSurface { static void OutptTxt(const char*) {} };
class PacketChatRouter {
public:
    using Handler = std::function<void(unsigned,const void*)>;
    Handler handler;
    int registrations = 0;
    static PacketChatRouter* GetInstance() { static PacketChatRouter router; return &router; }
    void RegisterHandler(unsigned char id, Handler fn, bool replay=false) {
        assert(id == ChatHijackId::AlliedCursor && !replay);
        handler = fn; ++registrations;
    }
};
class GameTickHook {
public:
    std::vector<void(*)(int)> callbacks;
    static GameTickHook* GetInstance() { static GameTickHook tick; return &tick; }
    void addCallback(void(*fn)(int)) { callbacks.push_back(fn); }
};
struct Sent { unsigned from, to; Packet packet; std::uint32_t at; };
std::vector<Sent> sent;
std::function<void(const Sent&)> onSend;
int HAPI_SendBuf(unsigned from, unsigned to, const char* bytes, int length) {
    assert(length == 65 && bytes);
    Sent event = {from,to,{},wallClock};
    std::memcpy(&event.packet,bytes,sizeof(event.packet));
    assert(Envelope(event.packet,ChatHijackId::AlliedCursor));
    assert(bytes[1] == 0 && bytes[6] == '-' && bytes[7] == '>' && bytes[64] == 0);
    sent.push_back(event);
    if (onSend) onSend(event); // Synchronous HAPI callbacks must not deadlock the controller mutex.
    return 1;
}

// PRODUCTION_SOURCE

struct World {
    TAdynmemStruct ta;
    PlayerInfoStruct info[10];
    World() {
        currentWorld = &ta; shared = SharedData{}; DataShare = &shared;
        wallClock = 10000; sent.clear(); onSend = nullptr;
        for (int slot=0;slot<10;++slot) {
            auto& player=ta.Players[slot]; player.PlayerActive=1;
            player.DirectPlayID=1000+slot; player.PlayerInfo=&info[slot];
            info[slot].PlayerLogoColor=slot;
            const std::string name="Player"+std::to_string(slot);
            std::memcpy(player.Name,name.c_str(),name.size()+1);
        }
        ta.Players[0].My_PlayerType=Player_LocalHuman;
        Ally(1,true);
        // Drive the real lifecycle reset; tests do not reach into controller globals.
        shared.TAProgress=0;
        AlliedCursors::UpdateLocal(nullptr,false);
        shared.TAProgress=TAInGame;
        AlliedCursors::Install();
    }
    void Ally(int slot,bool both) {
        ta.Players[0].AllyFlagAry[slot]=both;
        ta.Players[slot].AllyFlagAry[0]=both;
    }
    void Sample(int x=120,int y=240,int z=30,bool valid=true) {
        _Position_Dword point;
        point.X=static_cast<unsigned short>(x); point.Y=static_cast<unsigned short>(y); point.Z=static_cast<unsigned short>(z);
        AlliedCursors::UpdateLocal(&point,valid);
    }
    void Tick(unsigned now) {
        wallClock=now; ++ta.GameTime;
        for (const auto fn:GameTickHook::GetInstance()->callbacks) fn(ta.GameTime);
    }
    void Deliver(int slot,const Packet& packet) {
        assert(PacketChatRouter::GetInstance()->handler);
        PacketChatRouter::GetInstance()->handler(ta.Players[slot].DirectPlayID,&packet);
    }
    std::uint32_t LocalSession() {
        Sample(); Tick(wallClock);
        for (const auto& event:sent) if (event.packet.kind==Hello) return event.packet.session;
        assert(false);return 0;
    }
    std::uint32_t Handshake(int slot=1,std::uint32_t remote=7001) {
        const auto local=LocalSession();
        auto hello=Make(ChatHijackId::AlliedCursor,Hello,remote); Deliver(slot,hello);
        auto ack=Make(ChatHijackId::AlliedCursor,HelloAck,remote);ack.echoSession=local;Deliver(slot,ack);
        return local;
    }
    Packet PositionPacket(std::uint32_t serial=1,std::uint32_t remote=7001) {
        auto packet=Make(ChatHijackId::AlliedCursor,Position,remote);packet.sequence=serial;
        packet.x=321;packet.y=456;packet.z=12;return packet;
    }
};
bool AnyVisible(TAdynmemStruct* ta) {
    const auto views=AlliedCursors::Snapshot(ta);
    return std::any_of(views.begin(),views.end(),[](const AlliedCursorView& view){return view.visible;});
}
unsigned Positions() {
    return static_cast<unsigned>(std::count_if(sent.begin(),sent.end(),[](const Sent& p){return p.packet.kind==Position;}));
}
void TestInstallationAndLegacyPeer() {
    World world;
    AlliedCursors::Install();
    assert(PacketChatRouter::GetInstance()->registrations==1);
    assert(GameTickHook::GetInstance()->callbacks.size()==1);
    world.LocalSession();
    for (unsigned elapsed=0;elapsed<=20000;elapsed+=100) {
        world.Sample(); world.Tick(10000+elapsed);
    }
    assert(!sent.empty() && Positions()==0 && !AnyVisible(&world.ta));
    for (const auto& event:sent) assert(event.to==1001 && event.packet.kind==Hello);
}
void TestNativePrivacyAndPeerEligibility() {
    for (int reason=0;reason<9;++reason) {
        World world;
        if(reason==0)world.Ally(1,false);
        if(reason==1)world.ta.Players[0].AllyFlagAry[1]=0;
        if(reason==2)world.ta.Players[1].AllyFlagAry[0]=0;
        if(reason==3)world.info[1].PropertyMask=WATCH;
        if(reason==4)world.ta.Players[1].PlayerActive=0;
        if(reason==5)world.ta.Players[1].DirectPlayID=0;
        if(reason==6)world.ta.Players[1].DirectPlayID=world.ta.Players[0].DirectPlayID;
        if(reason==7)world.ta.Players[1].My_PlayerType=Player_LocalAI;
        if(reason==8)world.info[1].PlayerType=Player_LocalAI;
        world.Sample();world.Tick(10000);
        assert(sent.empty());
        auto hello=Make(ChatHijackId::AlliedCursor,Hello,7001);world.Deliver(1,hello);
        auto ack=Make(ChatHijackId::AlliedCursor,HelloAck,7001);ack.echoSession=12345;world.Deliver(1,ack);
        world.Deliver(1,world.PositionPacket());
        world.Tick(10200);assert(sent.empty() && !AnyVisible(&world.ta));
    }
    for (int reason=0;reason<5;++reason) {
        World world;
        if(reason==0)world.info[0].PropertyMask=WATCH;
        if(reason==1)world.ta.Players[0].PlayerActive=0;
        if(reason==2)world.ta.Players[0].DirectPlayID=0;
        if(reason==3)shared.PlayingDemo=true;
        if(reason==4)world.ta.Players[0].My_PlayerType=Player_LocalAI;
        world.Sample();world.Tick(10000);world.Deliver(1,world.PositionPacket());
        assert(sent.empty() && !AnyVisible(&world.ta));
    }
    World world;const auto local=world.LocalSession();
    auto enemyAck=Make(ChatHijackId::AlliedCursor,HelloAck,8002);enemyAck.echoSession=local;
    world.Deliver(2,enemyAck);world.Deliver(2,world.PositionPacket(1,8002));
    world.Tick(10200);assert(Positions()==0 && !AnyVisible(&world.ta));

    // A remote AI can borrow its human owner's DPID; it must not duplicate that owner's cursor/name.
    World duplicate;duplicate.Ally(2,true);
    duplicate.ta.Players[2].DirectPlayID=1001;duplicate.info[2].PlayerType=Player_LocalAI;
    duplicate.Handshake();duplicate.Deliver(1,duplicate.PositionPacket());
    const auto views=AlliedCursors::Snapshot(&duplicate.ta);
    assert(views[1].visible && !views[2].visible);
    sent.clear();duplicate.Sample();duplicate.Tick(10200);
    assert(Positions()==1);
    for(const auto& event:sent)assert(event.to==1001);
}
void TestHandshakeRenderingSnapshotAndPacing() {
    World world;world.Handshake();world.Deliver(1,world.PositionPacket());
    const auto views=AlliedCursors::Snapshot(&world.ta);
    assert(views[1].visible && views[1].dpid==1001 && views[1].x==321 && views[1].y==456 && views[1].z==12);
    assert(views[1].color==1 && std::string(views[1].name)=="Player1");
    assert(!views[0].visible && !views[2].visible);
    sent.clear();
    for (unsigned ms=0;ms<1000;++ms) {
        wallClock=10000+ms;world.Sample(120+ms%500,240);world.Tick(wallClock);
    }
    assert(Positions()==10);
    std::uint32_t previous=0;bool have=false;
    for (const auto& event:sent) {
        assert(event.to==1001);
        if(event.packet.kind!=Position)continue;
        assert(static_cast<std::uint32_t>(event.packet.x)>=120 && static_cast<std::uint32_t>(event.packet.y)==240);
        if(have)assert(event.at-previous>=MoveIntervalMs);
        previous=event.at;have=true;
    }
    sent.clear();wallClock=11100;AlliedCursors::UpdateLocal(nullptr,false);world.Tick(11100);
    assert(std::any_of(sent.begin(),sent.end(),[](const Sent& p){return p.packet.kind==Hidden;}));
    for (const auto& event:sent) if(event.packet.kind==Hidden)assert(!event.packet.x && !event.packet.y && !event.packet.z);
    auto hidden=Make(ChatHijackId::AlliedCursor,Hidden,7001);hidden.sequence=2;world.Deliver(1,hidden);
    assert(!AnyVisible(&world.ta));
}
void TestFreshnessAndBounds() {
    World world;world.Handshake();world.Deliver(1,world.PositionPacket());
    wallClock=11999;assert(AnyVisible(&world.ta));
    wallClock=12000;assert(!AnyVisible(&world.ta));
    world.Deliver(1,world.PositionPacket(2));assert(AnyVisible(&world.ta));
    auto bad=world.PositionPacket(3);bad.x=world.ta.FeatureMapSizeX*16;world.Deliver(1,bad);
    bad=world.PositionPacket(3);bad.y=(world.ta.FeatureMapSizeY-8)*16;world.Deliver(1,bad);
    auto views=AlliedCursors::Snapshot(&world.ta);assert(views[1].visible && views[1].x==321 && views[1].y==456);
    bad=world.PositionPacket(3);bad.reserved[0]=1;world.Deliver(1,bad);
    assert(AnyVisible(&world.ta));
    sent.clear();world.Tick(12001); // Local sample is stale although remote contact is fresh.
    assert(Positions()==0);
    assert(std::any_of(sent.begin(),sent.end(),[](const Sent& p){return p.packet.kind==Hidden;}));
}
void TestAllianceLossAndDpidRecycling() {
    for (int reason=0;reason<4;++reason) {
        World world;world.Handshake();world.Deliver(1,world.PositionPacket());assert(AnyVisible(&world.ta));
        if(reason==0)world.Ally(1,false);
        if(reason==1)world.info[1].PropertyMask=WATCH;
        if(reason==2)world.ta.Players[1].PlayerActive=0;
        if(reason==3)world.ta.Players[1].DirectPlayID=2001;
        assert(!AnyVisible(&world.ta));sent.clear();world.Sample();world.Tick(10200);
        assert(Positions()==0);
        if(reason==3) {
            const auto oldPacket=world.PositionPacket();
            PacketChatRouter::GetInstance()->handler(1001,&oldPacket);
            assert(!AnyVisible(&world.ta));
            for(const auto& event:sent)assert(event.to==2001 && event.packet.kind==Hello);
        } else assert(sent.empty());
    }
}
void TestMenuRestartAndOldSessionRejection() {
    World world;const auto old=world.Handshake();world.Deliver(1,world.PositionPacket());assert(AnyVisible(&world.ta));
    shared.TAProgress=0;AlliedCursors::UpdateLocal(nullptr,false);assert(!AnyVisible(&world.ta));
    sent.clear();world.Tick(10500);assert(sent.empty());
    shared.TAProgress=TAInGame;world.ta.GameTime=0;world.Sample();world.Tick(10600);
    std::uint32_t next=0;for(const auto& event:sent)if(event.packet.kind==Hello)next=event.packet.session;
    assert(next && next!=old);
    auto stale=Make(ChatHijackId::AlliedCursor,HelloAck,7001);stale.echoSession=old;world.Deliver(1,stale);
    world.Deliver(1,world.PositionPacket(2));sent.clear();world.Tick(10700);
    assert(Positions()==0 && !AnyVisible(&world.ta));

    World rollback;const auto before=rollback.Handshake();rollback.ta.GameTime=500;rollback.Sample();
    rollback.ta.GameTime=0;rollback.Sample();sent.clear();rollback.Tick(10100);
    std::uint32_t after=0;for(const auto& event:sent)if(event.packet.kind==Hello)after=event.packet.session;
    assert(after && after!=before);
    auto prior=Make(ChatHijackId::AlliedCursor,HelloAck,7001);prior.echoSession=before;rollback.Deliver(1,prior);
    rollback.Deliver(1,rollback.PositionPacket());assert(!AnyVisible(&rollback.ta));
}
void TestSynchronousSendReentryAndRevocation() {
    World world;
    onSend=[&](const Sent& event) {
        // If HAPI_SendBuf is called under g_mutex, this immediate callback hangs.
        AlliedCursors::Snapshot(&world.ta);
        if(event.packet.kind==Hello) {
            auto hello=Make(ChatHijackId::AlliedCursor,Hello,7001);world.Deliver(1,hello);
            auto ack=Make(ChatHijackId::AlliedCursor,HelloAck,7001);ack.echoSession=event.packet.session;world.Deliver(1,ack);
        }
    };
    world.Sample();world.Tick(10000);world.Sample();world.Tick(10100);
    assert(Positions()==1);onSend=nullptr;
    auto hello=Make(ChatHijackId::AlliedCursor,Hello,7001);world.Deliver(1,hello);
    sent.clear();wallClock=16000;world.Sample();
    onSend=[&](const Sent&) { world.Ally(1,false); AlliedCursors::Snapshot(&world.ta); };
    world.Tick(16000);onSend=nullptr;
    assert(!sent.empty() && Positions()==0); // Queued coordinate sends are re-authorized after callback revocation.
}
int main() {
#if ALLIED_CURSORS_ENABLE
    TestInstallationAndLegacyPeer();TestNativePrivacyAndPeerEligibility();
    TestHandshakeRenderingSnapshotAndPacing();TestFreshnessAndBounds();
    TestAllianceLossAndDpidRecycling();TestMenuRestartAndOldSessionRejection();
    TestSynchronousSendReentryAndRevocation();
    std::cout << "Allied cursor production-controller tests passed\n";
#else
    World world;world.Sample();world.Tick(10000);
    assert(sent.empty() && !AnyVisible(&world.ta));
    assert(PacketChatRouter::GetInstance()->registrations==0 && GameTickHook::GetInstance()->callbacks.empty());
    std::cout << "Allied cursor compiled-out controller tests passed\n";
#endif
}
'''


def main():
    source = (DDRAW / "AlliedCursors.cpp").read_text()
    source = re.sub(r'^\s*#include[^\n]*$', "", source.lstrip("\ufeff"), flags=re.M)
    harness = HARNESS.replace("// PRODUCTION_SOURCE", source)
    env = os.environ.copy()
    env["ASAN_OPTIONS"] = env.get("ASAN_OPTIONS", "") + ":detect_leaks=0"
    with tempfile.TemporaryDirectory(prefix="allied-controller-") as temp:
        cpp = Path(temp) / "allied_cursor_controller.cpp"
        cpp.write_text(harness)
        for enabled in (1,0):
            binary = Path(temp) / ("controller"+str(enabled))
            subprocess.run(shlex.split(os.environ.get("CXX", "g++")) + [
                "-std=c++11", "-Wall", "-Wextra", "-Werror", "-g", "-pthread",
                "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-I", str(DDRAW),
                "-DALLIED_CURSORS_ENABLE="+str(enabled), str(cpp), "-o", str(binary),
            ], check=True)
            subprocess.run([str(binary)], check=True, env=env, timeout=15)


if __name__ == "__main__":
    main()
