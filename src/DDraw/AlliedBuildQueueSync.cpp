#include "AlliedBuildQueueSync.h"
#if ALLIED_BUILD_QUEUE_ENABLE
#include "ChatHijackIds.h"
#include "GameTickHook.h"
#include "PacketChatRouter.h"
#include "tamem.h"
#include "tafunctions.h"
#include "unitrotate.h"
#include "iddrawsurface.h"
#include "dialog.h"
#include <array>
#include <cstdio>

// TAG_Venom: display-only transactions, paced independently of game speed.
namespace
{
	using namespace AlliedQueueProtocol;
	const DWORD ScanMs = 100, ControlMs = 2000, ProbeMs = 5000;
	const DWORD LeaseMs = 6000, StaleMs = 10000, RetryMs = 1000;
	const unsigned BurstPackets = 32;
	struct Peer
	{
		unsigned dpid = 0;
		Transmitter tx;
		Receiver rx;
		DWORD interestAt = 0, heardAt = 0, controlAt = 0, roundAt = 0, transactionAt = 0;
		bool subscribed = false, known = false, sentControl = false, ackDue = false;
	};
	struct Diagnostics
	{
		unsigned long long bytes = 0, packets = 0, dataPackets = 0, transactions = 0;
		unsigned long long avoided = 0, retries = 0, failures = 0, scans = 0, scanUs = 0, tickUs = 0;
		unsigned long long ackMsTotal = 0, ackCount = 0;
		unsigned peakAckMs = 0, peakScanUs = 0, peakTickUs = 0, peakBurst = 0, peakBps = 0;
		unsigned windowBytes = 0, windowPackets = 0, peakPps = 0;
		DWORD start = 0, windowAt = 0, dumpAt = 0;
	};
	std::array<Peer, 10> g_peers;
	std::vector<AlliedBuildQueueRecord> g_current;
	std::array<unsigned short, 4096> g_seen = {};
	Budget g_budget;
	Diagnostics g_stats;
	bool g_installed = false, g_diagnostics = TDRAW_ALLIED_BUILD_QUEUE_DIAGNOSTICS != 0;
	bool g_wanted = false, g_haveScan = false;
	unsigned g_dpid = 0, g_nextPeer = 0, g_packetsPerSecond = 800;
	std::uint32_t g_sequence = 0;
	int g_lastTick = -1;
	DWORD g_scanAt = 0;
	LARGE_INTEGER g_frequency = {};

	unsigned ElapsedUs(const LARGE_INTEGER& start)
	{
		LARGE_INTEGER end; QueryPerformanceCounter(&end);
		return static_cast<unsigned>((end.QuadPart - start.QuadPart) * 1000000 / g_frequency.QuadPart);
	}
	bool LivePlayer(const PlayerStruct& p)
	{
		return p.PlayerActive && p.DirectPlayID && p.PlayerInfo && !(p.PlayerInfo->PropertyMask & WATCH);
	}
	bool Authorized(TAdynmemStruct* ta, int me, int other)
	{
		return ta && me >= 0 && me < 10 && other >= 0 && other < 10 && me != other
			&& LivePlayer(ta->Players[me]) && LivePlayer(ta->Players[other])
			&& ta->Players[me].DirectPlayID != ta->Players[other].DirectPlayID
			&& ta->Players[me].AllyFlagAry[other] && ta->Players[other].AllyFlagAry[me];
	}
	bool WantsQueues()
	{
		return (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0
			&& (!LocalShare || !LocalShare->Dialog
				|| reinterpret_cast<Dialog*>(LocalShare->Dialog)->GetShowAlliedBuildQueuesEnabled());
	}
	void DumpStats(DWORD now, bool force = false)
	{
		if (!g_diagnostics || (!force && now - g_stats.dumpAt < 5000)) return;
		g_stats.dumpAt = now;
		FILE* file = NULL;
		if (fopen_s(&file, "allied-build-queue.csv", "a") != 0 || !file) return;
		// HAPI payload accepted for sending, excluding transport framing.
		fprintf(file, "%lu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%u,%u,%u,%llu,%llu,%u,%llu,%u,%llu,%llu,%u\n",
			now - g_stats.start, g_stats.bytes, g_stats.packets, g_stats.dataPackets,
			g_stats.transactions, g_stats.avoided, g_stats.retries, g_stats.failures,
			g_stats.peakBps, g_stats.peakBurst, g_stats.peakPps,
			g_stats.scans, g_stats.scanUs, g_stats.peakScanUs, g_stats.tickUs,
			g_stats.peakTickUs, g_stats.ackCount, g_stats.ackMsTotal, g_stats.peakAckMs);
		fclose(file);
	}
	void Reset(unsigned dpid, int tick, DWORD now)
	{
		DumpStats(now, true);
		for (auto& peer : g_peers) peer = Peer();
		g_current.clear(); g_haveScan = false; g_wanted = false;
		g_dpid = dpid; g_lastTick = tick; g_scanAt = now - ScanMs;
		// Keep sequence monotonic within this process across matches.
		g_budget = Budget(); g_budget.last = now;
		g_stats = Diagnostics(); g_stats.start = g_stats.windowAt = g_stats.dumpAt = now;
	}
	void Collect(TAdynmemStruct* ta, int me)
	{
		g_current.clear();
		g_seen.fill(0);
		if (!ta->BeginUnitsArray_p || !ta->EndOfUnitsArray_p || !ta->UnitDef) return;
		// The player's contiguous slot bank avoids visiting opponent banks.
		UnitStruct* begin = ta->Players[me].Units;
		UnitStruct* end = ta->Players[me].UnitsAry_End;
		if (!begin || !end || begin < ta->BeginUnitsArray_p || end >= ta->EndOfUnitsArray_p || end < begin)
		{ begin = ta->BeginUnitsArray_p; end = ta->EndOfUnitsArray_p; }
		else ++end; // Player.UnitsAry_End is inclusive; the global loop bound is exclusive.
		CUnitRotate* rotate = CUnitRotate::GetInstance();
		for (UnitStruct* unit = begin; unit < end; ++unit)
		{
			if (!(unit->UnitSelected & 0x10000000) || (unit->UnitSelected & 0x4000)) continue;
			PlayerStruct* owner = unit->Owner_PlayerPtr0 ? unit->Owner_PlayerPtr0 : unit->Owner_PlayerPtr1;
			if (owner != &ta->Players[me]) continue;
			unsigned visited = 0;
			// Cycle guard: no order pointer survives this read-only scan.
			for (UnitOrdersStruct* order = unit->UnitOrders; order && visited++ < 8192; order = order->NextOrder)
			{
				if (!AlliedBuildQueueSync::IsBuildOrder(order) || order->BuildUnitID >= ta->UNITINFOCount
					|| ta->UnitDef[order->BuildUnitID].bmcode != 0 || (!order->Pos.X && !order->Pos.Y)) continue;
				AlliedBuildQueueRecord record = { static_cast<unsigned short>(order->BuildUnitID),
					order->Pos.X, order->Pos.Y, order->Pos.Z,
					static_cast<unsigned char>(rotate ? rotate->TakeOrderRotation(order) & 3 : 0) };
				// Fixed scratch deduplication: no per-order allocation and no
				// repeated sorting when many builders share the same placements.
				unsigned hash = record.x * 73856093u ^ record.y * 19349663u
					^ record.z * 83492791u ^ record.buildUnitId * 2654435761u ^ record.rotation;
				size_t index = hash & (g_seen.size() - 1);
				while (g_seen[index] && !Equal(g_current[g_seen[index] - 1], record))
					index = (index + 1) & (g_seen.size() - 1);
				if (g_seen[index]) continue;
				g_current.push_back(record);
				g_seen[index] = static_cast<unsigned short>(g_current.size());
				if (g_current.size() == MaxRecords) break;
			}
			if (g_current.size() == MaxRecords) break;
		}
		Canonicalize(g_current);
		if (g_current.size() > MaxRecords) g_current.resize(MaxRecords);
	}
	bool Send(unsigned to, const Packet& packet)
	{
		if (!HAPI_SendBuf(g_dpid, to, reinterpret_cast<const char*>(&packet), sizeof(packet)))
		{ if (g_diagnostics) ++g_stats.failures; return false; }
		if (g_diagnostics)
		{
			g_stats.bytes += sizeof(packet); ++g_stats.packets;
			g_stats.windowBytes += sizeof(packet); ++g_stats.windowPackets;
			if (packet.kind != Interest) ++g_stats.dataPackets;
		}
		return true;
	}
	void HandlePacket(unsigned fromDpid, const void* buffer)
	{
		if (!g_installed || !buffer || (DataShare && DataShare->PlayingDemo)) return;
		const Packet& packet = *static_cast<const Packet*>(buffer);
		if (!Envelope(packet, ChatHijackId::AlliedBuildQueue)) return; // v1 ignored
		TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
		if (!ta || ta->GameTime < g_lastTick || ta->LocalHumanPlayer_PlayerID < 0
			|| ta->LocalHumanPlayer_PlayerID >= 10
			|| static_cast<unsigned>(ta->Players[ta->LocalHumanPlayer_PlayerID].DirectPlayID) != g_dpid) return;
		PlayerStruct* sender = FindPlayerByDPID(fromDpid);
		if (!sender || !Authorized(ta, ta->LocalHumanPlayer_PlayerID, sender->PlayerAryIndex)) return;
		Peer& peer = g_peers[sender->PlayerAryIndex];
		if (peer.dpid != fromDpid) { peer = Peer(); peer.dpid = fromDpid; }
		const DWORD now = GetTickCount();
		if (packet.kind == Interest)
		{
			if (packet.base > 3 || packet.chunkCount || packet.chunkIndex || packet.recordCount) return;
			peer.known = true;
			if (packet.base & 2) peer.heardAt = now;
			const bool wasSubscribed = peer.subscribed;
			peer.subscribed = (packet.base & 1) != 0;
			if (peer.subscribed)
			{
				peer.interestAt = now;
				if (!wasSubscribed) { g_scanAt = now - ScanMs; peer.sentControl = false; }
			}
			else peer.tx.active = false;
			if (g_diagnostics && peer.tx.active && packet.sequence == peer.tx.sequence)
			{
				unsigned age = now - peer.transactionAt;
				++g_stats.ackCount; g_stats.ackMsTotal += age;
				g_stats.peakAckMs = (std::max)(g_stats.peakAckMs, age);
			}
			peer.tx.Acknowledge(packet.sequence);
			return;
		}
		if (!g_wanted) return;
		for (size_t i = 0; i < packet.recordCount && i < RecordsPerPacket; ++i)
			if (!ta->UnitDef || packet.records[i].buildUnitId >= ta->UNITINFOCount
				|| ta->UnitDef[packet.records[i].buildUnitId].bmcode != 0) return;
		if (peer.rx.Accept(packet)) { peer.ackDue = true; peer.heardAt = now; peer.known = true; }
	}
	void OnGameTick(int gameTime)
	{
		if (!g_installed) return;
		LARGE_INTEGER tickStart = {}; if (g_diagnostics) QueryPerformanceCounter(&tickStart);
		TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
		const DWORD now = GetTickCount();
		if (!ta || (DataShare && DataShare->PlayingDemo)) { if (g_dpid) Reset(0, -1, now); return; }
		const int me = ta->LocalHumanPlayer_PlayerID;
		if (me < 0 || me >= 10) return;
		const unsigned dpid = ta->Players[me].DirectPlayID;
		if (dpid != g_dpid || gameTime < g_lastTick) Reset(dpid, gameTime, now);
		g_lastTick = gameTime;
		if (!LivePlayer(ta->Players[me])) { if (g_dpid) Reset(0, -1, now); return; }
		const bool wanted = WantsQueues();
		if (wanted != g_wanted)
		{
			g_wanted = wanted;
			for (auto& peer : g_peers) peer.ackDue = true;
		}
		bool anySubscriber = false;
		for (int i = 0; i < 10; ++i)
		{
			Peer& peer = g_peers[i];
			if (!Authorized(ta, me, i)) { peer = Peer(); continue; }
			if (peer.dpid != static_cast<unsigned>(ta->Players[i].DirectPlayID))
			{ peer = Peer(); peer.dpid = ta->Players[i].DirectPlayID; }
			if (peer.subscribed && now - peer.interestAt >= LeaseMs)
			{ peer.subscribed = false; peer.tx.active = false; }
			if (!ta->IsGamePaused && peer.rx.sequence && now - peer.heardAt >= StaleMs)
				peer.rx = Receiver();
			anySubscriber = anySubscriber || peer.subscribed;
		}
		if (anySubscriber && (!g_haveScan || now - g_scanAt >= ScanMs))
		{
			LARGE_INTEGER start = {}; if (g_diagnostics) QueryPerformanceCounter(&start);
			Collect(ta, me); g_haveScan = true; g_scanAt = now;
			if (g_diagnostics)
			{
				unsigned us = ElapsedUs(start); ++g_stats.scans; g_stats.scanUs += us;
				g_stats.peakScanUs = (std::max)(g_stats.peakScanUs, us);
			}
			for (auto& peer : g_peers)
			{
				if (!peer.subscribed || peer.tx.active) continue;
				if (!++g_sequence) ++g_sequence;
				if (peer.tx.Prepare(g_current, g_sequence))
				{
					peer.transactionAt = now; peer.roundAt = now;
					if (g_diagnostics) ++g_stats.transactions;
				}
				else if (g_diagnostics) ++g_stats.avoided;
			}
		}
		unsigned allowance = g_budget.Take(now, g_packetsPerSecond, BurstPackets), sent = 0;
		// Fair across peers; controls and data share one global byte budget.
		for (unsigned misses = 0; allowance && misses < 10; )
		{
			Peer& peer = g_peers[g_nextPeer++ % 10];
			if (!peer.dpid) { ++misses; continue; }
			const DWORD interval = peer.known ? ControlMs : ProbeMs;
			if (peer.ackDue || ((wanted || peer.subscribed) && (!peer.sentControl || now - peer.controlAt >= interval)))
			{
				Packet p = MakePacket(ChatHijackId::AlliedBuildQueue, Interest);
				p.base = (wanted ? 1 : 0) | (peer.subscribed ? 2 : 0);
				p.sequence = peer.rx.Acknowledgement();
				if (!Send(peer.dpid, p)) break;
				peer.ackDue = false; peer.sentControl = true; peer.controlAt = now;
			}
			else if (peer.subscribed && peer.tx.active)
			{
				if (peer.tx.nextChunk == peer.tx.ChunkCount())
				{
					if (now - peer.roundAt < RetryMs) { ++misses; continue; }
					peer.tx.nextChunk = 0; if (g_diagnostics) ++g_stats.retries;
				}
				const Packet p = peer.tx.Chunk(ChatHijackId::AlliedBuildQueue, peer.tx.nextChunk);
				if (!Send(peer.dpid, p)) break;
				if (++peer.tx.nextChunk == peer.tx.ChunkCount()) peer.roundAt = now;
			}
			else { ++misses; continue; }
			--allowance; ++sent; misses = 0;
		}
		if (g_diagnostics)
		{
			g_stats.peakBurst = (std::max)(g_stats.peakBurst, sent);
			if (now - g_stats.windowAt >= 1000)
			{
				g_stats.peakBps = (std::max)(g_stats.peakBps,
					static_cast<unsigned>(g_stats.windowBytes * 1000ull / (now - g_stats.windowAt)));
				g_stats.peakPps = (std::max)(g_stats.peakPps,
					static_cast<unsigned>(g_stats.windowPackets * 1000ull / (now - g_stats.windowAt)));
				g_stats.windowAt = now; g_stats.windowBytes = g_stats.windowPackets = 0;
			}
			unsigned us = ElapsedUs(tickStart); g_stats.tickUs += us;
			g_stats.peakTickUs = (std::max)(g_stats.peakTickUs, us); DumpStats(now);
		}
	}
}

namespace AlliedBuildQueueSync
{
	bool Enabled() { return g_installed; }
	bool CanSeePlayer(int slot)
	{
		TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
		return g_installed && ta && !(DataShare && DataShare->PlayingDemo)
			&& Authorized(ta, ta->LocalHumanPlayer_PlayerID, slot);
	}
	bool IsBuildOrder(const UnitOrdersStruct* order)
	{
		if (!order || !order->BuildUnitID || !COBSciptHandler_Begin || !COBSciptHandler_End) return false;
		const COBHandle* begin = *COBSciptHandler_Begin;
		const COBHandle* end = *COBSciptHandler_End;
		return begin && end && end > begin && order->COBHandler_index < end - begin
			&& (begin[order->COBHandler_index].COBScripMask & 1) != 0;
	}
	void Install()
	{
		if (g_installed) return;
		QueryPerformanceFrequency(&g_frequency); if (!g_frequency.QuadPart) g_frequency.QuadPart = 1;
		g_current.reserve(MaxRecords);
		if (g_diagnostics)
		{
			FILE* file = NULL;
			if (!fopen_s(&file, "allied-build-queue.csv", "a") && file)
			{
				fputs("elapsed_ms,bytes,packets,data_packets,transactions,unchanged_peer_scans,retry_rounds,send_failures,peak_Bps,peak_packets_per_tick,peak_pps,scans,scan_us_total,scan_us_peak,tick_us_total,tick_us_peak,ack_count,ack_ms_total,ack_ms_peak\n", file);
				fclose(file);
			}
		}
		Reset(0, -1, GetTickCount()); g_installed = true;
		PacketChatRouter::GetInstance()->RegisterHandler(ChatHijackId::AlliedBuildQueue, HandlePacket);
		GameTickHook::GetInstance()->addCallback(OnGameTick);
	}
	void Shutdown() { g_installed = false; Reset(0, -1, GetTickCount()); }
	const std::vector<AlliedBuildQueueRecord>& GetPlayerQueue(int slot)
	{
		static const std::vector<AlliedBuildQueueRecord> empty;
		TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
		if (!g_installed || !ta || (DataShare && DataShare->PlayingDemo)
			|| !Authorized(ta, ta->LocalHumanPlayer_PlayerID, slot)
			|| g_peers[slot].dpid != static_cast<unsigned>(ta->Players[slot].DirectPlayID)
			// Game-tick keepalives stop with the simulation; retain a completed
			// overlay during a pause while still enforcing identity/alliance checks.
			|| (!ta->IsGamePaused && GetTickCount() - g_peers[slot].heardAt >= StaleMs)) return empty;
		return g_peers[slot].rx.records;
	}
}
#endif // ALLIED_BUILD_QUEUE_ENABLE
