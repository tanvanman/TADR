#include "RepairHealthSync.h"

#include "ChatHijackIds.h"
#include "GameTickHook.h"
#include "PacketChatRouter.h"
#include "iddrawsurface.h"
#include "tamem.h"
#include "tafunctions.h"
#include "hook/hook.h"

#include <cmath>
#include <cstring>
#include <memory>
#include <vector>

namespace
{
	const unsigned int kApplyDamagePacketAddress = 0x00489CE0;
	const unsigned int kHealthBarPositiveBranchAddress = 0x0046A443;
	const unsigned int kHealthBarPositiveAddress = 0x0046A449;
	const unsigned int kHealthBarValueLoadedAddress = 0x0046A4A8;
	const unsigned int kUnitRenderEntryAddress = 0x004586B8;
	const unsigned int kUnitRenderExitAddress1 = 0x0045876A;
	const unsigned int kUnitRenderExitAddress2 = 0x00458790;
	const unsigned int kUnitRenderExitAddress3 = 0x0045879F;
	const unsigned int kRefreshUnitModelAddress = 0x0043DD20;
	const unsigned char kProtocolVersion = 10;
	const unsigned char kHealDamageType = 0x0A;
	const unsigned char kDamagePacketOpcode = 0x0B;
	const int kSendIntervalTicks = 6;
	const int kProgressActiveTicks = 30;
	const int kCorrectionStaleTicks = 60;
	const size_t kRecordsPerPacket = 6;
	typedef void (__thiscall *RefreshUnitModelFn)(void*, UnitStruct*);

#pragma pack(push, 1)
	struct HealthRecord
	{
		unsigned short unitIndex;
		unsigned short unitType;
		unsigned short health;
		float nanoframe;
	};

	struct HealthSyncPacket
	{
		unsigned char chatByte;
		unsigned char nullText;
		unsigned char msgId;
		unsigned char version;
		unsigned char count;
		HealthRecord records[kRecordsPerPacket];
	};
#pragma pack(pop)

	static_assert(sizeof(HealthSyncPacket) == 65,
		"HealthSyncPacket must fit one TA chat packet");
	static_assert(sizeof(UnitStruct) == 0x118, "UnitStruct stride must match TA");
	static_assert(offsetof(UnitStruct, UnitType) == 0x92, "UnitStruct.UnitType");
	static_assert(offsetof(UnitStruct, UnitInGameIndex) == 0xA8,
		"UnitStruct.UnitInGameIndex");
	static_assert(offsetof(UnitStruct, cOwnerID) == 0xFF, "UnitStruct.cOwnerID");
	static_assert(offsetof(UnitStruct, Nanoframe) == 0x104, "UnitStruct.Nanoframe");
	static_assert(offsetof(UnitStruct, Health) == 0x108, "UnitStruct.Health");

	struct RepairTarget
	{
		unsigned short unitType = 0;
		unsigned short observedHealth = 0;
		unsigned short health = 0;
		float observedNanoframe = 0.0f;
		float nanoframe = 0.0f;
		int lastProgressTick = -1;
		bool initialized = false;
	};

	struct HealthCorrection
	{
		unsigned short unitType = 0;
		unsigned short health = 0;
		float nanoframe = 0.0f;
		unsigned char ownerSlot = 0xff;
		int lastUpdateTick = -1;
	};

	struct RenderRestore
	{
		UnitStruct* unit = nullptr;
		float nanoframe = 0.0f;
	};

	bool g_installed = false;
	std::vector<std::unique_ptr<InlineSingleHook> > g_hooks;
	std::vector<RepairTarget> g_repairTargets;
	std::vector<HealthCorrection> g_corrections;
	std::vector<RenderRestore> g_renderRestoreStack;
	UnitStruct* g_cachedUnitBase = nullptr;
	unsigned int g_sessionDpid = 0;
	int g_lastTick = -1;
	int g_lastSendTick = -1;
	bool g_loggedFirstSend = false;
	bool g_loggedFirstReceive = false;
	bool g_loggedFirstDraw = false;
	bool g_loggedFirstConstructionSend = false;
	bool g_loggedFirstConstructionReceive = false;
	bool g_loggedFirstCompletedUnitSend = false;
	bool g_loggedFirstCompletedUnitReceive = false;
	bool g_loggedFirstNanoframeDraw = false;
	bool g_loggedFirstLocalConstruction = false;
	bool g_loggedFirstRemoteRecipient = false;
	bool g_loggedFirstVisibleProgress = false;
	bool g_loggedFirstHiddenProgress = false;

	bool EnsureTables(TAdynmemStruct* ta)
	{
		if (!ta || !ta->BeginUnitsArray_p || !ta->EndOfUnitsArray_p
			|| ta->EndOfUnitsArray_p <= ta->BeginUnitsArray_p)
		{
			return false;
		}

		if (g_cachedUnitBase == ta->BeginUnitsArray_p && !g_repairTargets.empty())
			return true;

		const ptrdiff_t bytes = reinterpret_cast<const char*>(ta->EndOfUnitsArray_p)
			- reinterpret_cast<const char*>(ta->BeginUnitsArray_p);
		const ptrdiff_t count = bytes / static_cast<ptrdiff_t>(sizeof(UnitStruct));
		if (count <= 0 || count > 65536)
			return false;

		g_repairTargets.assign(static_cast<size_t>(count), RepairTarget());
		g_corrections.assign(static_cast<size_t>(count), HealthCorrection());
		g_cachedUnitBase = ta->BeginUnitsArray_p;
		return true;
	}

	bool IsAliveUnitSlot(const UnitStruct* unit)
	{
		return unit && unit->UnitType
			&& (unit->UnitSelected & 0x10000000u) != 0;
	}

	bool IsAllocatedUnitSlot(const UnitStruct* unit, size_t unitIndex)
	{
		return unit && unit->UnitType
			&& unit->UnitInGameIndex == static_cast<short>(unitIndex);
	}

	bool HasPositiveHealth(const UnitStruct* unit)
	{
		return IsAliveUnitSlot(unit) && unit->Health > 0;
	}

	UnitStruct* UnitByIndex(TAdynmemStruct* ta, unsigned short unitIndex)
	{
		if (!EnsureTables(ta) || unitIndex == 0
			|| static_cast<size_t>(unitIndex) >= g_repairTargets.size())
		{
			return nullptr;
		}
		return &ta->BeginUnitsArray_p[unitIndex];
	}

	bool PositionInPlayerLos(const UnitStruct* unit, const PlayerStruct& player)
	{
		return unit && CheckUnitInPlayerLOS(
			const_cast<PlayerStruct*>(&player), const_cast<UnitStruct*>(unit)) != 0;
	}

	void ResetSession(unsigned int dpid, int gameTime)
	{
		g_repairTargets.clear();
		g_corrections.clear();
		g_cachedUnitBase = nullptr;
		g_sessionDpid = dpid;
		g_lastTick = gameTime;
		g_lastSendTick = -1;
		g_loggedFirstSend = false;
		g_loggedFirstReceive = false;
		g_loggedFirstDraw = false;
		g_loggedFirstConstructionSend = false;
		g_loggedFirstConstructionReceive = false;
		g_loggedFirstCompletedUnitSend = false;
		g_loggedFirstCompletedUnitReceive = false;
		g_loggedFirstNanoframeDraw = false;
		g_loggedFirstLocalConstruction = false;
		g_loggedFirstRemoteRecipient = false;
		g_loggedFirstVisibleProgress = false;
		g_loggedFirstHiddenProgress = false;
	}

	void ClearCorrection(unsigned short unitIndex)
	{
		if (static_cast<size_t>(unitIndex) < g_corrections.size())
			g_corrections[unitIndex] = HealthCorrection();
	}

	int __stdcall ApplyDamagePacketHook(PInlineX86StackBuffer stack)
	{
		const unsigned char* packet =
			*reinterpret_cast<const unsigned char**>(stack->Esp + 4);
		if (!g_installed || !packet || packet[0] != kDamagePacketOpcode)
			return 0;

		TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
		if (!ta || !EnsureTables(ta))
			return 0;

		unsigned short unitIndex = 0;
		std::memcpy(&unitIndex, packet + 1, sizeof(unitIndex));
		UnitStruct* target = UnitByIndex(ta, unitIndex);
		if (!target)
			return 0;

		const unsigned char damageType = packet[8];
		if (damageType != kHealDamageType)
			ClearCorrection(unitIndex);
		return 0;
	}

	void ObserveLocalProgress(TAdynmemStruct* ta, int localSlot, int gameTime)
	{
		for (size_t i = 1; i < g_repairTargets.size(); ++i)
		{
			UnitStruct* unit = &ta->BeginUnitsArray_p[i];
			RepairTarget& progress = g_repairTargets[i];
			if (!HasPositiveHealth(unit)
				|| unit->cOwnerID != localSlot || !unit->UnitType)
			{
				progress = RepairTarget();
				continue;
			}

			const unsigned short unitType = static_cast<unsigned short>(unit->UnitID);
			const unsigned short health = static_cast<unsigned short>(unit->Health);
			const float nanoframe = unit->Nanoframe;
			if (!progress.initialized || progress.unitType != unitType)
			{
				progress = RepairTarget();
				progress.initialized = true;
				progress.unitType = unitType;
				progress.observedHealth = health;
				progress.health = health;
				progress.observedNanoframe = nanoframe;
				progress.nanoframe = nanoframe;
				if (nanoframe != 0.0f)
				{
					progress.lastProgressTick = gameTime;
					if (!g_loggedFirstLocalConstruction)
					{
						IDDrawSurface::OutptFmtTxt(
							"[RepairHealthSync] observed first local construction unit=%u health=%u nano=%.5f",
							static_cast<unsigned>(i), health, nanoframe);
						g_loggedFirstLocalConstruction = true;
					}
				}
				continue;
			}

			if (health > progress.observedHealth
				|| (nanoframe != progress.observedNanoframe
					&& (nanoframe != 0.0f || progress.observedNanoframe != 0.0f)))
			{
				progress.lastProgressTick = gameTime;
			}
			progress.observedHealth = health;
			progress.health = health;
			progress.observedNanoframe = nanoframe;
			progress.nanoframe = nanoframe;
		}
	}

	void SendRecords(TAdynmemStruct* ta, int localSlot, int targetSlot,
		const std::vector<HealthRecord>& records)
	{
		const unsigned int fromDpid = ta->Players[localSlot].DirectPlayID;
		const unsigned int toDpid = ta->Players[targetSlot].DirectPlayID;
		if (fromDpid == 0 || toDpid == 0 || fromDpid == toDpid)
			return;

		for (size_t first = 0; first < records.size(); first += kRecordsPerPacket)
		{
			HealthSyncPacket packet;
			std::memset(&packet, 0, sizeof(packet));
			packet.chatByte = 0x05;
			packet.nullText = 0x00;
			packet.msgId = ChatHijackId::RepairHealthSync;
			packet.version = kProtocolVersion;
			const size_t remaining = records.size() - first;
			packet.count = static_cast<unsigned char>(
				remaining < kRecordsPerPacket ? remaining : kRecordsPerPacket);
			for (size_t i = 0; i < packet.count; ++i)
				packet.records[i] = records[first + i];

			HAPI_SendBuf(fromDpid, toDpid,
				reinterpret_cast<const char*>(&packet), sizeof(packet));
			if (!g_loggedFirstSend)
			{
				const HealthRecord& firstRecord = packet.records[0];
				IDDrawSurface::OutptFmtTxt(
					"[RepairHealthSync] sent first progress unit=%u health=%u nano=%.5f (%u unit%s)",
					firstRecord.unitIndex, firstRecord.health, firstRecord.nanoframe,
					packet.count, packet.count == 1 ? "" : "s");
				g_loggedFirstSend = true;
			}
			if (!g_loggedFirstConstructionSend)
			{
				for (size_t i = 0; i < packet.count; ++i)
				{
					const HealthRecord& record = packet.records[i];
					if (record.nanoframe == 0.0f)
						continue;
					IDDrawSurface::OutptFmtTxt(
						"[RepairHealthSync] sent first construction unit=%u health=%u nano=%.5f",
						record.unitIndex, record.health, record.nanoframe);
					g_loggedFirstConstructionSend = true;
					break;
				}
			}
			if (!g_loggedFirstCompletedUnitSend)
			{
				for (size_t i = 0; i < packet.count; ++i)
				{
					const HealthRecord& record = packet.records[i];
					if (record.nanoframe != 0.0f)
						continue;
					IDDrawSurface::OutptFmtTxt(
						"[RepairHealthSync] sent first completed-unit progress unit=%u health=%u",
						record.unitIndex, record.health);
					g_loggedFirstCompletedUnitSend = true;
					break;
				}
			}
		}
	}

	void HandleHealthSync(unsigned int fromDpid, const void* buffer)
	{
		const HealthSyncPacket* packet = static_cast<const HealthSyncPacket*>(buffer);
		if (!g_installed || !packet
			|| packet->chatByte != 0x05 || packet->nullText != 0x00
			|| packet->msgId != ChatHijackId::RepairHealthSync
			|| packet->version != kProtocolVersion
			|| packet->count == 0 || packet->count > kRecordsPerPacket)
		{
			return;
		}

		TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
		PlayerStruct* sender = FindPlayerByDPID(fromDpid);
		if (!ta || !sender || !sender->PlayerInfo
			|| GetInferredPlayerType(sender) != Player_RemoteHuman
			|| !EnsureTables(ta))
		{
			return;
		}

		const int senderSlot = sender->PlayerAryIndex;
		const int localSlot = ta->LocalHumanPlayer_PlayerID;
		if (senderSlot < 0 || senderSlot >= 10 || localSlot < 0 || localSlot >= 10)
			return;

		PlayerStruct& local = ta->Players[localSlot];
		unsigned accepted = 0;
		for (size_t i = 0; i < packet->count; ++i)
		{
			const HealthRecord& record = packet->records[i];
			UnitStruct* unit = UnitByIndex(ta, record.unitIndex);
			const bool constructionRecord = record.nanoframe > 0.0f;
			const bool allocatedConstruction = IsAllocatedUnitSlot(unit, record.unitIndex)
				&& unit->Nanoframe > 0.0f;
			if ((!IsAliveUnitSlot(unit) && !allocatedConstruction)
				|| unit->cOwnerID != senderSlot
				|| static_cast<unsigned short>(unit->UnitID) != record.unitType
				|| !unit->UnitType || (record.health == 0 && !constructionRecord)
				|| record.health > unit->UnitType->nMaxHP
				|| !std::isfinite(record.nanoframe)
				|| !PositionInPlayerLos(unit, local))
			{
				continue;
			}

			const int stockHealth = unit->Health;
			const float stockNanoframe = unit->Nanoframe;
			const bool applyHealth = record.health > unit->Health;
			const bool applyNanoframe = unit->Nanoframe > 0.0f
				&& record.nanoframe >= 0.0f
				&& record.nanoframe < unit->Nanoframe;
			bool refreshedModel = false;

			HealthCorrection& correction = g_corrections[record.unitIndex];
			if (!g_loggedFirstCompletedUnitReceive
				&& record.nanoframe == 0.0f && record.health > unit->Health)
			{
				IDDrawSurface::OutptFmtTxt(
					"[RepairHealthSync] received first completed-unit correction unit=%u health=%d/%u",
					record.unitIndex, unit->Health, record.health);
				g_loggedFirstCompletedUnitReceive = true;
			}
			correction.unitType = record.unitType;
			correction.health = record.health;
			correction.nanoframe = record.nanoframe;
			correction.ownerSlot = static_cast<unsigned char>(senderSlot);
			correction.lastUpdateTick = ta->GameTime;
			if (applyHealth)
				unit->Health = static_cast<short>(record.health);
			if (applyNanoframe)
			{
				unit->Nanoframe = record.nanoframe;
				unit->UnitSelected |= 0x2000u;
				if (unit->IsUnit)
				{
					RefreshUnitModelFn refreshUnitModel =
						reinterpret_cast<RefreshUnitModelFn>(kRefreshUnitModelAddress);
					refreshUnitModel(reinterpret_cast<void*>(unit->IsUnit), unit);
					refreshedModel = true;
				}
			}
			++accepted;
			if (!g_loggedFirstConstructionReceive && record.nanoframe != 0.0f)
			{
				IDDrawSurface::OutptFmtTxt(
					"[RepairHealthSync] applied first construction unit=%u health=%d/%u nano=%.5f/%.5f dirty=%u refresh=%u",
					record.unitIndex, stockHealth, record.health,
					stockNanoframe, record.nanoframe, applyNanoframe ? 1u : 0u,
					refreshedModel ? 1u : 0u);
				g_loggedFirstConstructionReceive = true;
			}
		}

		if (accepted != 0 && !g_loggedFirstReceive)
		{
			const HealthRecord& firstRecord = packet->records[0];
			UnitStruct* firstUnit = UnitByIndex(ta, firstRecord.unitIndex);
			IDDrawSurface::OutptFmtTxt(
				"[RepairHealthSync] received first progress unit=%u health=%d/%u nano=%.5f/%.5f (%u unit%s)",
				firstRecord.unitIndex, firstUnit ? firstUnit->Health : -1,
				firstRecord.health, firstUnit ? firstUnit->Nanoframe : 0.0f,
				firstRecord.nanoframe, accepted, accepted == 1 ? "" : "s");
			g_loggedFirstReceive = true;
		}
	}

	HealthCorrection* ActiveCorrection(TAdynmemStruct* ta, UnitStruct* unit)
	{
		if (!ta || !unit || !EnsureTables(ta))
			return nullptr;

		const int unitIndex = unit->UnitInGameIndex;
		if (unitIndex <= 0 || static_cast<size_t>(unitIndex) >= g_corrections.size())
			return nullptr;

		HealthCorrection& correction = g_corrections[unitIndex];
		if (correction.lastUpdateTick < 0
			|| ta->GameTime - correction.lastUpdateTick > kCorrectionStaleTicks
			|| !IsAliveUnitSlot(unit)
			|| correction.ownerSlot != unit->cOwnerID
			|| correction.unitType != static_cast<unsigned short>(unit->UnitID))
		{
			correction = HealthCorrection();
			return nullptr;
		}
		return &correction;
	}

	int __stdcall UnitRenderEntryHook(PInlineX86StackBuffer stack)
	{
		RenderRestore restore;
		g_renderRestoreStack.push_back(restore);
		if (!g_installed)
			return 0;

		TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
		unsigned char* renderObject = reinterpret_cast<unsigned char*>(stack->Edi);
		if (!ta || !renderObject)
			return 0;

		UnitStruct* unit = *reinterpret_cast<UnitStruct**>(renderObject + 0x0C);
		HealthCorrection* correction = ActiveCorrection(ta, unit);
		if (!correction || !std::isfinite(correction->nanoframe)
			|| correction->nanoframe < 0.0f || correction->nanoframe > 1.0f
			|| unit->Nanoframe <= 0.0f
			|| correction->nanoframe >= unit->Nanoframe)
		{
			return 0;
		}

		RenderRestore& active = g_renderRestoreStack.back();
		active.unit = unit;
		active.nanoframe = unit->Nanoframe;
		unit->Nanoframe = correction->nanoframe;
		if (!g_loggedFirstNanoframeDraw)
		{
			IDDrawSurface::OutptFmtTxt(
				"[RepairHealthSync] first nanoframe draw correction unit=%d stock=%.5f synced=%.5f",
				unit->UnitInGameIndex, active.nanoframe, correction->nanoframe);
			g_loggedFirstNanoframeDraw = true;
		}
		return 0;
	}

	int __stdcall UnitRenderExitHook(PInlineX86StackBuffer)
	{
		if (g_renderRestoreStack.empty())
			return 0;

		const RenderRestore restore = g_renderRestoreStack.back();
		g_renderRestoreStack.pop_back();
		if (restore.unit)
			restore.unit->Nanoframe = restore.nanoframe;
		return 0;
	}

	int __stdcall HealthBarPositiveBranchHook(PInlineX86StackBuffer stack)
	{
		if (!g_installed)
			return 0;

		TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
		UnitStruct* unit = reinterpret_cast<UnitStruct*>(stack->Esi);
		HealthCorrection* correction = ActiveCorrection(ta, unit);
		if (!correction || unit->Health > 0 || correction->health == 0)
			return 0;

		stack->rtnAddr_Pvoid = reinterpret_cast<void*>(kHealthBarPositiveAddress);
		return X86STRACKBUFFERCHANGE;
	}

	int __stdcall HealthBarValueLoadedHook(PInlineX86StackBuffer stack)
	{
		if (!g_installed)
			return 0;

		TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
		UnitStruct* unit = reinterpret_cast<UnitStruct*>(stack->Esi);
		HealthCorrection* correction = ActiveCorrection(ta, unit);
		if (!correction)
			return 0;

		const int stockHealth = static_cast<int>(stack->Ecx);
		if (correction->health <= stockHealth)
			return 0;

		stack->Ecx = correction->health;
		if (!g_loggedFirstDraw)
		{
			const int unitIndex = unit->UnitInGameIndex;
			IDDrawSurface::OutptFmtTxt(
				"[RepairHealthSync] first bar correction unit=%d stock=%d synced=%u",
				unitIndex, stockHealth, correction->health);
			g_loggedFirstDraw = true;
		}
		return X86STRACKBUFFERCHANGE;
	}

	void ExpireCorrections(TAdynmemStruct* ta, int localSlot, int gameTime)
	{
		PlayerStruct& local = ta->Players[localSlot];
		for (size_t i = 1; i < g_corrections.size(); ++i)
		{
			HealthCorrection& correction = g_corrections[i];
			if (correction.lastUpdateTick < 0)
				continue;

			UnitStruct* unit = &ta->BeginUnitsArray_p[i];
			if (gameTime - correction.lastUpdateTick > kCorrectionStaleTicks
				|| !IsAliveUnitSlot(unit)
				|| correction.ownerSlot != unit->cOwnerID
				|| correction.unitType != static_cast<unsigned short>(unit->UnitID)
				|| !PositionInPlayerLos(unit, local))
			{
				correction = HealthCorrection();
			}
		}
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
		const unsigned int localDpid = local.DirectPlayID;
		if (localDpid != g_sessionDpid || gameTime < g_lastTick)
			ResetSession(localDpid, gameTime);
		g_lastTick = gameTime;

		if (!EnsureTables(ta))
			return;
		ObserveLocalProgress(ta, localSlot, gameTime);
		ExpireCorrections(ta, localSlot, gameTime);

		if (!local.PlayerActive || localDpid == 0 || !local.PlayerInfo
			|| (local.PlayerInfo->PropertyMask & WATCH) != 0
			|| (g_lastSendTick >= 0 && gameTime - g_lastSendTick < kSendIntervalTicks))
		{
			return;
		}

		for (int targetSlot = 0; targetSlot < 10; ++targetSlot)
		{
			PlayerStruct& recipient = ta->Players[targetSlot];
			if (!recipient.PlayerActive || recipient.DirectPlayID == 0
				|| !recipient.PlayerInfo
				|| (recipient.PlayerInfo->PropertyMask & WATCH) != 0
				|| GetInferredPlayerType(&recipient) != Player_RemoteHuman)
			{
				continue;
			}
			if (!g_loggedFirstRemoteRecipient)
			{
				IDDrawSurface::OutptFmtTxt(
					"[RepairHealthSync] recognized remote recipient slot=%d dpid=%u",
					targetSlot, recipient.DirectPlayID);
				g_loggedFirstRemoteRecipient = true;
			}

			std::vector<HealthRecord> visible;
			for (size_t i = 1; i < g_repairTargets.size(); ++i)
			{
				RepairTarget& repaired = g_repairTargets[i];
				if (repaired.lastProgressTick < 0
					|| gameTime - repaired.lastProgressTick > kProgressActiveTicks)
				{
					continue;
				}

				UnitStruct* unit = &ta->BeginUnitsArray_p[i];
				if (!HasPositiveHealth(unit) || unit->cOwnerID != localSlot
					|| repaired.unitType != static_cast<unsigned short>(unit->UnitID))
				{
					continue;
				}

				if (!PositionInPlayerLos(unit, recipient))
				{
					if (!g_loggedFirstHiddenProgress)
					{
						IDDrawSurface::OutptFmtTxt(
							"[RepairHealthSync] native LOS rejected first progress unit=%u recipient=%d nano=%.5f",
							static_cast<unsigned>(i), targetSlot, repaired.nanoframe);
						g_loggedFirstHiddenProgress = true;
					}
					continue;
				}
				if (!g_loggedFirstVisibleProgress)
				{
					IDDrawSurface::OutptFmtTxt(
						"[RepairHealthSync] native LOS accepted first progress unit=%u recipient=%d nano=%.5f",
						static_cast<unsigned>(i), targetSlot, repaired.nanoframe);
					g_loggedFirstVisibleProgress = true;
				}

				HealthRecord record;
				record.unitIndex = static_cast<unsigned short>(i);
				record.unitType = repaired.unitType;
				record.health = repaired.health;
				record.nanoframe = repaired.nanoframe;
				visible.push_back(record);
			}

			if (!visible.empty())
				SendRecords(ta, localSlot, targetSlot, visible);
		}
		g_lastSendTick = gameTime;
	}
}

namespace RepairHealthSync
{
	void Install()
	{
		if (g_installed)
			return;

		g_installed = true;
		PacketChatRouter::GetInstance()->RegisterHandler(
			ChatHijackId::RepairHealthSync, HandleHealthSync, true);
		GameTickHook::GetInstance()->addCallback(OnGameTick);
		IDDrawSurface::OutptTxt("[RepairHealthSync] installed (static construction safe protocol 10)");
	}

	void Shutdown()
	{
		for (std::vector<RenderRestore>::reverse_iterator it = g_renderRestoreStack.rbegin();
			it != g_renderRestoreStack.rend(); ++it)
		{
			if (it->unit)
				it->unit->Nanoframe = it->nanoframe;
		}
		g_renderRestoreStack.clear();
		g_installed = false;
		g_hooks.clear();
		ResetSession(0, -1);
	}
}
