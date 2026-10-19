#include "ShareSplit.h"

#if SHARE_SPLIT_ENABLE

#include <windows.h>

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>

#include "GameTickHook.h"
#include "Profiler.h"
#include "hook/hook.h"
#include "iddrawsurface.h"
#include "tafunctions.h"
#include "tamem.h"

// tamem.h is #pragma pack(1). These anchor every offset the module depends on. If one fires, tamem.h
// moved: re-derive from the executable before touching anything else here.
static_assert(offsetof(TAdynmemStruct, LocalHumanPlayer_PlayerID) == 0x2A42, "TAdynmemStruct::LocalHumanPlayer_PlayerID moved");
static_assert(offsetof(TAdynmemStruct, WorkStatusMask)            == 0x2A44, "TAdynmemStruct::WorkStatusMask moved");
static_assert(offsetof(TAdynmemStruct, Players)                   == 0x1B63, "TAdynmemStruct::Players moved");
static_assert(offsetof(TAdynmemStruct, GameTime)                  == 0x38A47, "TAdynmemStruct::GameTime moved");
static_assert(sizeof(PlayerStruct)                                == 0x14B,  "PlayerStruct size changed (slot = byte offset / 0x14B)");
static_assert(offsetof(PlayerStruct, PlayerActive)                == 0x00,   "PlayerStruct::PlayerActive moved");
static_assert(offsetof(PlayerStruct, PlayerInfo)                  == 0x27,   "PlayerStruct::PlayerInfo moved");
static_assert(offsetof(PlayerStruct, My_PlayerType)               == 0x73,   "PlayerStruct::My_PlayerType moved");
static_assert(offsetof(PlayerStruct, PlayerRes)                   == 0x8C,   "PlayerStruct::PlayerRes moved");
static_assert(offsetof(PlayerStruct, ShareMetal)                  == 0xE4,   "PlayerStruct::ShareMetal moved");
static_assert(offsetof(PlayerStruct, ShareEnergy)                 == 0xE8,   "PlayerStruct::ShareEnergy moved");
static_assert(offsetof(PlayerStruct, resourcesShared)             == 0xEC,   "PlayerStruct::resourcesShared moved");
static_assert(offsetof(PlayerStruct, AllyFlagAry)                 == 0x108,  "PlayerStruct::AllyFlagAry moved");
static_assert(offsetof(PlayerStruct, field_113)                   == 0x113,  "PlayerStruct::field_113 (alliedBy[0]) moved");
static_assert(offsetof(PlayerStruct, WholeUnitsCounters)          == 0x140,  "PlayerStruct::WholeUnitsCounters moved");
static_assert(offsetof(PlayerStruct, UnitsNumber)                 == 0x144,  "PlayerStruct::UnitsNumber moved");
static_assert(offsetof(PlayerStruct, PlayerAryIndex)              == 0x146,  "PlayerStruct::PlayerAryIndex moved");
static_assert(offsetof(PlayerResourcesStruct, fCurrentEnergy)     == 0x00,   "PlayerResourcesStruct::fCurrentEnergy moved");
static_assert(offsetof(PlayerResourcesStruct, fCurrentMetal)      == 0x0C,   "PlayerResourcesStruct::fCurrentMetal moved");
static_assert(offsetof(PlayerResourcesStruct, fMaxEnergyStorage)  == 0x18,   "PlayerResourcesStruct::fMaxEnergyStorage moved");
static_assert(offsetof(PlayerResourcesStruct, fMaxMetalStorage)   == 0x1C,   "PlayerResourcesStruct::fMaxMetalStorage moved");
static_assert(offsetof(PlayerInfoStruct, PlayerType)              == 0x94,   "PlayerInfoStruct::PlayerType moved");
static_assert(offsetof(PlayerInfoStruct, SharedBits)              == 0x97,   "PlayerInfoStruct::SharedBits moved");
static_assert(offsetof(PlayerInfoStruct, PropertyMask)            == 0x9B,   "PlayerInfoStruct::PropertyMask moved");

namespace
{
	const int kSlots = 10;
	enum { kMetal = 0, kEnergy = 1 };

	// The most receivers one pulse may reach. Fallback F1 (one recipient per pulse, the least full) is this
	// constant set to 1; nothing else changes.
	const int kMaxReceivers = 9;
	const double kMinGive = 1.0;     // the smallest transfer worth a 17-byte packet

	// ---- TotalA.exe addresses (Escalation GOLD 10.2, md5 1e677a7f..) --------------------------------
	const DWORD kAutoShare     = 0x00457D30u;   // Share_AutoAllyResourcesAndMap, __stdcall(PlayerStruct* me)
	const DWORD kJneSite       = 0x00457D5Fu;   // `jne 0x457FA1` (0F 85 3C 02 00 00): GameTime % 60 != 0 -> map half
	const DWORD kJneLen        = 6u;
	const DWORD kMapHalf       = 0x00457FA1u;   // the map-share half (every 450 ticks)
	const DWORD kCallSite      = 0x0049563Au;   // `call 0x457D30` inside Game_RunSimulationStep (E8 rel32)
	const DWORD kXferMetal     = 0x00464C60u;   // Player_TransferMetalResource  __stdcall(src, dst, float, sendPacket) ret 0x10
	const DWORD kXferEnergy    = 0x00464B30u;   // Player_TransferEnergyResource
	const DWORD kKMetal        = 0x004FD4B8u;   // float 1/3
	const DWORD kKEnergy       = 0x004FD4BCu;   // float 1/2

	// ---- state ---------------------------------------------------------------------------------
	typedef void (__stdcall* TransferFn)(unsigned int src, unsigned int dst, float amount, int sendPacket);
	typedef void (__stdcall* AutoShareFn)(PlayerStruct* me);

	TransferFn  g_transfer[2] = { reinterpret_cast<TransferFn>(kXferMetal), reinterpret_cast<TransferFn>(kXferEnergy) };
	AutoShareFn g_autoShare   = reinterpret_cast<AutoShareFn>(kAutoShare);   // map half only once the jne is a jmp
	float       g_k[2]        = { 1.0f / 3.0f, 0.5f };                        // overwritten from the exe at install

	// Production counters, logged once per game.
	struct Counters
	{
		DWORD pulses[2];          // pulses that passed the sender gates
		DWORD sent[2];            // transfers sent
		double amount[2];         // total amount sent
		DWORD maxReceivers;       // most receivers in one pulse
		DWORD noReceiver[2];      // pulses whose pool had no eligible receiver
		DWORD senderRefused;      // pulses stopped by a sender gate vanilla lacks (dead, +0x22, spectator, index)
		DWORD invariant;          // gives dropped because they broke an invariant (must stay 0)
		DWORD playbackSkips;      // calls seen while watching a replay
		DWORD faults;             // access violations caught in a pulse (the module disables itself)
		DWORD cpuCount;
		double cpuSumUs, cpuMaxUs;
	};
	Counters g_c;

	bool  g_installed;
	bool  g_disabledAtRuntime;
	bool  g_playbackLogged;
	int   g_lastGameTime;
	bool  g_wasInGame;
	LARGE_INTEGER g_qpcFreq;

	void Flush(DWORD addr, size_t len)
	{
		FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<LPCVOID>(addr), len);
	}

	bool GameRunning()
	{
		return DataShare != nullptr && DataShare->TAProgress == TAInGame;
	}

	// ---- the split ---------------------------------------------------------------------------------
	struct Give { int slot; float amount; };
	struct Plan
	{
		double pool;            // (stock - threshold) * K, clamped to the sender's stock
		int    eligible;        // receivers that passed every test
		int    n;               // receivers that get something
		Give   give[kSlots];    // slot order
	};

	struct Cand { int slot; double fill; double room; double give; bool capped; bool open; };

	// Sorts by (fill, slot) ascending. At most 10 elements: insertion sort.
	void SortCands(Cand* c, int n)
	{
		for (int i = 1; i < n; ++i)
		{
			const Cand x = c[i];
			int j = i - 1;
			while (j >= 0 && (c[j].fill > x.fill || (c[j].fill == x.fill && c[j].slot > x.slot)))
			{
				c[j + 1] = c[j];
				--j;
			}
			c[j + 1] = x;
		}
	}

	// Water-filling over c[0..n) sorted by fill ascending (every room >= kMinGive). Equal shares, each capped at
	// its room; a capped receiver's room is paid in full and the rest is shared again. If any uncapped receiver
	// would get less than kMinGive, the FULLEST receiver is dropped and the split redone. Returns how many of the
	// leading candidates were kept; their `give` is set and every one of them is >= kMinGive.
	int WaterFill(double pool, Cand* c, int n)
	{
		while (n > 0 && pool >= kMinGive)
		{
			double left = pool;
			int open = n;
			for (int i = 0; i < n; ++i)
			{
				c[i].give = 0.0;
				c[i].capped = false;
				c[i].open = true;
			}
			while (open > 0 && left > 0.0)
			{
				const double share = left / open;
				bool hit[kSlots];
				bool any = false;
				for (int i = 0; i < n; ++i)
				{
					hit[i] = c[i].open && (c[i].room - c[i].give <= share);
					any = any || hit[i];
				}
				if (!any)
				{
					for (int i = 0; i < n; ++i)
						if (c[i].open)
							c[i].give += share;
					left = 0.0;
					break;
				}
				for (int i = 0; i < n; ++i)
					if (hit[i])
					{
						const double g = c[i].room - c[i].give;
						c[i].give += g;
						left -= g;
						c[i].capped = true;
						c[i].open = false;
						--open;
					}
			}
			bool dust = false;
			for (int i = 0; i < n; ++i)
				if (!c[i].capped && c[i].give < kMinGive)
					dust = true;
			if (!dust)
				return n;
			--n;                                   // drop the fullest receiver, redo
		}
		return 0;
	}

	bool IsSpectator(const PlayerInfoStruct* info)
	{
		return (info->PropertyMask & WATCH) != 0;
	}

	// 1 byte per slot at +0x113: "slot i allies me" (tamem.h calls the first byte field_113).
	bool AlliedBy(const PlayerStruct* p, int i)
	{
		return reinterpret_cast<const BYTE*>(p)[0x113 + i] != 0;
	}

	bool Alive(const PlayerStruct* p)
	{
		return p->UnitsNumber != 0 || p->WholeUnitsCounters == 0;
	}

	float Stock(const PlayerStruct* p, int r) { return r == kMetal ? p->PlayerRes.fCurrentMetal : p->PlayerRes.fCurrentEnergy; }
	float Max(const PlayerStruct* p, int r)   { return r == kMetal ? p->PlayerRes.fMaxMetalStorage : p->PlayerRes.fMaxEnergyStorage; }
	float Thr(const PlayerStruct* p, int r)   { return r == kMetal ? p->ShareMetal : p->ShareEnergy; }

	// Sender gates. Vanilla tests the share bit and the threshold; the rest are tests the transfer
	// path performs AFTER debiting (Net_Send*TransferPacket and Net_SendPacketToPlayer refuse a dead or
	// not-yet-ready player), which would lose the resources.
	enum SenderVerdict { kSenderOk, kSenderQuiet, kSenderRefused };
	SenderVerdict CheckSender(const PlayerStruct* players, int me, int r)
	{
		const PlayerStruct* m = &players[me];
		const PlayerInfoStruct* info = m->PlayerInfo;
		if (!m->PlayerActive || !info)
			return kSenderQuiet;
		if ((info->SharedBits & (r == kMetal ? 2 : 4)) == 0)
			return kSenderQuiet;                       // sharing of this resource is off: vanilla's first test
		const float stock = Stock(m, r);
		if (!(stock > Thr(m, r)))
			return kSenderQuiet;                       // strict; NaN fails (vanilla: fcomp ; test ah,0x41 ; jne)
		if (static_cast<int>(static_cast<unsigned char>(m->PlayerAryIndex)) != me ||
			me < 0 || me >= kSlots ||
			!Alive(m) || reinterpret_cast<const BYTE*>(m)[0x22] != 0 || IsSpectator(info) ||
			m->resourcesShared == nullptr)                 // the transfer debits through the economy object: null is a crash
			return kSenderRefused;
		return kSenderOk;
	}

	// Builds the plan for one resource and one sender over a Players[10] array. Pure: reads the array and
	// nothing else, so the install-time self-test and the offline harness run exactly this code.
	// `k` is the engine's constant (1/3 metal, 1/2 energy).
	SenderVerdict PlanPulse(const PlayerStruct* players, int me, int r, float k, Plan* out)
	{
		out->pool = 0.0;
		out->eligible = 0;
		out->n = 0;
		const SenderVerdict sv = CheckSender(players, me, r);
		if (sv != kSenderOk)
			return sv;

		const PlayerStruct* m = &players[me];
		const double myStock = Stock(m, r);
		const double myMax   = Max(m, r);
		double pool = (myStock - static_cast<double>(Thr(m, r))) * static_cast<double>(k);
		// A negative threshold means "share everything": never more than the stock. One part in a million is left unspent so the
		// engine's own cap (each transfer is limited to the sender's REMAINING stock, debited in float steps) can never bind and shorten
		// the last packet: n float roundings are at most n/2 ulp each, far below 1e-6 of the stock for n <= 9.
		const double cap = myStock * (1.0 - 1e-6);
		if (pool > cap)
			pool = cap;
		if (!(pool >= kMinGive) || !(pool <= 3.0e38) || !(myMax > 0.0))
			return kSenderOk;
		out->pool = pool;

		Cand c[kSlots];
		int n = 0;
		const double myFill = myStock / myMax;
		for (int i = 0; i < kSlots; ++i)
		{
			if (i == me)
				continue;
			const PlayerStruct* p = &players[i];
			if (!p->PlayerActive || p->My_PlayerType != 3 || p->PlayerAryIndex != i)
				continue;
			const PlayerInfoStruct* info = p->PlayerInfo;
			if (!info || info->PlayerType != 1 || IsSpectator(info))
				continue;
			if (!Alive(p) || reinterpret_cast<const BYTE*>(p)[0x22] != 0 || p->resourcesShared == nullptr)
				continue;
			if (m->AllyFlagAry[i] == 0 || !AlliedBy(m, i))
				continue;                              // mutual alliance only
			const double stock = Stock(p, r);
			const double max   = Max(p, r);
			if (!(max > 0.0))
				continue;
			const double fill = stock / max;
			const double room = max - stock;
			if (!(fill < myFill) || !(room >= kMinGive))
				continue;                              // not less full than the sender, or no free storage (also excludes NaN)
			c[n].slot = i;
			c[n].fill = fill;
			c[n].room = room;
			c[n].give = 0.0;
			c[n].capped = false;
			c[n].open = false;
			++n;
		}
		out->eligible = n;
		if (n == 0)
			return kSenderOk;

		SortCands(c, n);
		if (n > kMaxReceivers)
			n = kMaxReceivers;                         // the least full ones
		const int kept = WaterFill(pool, c, n);

		// Back to slot order (at most 9 elements).
		for (int a = 1; a < kept; ++a)
		{
			const Cand x = c[a];
			int b = a - 1;
			while (b >= 0 && c[b].slot > x.slot)
			{
				c[b + 1] = c[b];
				--b;
			}
			c[b + 1] = x;
		}
		for (int a = 0; a < kept; ++a)
		{
			float f = static_cast<float>(c[a].give);
			if (static_cast<double>(f) > c[a].give)
				f = std::nextafter(f, 0.0f);           // never above the planned (and room-capped) value
			out->give[out->n].slot = c[a].slot;
			out->give[out->n].amount = f;
			++out->n;
		}
		return kSenderOk;
	}

	// The invariants the plan must satisfy (checked again before sending and in the self-test).
	bool PlanHolds(const PlayerStruct* players, int r, const Plan& p)
	{
		double sum = 0.0;
		int last = -1;
		for (int i = 0; i < p.n; ++i)
		{
			const Give& g = p.give[i];
			if (g.slot < 0 || g.slot >= kSlots || g.slot <= last)
				return false;
			last = g.slot;
			const double a = g.amount;
			const PlayerStruct* d = &players[g.slot];
			const double room = static_cast<double>(Max(d, r)) - static_cast<double>(Stock(d, r));
			if (!(a >= kMinGive) || !(a <= room) || !(a < 3.0e38))
				return false;
			sum += a;
		}
		return sum <= p.pool * (1.0 + 1e-6);
	}

	// ---- execution -----------------------------------------------------------------------------------
	// One resource, one sender. Returns without doing anything unless the sender gates pass.
	void RunPulse(PlayerStruct* me, int r)
	{
		TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
		if (!ta)
			return;
		const int mySlot = static_cast<int>(me - &ta->Players[0]);
		if (mySlot < 0 || mySlot >= kSlots)
			return;

		LARGE_INTEGER t0, t1;
		QueryPerformanceCounter(&t0);

		Plan plan;
		const SenderVerdict sv = PlanPulse(&ta->Players[0], mySlot, r, g_k[r], &plan);
		if (sv == kSenderRefused)
		{
			++g_c.senderRefused;
		}
		else if (sv == kSenderOk && plan.pool >= kMinGive)
		{
			++g_c.pulses[r];
			if (plan.n == 0)
			{
				++g_c.noReceiver[r];
			}
			else if (!PlanHolds(&ta->Players[0], r, plan))
			{
				++g_c.invariant;                       // a bug: send nothing rather than a bad amount
			}
			else
			{
				const unsigned int src = static_cast<unsigned char>(me->PlayerAryIndex);
				for (int i = 0; i < plan.n; ++i)
				{
					const PlayerStruct* d = &ta->Players[plan.give[i].slot];
					g_transfer[r](src, static_cast<unsigned char>(d->PlayerAryIndex), plan.give[i].amount, 1);
					++g_c.sent[r];
					g_c.amount[r] += plan.give[i].amount;
				}
				if (static_cast<DWORD>(plan.n) > g_c.maxReceivers)
					g_c.maxReceivers = static_cast<DWORD>(plan.n);
			}
		}

		QueryPerformanceCounter(&t1);
		const double us = (t1.QuadPart - t0.QuadPart) * 1.0e6 / static_cast<double>(g_qpcFreq.QuadPart);
		++g_c.cpuCount;
		g_c.cpuSumUs += us;
		if (us > g_c.cpuMaxUs)
			g_c.cpuMaxUs = us;
	}

	// A fault inside a pulse (bad engine memory, or a bug here) must not take the game down: the module
	// switches itself off for the rest of the process and the map half keeps running through the original
	// function. SEH needs a function without C++ objects with destructors.
	void RunPulses(PlayerStruct* me)
	{
		__try
		{
			RunPulse(me, kMetal);
			RunPulse(me, kEnergy);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			++g_c.faults;
			g_disabledAtRuntime = true;
		}
	}

	// PROFILE_SCOPE owns an object with a destructor, which cannot share a function with __try.
	void RunPulsesProfiled(PlayerStruct* me)
	{
		PROFILE_SCOPE("Hook.ShareSplit.Pulse");
		RunPulses(me);
	}

	// Replaces `call 0x457D30` at 0x49563A: same __stdcall(PlayerStruct* me), same place in the step (before
	// Net_FlushPendingFrames), same gates, same order: metal, energy, then the original function, whose resource
	// half is now unreachable and whose map half runs as before.
	void __stdcall ShareSplitStep(PlayerStruct* me)
	{
		TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
		if (ta && me && !g_disabledAtRuntime && (ta->WorkStatusMask & 1) &&
			(static_cast<DWORD>(ta->GameTime) % 60u) == 0)
		{
			if (DataShare && DataShare->PlayingDemo)
				++g_c.playbackSkips;                   // recorded transfers arrive as packets: nothing of our own
			else
				RunPulsesProfiled(me);
		}
		g_autoShare(me);
	}

	// ---- logging ------------------------------------------------------------------------------------
	void ResetCounters()
	{
		std::memset(&g_c, 0, sizeof(g_c));
		g_playbackLogged = false;
	}

	void LogSummary(const char* when)
	{
		const bool any = g_c.pulses[0] || g_c.pulses[1] || g_c.senderRefused || g_c.playbackSkips || g_c.faults || g_c.invariant;
		if (!any)
			return;
		IDDrawSurface::OutptFmtTxt(
			"[ShareSplit] %s: metal pulses=%lu sent=%lu amount=%.0f, energy pulses=%lu sent=%lu amount=%.0f, maxReceivers=%lu, "
			"noReceiver=%lu/%lu, senderRefused=%lu, invariant=%lu, playbackSkips=%lu, faults=%lu, cpu n=%lu mean=%.2fus max=%.2fus",
			when, g_c.pulses[kMetal], g_c.sent[kMetal], g_c.amount[kMetal], g_c.pulses[kEnergy], g_c.sent[kEnergy], g_c.amount[kEnergy],
			g_c.maxReceivers, g_c.noReceiver[kMetal], g_c.noReceiver[kEnergy], g_c.senderRefused, g_c.invariant, g_c.playbackSkips,
			g_c.faults, g_c.cpuCount, g_c.cpuCount ? g_c.cpuSumUs / g_c.cpuCount : 0.0, g_c.cpuMaxUs);
	}

	// One line per game: who the module sees. The first real games verify the remote-side fields passively
	// (is `+0x113` set for teammates, is `+0x22` zero, which slots are type 3).
	void LogGameStart()
	{
		TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
		if (!ta)
			return;
		const int local = static_cast<unsigned char>(ta->LocalHumanPlayer_PlayerID);
		char buf[420];
		int off = snprintf(buf, sizeof(buf), "[ShareSplit] game start: local=%d work=0x%02X slots(type/ptype/allied/alliedBy/watch/b22/alive):",
			local, static_cast<unsigned char>(ta->WorkStatusMask));
		for (int i = 0; i < kSlots && off > 0 && off < static_cast<int>(sizeof(buf)) - 40; ++i)
		{
			const PlayerStruct* p = &ta->Players[i];
			if (!p->PlayerActive)
				continue;
			const PlayerInfoStruct* info = p->PlayerInfo;
			const bool allied = local >= 0 && local < kSlots && ta->Players[local].AllyFlagAry[i] != 0;
			const bool alliedBy = local >= 0 && local < kSlots && AlliedBy(&ta->Players[local], i);
			off += snprintf(buf + off, sizeof(buf) - off, " s%d:%d/%d/%d/%d/%d/%d/%d", i, p->My_PlayerType, info ? info->PlayerType : -1,
				allied ? 1 : 0, alliedBy ? 1 : 0, (info && IsSpectator(info)) ? 1 : 0, reinterpret_cast<const BYTE*>(p)[0x22], Alive(p) ? 1 : 0);
		}
		IDDrawSurface::OutptTxt(buf);
	}

	void OnGameTick(int gameTime)
	{
		// GameTickHook fires several times per game tick with an unchanged gameTime.
		if (gameTime == g_lastGameTime)
			return;
		bool newGame = gameTime < g_lastGameTime;
		g_lastGameTime = gameTime;

		const bool inGame = GameRunning();
		if (inGame && !g_wasInGame)
			newGame = true;
		g_wasInGame = inGame;

		if (newGame)
		{
			LogSummary("previous game");
			ResetCounters();
			LogGameStart();
		}
	}

	// ---- install-time checks -------------------------------------------------------------------------
	bool ReadBytes(DWORD addr, BYTE* out, size_t len)
	{
		__try
		{
			std::memcpy(out, reinterpret_cast<const void*>(addr), len);
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return false;
		}
	}

	bool Expect(DWORD addr, const BYTE* expected, size_t len, const char* what)
	{
		BYTE cur[80];
		if (len <= sizeof(cur) && ReadBytes(addr, cur, len) && std::memcmp(cur, expected, len) == 0)
			return true;
		IDDrawSurface::OutptFmtTxt("[ShareSplit] DISABLED: unexpected TotalA.exe bytes at 0x%08X (%s)", addr, what);
		return false;
	}

	// ---- premises: every byte this module relies on, verified before anything is patched ---------------------------------
	// A table so the offline harness can flip each byte in turn and prove the install refuses (and writes nothing).
	struct Premise { DWORD addr; const BYTE* bytes; size_t len; const char* what; };

	const BYTE kHead[6]       = { 0x83, 0xEC, 0x1C, 0x53, 0x55, 0x56 };
	const BYTE kLoadTa[6]     = { 0x8B, 0x35, 0xE8, 0x1D, 0x51, 0x00 };
	const BYTE kBitGate[13]   = { 0xF6, 0x86, 0x44, 0x2A, 0x00, 0x00, 0x01, 0x0F, 0x84, 0x01, 0x04, 0x00, 0x00 };
	const BYTE kDivide[27]    = { 0x8B, 0x86, 0x47, 0x8A, 0x03, 0x00, 0x33, 0xD2, 0xB9, 0x3C, 0x00, 0x00, 0x00, 0x8B, 0x5C, 0x24,
	                                     0x30, 0xF7, 0xF1, 0x85, 0xD2, 0x0F, 0x85, 0x3C, 0x02, 0x00, 0x00 };
	const BYTE kMapEntry[12]  = { 0x8B, 0x35, 0xE8, 0x1D, 0x51, 0x00, 0x8B, 0x86, 0x47, 0x8A, 0x03, 0x00 };
	const BYTE kEpilogue[3]   = { 0xC2, 0x04, 0x00 };
	const BYTE kGateM[7]      = { 0xF6, 0x82, 0x97, 0x00, 0x00, 0x00, 0x02 };
	const BYTE kGateE[7]      = { 0xF6, 0x82, 0x97, 0x00, 0x00, 0x00, 0x04 };
	const BYTE kThrM[19]      = { 0xD9, 0x83, 0x98, 0x00, 0x00, 0x00, 0xD8, 0x9B, 0xE4, 0x00, 0x00, 0x00, 0xDF, 0xE0, 0xF6, 0xC4, 0x41, 0x0F, 0x85 };
	const BYTE kThrE[19]      = { 0xD9, 0x83, 0x8C, 0x00, 0x00, 0x00, 0xD8, 0x9B, 0xE8, 0x00, 0x00, 0x00, 0xDF, 0xE0, 0xF6, 0xC4, 0x41, 0x0F, 0x85 };
	const BYTE kKmulM[6]      = { 0xD8, 0x0D, 0xB8, 0xD4, 0x4F, 0x00 };
	const BYTE kKmulE[6]      = { 0xD8, 0x0D, 0xBC, 0xD4, 0x4F, 0x00 };
	const BYTE kCallM[5]      = { 0xE8, 0xE3, 0xCD, 0x00, 0x00 };       // call 0x464C60
	const BYTE kCallE[5]      = { 0xE8, 0x95, 0xCB, 0x00, 0x00 };       // call 0x464B30
	const BYTE kKBytesM[4]    = { 0xAB, 0xAA, 0xAA, 0x3E };             // 1/3f
	const BYTE kKBytesE[4]    = { 0x00, 0x00, 0x00, 0x3F };             // 0.5f
	const BYTE kCallSiteB[6]  = { 0x51, 0xE8, 0xF1, 0x26, 0xFC, 0xFF };  // push ecx ; call 0x457D30
	const BYTE kAfterCall[12] = { 0x6A, 0x00, 0xB9, 0x00, 0x30, 0x51, 0x00, 0xE8, 0x55, 0xC2, 0xFC, 0xFF };
	const BYTE kGatesA[4]     = { 0x85, 0xFF, 0x74, 0x5D };
	const BYTE kGatesB[9]     = { 0xA1, 0xBC, 0x6D, 0x50, 0x00, 0x85, 0xC0, 0x74, 0x54 };
	const BYTE kXferHead[12]  = { 0x8B, 0x44, 0x24, 0x04, 0x53, 0x55, 0x56, 0x3C, 0x0A, 0x57, 0x0F, 0x84 };
	const BYTE kZeroTest[19]  = { 0xDD, 0xD8, 0xD9, 0x44, 0x24, 0x1C, 0xD8, 0x1D, 0x38, 0xD5, 0x4F, 0x00, 0xDF, 0xE0, 0xF6, 0xC4,
	                                     0x40, 0x0F, 0x85 };
	const BYTE kRet10[3]      = { 0xC2, 0x10, 0x00 };
	const BYTE kSenderHead[18] = { 0x8B, 0x44, 0x24, 0x04, 0x8B, 0x15, 0xE8, 0x1D, 0x51, 0x00, 0x25, 0xFF, 0x00, 0x00, 0x00, 0x83,
	                                      0xEC, 0x14 };
	// the metal sender's "both alive" gate and the energy sender's packet kind
	const BYTE kAliveGate[24] = { 0x66, 0x83, 0xBE, 0xA7, 0x1C, 0x00, 0x00, 0x00, 0x75, 0x0E, 0x8B, 0x86, 0xA3, 0x1C, 0x00, 0x00,
	                                     0x85, 0xC0, 0x0F, 0x85, 0xE0, 0x00, 0x00, 0x00 };
	const BYTE kKindEnergy[8] = { 0xC7, 0x44, 0x24, 0x09, 0x01, 0x00, 0x00, 0x00 };
	const BYTE kKindMetal[8]  = { 0xC7, 0x44, 0x24, 0x09, 0x02, 0x00, 0x00, 0x00 };
	// the packet gate: session bit, then the recipient must be type 3 and +0x22 == 0
	const BYTE kPktGate[13]   = { 0xF6, 0x82, 0x44, 0x2A, 0x00, 0x00, 0x01, 0x0F, 0x84, 0x43, 0x01, 0x00, 0x00 };
	const BYTE kPktType3[10]  = { 0x80, 0x79, 0x73, 0x03, 0x0F, 0x85, 0xFD, 0x00, 0x00, 0x00 };

	const Premise kPremises[] =
	{
		{ kAutoShare, kHead, sizeof(kHead), "auto-share prologue" },
		{ 0x00457D36u, kLoadTa, sizeof(kLoadTa), "auto-share loads ta" },
		{ 0x00457D3Du, kBitGate, sizeof(kBitGate), "auto-share session-bit gate" },
		{ 0x00457D4Au, kDivide, sizeof(kDivide), "auto-share GameTime % 60 and jne" },
		{ 0x00457F9Bu, kMapEntry, sizeof(kMapEntry), "map half entry" },
		{ 0x00458152u, kEpilogue, sizeof(kEpilogue), "auto-share ret 4" },
		{ 0x00457D6Au, kGateM, sizeof(kGateM), "metal share bit" },
		{ 0x00457E88u, kGateE, sizeof(kGateE), "energy share bit" },
		{ 0x00457D77u, kThrM, sizeof(kThrM), "metal threshold compare" },
		{ 0x00457E95u, kThrE, sizeof(kThrE), "energy threshold compare" },
		{ 0x00457E40u, kKmulM, sizeof(kKmulM), "metal K multiply" },
		{ 0x00457F5Eu, kKmulE, sizeof(kKmulE), "energy K multiply" },
		{ 0x00457E78u, kCallM, sizeof(kCallM), "metal transfer call" },
		{ 0x00457F96u, kCallE, sizeof(kCallE), "energy transfer call" },
		{ kKMetal, kKBytesM, sizeof(kKBytesM), "metal K constant" },
		{ kKEnergy, kKBytesE, sizeof(kKBytesE), "energy K constant" },
		{ 0x00495639u, kCallSiteB, sizeof(kCallSiteB), "call site of the auto-share" },
		{ 0x0049563Fu, kAfterCall, sizeof(kAfterCall), "code after the auto-share call" },
		{ 0x0049560Bu, kGatesA, sizeof(kGatesA), "step-argument gate before the call" },
		{ 0x0049560Fu, kGatesB, sizeof(kGatesB), "net-frame-timing gate before the call" },
		{ kXferMetal, kXferHead, sizeof(kXferHead), "metal transfer entry" },
		{ kXferEnergy, kXferHead, sizeof(kXferHead), "energy transfer entry" },
		{ 0x00464CB8u, kZeroTest, sizeof(kZeroTest), "metal transfer zero test" },
		{ 0x00464B88u, kZeroTest, sizeof(kZeroTest), "energy transfer zero test" },
		{ 0x00464D8Eu, kRet10, sizeof(kRet10), "metal transfer ret 0x10" },
		{ 0x00464C5Au, kRet10, sizeof(kRet10), "energy transfer ret 0x10" },
		{ 0x00457050u, kSenderHead, sizeof(kSenderHead), "metal packet sender" },
		{ 0x00456EE0u, kSenderHead, sizeof(kSenderHead), "energy packet sender" },
		{ 0x004570C0u, kAliveGate, sizeof(kAliveGate), "packet sender alive gate" },
		{ 0x0045712Du, kKindMetal, sizeof(kKindMetal), "metal packet kind" },
		{ 0x00456FBDu, kKindEnergy, sizeof(kKindEnergy), "energy packet kind" },
		{ 0x00451C87u, kPktGate, sizeof(kPktGate), "Net_SendPacketToPlayer session gate" },
		{ 0x00451CD0u, kPktType3, sizeof(kPktType3), "Net_SendPacketToPlayer recipient type" },
	};
	const size_t kPremiseCount = sizeof(kPremises) / sizeof(kPremises[0]);

	bool VerifyPremises()
	{
		for (size_t i = 0; i < kPremiseCount; ++i)
			if (!Expect(kPremises[i].addr, kPremises[i].bytes, kPremises[i].len, kPremises[i].what))
				return false;
		return true;
	}

	// ---- self-test -------------------------------------------------------------------------------------
	// Runs before anything is patched; touches no engine code. Synthetic players are built with the
	// engine's own layout (static_asserts above), so the plan is exercised exactly as it runs in game.
	struct World
	{
		PlayerStruct          p[kSlots];
		PlayerInfoStruct      info[kSlots];
		PlayerSharedResourcesStruct eco[kSlots];
	};

	void SetP(World& w, int i, float stock, float max, int type3 = 1)
	{
		PlayerStruct& p = w.p[i];
		p.PlayerActive = type3 ? 1 : 0;
		p.PlayerInfo = &w.info[i];
		p.My_PlayerType = 3;
		p.PlayerAryIndex = static_cast<char>(i);
		p.resourcesShared = &w.eco[i];
		p.PlayerRes.fCurrentMetal = stock;
		p.PlayerRes.fMaxMetalStorage = max;
		p.PlayerRes.fCurrentEnergy = stock;
		p.PlayerRes.fMaxEnergyStorage = max;
		p.ShareMetal = 0.0f;
		p.ShareEnergy = 0.0f;
		w.info[i].PlayerType = 1;
		w.info[i].SharedBits = 6;
		w.info[i].PropertyMask = 0;
		p.UnitsNumber = 5;
		p.WholeUnitsCounters = 5;
	}

	void Ally(World& w, int a, int b, bool ab, bool ba)       // a allies b / b allies a, from a's record and b's record
	{
		w.p[a].AllyFlagAry[b] = ab ? 1 : 0;
		reinterpret_cast<BYTE*>(&w.p[a])[0x113 + b] = ba ? 1 : 0;
	}

	void Reset(World& w)
	{
		std::memset(&w, 0, sizeof(w));
		for (int i = 0; i < kSlots; ++i)
			SetP(w, i, 0.0f, 0.0f, 0);                  // all slots inactive
	}

	bool RunSelfTest()
	{
		static World w;
		char why[96];
		why[0] = '\0';
		bool ok = true;
		#define SS_FAIL(text) do { ok = false; snprintf(why, sizeof(why), "%s", text); goto done; } while (0)
		Plan p;
		const float K = 1.0f / 3.0f;

		// 1. one receiver, room is not binding: the same amount vanilla would send
		Reset(w);
		SetP(w, 0, 900.0f, 1000.0f);  SetP(w, 1, 100.0f, 1000.0f);  Ally(w, 0, 1, true, true);
		w.p[0].ShareMetal = 300.0f;
		if (PlanPulse(w.p, 0, kMetal, K, &p) != kSenderOk || p.n != 1 || p.give[0].slot != 1) SS_FAIL("one receiver");
		if (std::fabs(p.give[0].amount - (900.0f - 300.0f) * K) > 0.001f)                  SS_FAIL("one receiver amount");
		if (!PlanHolds(w.p, kMetal, p))                                                   SS_FAIL("one receiver invariants");

		// 2. four equal receivers share equally, in slot order
		Reset(w);
		SetP(w, 0, 900.0f, 1000.0f);
		for (int i = 1; i <= 4; ++i) { SetP(w, i, 100.0f, 1000.0f); Ally(w, 0, i, true, true); }
		PlanPulse(w.p, 0, kMetal, K, &p);
		if (p.n != 4 || p.give[0].slot != 1 || p.give[3].slot != 4)                        SS_FAIL("four receivers order");
		for (int i = 1; i < p.n; ++i)
			if (p.give[i].amount != p.give[0].amount)                                      SS_FAIL("equal shares");
		if (!PlanHolds(w.p, kMetal, p))                                                   SS_FAIL("four receivers invariants");

		// 3. a nearly full receiver (still less full than the sender) is capped at its room and the rest goes to the other
		Reset(w);
		SetP(w, 0, 990.0f, 1000.0f);  SetP(w, 1, 100.0f, 1000.0f);  SetP(w, 2, 580.0f, 600.0f);
		Ally(w, 0, 1, true, true);  Ally(w, 0, 2, true, true);
		PlanPulse(w.p, 0, kMetal, K, &p);
		if (p.n != 2 || std::fabs(p.give[1].amount - 20.0f) > 0.01f || p.give[0].amount < 300.0f) SS_FAIL("cap and redistribute");
		if (!PlanHolds(w.p, kMetal, p))                                                   SS_FAIL("cap invariants");

		// 4. no receiver qualifies: everyone fuller than the sender, or less full but with under one unit of room
		Reset(w);
		SetP(w, 0, 900.0f, 1000.0f);  SetP(w, 1, 1000.0f, 1000.0f);  Ally(w, 0, 1, true, true);
		PlanPulse(w.p, 0, kMetal, K, &p);
		if (p.n != 0 || p.eligible != 0)                                                  SS_FAIL("all full");
		Reset(w);
		SetP(w, 0, 1000.0f, 1000.0f);  SetP(w, 1, 99.5f, 100.0f);  Ally(w, 0, 1, true, true);
		PlanPulse(w.p, 0, kMetal, K, &p);
		if (p.n != 0 || p.eligible != 0)                                                  SS_FAIL("room under one unit");

		// 5. vanilla's defect: slot 0 nearly empty, slot 5 full but below the sender's stock: the split reaches slot 0
		Reset(w);
		SetP(w, 3, 900.0f, 1000.0f);  SetP(w, 0, 20.0f, 1000.0f);  SetP(w, 5, 890.0f, 890.0f);
		Ally(w, 3, 0, true, true);  Ally(w, 3, 5, true, true);
		PlanPulse(w.p, 3, kMetal, K, &p);
		if (p.n != 1 || p.give[0].slot != 0 || p.give[0].amount < 290.0f)                  SS_FAIL("vanilla's slot-5 case");

		// 6. one-way alliances are not enough, in either direction
		Reset(w);
		SetP(w, 0, 900.0f, 1000.0f);  SetP(w, 1, 100.0f, 1000.0f);  SetP(w, 2, 100.0f, 1000.0f);
		Ally(w, 0, 1, true, false);  Ally(w, 0, 2, false, true);
		PlanPulse(w.p, 0, kMetal, K, &p);
		if (p.n != 0)                                                                     SS_FAIL("one-way alliance");

		// 7. a receiver fuller than the sender (by ratio) is left out even with a lower absolute stock
		Reset(w);
		SetP(w, 0, 500.0f, 1000.0f);  SetP(w, 1, 450.0f, 460.0f);  SetP(w, 2, 100.0f, 1000.0f);
		Ally(w, 0, 1, true, true);  Ally(w, 0, 2, true, true);
		PlanPulse(w.p, 0, kMetal, K, &p);
		if (p.n != 1 || p.give[0].slot != 2)                                              SS_FAIL("fuller receiver");

		// 8. dust: a pool of 1.5 over two receivers goes whole to the emptier one
		Reset(w);
		SetP(w, 0, 4.5f, 1000.0f);  SetP(w, 1, 1.0f, 1000.0f);  SetP(w, 2, 2.0f, 1000.0f);
		Ally(w, 0, 1, true, true);  Ally(w, 0, 2, true, true);
		PlanPulse(w.p, 0, kMetal, K, &p);
		if (p.n != 1 || p.give[0].slot != 1 || p.give[0].amount < 1.4f)                    SS_FAIL("dust fold");
		Reset(w);
		SetP(w, 0, 2.4f, 1000.0f);  SetP(w, 1, 1.0f, 1000.0f);  Ally(w, 0, 1, true, true);
		if (PlanPulse(w.p, 0, kMetal, K, &p) != kSenderOk || p.n != 0)                     SS_FAIL("pool below one unit");

		// 9. receivers the transfer path would lose or crash on: dead, +0x22, spectator, no economy object,
		//    wrong type, index mismatch, AI (PlayerInfo type 2)
		Reset(w);
		SetP(w, 0, 900.0f, 1000.0f);
		for (int i = 1; i <= 7; ++i) { SetP(w, i, 100.0f, 1000.0f); Ally(w, 0, i, true, true); }
		w.p[1].UnitsNumber = 0;  w.p[1].WholeUnitsCounters = 3;
		reinterpret_cast<BYTE*>(&w.p[2])[0x22] = 1;
		w.info[3].PropertyMask = WATCH;
		w.p[4].resourcesShared = nullptr;
		w.p[5].My_PlayerType = 1;
		w.p[6].PlayerAryIndex = 9;
		w.info[7].PlayerType = 2;
		PlanPulse(w.p, 0, kMetal, K, &p);
		if (p.n != 0)                                                                     SS_FAIL("receivers that must be excluded");

		// 10. sender gates: share bit off, stock not above the threshold, NaN, dead, +0x22, spectator
		Reset(w);
		SetP(w, 0, 900.0f, 1000.0f);  SetP(w, 1, 100.0f, 1000.0f);  Ally(w, 0, 1, true, true);
		w.info[0].SharedBits = 4;
		if (PlanPulse(w.p, 0, kMetal, K, &p) != kSenderQuiet || p.n != 0)                  SS_FAIL("share bit off");
		w.info[0].SharedBits = 6;  w.p[0].ShareMetal = 900.0f;
		if (PlanPulse(w.p, 0, kMetal, K, &p) != kSenderQuiet || p.n != 0)                  SS_FAIL("stock equal to the threshold");
		w.p[0].ShareMetal = 0.0f;  w.p[0].PlayerRes.fCurrentMetal = std::nanf("");
		if (PlanPulse(w.p, 0, kMetal, K, &p) != kSenderQuiet || p.n != 0)                  SS_FAIL("NaN stock");
		w.p[0].PlayerRes.fCurrentMetal = 900.0f;  reinterpret_cast<BYTE*>(&w.p[0])[0x22] = 1;
		if (PlanPulse(w.p, 0, kMetal, K, &p) != kSenderRefused || p.n != 0)                SS_FAIL("sender +0x22");
		reinterpret_cast<BYTE*>(&w.p[0])[0x22] = 0;  w.info[0].PropertyMask = WATCH;
		if (PlanPulse(w.p, 0, kMetal, K, &p) != kSenderRefused || p.n != 0)                SS_FAIL("sender spectator");
		w.info[0].PropertyMask = 0;  w.p[0].UnitsNumber = 0;  w.p[0].WholeUnitsCounters = 1;
		if (PlanPulse(w.p, 0, kMetal, K, &p) != kSenderRefused || p.n != 0)                SS_FAIL("sender dead");
		w.p[0].UnitsNumber = 5;  w.p[0].resourcesShared = nullptr;
		if (PlanPulse(w.p, 0, kMetal, K, &p) != kSenderRefused || p.n != 0)                SS_FAIL("sender without an economy object");

		// 11. a negative threshold ("share everything") clamps the pool to the stock
		Reset(w);
		SetP(w, 0, 100.0f, 1000.0f);  SetP(w, 1, 10.0f, 1000.0f);  Ally(w, 0, 1, true, true);
		w.p[0].ShareMetal = -727379969.0f;
		PlanPulse(w.p, 0, kMetal, K, &p);
		if (p.n != 1 || p.give[0].amount > 100.0f || p.give[0].amount < 99.0f)           SS_FAIL("negative threshold");

		// 12. energy uses its own fields and K = 1/2; the cap on receivers keeps the least full ones
		Reset(w);
		SetP(w, 0, 900.0f, 1000.0f);
		for (int i = 1; i <= 9; ++i) { SetP(w, i, 100.0f + i, 1000.0f); Ally(w, 0, i, true, true); }
		w.p[0].PlayerRes.fCurrentMetal = 0.0f;
		PlanPulse(w.p, 0, kEnergy, 0.5f, &p);
		if (p.n != kMaxReceivers || !PlanHolds(w.p, kEnergy, p))                          SS_FAIL("nine receivers");
		if (std::fabs(p.pool - 450.0) > 0.01)                                             SS_FAIL("energy pool");

		// 13. randomised invariants (fixed seed): sum <= pool, give <= room, give >= 1, equal among uncapped
		{
			unsigned s = 12345u;
			for (int iter = 0; iter < 4000; ++iter)
			{
				Reset(w);
				s = s * 1664525u + 1013904223u;  const float myMax = 100.0f + (s >> 16) % 60000;
				s = s * 1664525u + 1013904223u;  const float myStock = myMax * ((s >> 8) % 1001) / 1000.0f;
				SetP(w, 0, myStock, myMax);
				s = s * 1664525u + 1013904223u;  w.p[0].ShareMetal = myStock * ((s >> 8) % 901) / 1000.0f;
				for (int i = 1; i < kSlots; ++i)
				{
					s = s * 1664525u + 1013904223u;
					if ((s >> 20) % 4 == 0) continue;
					s = s * 1664525u + 1013904223u;  const float mx = 1.0f + (s >> 12) % 70000;
					s = s * 1664525u + 1013904223u;  const float st = mx * ((s >> 8) % 1001) / 1000.0f;
					SetP(w, i, st, mx);
					Ally(w, 0, i, true, true);
				}
				PlanPulse(w.p, 0, kMetal, K, &p);
				if (!PlanHolds(w.p, kMetal, p))                                           SS_FAIL("random invariants");
				double lo = 1e300, hi = -1e300;
				for (int a = 0; a < p.n; ++a)
				{
					const PlayerStruct& d = w.p[p.give[a].slot];
					const double room = static_cast<double>(d.PlayerRes.fMaxMetalStorage) - d.PlayerRes.fCurrentMetal;
					if (p.give[a].amount < room - 0.05) { if (p.give[a].amount < lo) lo = p.give[a].amount; if (p.give[a].amount > hi) hi = p.give[a].amount; }
				}
				if (hi >= lo && hi - lo > 0.01)                                           SS_FAIL("random: equal shares among uncapped");
			}
		}

	done:
		#undef SS_FAIL
		if (!ok)
			IDDrawSurface::OutptFmtTxt("[ShareSplit] DISABLED: self-test failed: %s", why);
		return ok;
	}

	// ---- patch bookkeeping ------------------------------------------------------------------------------
	// SingleHook keeps the pointer to its replacement bytes, so they live here.
	DWORD      g_callRel32;
	SingleHook* g_callHook;
	SingleHook* g_jmpHook;

	void RemoveAllHooks()
	{
		// Reverse order of installation; each destructor writes the original bytes back.
		delete g_jmpHook;  g_jmpHook = nullptr;
		delete g_callHook; g_callHook = nullptr;
		Flush(0x00457000u, 0x1000u);
		Flush(0x00495000u, 0x1000u);
	}

	// Compares memory against what the hooks were asked to write; SingleHook::Hook() reports nothing.
	bool PatchesReadBack()
	{
		BYTE b[8];
		// 1. `jmp 0x457FA1` + one nop at 0x457D5F
		if (!ReadBytes(kJneSite, b, kJneLen))
			return false;
		DWORD rel = 0;
		std::memcpy(&rel, b + 1, 4);
		if (b[0] != 0xE9 || rel != kMapHalf - (kJneSite + 5) || b[5] != 0x90)
			return false;
		// 2. the rel32 of the call at 0x49563A
		if (!ReadBytes(kCallSite, b, 5))
			return false;
		std::memcpy(&rel, b + 1, 4);
		return b[0] == 0xE8 && rel == g_callRel32;
	}
}

namespace ShareSplit
{
	void Install()
	{
		if (g_installed)
			return;

		if (!VerifyPremises())
			return;                        // logged inside; refuse to patch an unexpected build

		if (!QueryPerformanceFrequency(&g_qpcFreq) || g_qpcFreq.QuadPart <= 0)
		{
			IDDrawSurface::OutptTxt("[ShareSplit] DISABLED: no performance counter");
			return;
		}
		std::memcpy(&g_k[kMetal],  reinterpret_cast<const void*>(kKMetal),  sizeof(float));
		std::memcpy(&g_k[kEnergy], reinterpret_cast<const void*>(kKEnergy), sizeof(float));
		if (!RunSelfTest())
			return;                        // logged inside

		ResetCounters();
		g_disabledAtRuntime = false;
		g_lastGameTime = 0;
		g_wasInGame = false;
		g_callRel32 = reinterpret_cast<DWORD>(&ShareSplitStep) - (kCallSite + 5);

		// The new call target first, then the dead-code jump: with either one alone the game still behaves
		// (vanilla's resource half plus our split would double-send for one step, which cannot happen because
		// Install runs before the first simulation step).
		g_callHook = new SingleHook(kCallSite + 1, 4, INLINE_UNPROTECTEVINMENT, reinterpret_cast<LPBYTE>(&g_callRel32));
		Flush(kCallSite + 1, 4);
		g_jmpHook = new SingleHook(kJneSite, kJneLen, INLINE_SINGLEJMP, reinterpret_cast<LPBYTE>(kMapHalf));
		Flush(kJneSite, kJneLen);

		if (!PatchesReadBack())
		{
			RemoveAllHooks();
			IDDrawSurface::OutptTxt("[ShareSplit] DISABLED: patched bytes did not read back; everything restored");
			return;
		}

		GameTickHook::GetInstance()->addCallback(OnGameTick);
		g_installed = true;

		IDDrawSurface::OutptFmtTxt("[ShareSplit] installed: resource half bypassed @0x%08X, step call @0x%08X, K metal=%.6f energy=%.6f, max receivers %d",
			kJneSite, kCallSite, g_k[kMetal], g_k[kEnergy], kMaxReceivers);
	}

	void Shutdown()
	{
		if (!g_installed)
			return;
		LogSummary("last game");
		RemoveAllHooks();
		g_installed = false;
	}
}

#endif // SHARE_SPLIT_ENABLE
