#include "config.h"
#include "AlliedCursors.h"

#if ALLIED_CURSORS_ENABLE
#include "oddraw.h"
#include "tamem.h"
#include "iddrawsurface.h"
#include "tafunctions.h"
#include "AlliedCursorProtocol.h"
#include "ChatHijackIds.h"
#include "GameTickHook.h"
#include "PacketChatRouter.h"
#include <array>
#include <cstring>
#include <mutex>

namespace
{
    using namespace AlliedCursorProtocol;
    struct Remote
    {
        unsigned dpid = 0;
        Peer state;
        Transmitter tx;
        std::uint32_t probeAt = 0, ackAt = 0;
        bool probed = false, acknowledged = false;
    };
    struct LocalSample
    {
        Point point;
        std::uint32_t at = 0;
        bool valid = false;
    };
    struct Outbound
    {
        unsigned from = 0, to = 0;
        Packet packet = {};
    };
    std::mutex g_mutex;
    std::array<Remote, 10> g_peers;
    LocalSample g_sample;
    Budget g_budget;
    // Used for equality only; no game/unit pointer is retained by a cursor.
    TAdynmemStruct* g_world = NULL;
    unsigned g_dpid = 0, g_session = 0, g_serial = 0, g_nextPeer = 0;
    int g_localSlot = -1, g_lastTick = -1;
    bool g_installed = false;

    bool LivePlayer(const PlayerStruct& player)
    {
        return player.PlayerActive && player.DirectPlayID && player.PlayerInfo
            && !(player.PlayerInfo->PropertyMask & WATCH);
    }
    bool Playable(TAdynmemStruct* ta)
    {
        if (!ta || !DataShare || DataShare->TAProgress != TAInGame || DataShare->PlayingDemo)
            return false;
        const int me = ta->LocalHumanPlayer_PlayerID;
        return me >= 0 && me < 10 && LivePlayer(ta->Players[me])
            && ta->Players[me].My_PlayerType == Player_LocalHuman;
    }
    bool Authorized(TAdynmemStruct* ta, int other)
    {
        return other >= 0 && other < 10 && other != g_localSlot
            && LivePlayer(ta->Players[other])
            && GetInferredPlayerType(&ta->Players[other]) == Player_RemoteHuman
            && static_cast<unsigned>(ta->Players[other].DirectPlayID) != g_dpid
            && ta->Players[g_localSlot].AllyFlagAry[other]
            && ta->Players[other].AllyFlagAry[g_localSlot];
    }
    bool InMap(TAdynmemStruct* ta, const Point& p)
    {
        return p.x >= 0 && p.y >= 0 && ta->FeatureMapSizeX > 0 && ta->FeatureMapSizeY > 8
            && static_cast<std::int64_t>(p.x) < static_cast<std::int64_t>(ta->FeatureMapSizeX) * 16
            && static_cast<std::int64_t>(p.y) < static_cast<std::int64_t>(ta->FeatureMapSizeY - 8) * 16;
    }
    void Clear()
    {
        g_peers = {}; g_sample = LocalSample(); g_budget = Budget(); g_nextPeer = 0;
        g_world = NULL; g_dpid = 0; g_localSlot = -1; g_lastTick = -1;
    }
    bool Context(TAdynmemStruct* ta, int tick)
    {
        if (!g_installed || !Playable(ta)) { Clear(); return false; }
        const int slot = ta->LocalHumanPlayer_PlayerID;
        const unsigned dpid = ta->Players[slot].DirectPlayID;
        if (ta != g_world || slot != g_localSlot || dpid != g_dpid || tick < g_lastTick)
        {
            Clear(); g_world = ta; g_localSlot = slot; g_dpid = dpid;
            // Nonzero per-match token; all HelloAck messages must echo it.
            if (!g_session) g_session = GetTickCount();
            if (!++g_session) ++g_session;
        }
        g_lastTick = tick;
        return true;
    }
    void RefreshAllies(TAdynmemStruct* ta, std::uint32_t now)
    {
        for (int slot = 0; slot < 10; ++slot)
        {
            Remote& peer = g_peers[slot];
            if (!Authorized(ta, slot)) { peer = Remote(); continue; }
            const unsigned dpid = ta->Players[slot].DirectPlayID;
            if (peer.dpid != dpid) { peer = Remote(); peer.dpid = dpid; }
            const bool capable = peer.state.capable;
            peer.state.Expire(now);
            if (capable && !peer.state.capable) peer.tx = Transmitter();
        }
    }
    void Receive(unsigned fromDpid, const void* buffer)
    {
        if (!buffer) return;
        Packet packet;
        std::memcpy(&packet, buffer, sizeof(packet));
        if (!Envelope(packet, ChatHijackId::AlliedCursor)) return;
        std::lock_guard<std::mutex> lock(g_mutex);
        TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
        if (!Context(ta, ta ? ta->GameTime : -1)) return;
        const std::uint32_t now = GetTickCount();
        RefreshAllies(ta, now);
        for (int slot = 0; slot < 10; ++slot)
        {
            Remote& peer = g_peers[slot];
            if (!peer.dpid || peer.dpid != fromDpid) continue;
            Point point; point.x = packet.x; point.y = packet.y; point.z = packet.z;
            if (packet.kind == Position && !InMap(ta, point)) return;
            const std::uint32_t oldSession = peer.state.session;
            const bool wasCapable = peer.state.capable;
            if (peer.state.Accept(packet, g_session, now)
                && (oldSession != peer.state.session || (!wasCapable && peer.state.capable)))
                peer.tx = Transmitter();
            return;
        }
    }
    void OnGameTick(int)
    {
        // At most one control request, one reply and one cursor per ally.
        // Build all packets under the lock; sending can synchronously re-enter.
        std::array<Outbound, 30> pending;
        std::size_t count = 0;
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
            // A presentation callback can have observed a newer native time
            // while this callback waited for the mutex. Read it after locking.
            if (!Context(ta, ta ? ta->GameTime : -1)) return;
            const std::uint32_t now = GetTickCount();
            RefreshAllies(ta, now);
            const bool valid = g_sample.valid && now - g_sample.at < 500 && InMap(ta, g_sample.point);
            const unsigned first = g_nextPeer++ % 10;
            for (unsigned index = 0; index < 10; ++index)
            {
                Remote& peer = g_peers[(first + index) % 10];
                if (!peer.dpid) continue;
                if (peer.state.pendingAck && (!peer.acknowledged || now - peer.ackAt >= AckIntervalMs)
                    && g_budget.Take(now))
                {
                    Outbound& out = pending[count++]; out.from = g_dpid; out.to = peer.dpid;
                    out.packet = Make(ChatHijackId::AlliedCursor, HelloAck, g_session);
                    out.packet.echoSession = peer.state.pendingAck;
                    peer.state.pendingAck = 0; peer.acknowledged = true; peer.ackAt = now;
                }
                if ((!peer.probed || now - peer.probeAt >= ProbeMs) && g_budget.Take(now))
                {
                    Outbound& out = pending[count++]; out.from = g_dpid; out.to = peer.dpid;
                    out.packet = Make(ChatHijackId::AlliedCursor, Hello, g_session);
                    peer.probed = true; peer.probeAt = now;
                }
                if (peer.state.capable && peer.tx.Due(g_sample.point, valid, now) && g_budget.Take(now))
                {
                    Outbound& out = pending[count++]; out.from = g_dpid; out.to = peer.dpid;
                    out.packet = Make(ChatHijackId::AlliedCursor, valid ? Position : Hidden, g_session);
                    if (!++g_serial) ++g_serial;
                    out.packet.sequence = g_serial;
                    if (valid)
                    {
                        out.packet.x = g_sample.point.x;
                        out.packet.y = g_sample.point.y;
                        out.packet.z = g_sample.point.z;
                    }
                    peer.tx.Sent(g_sample.point, valid, now);
                }
            }
        }
        for (std::size_t i = 0; i < count; ++i)
        {
            const Outbound& out = pending[i];
            bool allowed = false;
            {
                std::lock_guard<std::mutex> lock(g_mutex);
                TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
                if (Context(ta, ta ? ta->GameTime : -1) && out.from == g_dpid
                    && out.packet.session == g_session)
                    for (int slot = 0; slot < 10; ++slot)
                        if (Authorized(ta, slot) && static_cast<unsigned>(ta->Players[slot].DirectPlayID) == out.to)
                            allowed = out.packet.kind <= HelloAck || g_peers[slot].state.capable;
            }
            if (allowed) HAPI_SendBuf(out.from, out.to, reinterpret_cast<const char*>(&out.packet), sizeof(out.packet));
        }
    }
}

namespace AlliedCursors
{
    void Install()
    {
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            if (g_installed) return;
            g_installed = true; Clear();
        }
        PacketChatRouter::GetInstance()->RegisterHandler(ChatHijackId::AlliedCursor, Receive);
        GameTickHook::GetInstance()->addCallback(OnGameTick);
        IDDrawSurface::OutptTxt("[AlliedCursors] build=light-v1 wire=1 max-hz=10");
    }
    void UpdateLocal(const _Position_Dword* position, bool valid)
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
        if (!Context(ta, ta ? ta->GameTime : -1)) return;
        g_sample = LocalSample(); g_sample.at = GetTickCount();
        if (!valid || !position) return;
        g_sample.point.x = position->X; g_sample.point.y = position->Y;
        g_sample.point.z = static_cast<short>(position->Z) < 0 ? ta->SeaLevel : position->Z;
        g_sample.valid = InMap(ta, g_sample.point);
    }
    std::array<AlliedCursorView, 10> Snapshot(TAdynmemStruct* ta)
    {
        std::array<AlliedCursorView, 10> result = {};
        std::lock_guard<std::mutex> lock(g_mutex);
        if (!Context(ta, ta ? ta->GameTime : -1)) return result;
        const std::uint32_t now = GetTickCount();
        RefreshAllies(ta, now);
        for (int slot = 0; slot < 10; ++slot)
        {
            const Remote& peer = g_peers[slot];
            if (!peer.dpid || !peer.state.Visible(now) || !InMap(ta, peer.state.point)) continue;
            AlliedCursorView& view = result[slot];
            view.visible = true; view.dpid = peer.dpid; view.playerSlot = slot;
            view.x = peer.state.point.x; view.y = peer.state.point.y; view.z = peer.state.point.z;
            view.receivedAt = peer.state.receivedAt;
            view.color = static_cast<unsigned char>(ta->Players[slot].PlayerInfo->PlayerLogoColor);
            std::size_t length = 0;
            while (length + 1 < sizeof(view.name) && length < sizeof(ta->Players[slot].Name)
                && ta->Players[slot].Name[length])
            {
                view.name[length] = ta->Players[slot].Name[length];
                ++length;
            }
            view.name[length] = '\0';
        }
        return result;
    }
}
#else
namespace AlliedCursors
{
    void Install() {}
    void UpdateLocal(const _Position_Dword*, bool) {}
    std::array<AlliedCursorView, 10> Snapshot(TAdynmemStruct*) { return {}; }
}
#endif
