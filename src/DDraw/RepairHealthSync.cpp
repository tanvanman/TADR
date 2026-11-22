#include "RepairHealthSync.h"
#include "config.h"

#if REPAIR_HEALTH_SYNC_ENABLE

#include "RepairHealthSyncState.h"

#include "ChatHijackIds.h"
#include "GameTickHook.h"
#include "PacketChatRouter.h"
#include "iddrawsurface.h"
#include "tamem.h"
#include "tafunctions.h"
#include "hook/hook.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <cstring>
#include <memory>
#include <vector>

namespace
{
	using namespace RepairHealthSyncState;

	// Engine hook sites (all verified byte-for-byte before installing).
	const unsigned int kApplyDamagePacketAddress = 0x00489CE0;
	const unsigned int kApplyDamageNetworkReturnAddress = 0x00455417;
	const unsigned int kApplyDamageLocalReturnAddress = 0x00489C8E;
	const unsigned int kBuildFinishedAddress = 0x0041B8D0;
	const unsigned int kStockSyncSendAddress = 0x0048B200;
	const unsigned int kStockSyncRecvAddress = 0x0048B3F0;
	const unsigned int kSelectedUnitHealthValueAddress = 0x0046B069;
	const unsigned int kBottomStateCompareAddress = 0x0046ACD7;
	const unsigned int kBottomStateCompareExitAddress = 0x0046ACDD;
	const unsigned int kCobEmitSfxAddress = 0x00480EB0;
	const unsigned int kNanoframeRenderEntryAddress = 0x00458DF1;
	const unsigned int kNanoframeRenderExitAddress1 = 0x00458E08;
	const unsigned int kNanoframeRenderExitAddress2 = 0x00458F85;

	const DWORD kZeroFlag = 0x40;
	const unsigned int kCobWhiteSmokeSfx = 0x101;
	const unsigned int kCobBlackSmokeSfx = 0x102;
	const unsigned int kDamageSmokeThresholdPercent = 66;

	// Protocol version 13: unclamped commutative owner-local deltas + 0x10
	// completion + bounce-free idle ownerHealth reconciliation.
	const unsigned char kProtocolVersion = 13;
	const unsigned char kParalyzeDamageType = 0x02;
	const unsigned char kHealDamageType = 0x0A;
	const unsigned char kDamagePacketOpcode = 0x0B;
	const unsigned char kRecordCompleted = 0x01;
	const size_t kRecordsPerPacket = 5;
	const size_t kRenderStackCapacity = 32;
	const int kMaxTrackedClampLoss = SHRT_MAX * 4;

#pragma pack(push, 1)
	struct HealthDeltaRecordWire
	{
		unsigned short unitIndex;
		unsigned short unitType;
		short healthDelta;
		unsigned short ownerHealth;
		unsigned short nanoframe;
		unsigned char flags;
	};

	struct HealthSyncPacket
	{
		unsigned char chatByte;
		unsigned char nullText;
		unsigned char msgId;
		unsigned char size;
		unsigned char version;
		unsigned char count;
		unsigned int ownerGameTime;
		HealthDeltaRecordWire records[kRecordsPerPacket];
	};
#pragma pack(pop)

	static_assert(sizeof(HealthDeltaRecordWire) == 11,
		"HealthDeltaRecordWire must pack cleanly into 65-byte chat envelope");
	static_assert(sizeof(HealthSyncPacket) == 65,
		"HealthSyncPacket must fit one TA chat packet");
	static_assert(sizeof(UnitStruct) == 0x118, "UnitStruct stride must match TA");
	static_assert(offsetof(UnitStruct, UnitType) == 0x92, "UnitStruct.UnitType");
	static_assert(offsetof(UnitStruct, UnitInGameIndex) == 0xA8,
		"UnitStruct.UnitInGameIndex");
	static_assert(offsetof(UnitStruct, cOwnerID) == 0xFF, "UnitStruct.cOwnerID");
	static_assert(offsetof(UnitStruct, Nanoframe) == 0x104, "UnitStruct.Nanoframe");
	static_assert(offsetof(UnitStruct, Health) == 0x108, "UnitStruct.Health");
	static_assert(offsetof(Object3doStruct, ThisUnit) == 0x0C,
		"Object3doStruct.ThisUnit");
	static_assert(offsetof(UnitDefStruct, CANBUILD_ptr) == 0x156,
		"UnitDefStruct.CANBUILD_ptr");
	static_assert(offsetof(PlayerStruct, field_18) == 0x18,
		"PlayerStruct +0x18 holds last 0x2C ownerGameTime");

	struct RepairTarget
	{
		Object3doStruct* identity = nullptr;
		unsigned short unitType = 0;
		int lastHealth = 0;
		float lastNanoframe = 0.0f;
		short nonBuildLocalDeltaDuringConstruction = 0;
		std::array<short, 10> pendingDelta = {};
		std::array<unsigned char, 10> lastSentNanoframeStep = {};
		unsigned short pendingMask = 0;
		unsigned int lastLocalActivityMs = 0;
		bool initialized = false;
		bool completed = false;
	};

	struct HealthCorrection
	{
		Object3doStruct* identity = nullptr;
		unsigned short unitType = 0;
		int rawDisplayHealth = 0;
		int unclampedClampLoss = 0;
		float displayNanoframe = 0.0f;
		unsigned int last2cOwnerTick = 0;
		unsigned int lastDamageMs = 0;
		int lastDamageTick = -1;
		int lastUpdateTick = -1;
		unsigned char ownerSlot = 0xff;
		bool hasHealthCorrection = false;
		bool hasNanoframeCorrection = false;
		bool completed = false;
		bool hasLast2cOwnerTick = false;
		bool hasLastDamage = false;
	};

	struct RenderRestore
	{
		UnitStruct* unit = nullptr;
		float nanoframe = 0.0f;
	};

	struct HookSite
	{
		unsigned int address;
		unsigned int length;
		unsigned char expected[10];
		InlineX86HookRouter callback;
	};

	bool g_installed = false;
	std::vector<std::unique_ptr<InlineSingleHook> > g_hooks;
	std::vector<RepairTarget> g_repairTargets;
	std::vector<HealthCorrection> g_corrections;
	std::vector<unsigned short> g_candidates;
	std::array<unsigned short, 10> g_sendCursors = {};
	std::array<Cadence, 10> g_recipientCadence;
	std::array<unsigned int, 10> g_recipientDpids = {};
	Cadence g_sampleCadence;
	Cadence g_packetCadence;
	int g_nextRecipient = 0;
	std::array<RenderRestore, kRenderStackCapacity> g_renderRestoreStack;
	size_t g_renderDepth = 0;
	UnitStruct* g_cachedUnitBase = nullptr;
	size_t g_cachedUnitCount = 0;
	unsigned int g_sessionDpid = 0;
	int g_lastTick = -1;
	bool g_loggedFirstSend = false;
	bool g_loggedFirstReceive = false;
	bool g_loggedFirstCompletion = false;
	bool g_loggedFirstHealthDraw = false;
	bool g_loggedFirstNanoframeDraw = false;
	bool g_loggedFirstSmokeSuppression = false;
	volatile LONG g_bottomStateRefreshUnit = 0;

	bool EnsureTables(TAdynmemStruct* ta)
	{
		if (!ta || !ta->BeginUnitsArray_p || !ta->EndOfUnitsArray_p
			|| ta->EndOfUnitsArray_p <= ta->BeginUnitsArray_p)
		{
			return false;
		}

		const uintptr_t bytes = reinterpret_cast<uintptr_t>(ta->EndOfUnitsArray_p)
			- reinterpret_cast<uintptr_t>(ta->BeginUnitsArray_p);
		const size_t count = bytes / sizeof(UnitStruct);
		if (bytes % sizeof(UnitStruct) != 0 || count == 0 || count > 65536)
			return false;

		const size_t unitCount = static_cast<size_t>(count);
		if (g_cachedUnitBase == ta->BeginUnitsArray_p
			&& g_cachedUnitCount == unitCount
			&& g_repairTargets.size() == unitCount
			&& g_corrections.size() == unitCount)
		{
			return true;
		}

		g_repairTargets.assign(unitCount, RepairTarget());
		g_corrections.assign(unitCount, HealthCorrection());
		g_candidates.clear();
		g_candidates.reserve(unitCount);
		g_sendCursors.fill(1);
		g_cachedUnitBase = ta->BeginUnitsArray_p;
		g_cachedUnitCount = unitCount;
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
			&& static_cast<unsigned short>(unit->UnitInGameIndex) == unitIndex;
	}

	bool IsDisplayableUnitSlot(const UnitStruct* unit, size_t unitIndex)
	{
		return IsAllocatedUnitSlot(unit, unitIndex)
			&& (unit->UnitSelected & 0x4000u) == 0
			&& (IsAliveUnitSlot(unit) || unit->Nanoframe > 0.0f);
	}

	bool TryGetUnitIndex(const TAdynmemStruct* ta, const UnitStruct* unit, size_t& outIndex)
	{
		if (!ta || !unit || !ta->BeginUnitsArray_p || !ta->EndOfUnitsArray_p)
			return false;
		const uintptr_t addr = reinterpret_cast<uintptr_t>(unit);
		const uintptr_t base = reinterpret_cast<uintptr_t>(ta->BeginUnitsArray_p);
		const uintptr_t end = reinterpret_cast<uintptr_t>(ta->EndOfUnitsArray_p);
		if (addr < base || addr >= end || (addr - base) % sizeof(UnitStruct) != 0)
			return false;
		const size_t idx = (addr - base) / sizeof(UnitStruct);
		if (idx == 0 || idx >= g_repairTargets.size())
			return false;
		outIndex = idx;
		return true;
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
		return unit && player.LOS_MEMORY_p && CheckUnitInPlayerLOS(
			const_cast<PlayerStruct*>(&player), const_cast<UnitStruct*>(unit)) != 0;
	}

	unsigned short EncodeNanoframe(float nanoframe)
	{
		if (nanoframe <= 0.0f)
			return 0;
		if (nanoframe >= 1.0f)
			return USHRT_MAX;
		return static_cast<unsigned short>(nanoframe * USHRT_MAX + 0.5f);
	}

	float DecodeNanoframe(unsigned short nanoframe)
	{
		return static_cast<float>(nanoframe) / static_cast<float>(USHRT_MAX);
	}

	unsigned char QuantizeRatio(unsigned int value, unsigned int maximum)
	{
		if (maximum == 0)
			return 0;
		const unsigned int step = (value * 100u + maximum / 2u) / maximum;
		return static_cast<unsigned char>(step > 100u ? 100u : step);
	}

	unsigned char QuantizeNanoframe(float nanoframe)
	{
		return QuantizeRatio(EncodeNanoframe(nanoframe), USHRT_MAX);
	}

	unsigned int MaximumDisplayHealth(const UnitStruct* unit)
	{
		if (!unit || !unit->UnitType)
			return 0;
		// TotalA.exe reads UnitDefStruct + 0x1FA as a 32-bit dword (nMaxHP + data8).
		unsigned int rawMaxHp = 0;
		std::memcpy(&rawMaxHp, &unit->UnitType->nMaxHP, sizeof(rawMaxHp));
		return (std::min)(rawMaxHp, static_cast<unsigned int>(SHRT_MAX));
	}

	unsigned short ClampedDisplayHealth(const HealthCorrection& correction, const UnitStruct* unit)
	{
		return ClampHealth(correction.rawDisplayHealth, MaximumDisplayHealth(unit));
	}

	unsigned int ReadPlayerLast2cGameTime(const PlayerStruct& player)
	{
		unsigned int ownerTick = 0;
		std::memcpy(&ownerTick,
			reinterpret_cast<const unsigned char*>(&player) + 0x18,
			sizeof(ownerTick));
		return ownerTick;
	}

	int ResolveOwnerSlotForUnit(const TAdynmemStruct* ta, const UnitStruct* unit)
	{
		if (!ta || !unit)
			return -1;
		const unsigned char owner = static_cast<unsigned char>(unit->cOwnerID);
		if (owner < 10)
			return static_cast<int>(owner);
		const uintptr_t addr = reinterpret_cast<uintptr_t>(unit);
		for (int slot = 0; slot < 10; ++slot)
		{
			const PlayerStruct& player = ta->Players[slot];
			if (!player.Units || !player.UnitsAry_End)
				continue;
			const uintptr_t first = reinterpret_cast<uintptr_t>(player.Units);
			const uintptr_t last = reinterpret_cast<uintptr_t>(player.UnitsAry_End);
			if (addr >= first && addr <= last)
				return slot;
		}
		return -1;
	}

	void BindCorrectionIdentity(HealthCorrection& correction, const UnitStruct* unit,
		unsigned char ownerSlot)
	{
		const unsigned short type = static_cast<unsigned short>(unit->UnitID);
		if (correction.identity == unit->Object3do
			&& correction.unitType == type
			&& correction.ownerSlot == ownerSlot)
		{
			return;
		}
		const bool keep2cState = (correction.identity == nullptr)
			&& correction.ownerSlot == ownerSlot;
		const bool had2cTick = keep2cState && correction.hasLast2cOwnerTick;
		const unsigned int saved2cTick = correction.last2cOwnerTick;
		const bool hadDamage = keep2cState && correction.hasLastDamage;
		const unsigned int savedDamageMs = correction.lastDamageMs;
		const int savedDamageTick = correction.lastDamageTick;

		correction = HealthCorrection();
		correction.identity = unit->Object3do;
		correction.unitType = type;
		correction.ownerSlot = ownerSlot;
		if (had2cTick)
		{
			correction.hasLast2cOwnerTick = true;
			correction.last2cOwnerTick = saved2cTick;
		}
		if (hadDamage)
		{
			correction.hasLastDamage = true;
			correction.lastDamageMs = savedDamageMs;
			correction.lastDamageTick = savedDamageTick;
		}
	}

	void RestoreRenderFrames()
	{
		const size_t storedDepth = (std::min)(g_renderDepth, kRenderStackCapacity);
		for (size_t i = storedDepth; i > 0; --i)
		{
			RenderRestore& restore = g_renderRestoreStack[i - 1];
			if (restore.unit)
				restore.unit->Nanoframe = restore.nanoframe;
			restore = RenderRestore();
		}
		g_renderDepth = 0;
	}

	void ResetSession(unsigned int dpid, int gameTime)
	{
		g_repairTargets.clear();
		g_corrections.clear();
		g_candidates.clear();
		g_sendCursors.fill(1);
		g_recipientCadence.fill(Cadence());
		g_recipientDpids.fill(0);
		g_sampleCadence = Cadence();
		g_packetCadence = Cadence();
		g_nextRecipient = 0;
		g_cachedUnitBase = nullptr;
		g_cachedUnitCount = 0;
		g_sessionDpid = dpid;
		g_lastTick = gameTime;
		g_loggedFirstSend = false;
		g_loggedFirstReceive = false;
		g_loggedFirstCompletion = false;
		g_loggedFirstHealthDraw = false;
		g_loggedFirstNanoframeDraw = false;
		g_loggedFirstSmokeSuppression = false;
		InterlockedExchange(&g_bottomStateRefreshUnit, 0);
	}

	void RequestHoveredBottomStateRefresh(TAdynmemStruct* ta, unsigned short unitIndex)
	{
		if (ta && ta->MouseOverUnit == unitIndex)
			InterlockedExchange(&g_bottomStateRefreshUnit, unitIndex);
	}

	int __stdcall BottomStateRefreshHook(PInlineX86StackBuffer stack)
	{
		if (!g_installed)
			return 0;

		const LONG unitIndex = InterlockedExchange(&g_bottomStateRefreshUnit, 0);
		TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
		if (unitIndex <= 0 || !ta || ta->MouseOverUnit != unitIndex)
			return 0;

		// Skip only TA's cache comparison and enter its ordinary redraw branch.
		// That branch initializes every register consumed after the comparison.
		stack->EFlags_Dw &= ~kZeroFlag;
		stack->Edx = 0;
		stack->rtnAddr_Pvoid = reinterpret_cast<LPVOID>(kBottomStateCompareExitAddress);
		return X86STRACKBUFFERCHANGE;
	}

	void InitializeLocalTarget(RepairTarget& target, const UnitStruct* unit,
		bool initializeFromStockCreation)
	{
		target = RepairTarget();
		target.initialized = true;
		target.identity = unit->Object3do;
		target.unitType = static_cast<unsigned short>(unit->UnitID);
		target.completed = (unit->Nanoframe <= 0.0f);
		target.lastNanoframe = unit->Nanoframe;
		if (!target.completed && initializeFromStockCreation)
		{
			// Remote peers start every constructing unit at Health=0, Nanoframe=1.0f.
			target.lastHealth = 0;
			target.lastSentNanoframeStep.fill(100);
		}
		else
		{
			target.lastHealth = static_cast<int>(unit->Health);
			target.lastSentNanoframeStep.fill(QuantizeNanoframe(unit->Nanoframe));
		}
	}

	void TransitionLocalTargetToCompleted(RepairTarget& target, const UnitStruct* unit,
		unsigned int now)
	{
		const short carryDelta = target.initialized
			? target.nonBuildLocalDeltaDuringConstruction : 0;
		if (!target.initialized || target.identity != unit->Object3do
			|| target.unitType != static_cast<unsigned short>(unit->UnitID))
		{
			target = RepairTarget();
			target.initialized = true;
			target.identity = unit->Object3do;
			target.unitType = static_cast<unsigned short>(unit->UnitID);
		}
		target.completed = true;
		target.lastHealth = static_cast<int>(unit->Health);
		target.lastNanoframe = 0.0f;
		target.nonBuildLocalDeltaDuringConstruction = 0;
		target.lastSentNanoframeStep.fill(0);
		target.pendingDelta.fill(carryDelta);
		target.pendingMask = 0;
		if (carryDelta != 0)
			target.lastLocalActivityMs = now;
	}

	int __stdcall ApplyDamagePacketHook(PInlineX86StackBuffer stack)
	{
		if (!g_installed || !stack)
			return 0;

		const DWORD returnAddress = *reinterpret_cast<const DWORD*>(stack->Esp);
		const unsigned char* packet =
			*reinterpret_cast<const unsigned char**>(stack->Esp + 4);
		if (!packet || packet[0] != kDamagePacketOpcode)
			return 0;

		TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
		if (!ta || !EnsureTables(ta))
			return 0;

		unsigned short unitIndex = 0;
		std::memcpy(&unitIndex, packet + 1, sizeof(unitIndex));
		if (unitIndex == 0 || static_cast<size_t>(unitIndex) >= g_repairTargets.size())
			return 0;

		UnitStruct* unit = &ta->BeginUnitsArray_p[unitIndex];
		if (!IsDisplayableUnitSlot(unit, unitIndex) || !IsAliveUnitSlot(unit))
			return 0;

		const unsigned char damageType = packet[8];
		if (damageType == kParalyzeDamageType)
			return 0;

		unsigned short rawAmount = 0;
		std::memcpy(&rawAmount, packet + 5, sizeof(rawAmount));
		const unsigned int maxHp = MaximumDisplayHealth(unit);
		if (maxHp == 0)
			return 0;

		const int localSlot = ta->LocalHumanPlayer_PlayerID;
		if (localSlot < 0 || localSlot >= 10)
			return 0;

		const bool isHeal = (damageType == kHealDamageType);
		const int signedDamage = isHeal ? 0 : static_cast<int>(static_cast<short>(rawAmount));
		if (!isHeal && signedDamage <= 0)
			return 0;

		const int curHp = static_cast<int>(unit->Health);

		// Path 1: Owner-side tracking for our own local units.
		if (unit->cOwnerID == localSlot)
		{
			if (DataShare->PlayingDemo)
				return 0;

			RepairTarget& target = g_repairTargets[unitIndex];
			if (!target.initialized || target.identity != unit->Object3do
				|| target.unitType != static_cast<unsigned short>(unit->UnitID))
			{
				InitializeLocalTarget(target, unit, unit->Nanoframe > 0.0f);
			}

			int netChange = 0;
			if (isHeal)
			{
				const int clampedCur = curHp > 0 ? curHp : 0;
				const int newHp = static_cast<int>(ClampHealth(clampedCur + rawAmount, maxHp));
				netChange = newHp - curHp;
			}
			else
			{
				netChange = -signedDamage;
			}

			if (returnAddress == kApplyDamageNetworkReturnAddress)
			{
				// Networked 0x0B from another peer (enemy shot or ally repair):
				// every client applies this exact 0x0B natively, so shift lastHealth
				// by the same change to exclude it from ownerLocalDelta.
				target.lastHealth += netChange;
			}
			else if (returnAddress == kApplyDamageLocalReturnAddress)
			{
				// Owner-local 0x0B (self-repair, auto-heal, self-reclaim, lava, or
				// friendly fire): stock TA never networks this for local units.
				// Leaving lastHealth untouched causes CollectProgress to measure
				// the exact delta; during Phase 0 also record non-build deltas so
				// they survive the 0x10 completion transition.
				target.lastLocalActivityMs = GetTickCount();
				if (!target.completed && unit->Nanoframe > 0.0f)
				{
					int clampedLocal = netChange;
					if (!isHeal && curHp > 0 && signedDamage > curHp)
						clampedLocal = -curHp;
					target.nonBuildLocalDeltaDuringConstruction = ClampSignedDelta(
						static_cast<int>(target.nonBuildLocalDeltaDuringConstruction) + clampedLocal);
				}
			}
			return 0;
		}

		// Path 2: Receiver-side tracking for remote units (strictly display-only).
		const unsigned char ownerSlot = static_cast<unsigned char>(unit->cOwnerID);
		if (ownerSlot >= 10)
			return 0;

		HealthCorrection& correction = g_corrections[unitIndex];
		BindCorrectionIdentity(correction, unit, ownerSlot);

		if (isHeal)
		{
			if (correction.hasHealthCorrection)
			{
				// Keep unclamped so out-of-order combat hits still cancel cleanly.
				correction.rawDisplayHealth += static_cast<int>(rawAmount);
				correction.lastUpdateTick = ta->GameTime;
				RequestHoveredBottomStateRefresh(ta, unitIndex);
			}
		}
		else
		{
			correction.hasLastDamage = true;
			correction.lastDamageMs = GetTickCount();
			correction.lastDamageTick = ta->GameTime;

			if (correction.hasHealthCorrection)
			{
				// Keep unclamped so a repair delta arriving after a low-HP hit
				// reproduces exact HP: (200 - 350) + 400 == 250.
				correction.rawDisplayHealth -= signedDamage;
				correction.lastUpdateTick = ta->GameTime;
				RequestHoveredBottomStateRefresh(ta, unitIndex);
			}
			else
			{
				// Stock TA clamps remote UnitStruct::Health at 0. Track any lost
				// overshoot so both 0x10 completion and subsequent repair deltas
				// start from the true unclamped baseline.
				const int stockHp = curHp > 0 ? curHp : 0;
				if (signedDamage > stockHp)
				{
					const int lostToClamp = signedDamage - stockHp;
					correction.unclampedClampLoss = (std::min)(
						kMaxTrackedClampLoss,
						correction.unclampedClampLoss + lostToClamp);
				}
			}
		}
		return 0;
	}

	int __stdcall BuildFinishedObserverHook(PInlineX86StackBuffer stack)
	{
		if (!g_installed || !stack)
			return 0;

		TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
		if (!ta || !EnsureTables(ta))
			return 0;

		UnitStruct* builder = *reinterpret_cast<UnitStruct**>(stack->Esp + 4);
		UnitStruct* builtUnit = *reinterpret_cast<UnitStruct**>(stack->Esp + 8);
		if (!builder || !IsAliveUnitSlot(builder) || !builder->UnitType->CANBUILD_ptr
			|| !builtUnit || !IsAliveUnitSlot(builtUnit))
		{
			return 0;
		}

		size_t unitIndex = 0;
		if (!TryGetUnitIndex(ta, builtUnit, unitIndex)
			|| !IsDisplayableUnitSlot(builtUnit, unitIndex))
		{
			return 0;
		}

		const int localSlot = ta->LocalHumanPlayer_PlayerID;
		if (localSlot < 0 || localSlot >= 10)
			return 0;

		if (builtUnit->cOwnerID == localSlot)
		{
			if (!DataShare->PlayingDemo)
			{
				TransitionLocalTargetToCompleted(
					g_repairTargets[unitIndex], builtUnit, GetTickCount());
			}
			return 0;
		}

		// Remote unit completing via stock 0x10: TA clears Nanoframe to 0 right
		// after this entry point without initializing Health. Establish the
		// display-only Phase 1 health immediately so finished units never show
		// 0% health and black smoke, even against unpatched clients or in fog.
		if (builtUnit->Nanoframe <= 0.0f || !std::isfinite(builtUnit->Nanoframe))
			return 0;

		const unsigned char ownerSlot = static_cast<unsigned char>(builtUnit->cOwnerID);
		if (ownerSlot >= 10)
			return 0;

		const unsigned int maxHp = MaximumDisplayHealth(builtUnit);
		if (maxHp == 0)
			return 0;

		HealthCorrection& correction = g_corrections[unitIndex];
		BindCorrectionIdentity(correction, builtUnit, ownerSlot);
		if (!correction.completed)
		{
			correction.rawDisplayHealth = ComputeCompletedRawHealth(
				static_cast<int>(builtUnit->Health),
				builtUnit->Nanoframe,
				correction.unclampedClampLoss,
				maxHp);
			correction.hasHealthCorrection = true;
			correction.completed = true;
		}
		correction.hasNanoframeCorrection = false;
		correction.displayNanoframe = 0.0f;
		correction.unclampedClampLoss = 0;
		correction.lastUpdateTick = ta->GameTime;
		RequestHoveredBottomStateRefresh(ta, static_cast<unsigned short>(unitIndex));

		if (!g_loggedFirstCompletion)
		{
			IDDrawSurface::OutptFmtTxt(
				"[RepairHealthSync] first display-only 0x10 completion unit=%u displayHp=%u/%u",
				static_cast<unsigned>(unitIndex),
				ClampedDisplayHealth(correction, builtUnit), maxHp);
			g_loggedFirstCompletion = true;
		}
		return 0;
	}

	int __stdcall StockSyncSendObserverHook(PInlineX86StackBuffer stack)
	{
		if (!g_installed || !stack || DataShare->PlayingDemo)
			return 0;

		TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
		if (!ta || !EnsureTables(ta))
			return 0;

		UnitStruct* unit = *reinterpret_cast<UnitStruct**>(stack->Esp + 8);
		size_t unitIndex = 0;
		if (!TryGetUnitIndex(ta, unit, unitIndex))
			return 0;

		const int localSlot = ta->LocalHumanPlayer_PlayerID;
		if (localSlot < 0 || localSlot >= 10)
			return 0;

		if (!IsDisplayableUnitSlot(unit, unitIndex) || unit->cOwnerID != localSlot)
		{
			g_repairTargets[unitIndex] = RepairTarget();
			return 0;
		}

		// Stock 0x2C is packing this unit's authoritative Health and Nanoframe
		// for all peers right now, so any unsent deltas are now folded into 0x2C.
		RepairTarget& target = g_repairTargets[unitIndex];
		target.initialized = true;
		target.identity = unit->Object3do;
		target.unitType = static_cast<unsigned short>(unit->UnitID);
		target.completed = (unit->Nanoframe <= 0.0f);
		target.lastHealth = static_cast<int>(unit->Health);
		target.lastNanoframe = unit->Nanoframe;
		target.nonBuildLocalDeltaDuringConstruction = 0;
		target.pendingDelta.fill(0);
		target.lastSentNanoframeStep.fill(QuantizeNanoframe(unit->Nanoframe));
		target.pendingMask = 0;
		return 0;
	}

	int __stdcall StockSyncRecvObserverHook(PInlineX86StackBuffer stack)
	{
		if (!g_installed || !stack)
			return 0;

		TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
		if (!ta || !EnsureTables(ta))
			return 0;

		UnitStruct* unit = *reinterpret_cast<UnitStruct**>(stack->Esp + 8);
		size_t unitIndex = 0;
		if (!TryGetUnitIndex(ta, unit, unitIndex))
			return 0;

		const int ownerSlot = ResolveOwnerSlotForUnit(ta, unit);
		if (ownerSlot == ta->LocalHumanPlayer_PlayerID)
			return 0;
		HealthCorrection& correction = g_corrections[unitIndex];
		const bool keepDamageClock = (ownerSlot >= 0 && ownerSlot < 10
			&& correction.ownerSlot == static_cast<unsigned char>(ownerSlot)
			&& correction.hasLastDamage);
		const unsigned int savedDamageMs = correction.lastDamageMs;
		const int savedDamageTick = correction.lastDamageTick;

		correction = HealthCorrection();
		if (ownerSlot >= 0 && ownerSlot < 10)
		{
			correction.ownerSlot = static_cast<unsigned char>(ownerSlot);
			correction.last2cOwnerTick = ReadPlayerLast2cGameTime(ta->Players[ownerSlot]);
			correction.hasLast2cOwnerTick = true;
			if (keepDamageClock)
			{
				correction.hasLastDamage = true;
				correction.lastDamageMs = savedDamageMs;
				correction.lastDamageTick = savedDamageTick;
			}
		}
		RequestHoveredBottomStateRefresh(ta, static_cast<unsigned short>(unitIndex));
		return 0;
	}

	void CollectProgress(TAdynmemStruct* ta, int localSlot,
		unsigned short recipients, unsigned int now)
	{
		g_candidates.clear();
		const PlayerStruct& local = ta->Players[localSlot];
		const uintptr_t base = reinterpret_cast<uintptr_t>(ta->BeginUnitsArray_p);
		const uintptr_t end = reinterpret_cast<uintptr_t>(ta->EndOfUnitsArray_p);
		const uintptr_t first = reinterpret_cast<uintptr_t>(local.Units);
		const uintptr_t last = reinterpret_cast<uintptr_t>(local.UnitsAry_End);
		if (first < base || last < first || last >= end
			|| (first - base) % sizeof(UnitStruct) || (last - base) % sizeof(UnitStruct))
		{
			return;
		}

		// TA's AutoHealAndAimLoop uses an inclusive per-player end (0x48AFDE).
		// Visit only our slot range, once per 200 ms, with no LOS calls here.
		const size_t lastIndex = (last - base) / sizeof(UnitStruct);
		for (size_t i = (first - base) / sizeof(UnitStruct); i <= lastIndex; ++i)
		{
			UnitStruct* unit = &ta->BeginUnitsArray_p[i];
			RepairTarget& target = g_repairTargets[i];
			if (i == 0 || !IsDisplayableUnitSlot(unit, i)
				|| unit->cOwnerID != localSlot || unit->Health <= 0
				|| !std::isfinite(unit->Nanoframe)
				|| unit->Nanoframe < 0.0f || unit->Nanoframe > 1.0f)
			{
				target = RepairTarget();
				continue;
			}

			const unsigned int maxHp = MaximumDisplayHealth(unit);
			if (maxHp == 0)
			{
				target = RepairTarget();
				continue;
			}

			const unsigned short type = static_cast<unsigned short>(unit->UnitID);
			const int health = static_cast<int>(unit->Health);
			const float nanoframe = unit->Nanoframe;

			if (!target.initialized || target.unitType != type
				|| target.identity != unit->Object3do)
			{
				InitializeLocalTarget(target, unit, nanoframe > 0.0f);
			}

			if (!target.completed && nanoframe <= 0.0f)
			{
				// Fallback if construction completed without passing through 0x41B8D0.
				TransitionLocalTargetToCompleted(target, unit, now);
			}
			else
			{
				const int ownerLocalDelta = health - target.lastHealth;
				target.lastHealth = health;
				target.lastNanoframe = nanoframe;
				if (ownerLocalDelta != 0)
				{
					target.lastLocalActivityMs = now;
					for (int slot = 0; slot < 10; ++slot)
					{
						if ((recipients & (1u << slot)) != 0)
						{
							target.pendingDelta[slot] = ClampSignedDelta(
								static_cast<int>(target.pendingDelta[slot]) + ownerLocalDelta);
						}
					}
				}
			}

			const unsigned char nanoStep = QuantizeNanoframe(nanoframe);
			const bool reachedFullHealth = (static_cast<unsigned int>(health) >= maxHp);
			const bool idleSettled = (now - target.lastLocalActivityMs >= kFlushIdleMs);
			unsigned short readyMask = 0;
			for (int slot = 0; slot < 10; ++slot)
			{
				const unsigned short bit = static_cast<unsigned short>(1u << slot);
				if ((recipients & bit) == 0)
				{
					target.pendingDelta[slot] = 0;
					target.lastSentNanoframeStep[slot] = nanoStep;
					continue;
				}
				const bool healthReady = ShouldFlushDelta(
					target.pendingDelta[slot], maxHp, reachedFullHealth, idleSettled);
				const bool nanoReady = (!target.completed
					&& nanoStep != target.lastSentNanoframeStep[slot]);
				if (healthReady || nanoReady)
					readyMask |= bit;
			}

			target.pendingMask = readyMask;
			if (readyMask != 0)
				g_candidates.push_back(static_cast<unsigned short>(i));
		}
	}

	void HandleHealthSync(unsigned int fromDpid, const void* buffer)
	{
		const HealthSyncPacket* packet = static_cast<const HealthSyncPacket*>(buffer);
		if (!g_installed || !packet
			|| packet->chatByte != 0x05 || packet->nullText != 0x00
			|| packet->msgId != ChatHijackId::RepairHealthSync
			|| packet->size != sizeof(HealthSyncPacket)
			|| packet->version != kProtocolVersion
			|| packet->count == 0 || packet->count > kRecordsPerPacket)
		{
			return;
		}

		TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
		PlayerStruct* sender = FindPlayerByDPID(fromDpid);
		if (!ta || DataShare->TAProgress != TAInGame || !sender
			|| !sender->PlayerActive || !sender->PlayerInfo
			|| (sender->PlayerInfo->PropertyMask & WATCH) != 0
			|| GetInferredPlayerType(sender) != Player_RemoteHuman
			|| !EnsureTables(ta))
		{
			return;
		}

		const int senderSlot = sender->PlayerAryIndex;
		const int localSlot = ta->LocalHumanPlayer_PlayerID;
		if (senderSlot < 0 || senderSlot >= 10 || localSlot < 0 || localSlot >= 10
			|| senderSlot == localSlot)
		{
			return;
		}

		PlayerStruct& local = ta->Players[localSlot];
		const unsigned int nowMs = GetTickCount();
		const bool playingDemo = (DataShare->PlayingDemo != 0);
		unsigned int accepted = 0;

		for (size_t i = 0; i < packet->count; ++i)
		{
			const HealthDeltaRecordWire& record = packet->records[i];
			UnitStruct* unit = UnitByIndex(ta, record.unitIndex);
			if (!unit || !IsDisplayableUnitSlot(unit, record.unitIndex)
				|| unit->cOwnerID != senderSlot
				|| static_cast<unsigned short>(unit->UnitID) != record.unitType
				|| !PositionInPlayerLos(unit, local))
			{
				continue;
			}

			const unsigned int maxHp = MaximumDisplayHealth(unit);
			if (maxHp == 0 || record.ownerHealth > maxHp)
				continue;

			HealthCorrection& correction = g_corrections[record.unitIndex];
			BindCorrectionIdentity(correction, unit, static_cast<unsigned char>(senderSlot));

			// Reject deltas sent on or before a stock 0x2C status sync we already
			// applied, preventing cross-channel double-counting.
			if (correction.hasLast2cOwnerTick
				&& IsTickAtOrBefore(packet->ownerGameTime, correction.last2cOwnerTick))
			{
				continue;
			}

			const bool recordCompleted = (record.flags & kRecordCompleted) != 0;
			const bool unitCompleted = correction.completed || (unit->Nanoframe <= 0.0f);

			if (!recordCompleted && unitCompleted)
			{
				// Stale Phase 0 construction packet arriving after 0x10 completion.
				continue;
			}

			const bool inRecentCombat = IsInRecentCombat(
				correction.hasLastDamage, playingDemo,
				nowMs, correction.lastDamageMs,
				ta->GameTime, correction.lastDamageTick);

			if (!inRecentCombat)
			{
				// Unit has not taken 0x0B damage for at least 1.0 s: reconcile
				// directly to ownerHealth to self-heal any dropped packet,
				// ally overheal, or shared/resurrected offset with zero bounce.
				correction.rawDisplayHealth = static_cast<int>(record.ownerHealth);
				correction.hasHealthCorrection = true;
				correction.unclampedClampLoss = 0;
				if (recordCompleted)
				{
					correction.completed = true;
					correction.hasNanoframeCorrection = false;
					correction.displayNanoframe = 0.0f;
				}
				else
				{
					correction.displayNanoframe = DecodeNanoframe(record.nanoframe);
					correction.hasNanoframeCorrection = true;
				}
			}
			else if (recordCompleted && !unitCompleted)
			{
				// Active combat + Phase 1 delta arrived ahead of stock 0x10:
				// establish the unclamped completion baseline once, then add delta.
				const int baseHp = ComputeCompletedRawHealth(
					static_cast<int>(unit->Health),
					unit->Nanoframe,
					correction.unclampedClampLoss,
					maxHp);
				correction.rawDisplayHealth = baseHp + static_cast<int>(record.healthDelta);
				correction.hasHealthCorrection = true;
				correction.completed = true;
				correction.hasNanoframeCorrection = false;
				correction.displayNanoframe = 0.0f;
				correction.unclampedClampLoss = 0;
			}
			else
			{
				// Active combat: ignore ownerHealth and apply only the unclamped
				// commutative delta so network ping can never bounce the bar.
				if (record.healthDelta != 0)
				{
					const int baseHp = correction.hasHealthCorrection
						? correction.rawDisplayHealth
						: ((std::max)(0, static_cast<int>(unit->Health)) - correction.unclampedClampLoss);
					correction.rawDisplayHealth = baseHp + static_cast<int>(record.healthDelta);
					correction.hasHealthCorrection = true;
					correction.unclampedClampLoss = 0;
				}
				if (!recordCompleted)
				{
					correction.displayNanoframe = DecodeNanoframe(record.nanoframe);
					correction.hasNanoframeCorrection = true;
				}
				else
				{
					correction.completed = true;
					correction.hasNanoframeCorrection = false;
					correction.displayNanoframe = 0.0f;
				}
			}

			correction.lastUpdateTick = ta->GameTime;
			RequestHoveredBottomStateRefresh(ta, record.unitIndex);
			++accepted;
		}

		if (accepted != 0 && !g_loggedFirstReceive)
		{
			IDDrawSurface::OutptFmtTxt(
				"[RepairHealthSync] received first delta display correction (%u record%s)",
				accepted, accepted == 1 ? "" : "s");
			g_loggedFirstReceive = true;
		}
	}

	HealthCorrection* ActiveCorrection(TAdynmemStruct* ta, UnitStruct* unit)
	{
		if (!ta || !unit || !EnsureTables(ta))
			return nullptr;

		const int unitIndex = static_cast<unsigned short>(unit->UnitInGameIndex);
		if (unitIndex <= 0 || static_cast<size_t>(unitIndex) >= g_corrections.size()
			|| unit != &ta->BeginUnitsArray_p[unitIndex])
		{
			return nullptr;
		}

		HealthCorrection& correction = g_corrections[unitIndex];
		const int localSlot = ta->LocalHumanPlayer_PlayerID;
		if ((!correction.hasHealthCorrection && !correction.hasNanoframeCorrection)
			|| localSlot < 0 || localSlot >= 10
			|| !IsDisplayableUnitSlot(unit, static_cast<size_t>(unitIndex))
			|| correction.identity != unit->Object3do
			|| correction.ownerSlot != unit->cOwnerID
			|| correction.unitType != static_cast<unsigned short>(unit->UnitID))
		{
			if (correction.hasHealthCorrection || correction.hasNanoframeCorrection)
				correction = HealthCorrection();
			return nullptr;
		}

		// Never expose health or construction outside native LOS, but preserve
		// the delta state across temporary LOS loss so re-scouting stays accurate.
		if (!PositionInPlayerLos(unit, ta->Players[localSlot]))
			return nullptr;

		return &correction;
	}

	UnitStruct* ScriptUnit(TAdynmemStruct* ta, const void* scriptData)
	{
		if (!ta || !scriptData || !EnsureTables(ta))
			return nullptr;

		// tplayx's optional expanded-script-slots patch moves pObject3D from
		// 0x540 to 0x2920. Accept either layout, then validate the result against
		// TA's live unit array and the unit's own object pointer.
		const size_t objectOffsets[] = { 0x540, 0x2920 };
		for (size_t offset : objectOffsets)
		{
			__try
			{
				Object3doStruct* object = *reinterpret_cast<Object3doStruct* const*>(
					reinterpret_cast<const unsigned char*>(scriptData) + offset);
				if (!object)
					continue;
				UnitStruct* unit = object->ThisUnit;
				if (!unit)
					continue;
				const uintptr_t address = reinterpret_cast<uintptr_t>(unit);
				const uintptr_t base = reinterpret_cast<uintptr_t>(ta->BeginUnitsArray_p);
				const uintptr_t end = reinterpret_cast<uintptr_t>(ta->EndOfUnitsArray_p);
				if (address >= base && address < end
					&& (address - base) % sizeof(UnitStruct) == 0
					&& unit->Object3do == object)
				{
					return unit;
				}
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
			}
		}
		return nullptr;
	}

	bool ShouldSuppressDamageSmoke(TAdynmemStruct* ta, UnitStruct* unit,
		unsigned int sfxType)
	{
		if (sfxType != kCobWhiteSmokeSfx && sfxType != kCobBlackSmokeSfx)
			return false;
		HealthCorrection* correction = ActiveCorrection(ta, unit);
		const unsigned int maximum = MaximumDisplayHealth(unit);
		if (!correction || !correction->hasHealthCorrection || maximum == 0)
			return false;
		const unsigned int displayed = ClampedDisplayHealth(*correction, unit);
		return displayed * 100u >= maximum * kDamageSmokeThresholdPercent;
	}

	int __stdcall CobEmitSfxHook(PInlineX86StackBuffer stack)
	{
		if (!g_installed || !stack)
			return 0;

		__try
		{
			const unsigned int sfxType = *reinterpret_cast<const unsigned int*>(
				stack->Esp + 8);
			if (sfxType != kCobWhiteSmokeSfx && sfxType != kCobBlackSmokeSfx)
				return 0;
			TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
			UnitStruct* unit = ScriptUnit(ta, reinterpret_cast<const void*>(stack->Ecx));
			if (!ShouldSuppressDamageSmoke(ta, unit, sfxType))
				return 0;

			LPVOID returnAddress = *reinterpret_cast<LPVOID*>(stack->Esp);
			stack->Eax = 0;
			stack->Esp += 12; // return address plus the two stdcall arguments
			stack->rtnAddr_Pvoid = returnAddress;
			if (!g_loggedFirstSmokeSuppression)
			{
				IDDrawSurface::OutptFmtTxt(
					"[RepairHealthSync] first stale damage-smoke suppression unit=%d sfx=%03X",
					unit->UnitInGameIndex, sfxType);
				g_loggedFirstSmokeSuppression = true;
			}
			return X86STRACKBUFFERCHANGE;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return 0;
		}
	}

	void BeginNanoframeDrawImpl(UnitStruct* unit)
	{
		if (!g_installed)
			return;

		RenderRestore* restore = nullptr;
		if (g_renderDepth < kRenderStackCapacity)
		{
			restore = &g_renderRestoreStack[g_renderDepth];
			*restore = RenderRestore();
		}
		++g_renderDepth;
		if (!restore)
			return;

		TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
		if (!ta || !unit)
			return;

		// A nested render of the same unit already sees the outer temporary
		// value. Do not revalidate it against itself or clear its correction.
		for (size_t i = 0; i + 1 < g_renderDepth && i < kRenderStackCapacity; ++i)
		{
			if (g_renderRestoreStack[i].unit == unit)
				return;
		}
		HealthCorrection* correction = ActiveCorrection(ta, unit);
		if (!correction || !correction->hasNanoframeCorrection
			|| unit->Nanoframe <= 0.0f
			|| correction->displayNanoframe < 0.0f
			|| correction->displayNanoframe >= unit->Nanoframe)
		{
			return;
		}

		restore->unit = unit;
		restore->nanoframe = unit->Nanoframe;
		unit->Nanoframe = correction->displayNanoframe;
		if (!g_loggedFirstNanoframeDraw)
		{
			IDDrawSurface::OutptFmtTxt(
				"[RepairHealthSync] first nanoframe-render correction unit=%d stock=%.3f synced=%.3f",
				unit->UnitInGameIndex, restore->nanoframe, correction->displayNanoframe);
			g_loggedFirstNanoframeDraw = true;
		}
	}

	int __stdcall UnitRenderExitHook(PInlineX86StackBuffer)
	{
		if (!g_installed || g_renderDepth == 0)
			return 0;

		--g_renderDepth;
		if (g_renderDepth < kRenderStackCapacity)
		{
			RenderRestore& restore = g_renderRestoreStack[g_renderDepth];
			if (restore.unit)
				restore.unit->Nanoframe = restore.nanoframe;
			restore = RenderRestore();
		}
		return 0;
	}

	int __stdcall SelectedUnitHealthValueHook(PInlineX86StackBuffer stack)
	{
		if (!g_installed)
			return 0;

		TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
		UnitStruct* unit = reinterpret_cast<UnitStruct*>(stack->Eax);
		if (!ta || !unit)
			return 0;
		HealthCorrection* correction = ActiveCorrection(ta, unit);
		const int stockHealth = static_cast<int>(stack->Edx);
		if (!correction || !correction->hasHealthCorrection)
			return 0;

		const unsigned short displayed = ClampedDisplayHealth(*correction, unit);
		if (displayed == (std::max)(stockHealth, 0))
			return 0;

		stack->Edx = displayed;
		if (!g_loggedFirstHealthDraw)
		{
			IDDrawSurface::OutptFmtTxt(
				"[RepairHealthSync] first display-only selected health bar unit=%d stock=%d synced=%u",
				unit->UnitInGameIndex, stockHealth, displayed);
			g_loggedFirstHealthDraw = true;
		}
		return 0;
	}

	unsigned short Recipients(TAdynmemStruct* ta, int localSlot)
	{
		unsigned short mask = 0;
		for (int slot = 0; slot < 10; ++slot)
		{
			PlayerStruct& player = ta->Players[slot];
			const bool eligible = slot != localSlot && player.PlayerActive
				&& player.DirectPlayID != 0 && player.PlayerInfo
				&& (player.PlayerInfo->PropertyMask & WATCH) == 0
				&& GetInferredPlayerType(&player) == Player_RemoteHuman;
			const unsigned int dpid = eligible ? player.DirectPlayID : 0;
			if (g_recipientDpids[slot] != dpid)
			{
				g_recipientDpids[slot] = dpid;
				g_recipientCadence[slot] = Cadence();
				g_sendCursors[slot] = 1;
			}
			if (eligible)
				mask |= 1u << slot;
		}
		return mask;
	}

	void SendOnePacket(TAdynmemStruct* ta, int localSlot,
		int targetSlot, unsigned int now)
	{
		if (g_candidates.empty())
			return;

		const unsigned short recipient = static_cast<unsigned short>(1u << targetSlot);
		const auto start = std::lower_bound(
			g_candidates.begin(), g_candidates.end(), g_sendCursors[targetSlot]);
		const size_t startIndex = start == g_candidates.end()
			? 0 : static_cast<size_t>(start - g_candidates.begin());

		HealthSyncPacket packet = {};
		packet.chatByte = 0x05;
		packet.msgId = ChatHijackId::RepairHealthSync;
		packet.size = sizeof(packet);
		packet.version = kProtocolVersion;
		packet.ownerGameTime = static_cast<unsigned int>(ta->GameTime);

		std::array<unsigned short, kRecordsPerPacket> packedIndices = {};
		std::array<unsigned char, kRecordsPerPacket> packedNanoSteps = {};

		const size_t checks = (std::min)(g_candidates.size(),
			static_cast<size_t>(kCandidateChecksPerPass));
		for (size_t visited = 0; visited < checks; ++visited)
		{
			const unsigned short index =
				g_candidates[(startIndex + visited) % g_candidates.size()];
			g_sendCursors[targetSlot] = index == USHRT_MAX ? 1 : index + 1;
			RepairTarget& target = g_repairTargets[index];
			if ((target.pendingMask & recipient) == 0)
				continue;

			UnitStruct* unit = &ta->BeginUnitsArray_p[index];
			if (!IsDisplayableUnitSlot(unit, index) || unit->cOwnerID != localSlot
				|| static_cast<unsigned short>(unit->UnitID) != target.unitType
				|| target.identity != unit->Object3do
				|| unit->Health <= 0 || !std::isfinite(unit->Nanoframe)
				|| unit->Nanoframe < 0.0f || unit->Nanoframe > 1.0f)
			{
				target = RepairTarget();
				continue;
			}

			// Visibility is checked immediately before sending. Hidden targets
			// keep their accumulated delta queued until scouted (or folded into
			// stock 0x2C), while this pass stays strictly bounded to 64 checks.
			if (!PositionInPlayerLos(unit, ta->Players[targetSlot]))
				continue;

			const size_t slotIdx = packet.count++;
			HealthDeltaRecordWire& record = packet.records[slotIdx];
			record.unitIndex = index;
			record.unitType = target.unitType;
			record.healthDelta = target.pendingDelta[targetSlot];
			record.ownerHealth = ClampHealth(
				static_cast<int>(unit->Health), MaximumDisplayHealth(unit));
			record.nanoframe = EncodeNanoframe(unit->Nanoframe);
			record.flags = target.completed ? kRecordCompleted : 0;

			packedIndices[slotIdx] = index;
			packedNanoSteps[slotIdx] = QuantizeNanoframe(unit->Nanoframe);
			if (packet.count == kRecordsPerPacket)
				break;
		}

		if (packet.count == 0)
			return;

		// Only clear pending deltas if TA's send queue accepted the packet.
		// If the condenser is momentarily full, deltas stay queued and retry
		// on the next bounded pass without losing any HP.
		if (!HAPI_SendBuf(ta->Players[localSlot].DirectPlayID,
			ta->Players[targetSlot].DirectPlayID,
			reinterpret_cast<const char*>(&packet), sizeof(packet)))
		{
			return;
		}

		g_recipientCadence[targetSlot].Advance(now);
		for (size_t i = 0; i < packet.count; ++i)
		{
			RepairTarget& target = g_repairTargets[packedIndices[i]];
			target.pendingDelta[targetSlot] = 0;
			target.lastSentNanoframeStep[targetSlot] = packedNanoSteps[i];
			target.pendingMask &= ~recipient;
		}
		if (!g_loggedFirstSend)
		{
			IDDrawSurface::OutptFmtTxt(
				"[RepairHealthSync] first delta update (%u records, global 20 packets/s cap)",
				packet.count);
			g_loggedFirstSend = true;
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

		const unsigned int localDpid = ta->Players[localSlot].DirectPlayID;
		if (localDpid != g_sessionDpid || gameTime < g_lastTick)
			ResetSession(localDpid, gameTime);
		g_lastTick = gameTime;

		PlayerStruct& local = ta->Players[localSlot];
		if (DataShare->PlayingDemo || DataShare->TAProgress != TAInGame
			|| ta->IsGamePaused || !local.PlayerActive || localDpid == 0
			|| !local.PlayerInfo || (local.PlayerInfo->PropertyMask & WATCH) != 0)
		{
			return;
		}

		const unsigned short recipients = Recipients(ta, localSlot);
		if (recipients == 0)
			return;
		const unsigned int now = GetTickCount();
		if (!g_packetCadence.Due(now, kPacketSpacingMs))
			return;
		// No catch-up bursts after a stall, and no faster sending at +10.
		g_packetCadence.Advance(now);
		if (!EnsureTables(ta))
			return;
		if (g_sampleCadence.Due(now, kSampleMs))
		{
			CollectProgress(ta, localSlot, recipients, now);
			g_sampleCadence.Advance(now);
		}

		for (int visited = 0; visited < 10; ++visited)
		{
			const int slot = g_nextRecipient;
			g_nextRecipient = (g_nextRecipient + 1) % 10;
			if ((recipients & (1u << slot)) == 0
				|| !g_recipientCadence[slot].Due(now, kRecipientSpacingMs))
			{
				continue;
			}
			SendOnePacket(ta, localSlot, slot, now);
			break;
		}
	}
}

namespace RepairHealthSync
{
	void Install()
	{
		if (g_installed)
			return;

		const HookSite sites[] = {
			{ kApplyDamagePacketAddress, 8, { 0x53, 0x56, 0x8B, 0x15, 0xE8, 0x1D, 0x51, 0x00 }, ApplyDamagePacketHook },
			{ kBuildFinishedAddress, 6, { 0x56, 0x57, 0x8B, 0x7C, 0x24, 0x0C }, BuildFinishedObserverHook },
			{ kStockSyncSendAddress, 5, { 0xA1, 0xE8, 0x1D, 0x51, 0x00 }, StockSyncSendObserverHook },
			{ kStockSyncRecvAddress, 5, { 0xA1, 0xE8, 0x1D, 0x51, 0x00 }, StockSyncRecvObserverHook },
			{ kSelectedUnitHealthValueAddress, 8, { 0x8B, 0xC6, 0x8B, 0x08, 0x89, 0x4C, 0x24, 0x14 }, SelectedUnitHealthValueHook },
			{ kBottomStateCompareAddress, 6, { 0x8B, 0xF0, 0x33, 0xD2, 0xF3, 0xA6 }, BottomStateRefreshHook },
			{ kCobEmitSfxAddress, 5, { 0x83, 0xEC, 0x18, 0x33, 0xD2 }, CobEmitSfxHook },
			// TeamColorNanolathe owns this shared entry hook and calls
			// BeginNanoframeDraw before TA reads UnitStruct::Nanoframe.
			{ kNanoframeRenderEntryAddress, 10, { 0x89, 0x54, 0x24, 0x14, 0xD9, 0x82, 0x04, 0x01, 0x00, 0x00 }, nullptr },
			{ kNanoframeRenderExitAddress1, 5, { 0x33, 0xC0, 0x5F, 0x5E, 0x5D }, UnitRenderExitHook },
			{ kNanoframeRenderExitAddress2, 8, { 0x5F, 0x5E, 0x5D, 0xB8, 0x01, 0x00, 0x00, 0x00 }, UnitRenderExitHook }
		};
		for (const HookSite& site : sites)
		{
			if (std::memcmp(reinterpret_cast<const void*>(site.address), site.expected, site.length) != 0)
			{
				IDDrawSurface::OutptFmtTxt(
					"[RepairHealthSync] disabled: incompatible hook at %08X", site.address);
				return;
			}
		}
		g_hooks.reserve(sizeof(sites) / sizeof(sites[0]));
		for (const HookSite& site : sites)
		{
			if (site.callback)
			{
				g_hooks.push_back(std::make_unique<InlineSingleHook>(
					site.address, site.length, INLINE_5BYTESLAGGERJMP, site.callback));
			}
		}

		PacketChatRouter::GetInstance()->RegisterHandler(
			ChatHijackId::RepairHealthSync, HandleHealthSync, true);
		GameTickHook::GetInstance()->addCallback(OnGameTick);
		g_installed = true;
		IDDrawSurface::OutptTxt(
			"[RepairHealthSync] installed (display-only delta+completion v13, local scan 200ms, global packet spacing 50ms)");
	}

	void Shutdown()
	{
		g_installed = false;
		RestoreRenderFrames();
		g_hooks.clear();
		ResetSession(0, -1);
	}

	void BeginNanoframeDraw(UnitStruct* unit)
	{
		BeginNanoframeDrawImpl(unit);
	}
}

#endif // REPAIR_HEALTH_SYNC_ENABLE
