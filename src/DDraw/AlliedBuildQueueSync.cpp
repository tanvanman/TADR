#include "AlliedBuildQueueSync.h"

#include "ChatHijackIds.h"
#include "GameTickHook.h"
#include "PacketChatRouter.h"
#include "tamem.h"
#include "tafunctions.h"
#include "unitrotate.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <set>
#include <vector>

namespace
{
	const unsigned char kProtocolVersion = 1;
	const size_t kRecordsPerPacket = 6;
	// TA chat packets can carry six records. Keep the snapshot below the
	// one-byte chunk-counter limit while allowing substantial line-build queues.
	const size_t kMaxChunks = 200;
	const size_t kMaxRecords = kRecordsPerPacket * kMaxChunks;
	const int kRefreshTicks = 90;
	const int kStaleTicks = 300;

#pragma pack(push, 1)
	struct QueueRecordWire
	{
		unsigned short buildUnitId;
		unsigned short x;
		unsigned short y;
		unsigned short z;
		unsigned char rotation;
	};

	struct QueueSnapshotPacket
	{
		unsigned char chatByte;
		unsigned char nullText;
		unsigned char msgId;
		unsigned char size;
		unsigned char version;
		unsigned short sequence;
		unsigned char chunkIndex;
		unsigned char chunkCount;
		unsigned char recordCount;
		QueueRecordWire records[kRecordsPerPacket];
		unsigned char reserved;
	};
#pragma pack(pop)

	static_assert(sizeof(QueueSnapshotPacket) == 65, "QueueSnapshotPacket must fit one TA chat packet");

	struct PendingSnapshot
	{
		bool active = false;
		unsigned short sequence = 0;
		unsigned char chunkCount = 0;
		std::vector<std::vector<AlliedBuildQueueRecord> > chunks;
		std::vector<bool> received;
	};

	struct RemoteQueue
	{
		std::vector<AlliedBuildQueueRecord> records;
		PendingSnapshot pending;
		unsigned short completedSequence = 0;
		bool hasCompletedSequence = false;
		int lastUpdateTick = -1;
	};

	bool g_installed = false;
	std::array<RemoteQueue, 10> g_remoteQueues;
	std::vector<AlliedBuildQueueRecord> g_lastLocalQueue;
	unsigned short g_nextSequence = 0;
	unsigned g_sessionDpid = 0;
	int g_lastTick = -1;
	int g_lastSendTick = -1;

	bool SameRecord(const AlliedBuildQueueRecord& a, const AlliedBuildQueueRecord& b)
	{
		return a.buildUnitId == b.buildUnitId
			&& a.x == b.x
			&& a.y == b.y
			&& a.z == b.z
			&& a.rotation == b.rotation;
	}

	bool RecordLess(const AlliedBuildQueueRecord& a, const AlliedBuildQueueRecord& b)
	{
		if (a.x != b.x) return a.x < b.x;
		if (a.y != b.y) return a.y < b.y;
		if (a.z != b.z) return a.z < b.z;
		if (a.buildUnitId != b.buildUnitId) return a.buildUnitId < b.buildUnitId;
		return a.rotation < b.rotation;
	}

	bool SameQueue(
		const std::vector<AlliedBuildQueueRecord>& a,
		const std::vector<AlliedBuildQueueRecord>& b)
	{
		if (a.size() != b.size()) return false;
		for (size_t i = 0; i < a.size(); ++i)
		{
			if (!SameRecord(a[i], b[i])) return false;
		}
		return true;
	}

	bool IsNewerSequence(unsigned short candidate, unsigned short current)
	{
		return static_cast<short>(candidate - current) > 0;
	}

	bool IsMutualAlly(TAdynmemStruct* ta, int first, int second)
	{
		if (!ta || first < 0 || first >= 10 || second < 0 || second >= 10 || first == second)
			return false;

		return ta->Players[first].AllyFlagAry[second] != 0
			&& ta->Players[second].AllyFlagAry[first] != 0;
	}

	bool IsLiveUnit(const UnitStruct* unit)
	{
		return unit != NULL
			&& (unit->UnitSelected & 0x10000000) != 0
			&& (unit->UnitSelected & 0x4000) == 0;
	}

	bool IsBuildOrder(const UnitOrdersStruct* order)
	{
		if (!order || order->BuildUnitID == 0)
			return false;

		if (!COBSciptHandler_Begin || !*COBSciptHandler_Begin)
			return false;

		return ((*COBSciptHandler_Begin)[order->COBHandler_index].COBScripMask & 1) != 0;
	}

	void ResetSession(unsigned dpid, int gameTime)
	{
		for (size_t i = 0; i < g_remoteQueues.size(); ++i)
			g_remoteQueues[i] = RemoteQueue();

		g_lastLocalQueue.clear();
		g_nextSequence = 0;
		g_sessionDpid = dpid;
		g_lastTick = gameTime;
		g_lastSendTick = -1;
	}

	std::vector<AlliedBuildQueueRecord> CollectLocalQueue(TAdynmemStruct* ta, int localSlot)
	{
		std::set<AlliedBuildQueueRecord,
			bool (*)(const AlliedBuildQueueRecord&, const AlliedBuildQueueRecord&)> records(RecordLess);
		if (!ta || localSlot < 0 || localSlot >= 10
			|| !ta->BeginUnitsArray_p || !ta->EndOfUnitsArray_p || !ta->UnitDef)
		{
			return std::vector<AlliedBuildQueueRecord>();
		}

		for (UnitStruct* unit = ta->BeginUnitsArray_p; unit < ta->EndOfUnitsArray_p; ++unit)
		{
			if (!IsLiveUnit(unit))
				continue;

			PlayerStruct* owner = unit->Owner_PlayerPtr0 ? unit->Owner_PlayerPtr0 : unit->Owner_PlayerPtr1;
			if (!owner || owner->PlayerAryIndex != localSlot)
				continue;

			for (UnitOrdersStruct* order = unit->UnitOrders; order; order = order->NextOrder)
			{
				if (!IsBuildOrder(order))
					continue;

				const unsigned buildUnitId = order->BuildUnitID;
				if (buildUnitId >= ta->UNITINFOCount || ta->UnitDef[buildUnitId].bmcode != 0)
					continue;

				if (order->Pos.X == 0 && order->Pos.Y == 0)
					continue;

				CUnitRotate* rotate = CUnitRotate::GetInstance();
				AlliedBuildQueueRecord record = {
					static_cast<unsigned short>(buildUnitId),
					order->Pos.X,
					order->Pos.Y,
					order->Pos.Z,
					static_cast<unsigned char>(rotate ? rotate->TakeOrderRotation(order) & 3 : 0)
				};

				records.insert(record);
				if (records.size() >= kMaxRecords)
					break;
			}

			if (records.size() >= kMaxRecords)
				break;
		}

		return std::vector<AlliedBuildQueueRecord>(records.begin(), records.end());
	}

	void SendSnapshot(
		TAdynmemStruct* ta,
		int localSlot,
		int targetSlot,
		const std::vector<AlliedBuildQueueRecord>& records,
		unsigned short sequence)
	{
		const unsigned fromDpid = ta->Players[localSlot].DirectPlayID;
		const unsigned toDpid = ta->Players[targetSlot].DirectPlayID;
		if (fromDpid == 0 || toDpid == 0 || fromDpid == toDpid)
			return;

		const size_t chunkCountSize = records.empty()
			? 1
			: (records.size() + kRecordsPerPacket - 1) / kRecordsPerPacket;
		const unsigned char chunkCount = static_cast<unsigned char>(chunkCountSize);

		for (unsigned char chunkIndex = 0; chunkIndex < chunkCount; ++chunkIndex)
		{
			QueueSnapshotPacket packet;
			std::memset(&packet, 0, sizeof(packet));
			packet.chatByte = 0x05;
			packet.nullText = 0x00;
			packet.msgId = ChatHijackId::AlliedBuildQueue;
			packet.size = sizeof(packet);
			packet.version = kProtocolVersion;
			packet.sequence = sequence;
			packet.chunkIndex = chunkIndex;
			packet.chunkCount = chunkCount;

			const size_t firstRecord = static_cast<size_t>(chunkIndex) * kRecordsPerPacket;
			const size_t remaining = firstRecord < records.size() ? records.size() - firstRecord : 0;
			packet.recordCount = static_cast<unsigned char>(
				remaining < kRecordsPerPacket ? remaining : kRecordsPerPacket);

			for (unsigned char i = 0; i < packet.recordCount; ++i)
			{
				const AlliedBuildQueueRecord& source = records[firstRecord + i];
				QueueRecordWire& target = packet.records[i];
				target.buildUnitId = source.buildUnitId;
				target.x = source.x;
				target.y = source.y;
				target.z = source.z;
				target.rotation = source.rotation;
			}

			HAPI_SendBuf(fromDpid, toDpid, reinterpret_cast<const char*>(&packet), sizeof(packet));
		}
	}

	void CompletePendingSnapshot(RemoteQueue& remote, int gameTime)
	{
		for (size_t i = 0; i < remote.pending.received.size(); ++i)
		{
			if (!remote.pending.received[i])
				return;
		}

		std::vector<AlliedBuildQueueRecord> completed;
		for (size_t i = 0; i < remote.pending.chunks.size(); ++i)
		{
			completed.insert(
				completed.end(),
				remote.pending.chunks[i].begin(),
				remote.pending.chunks[i].end());
		}

		remote.records.swap(completed);
		remote.completedSequence = remote.pending.sequence;
		remote.hasCompletedSequence = true;
		remote.lastUpdateTick = gameTime;
		remote.pending = PendingSnapshot();

	}

	void HandleQueueSnapshot(unsigned fromDpid, const void* buffer)
	{
		const QueueSnapshotPacket* packet = static_cast<const QueueSnapshotPacket*>(buffer);
		if (!packet
			|| packet->chatByte != 0x05
			|| packet->nullText != 0x00
			|| packet->msgId != ChatHijackId::AlliedBuildQueue
			|| packet->size != sizeof(QueueSnapshotPacket)
			|| packet->version != kProtocolVersion
			|| packet->chunkCount == 0
			|| packet->chunkCount > kMaxChunks
			|| packet->chunkIndex >= packet->chunkCount
			|| packet->recordCount > kRecordsPerPacket)
		{
			return;
		}

		TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
		if (!ta)
			return;

		const int localSlot = ta->LocalHumanPlayer_PlayerID;
		PlayerStruct* sender = FindPlayerByDPID(fromDpid);
		if (!sender)
			return;

		const int senderSlot = sender->PlayerAryIndex;
		if (!IsMutualAlly(ta, localSlot, senderSlot))
			return;

		RemoteQueue& remote = g_remoteQueues[senderSlot];
		if (remote.hasCompletedSequence
			&& packet->sequence != remote.completedSequence
			&& !IsNewerSequence(packet->sequence, remote.completedSequence))
		{
			return;
		}

		if (!remote.pending.active || remote.pending.sequence != packet->sequence)
		{
			remote.pending = PendingSnapshot();
			remote.pending.active = true;
			remote.pending.sequence = packet->sequence;
			remote.pending.chunkCount = packet->chunkCount;
			remote.pending.chunks.resize(packet->chunkCount);
			remote.pending.received.assign(packet->chunkCount, false);
		}

		if (remote.pending.chunkCount != packet->chunkCount)
			return;

		std::vector<AlliedBuildQueueRecord>& chunk = remote.pending.chunks[packet->chunkIndex];
		chunk.clear();
		for (unsigned char i = 0; i < packet->recordCount; ++i)
		{
			const QueueRecordWire& source = packet->records[i];
			if (source.buildUnitId == 0 || source.buildUnitId >= ta->UNITINFOCount)
				continue;

			AlliedBuildQueueRecord record = {
				source.buildUnitId,
				source.x,
				source.y,
				source.z,
				static_cast<unsigned char>(source.rotation & 3)
			};
			chunk.push_back(record);
		}
		remote.pending.received[packet->chunkIndex] = true;

		CompletePendingSnapshot(remote, ta->GameTime);
	}

	void OnGameTick(int gameTime)
	{
		if (!g_installed)
			return;

		TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
		if (!ta)
			return;

		const int localSlot = ta->LocalHumanPlayer_PlayerID;
		if (localSlot < 0 || localSlot >= 10)
			return;

		PlayerStruct& local = ta->Players[localSlot];
		const unsigned localDpid = local.DirectPlayID;
		if (localDpid != g_sessionDpid || gameTime < g_lastTick)
			ResetSession(localDpid, gameTime);
		g_lastTick = gameTime;

		for (int i = 0; i < 10; ++i)
		{
			RemoteQueue& remote = g_remoteQueues[i];
			if (!IsMutualAlly(ta, localSlot, i)
				|| (remote.lastUpdateTick >= 0 && gameTime - remote.lastUpdateTick > kStaleTicks))
			{
				remote = RemoteQueue();
			}
		}

		if (!local.PlayerActive || localDpid == 0 || !local.PlayerInfo
			|| (local.PlayerInfo->PropertyMask & WATCH) != 0)
		{
			return;
		}

		const std::vector<AlliedBuildQueueRecord> current = CollectLocalQueue(ta, localSlot);
		const bool changed = !SameQueue(current, g_lastLocalQueue);
		const bool refreshDue = g_lastSendTick < 0 || gameTime - g_lastSendTick >= kRefreshTicks;
		if (!changed && !refreshDue)
			return;

		g_lastLocalQueue = current;
		++g_nextSequence;
		if (g_nextSequence == 0)
			++g_nextSequence;

		for (int i = 0; i < 10; ++i)
		{
			PlayerStruct& target = ta->Players[i];
			if (!target.PlayerActive || target.DirectPlayID == 0 || !target.PlayerInfo
				|| (target.PlayerInfo->PropertyMask & WATCH) != 0
				|| !IsMutualAlly(ta, localSlot, i))
			{
				continue;
			}

			SendSnapshot(ta, localSlot, i, current, g_nextSequence);
		}

		g_lastSendTick = gameTime;
	}
}

namespace AlliedBuildQueueSync
{
	void Install()
	{
		if (g_installed)
			return;

		g_installed = true;
		PacketChatRouter::GetInstance()->RegisterHandler(
			ChatHijackId::AlliedBuildQueue,
			HandleQueueSnapshot);
		GameTickHook::GetInstance()->addCallback(OnGameTick);
	}

	void Shutdown()
	{
		g_installed = false;
		ResetSession(0, -1);
	}

	const std::vector<AlliedBuildQueueRecord>& GetPlayerQueue(int playerSlot)
	{
		static const std::vector<AlliedBuildQueueRecord> empty;
		if (playerSlot < 0 || playerSlot >= 10)
			return empty;
		return g_remoteQueues[playerSlot].records;
	}
}
