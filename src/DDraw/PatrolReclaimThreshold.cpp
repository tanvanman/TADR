#include "PatrolReclaimThreshold.h"

#if PATROL_RECLAIM_THRESHOLD_ENABLE

#include <windows.h>

#include <cstddef>
#include <cstdio>
#include <cstring>

#include "GameTickHook.h"
#include "Profiler.h"
#include "hook/hook.h"
#include "iddrawsurface.h"
#include "tafunctions.h"
#include "tamem.h"

// tamem.h is #pragma pack(1). These anchor every offset the module, and the hand-written stubs
// below, depend on. If one fires, tamem.h moved: re-derive from the executable before touching
// anything else here.
static_assert(offsetof(TAdynmemStruct, LocalHumanPlayer_PlayerID) == 0x2A42, "TAdynmemStruct::LocalHumanPlayer_PlayerID moved");
static_assert(offsetof(TAdynmemStruct, Players)                   == 0x1B63, "TAdynmemStruct::Players moved");
static_assert(sizeof(PlayerStruct)                                == 0x14B,  "PlayerStruct size changed (slot = byte offset / 0x14B)");
static_assert(offsetof(PlayerStruct, PlayerRes)                   == 0x8C,   "PlayerStruct::PlayerRes moved");
static_assert(offsetof(PlayerResourcesStruct, fCurrentEnergy)     == 0x00,   "PlayerResourcesStruct::fCurrentEnergy moved");
static_assert(offsetof(PlayerResourcesStruct, fCurrentMetal)      == 0x0C,   "PlayerResourcesStruct::fCurrentMetal moved");
static_assert(offsetof(PlayerResourcesStruct, fMaxEnergyStorage)  == 0x18,   "PlayerResourcesStruct::fMaxEnergyStorage moved");
static_assert(offsetof(PlayerResourcesStruct, fMaxMetalStorage)   == 0x1C,   "PlayerResourcesStruct::fMaxMetalStorage moved");
static_assert(offsetof(UnitStruct, Owner_PlayerPtr0)              == 0x96,   "UnitStruct::Owner_PlayerPtr0 moved");

namespace
{
	const int kSlots = 10;
	enum { kMetal = 0, kEnergy = 1 };

	// ---- TotalA.exe addresses -----------------------------------------------------------------
	// Mission handlers (both `__stdcall(UnitStruct* unit, UnitOrdersStruct* order, int wakeMask)`).
	const DWORD kGroundEntry   = 0x00405980u;   // MissionTick_RepairPatrol
	const DWORD kGroundResume  = 0x00405985u;   // after `sub esp,0x30 ; xor eax,eax`
	const DWORD kVtolEntry     = 0x004152F0u;   // MissionTick_VTOL_RepairPatrol
	const DWORD kVtolResume    = 0x004152F7u;   // after `mov cl,[esp+0xC] ; sub esp,0x44`
	// The air handler's feature search: `call 0x47EA40` followed by `test eax,eax ; je return_2`.
	const DWORD kPickerCall    = 0x0041564Fu;
	const DWORD kPicker        = 0x0047EA40u;
	// Inside the routine that registers the built-in command tables on every map load: the third
	// `push 0x501FD0 ; call registrar` pair. The registrar is at 0x4B7760.
	const DWORD kRegSite       = 0x004195D3u;
	const DWORD kRegResume     = 0x004195D8u;   // the vanilla `call registrar`
	// The two read-only doubles (0.2) the patrol handlers multiply max storage by.
	const DWORD kConstGround   = 0x004FC950u;
	const DWORD kConstAir      = 0x004FCC58u;
	// The engine's sorted command vector (12-byte records: name, handler, permission level): begin
	// and end pointers, stored at unaligned addresses.
	const DWORD kCmdVecBegin   = 0x0051FC9Du;
	const DWORD kCmdVecEnd     = 0x0051FCA1u;
	// Runtime mission-order table: a vector of 25-byte records. +4 is the handler, +0x15 a pointer to
	// the order's name.
	const DWORD kTableBegin    = 0x00512344u;
	const DWORD kTableEnd      = 0x00512348u;
	const DWORD kRecordSize    = 25u;
	const DWORD kRecordHandler = 4u;
	const DWORD kRecordName    = 0x15u;

	// The eight `fmul qword ptr [0.2]` instructions (`DC 0D <abs32>`); the operand is at +2. `energy`
	// says which max-storage load precedes it (`fld dword [reg+0xA4]` energy, `[reg+0xA8]` metal).
	struct Site { DWORD addr; DWORD constant; bool energy; };
	const Site kSites[8] =
	{
		{ 0x004059F6u, kConstGround, true  },   // ground step 1: energy low? (skip assist scan)
		{ 0x00405B2Au, kConstGround, true  },   // ground step 2: energy low?
		{ 0x00405B3Fu, kConstGround, false },   // ground step 2: metal low?
		{ 0x00405BB4u, kConstGround, false },   // ground picker: metal low?
		{ 0x00405C39u, kConstGround, true  },   // ground picker: energy low?
		{ 0x0041548Fu, kConstAir,    true  },   // air step 1: energy low?
		{ 0x00415672u, kConstAir,    false },   // air picker: metal low?
		{ 0x004156B3u, kConstAir,    true  },   // air picker: energy low?
	};

	// ---- state ---------------------------------------------------------------------------------
	// An 8-byte double moved as two integers, so no FPU or SSE state is touched on the simulation path.
	struct Frac { DWORD w[2]; };

	// What the patched fmul instructions read. Written at every patrol handler entry, read by the
	// handler's own x87 code (and by the air gate) later in the same call.
	__declspec(align(8)) Frac g_liveEnergy;
	__declspec(align(8)) Frac g_liveMetal;

	int  g_pct[2][kSlots];      // typed percentage per slot, -1 = never typed (engine default)
	Frac g_frac[2][kSlots];     // the bytes that go into g_live* for that slot
	Frac g_vanilla;             // the engine's own 0.2, captured at install

	DWORD g_groundCalls, g_vtolCalls, g_customCalls, g_unresolved, g_gateSkips;
	DWORD g_lastBeat;
	int   g_lastGameTime;
	bool  g_wasInGame;
	bool  g_installed;

	// Read by the stubs. Plain variables (not constants) so inline asm can address them.
	DWORD g_groundResume = kGroundResume;
	DWORD g_vtolResume   = kVtolResume;
	DWORD g_regResume    = kRegResume;
	DWORD g_pickerAddr   = kPicker;
	DWORD g_gateRel32;          // rel32 of the retargeted picker call: gate stub - end of call

	void Flush(DWORD addr, size_t len)
	{
		FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<LPCVOID>(addr), len);
	}

	// 0.00 .. 1.00 as compile-time literals: the stored thresholds never depend on the FPU precision
	// mode at run time. 0.20 is bit-identical to the engine's constant (checked at install).
	const double kFraction[101] =
	{
		0.00, 0.01, 0.02, 0.03, 0.04, 0.05, 0.06, 0.07, 0.08, 0.09,
		0.10, 0.11, 0.12, 0.13, 0.14, 0.15, 0.16, 0.17, 0.18, 0.19,
		0.20, 0.21, 0.22, 0.23, 0.24, 0.25, 0.26, 0.27, 0.28, 0.29,
		0.30, 0.31, 0.32, 0.33, 0.34, 0.35, 0.36, 0.37, 0.38, 0.39,
		0.40, 0.41, 0.42, 0.43, 0.44, 0.45, 0.46, 0.47, 0.48, 0.49,
		0.50, 0.51, 0.52, 0.53, 0.54, 0.55, 0.56, 0.57, 0.58, 0.59,
		0.60, 0.61, 0.62, 0.63, 0.64, 0.65, 0.66, 0.67, 0.68, 0.69,
		0.70, 0.71, 0.72, 0.73, 0.74, 0.75, 0.76, 0.77, 0.78, 0.79,
		0.80, 0.81, 0.82, 0.83, 0.84, 0.85, 0.86, 0.87, 0.88, 0.89,
		0.90, 0.91, 0.92, 0.93, 0.94, 0.95, 0.96, 0.97, 0.98, 0.99,
		1.00
	};

	void FractionBits(int percent, Frac* out)
	{
		std::memcpy(out, &kFraction[percent], sizeof(Frac));
	}

	bool SameBits(const Frac& a, const Frac& b)
	{
		return a.w[0] == b.w[0] && a.w[1] == b.w[1];
	}

	void ResetCounters()
	{
		g_groundCalls = g_vtolCalls = g_customCalls = g_unresolved = g_gateSkips = 0;
	}

	// Back to the engine default for every slot, including what the patched instructions read.
	void ResetState()
	{
		for (int r = 0; r < 2; ++r)
			for (int i = 0; i < kSlots; ++i)
			{
				g_pct[r][i]  = -1;
				g_frac[r][i] = g_vanilla;
			}
		g_liveEnergy = g_vanilla;
		g_liveMetal  = g_vanilla;
	}

	bool AnySlotCustom()
	{
		for (int r = 0; r < 2; ++r)
			for (int i = 0; i < kSlots; ++i)
				if (g_pct[r][i] >= 0)
					return true;
		return false;
	}

	// ---- input parsing -------------------------------------------------------------------------
	// Accepts "<digits>" or "<digits>%": at least one digit, at most one trailing '%', nothing else.
	// Longer digit runs saturate instead of overflowing; the result is clamped to 100.
	bool ParsePercent(const char* raw, int* out)
	{
		if (!raw)
			return false;
		const char* p = raw;
		if (*p < '0' || *p > '9')
			return false;

		long acc = 0;
		int digits = 0;
		for (; *p >= '0' && *p <= '9'; ++p, ++digits)
			acc = (digits < 9) ? acc * 10 + (*p - '0') : 1000000000L;

		if (*p == '%')
			++p;
		if (*p != '\0')
			return false;

		*out = (acc > 100) ? 100 : static_cast<int>(acc);
		return true;
	}

	// ---- owner -> slot ---------------------------------------------------------------------------
	// The owner of a unit is a pointer into TAdynmemStruct::Players; its slot is the exact multiple of
	// sizeof(PlayerStruct) from the array start. Anything else (null, outside the array, not on an
	// element boundary) yields -1 and the caller falls back to the engine default.
	int SlotOfOwner(const void* owner, const void* playersBase)
	{
		if (!owner || !playersBase)
			return -1;
		const BYTE* o = static_cast<const BYTE*>(owner);
		const BYTE* b = static_cast<const BYTE*>(playersBase);
		if (o < b)
			return -1;
		const size_t off = static_cast<size_t>(o - b);
		if (off >= kSlots * sizeof(PlayerStruct) || (off % sizeof(PlayerStruct)) != 0)
			return -1;
		return static_cast<int>(off / sizeof(PlayerStruct));
	}

	// Copies one slot's thresholds into what the patched instructions read. Two 8-byte copies per
	// resource, integer moves only.
	void SelectSlot(int slot)
	{
		if (slot < 0 || slot >= kSlots)
		{
			g_liveMetal  = g_vanilla;
			g_liveEnergy = g_vanilla;
			return;
		}
		g_liveMetal  = g_frac[kMetal][slot];
		g_liveEnergy = g_frac[kEnergy][slot];
		if (g_pct[kMetal][slot] >= 0 || g_pct[kEnergy][slot] >= 0)
			++g_customCalls;
	}

	// Runs at the entry of every patrol handler call, before any of its instructions.
	void SelectForUnit(const UnitStruct* unit, bool air)
	{
		PROFILE_SCOPE("Hook.PatrolReclaimThreshold.Select");

		++(air ? g_vtolCalls : g_groundCalls);

		int slot = -1;
		const TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
		if (unit && ta)
			slot = SlotOfOwner(unit->Owner_PlayerPtr0, &ta->Players[0]);
		if (slot < 0)
			++g_unresolved;
		SelectSlot(slot);
	}

	void __stdcall SelectGround(const UnitStruct* unit) { SelectForUnit(unit, false); }
	void __stdcall SelectVtol(const UnitStruct* unit)   { SelectForUnit(unit, true); }

	void (__stdcall* g_pSelectGround)(const UnitStruct*) = &SelectGround;
	void (__stdcall* g_pSelectVtol)(const UnitStruct*)   = &SelectVtol;

	// ---- entry stubs ---------------------------------------------------------------------------
	// Each handler's first instructions are replaced by a `jmp` to a stub that selects the owner's
	// thresholds, replays the replaced instructions and resumes. The unit is the first stack argument
	// ([esp+4] at entry, [esp+0x10] after the pushes). eax/ecx/edx are saved, the callee preserves the
	// rest, and flags are dead: the first replayed instruction (`sub esp`) writes them.
	__declspec(naked) void GroundPatrolEntry()
	{
		__asm
		{
			push eax
			push ecx
			push edx
			push dword ptr [esp + 0x10]
			call dword ptr [g_pSelectGround]
			pop edx
			pop ecx
			pop eax
			sub esp, 0x30                       // replaced: 83 EC 30
			xor eax, eax                        // replaced: 33 C0
			jmp dword ptr [g_groundResume]
		}
	}

	__declspec(naked) void VtolPatrolEntry()
	{
		__asm
		{
			push eax
			push ecx
			push edx
			push dword ptr [esp + 0x10]
			call dword ptr [g_pSelectVtol]
			pop edx
			pop ecx
			pop eax
			mov cl, byte ptr [esp + 0xC]        // replaced: 8A 4C 24 0C
			sub esp, 0x44                       // replaced: 83 EC 44
			jmp dword ptr [g_vtolResume]
		}
	}

	// ---- air gate ------------------------------------------------------------------------------
	// Target of the air handler's retargeted `call 0x47EA40` (feature search). Ground constructors skip
	// the search while energy and metal are both at or above their thresholds; this applies the same
	// rule to air. Same six stdcall arguments and `ret 0x18` as the picker; edi is the unit. The compare
	// is the ground handler's own sequence (0x405B18..0x405B54): low = threshold > current in x87,
	// false for equality and NaN.
	//   both not low -> return 0 (the picker's "nothing found"); the handler then returns 2.
	//   otherwise    -> jump to the picker with the caller's registers.
	__declspec(naked) void VtolPickerGate()
	{
		__asm
		{
			push eax
			push ecx
			mov ecx, dword ptr [edi + 0x96]     // owner
			fld dword ptr [ecx + 0x8C]          // current energy
			fld dword ptr [ecx + 0xA4]          // max energy
			fmul qword ptr [g_liveEnergy]
			fcompp                              // st0 = threshold vs st1 = current, pops both
			fnstsw ax
			test ah, 0x41
			je run_picker                       // neither C0 nor C3: threshold > current: energy low
			fld dword ptr [ecx + 0xA8]          // max metal
			fmul qword ptr [g_liveMetal]
			fld dword ptr [ecx + 0x98]          // current metal
			fxch st(1)
			fcompp
			fnstsw ax
			test ah, 0x41
			je run_picker                       // metal low
			inc dword ptr [g_gateSkips]
			pop ecx
			pop eax
			xor eax, eax
			ret 0x18
		run_picker:
			pop ecx
			pop eax
			jmp dword ptr [g_pickerAddr]
		}
	}

	// ---- commands ------------------------------------------------------------------------------
	// Opaque: TA's pre-tokenized command line, only touched through the engine's own accessor.
	struct TaTokenLine;
	typedef char* (__thiscall* TokenLine_GetArgPtr_t)(TaTokenLine* self, int index, char* fallback);
	TokenLine_GetArgPtr_t TokenLine_GetArgPtr = reinterpret_cast<TokenLine_GetArgPtr_t>(0x004B73C0u);

	bool GameRunning()
	{
		return DataShare != nullptr && DataShare->TAProgress == TAInGame;
	}

	// Local slot of a running game, range- and PlayerActive-checked (`+control` repoints
	// LocalHumanPlayer_PlayerID, so state is per slot). Unlike the engine's +setshare* handlers this
	// does not test bit 0 of TAdynmem+0x2A44: the byte read 0x0C (bit 0 clear) in a running skirmish.
	PlayerStruct* LocalPlayerOrNull(int* outSlot)
	{
		TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
		if (!ta || !GameRunning())
			return nullptr;
		const unsigned char id = static_cast<unsigned char>(ta->LocalHumanPlayer_PlayerID);
		if (id >= kSlots || !ta->Players[id].PlayerActive)
			return nullptr;
		*outSlot = id;
		return &ta->Players[id];
	}

	struct TableProbe { bool ok; DWORD ground; DWORD vtol; };

	// Reads the two patrol records from the runtime mission table. Another module (the recorder's
	// order override) can replace a record's handler; then the copy in this module never runs.
	TableProbe ProbeMissionTable()
	{
		TableProbe r = { false, 0, 0 };
		__try
		{
			const BYTE* begin = *reinterpret_cast<const BYTE* const*>(kTableBegin);
			const BYTE* end   = *reinterpret_cast<const BYTE* const*>(kTableEnd);
			if (!begin || end < begin || static_cast<size_t>(end - begin) > 4096u * kRecordSize)
				return r;
			for (const BYTE* rec = begin; rec + kRecordSize <= end; rec += kRecordSize)
			{
				const char* name = *reinterpret_cast<const char* const*>(rec + kRecordName);
				if (!name)
					continue;
				if (_stricmp(name, "RepairPatrol") == 0)
					r.ground = *reinterpret_cast<const DWORD*>(rec + kRecordHandler);
				else if (_stricmp(name, "VTOL_RepairPatrol") == 0)
					r.vtol = *reinterpret_cast<const DWORD*>(rec + kRecordHandler);
			}
			r.ok = true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			r.ok = false;
		}
		return r;
	}

	// The copy only bounds what is handed to NewChatText (191 characters).
	void Say(const char* text)
	{
		char buf[192];
		snprintf(buf, sizeof(buf), "%s", text);
		IDDrawSurface::OutptFmtTxt("[PatrolReclaimThreshold] chat: %s", buf);
		NewChatText(buf, 2, 0, 10);
	}

	void __stdcall CmdSetReclaimMetal(char* argv[]);
	void __stdcall CmdSetReclaimEnergy(char* argv[]);

	// Looks the two commands up in the engine's live command vector (12-byte records: name, handler,
	// permission level). "registered" means the name is there with this module's handler.
	struct CommandProbe { bool ok; bool metal; bool energy; bool foreignMetal; bool foreignEnergy; int total; };
	CommandProbe ProbeCommands()
	{
		CommandProbe r = { false, false, false, false, false, 0 };
		__try
		{
			const BYTE* begin = *reinterpret_cast<const BYTE* const*>(kCmdVecBegin);
			const BYTE* end   = *reinterpret_cast<const BYTE* const*>(kCmdVecEnd);
			if (!begin || end < begin || static_cast<size_t>(end - begin) > 4096u * 12u)
				return r;
			for (const BYTE* rec = begin; rec + 12 <= end; rec += 12)
			{
				++r.total;
				const char* name = *reinterpret_cast<const char* const*>(rec);
				const DWORD handler = *reinterpret_cast<const DWORD*>(rec + 4);
				if (!name)
					continue;
				if (_stricmp(name, "setreclaimmetal") == 0)
				{
					if (handler == reinterpret_cast<DWORD>(&CmdSetReclaimMetal)) r.metal = true; else r.foreignMetal = true;
				}
				else if (_stricmp(name, "setreclaimenergy") == 0)
				{
					if (handler == reinterpret_cast<DWORD>(&CmdSetReclaimEnergy)) r.energy = true; else r.foreignEnergy = true;
				}
			}
			r.ok = true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			r.ok = false;
		}
		return r;
	}

	void LogCommandProbe()
	{
		const CommandProbe c = ProbeCommands();
		if (!c.ok)
		{
			IDDrawSurface::OutptTxt("[PatrolReclaimThreshold] commands: engine command table unreadable");
			return;
		}
		IDDrawSurface::OutptFmtTxt("[PatrolReclaimThreshold] commands: setreclaimmetal=%s setreclaimenergy=%s; %d engine commands",
			c.metal ? "registered" : (c.foreignMetal ? "FOREIGN HANDLER" : "MISSING"),
			c.energy ? "registered" : (c.foreignEnergy ? "FOREIGN HANDLER" : "MISSING"), c.total);
	}

	int ShownPercent(int resource, int slot)
	{
		return g_pct[resource][slot] >= 0 ? g_pct[resource][slot] : 20;
	}

	void ReportThresholds(int slot)
	{
		char msg[128];
		snprintf(msg, sizeof(msg), "Patrol reclaim: metal below %d%% of storage, energy below %d%%.",
			ShownPercent(kMetal, slot), ShownPercent(kEnergy, slot));
		Say(msg);

		const TableProbe t = ProbeMissionTable();
		if (t.ok && t.ground != kGroundEntry)
			Say("Ground patrol is handled by another module: these thresholds do not apply to it.");
		if (t.ok && t.vtol != kVtolEntry)
			Say("Air patrol is handled by another module: these thresholds do not apply to it.");
	}

	// Stores one slot's percentage and the matching 8-byte fraction.
	void ApplyPercent(int slot, int resource, int percent)
	{
		g_pct[resource][slot] = percent;
		FractionBits(percent, &g_frac[resource][slot]);
	}

	void HandleSetReclaim(TaTokenLine* line, int resource)
	{
		if (!GameRunning())
		{
			Say("Patrol reclaim: only available while a game is running, nothing changed.");
			return;
		}
		int slot = -1;
		if (!LocalPlayerOrNull(&slot))
		{
			Say("Patrol reclaim: no local player right now, nothing changed.");
			return;
		}

		if (DataShare && DataShare->PlayingDemo)
		{
			Say("Patrol reclaim thresholds have no effect while watching a replay.");
			return;
		}

		const char* raw = TokenLine_GetArgPtr(line, /*index=*/1, /*fallback=*/const_cast<char*>(""));
		if (!raw || !*raw)
		{
			ReportThresholds(slot);
			return;
		}

		int percent = 0;
		if (!ParsePercent(raw, &percent))
		{
			char msg[160];
			snprintf(msg, sizeof(msg),
				"Usage: +setreclaim%s <0-100>[%%]  (patrols reclaim while %s is below that share of storage; now %d%%)",
				resource == kMetal ? "metal" : "energy", resource == kMetal ? "metal" : "energy",
				ShownPercent(resource, slot));
			Say(msg);
			if (resource == kEnergy)
				Say("Energy also decides when patrols assist: they assist only while energy is at or above it.");
			return;
		}

		ApplyPercent(slot, resource, percent);
		ReportThresholds(slot);
	}

	// File_DispatchNamedCommand pushes one token-line pointer; the argument array is that same object.
	void __stdcall CmdSetReclaimMetal(char* argv[])  { HandleSetReclaim(reinterpret_cast<TaTokenLine*>(argv), kMetal); }
	void __stdcall CmdSetReclaimEnergy(char* argv[]) { HandleSetReclaim(reinterpret_cast<TaTokenLine*>(argv), kEnergy); }

	// The registrar keeps the name pointers and the table entries' contents, so this must stay alive.
	InternalCommandTableEntryStruct g_commandTable[] =
	{
		{ "setreclaimmetal",  &CmdSetReclaimMetal,  InternalCommandRunLevel::CMD_LEVEL_NORMAL },
		{ "setreclaimenergy", &CmdSetReclaimEnergy, InternalCommandRunLevel::CMD_LEVEL_NORMAL },
		{ NULL, NULL, InternalCommandRunLevel::CMD_LEVEL_NULL }
	};

	// Runs before the vanilla `push 0x501FD0 ; call registrar`: registers this module's table, replays
	// the push and resumes. eax/ecx/edx are dead here (the registrar writes them before reading); flags
	// are dead across the call.
	__declspec(naked) void RegisterCommandsStub()
	{
		__asm
		{
			push offset g_commandTable
			call dword ptr [InitInternalCommand]
			push 0x00501FD0                     // replaced: 68 D0 1F 50 00
			jmp dword ptr [g_regResume]
		}
	}

	// ---- game-start bookkeeping ------------------------------------------------------------------
	void LogGameStart()
	{
		LogCommandProbe();
		const TableProbe t = ProbeMissionTable();
		if (!t.ok)
		{
			IDDrawSurface::OutptTxt("[PatrolReclaimThreshold] game start: thresholds reset; mission table unreadable");
			return;
		}
		IDDrawSurface::OutptFmtTxt("[PatrolReclaimThreshold] game start: thresholds reset; ground=0x%08X (%s) vtol=0x%08X (%s)",
			t.ground, t.ground == kGroundEntry ? "vanilla" : "REPLACED",
			t.vtol,   t.vtol   == kVtolEntry   ? "vanilla" : "REPLACED");
	}

	void MaybeHeartbeat()
	{
		if (!AnySlotCustom())
			return;
		const DWORD now = GetTickCount();
		if (now - g_lastBeat < 60000u)
			return;
		g_lastBeat = now;
		IDDrawSurface::OutptFmtTxt("[PatrolReclaimThreshold] [heartbeat] ground=%lu vtol=%lu custom=%lu unresolved=%lu gateSkips=%lu",
			g_groundCalls, g_vtolCalls, g_customCalls, g_unresolved, g_gateSkips);
	}

	void OnGameTick(int gameTime)
	{
		// GameTickHook fires several times per game tick with an unchanged gameTime.
		if (gameTime == g_lastGameTime)
			return;

		// A game time at or below the last one seen means a new game started; the in-game edge below
		// catches the same event from the other side.
		bool newGame = gameTime < g_lastGameTime;
		g_lastGameTime = gameTime;

		const bool inGame = (DataShare != nullptr) && (DataShare->TAProgress == TAInGame);
		if (inGame && !g_wasInGame)
			newGame = true;
		g_wasInGame = inGame;

		if (newGame)
		{
			ResetState();
			LogGameStart();
		}
		MaybeHeartbeat();
	}

	// ---- install-time checks ---------------------------------------------------------------------
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
		IDDrawSurface::OutptFmtTxt("[PatrolReclaimThreshold] DISABLED: unexpected TotalA.exe bytes at 0x%08X (%s)", addr, what);
		return false;
	}

	// Site = `fld dword [reg+max] ; fmul qword [constant]`: the load's register is ecx (0x81) or eax (0x80).
	bool SiteIsExpected(const Site& s)
	{
		BYTE b[12];
		if (!ReadBytes(s.addr - 6, b, sizeof(b)))
			return false;
		const bool loadOk = b[0] == 0xD9 && (b[1] == 0x81 || b[1] == 0x80) && b[2] == (s.energy ? 0xA4 : 0xA8) &&
			b[3] == 0 && b[4] == 0 && b[5] == 0;
		DWORD operand = 0;
		std::memcpy(&operand, b + 8, 4);
		return loadOk && b[6] == 0xDC && b[7] == 0x0D && operand == s.constant;
	}

	bool VerifyPremises()
	{
		static const BYTE kZeroPointTwo[8]  = { 0x9A, 0x99, 0x99, 0x99, 0x99, 0x99, 0xC9, 0x3F };
		static const BYTE kGroundHead[5]    = { 0x83, 0xEC, 0x30, 0x33, 0xC0 };
		static const BYTE kVtolHead[7]      = { 0x8A, 0x4C, 0x24, 0x0C, 0x83, 0xEC, 0x44 };
		// Argument setup and call to the picker, then the miss branch, then the miss target.
		static const BYTE kPickerArgs[12]   = { 0xB8, 0x00, 0x00, 0xF0, 0x00, 0x52, 0x51, 0x8D, 0x57, 0x6A, 0x50, 0x52 };
		static const BYTE kPickerCallAndMiss[13] =
			{ 0xE8, 0xEC, 0x93, 0x06, 0x00, 0x85, 0xC0, 0x0F, 0x84, 0x61, 0x01, 0x00, 0x00 };
		static const BYTE kReturn2[5]       = { 0xB8, 0x02, 0x00, 0x00, 0x00 };
		static const BYTE kLoadUnitInEdi[4] = { 0x8B, 0x7C, 0x24, 0x58 };            // mov edi,[esp+0x58]
		static const BYTE kOwnerFromEdi[6]  = { 0x8B, 0x8F, 0x96, 0x00, 0x00, 0x00 };  // mov ecx,[edi+0x96]
		static const BYTE kRegPair[10]      = { 0x68, 0xD0, 0x1F, 0x50, 0x00, 0xE8, 0x83, 0xE1, 0x09, 0x00 };
		// The ground handler's own "energy low / metal low" step (0x405B18..0x405B5A), which the air
		// gate copies. Checked from its second instruction: the first, `mov ecx,[esi+0x96]`, is where
		// the assist-only patrol mode may already have placed its own jump.
		static const BYTE kGroundStep2[60] =
		{
			0xD9, 0x81, 0x8C, 0x00, 0x00, 0x00, 0xD9, 0x81, 0xA4, 0x00, 0x00, 0x00, 0xDC, 0x0D, 0x50, 0xC9,
			0x4F, 0x00, 0xDE, 0xD9, 0xDF, 0xE0, 0xF6, 0xC4, 0x41, 0x74, 0x21, 0xD9, 0x81, 0xA8, 0x00, 0x00,
			0x00, 0xDC, 0x0D, 0x50, 0xC9, 0x4F, 0x00, 0xD9, 0x81, 0x98, 0x00, 0x00, 0x00, 0xD9, 0xC9, 0xDE,
			0xD9, 0xDF, 0xE0, 0xF6, 0xC4, 0x41, 0x0F, 0x85, 0xF0, 0x01, 0x00, 0x00
		};

		if (!Expect(kConstGround, kZeroPointTwo, 8, "ground 0.2 constant") ||
			!Expect(kConstAir,    kZeroPointTwo, 8, "air 0.2 constant"))
			return false;
		for (const Site& s : kSites)
			if (!SiteIsExpected(s))
			{
				IDDrawSurface::OutptFmtTxt("[PatrolReclaimThreshold] DISABLED: unexpected TotalA.exe bytes at 0x%08X (0.2 multiply site)", s.addr);
				return false;
			}
		return Expect(kGroundEntry, kGroundHead, sizeof(kGroundHead), "ground patrol entry") &&
			Expect(kVtolEntry, kVtolHead, sizeof(kVtolHead), "air patrol entry") &&
			Expect(0x00405B1Eu, kGroundStep2, sizeof(kGroundStep2), "ground energy/metal-low step") &&
			Expect(0x00415643u, kPickerArgs, sizeof(kPickerArgs), "air picker arguments") &&
			Expect(kPickerCall, kPickerCallAndMiss, sizeof(kPickerCallAndMiss), "air picker call") &&
			Expect(0x004157BDu, kReturn2, sizeof(kReturn2), "air handler return 2") &&
			Expect(0x00415358u, kLoadUnitInEdi, sizeof(kLoadUnitInEdi), "air handler loads unit into edi") &&
			Expect(0x0041565Cu, kOwnerFromEdi, sizeof(kOwnerFromEdi), "air handler reads owner through edi") &&
			Expect(kRegSite, kRegPair, sizeof(kRegPair), "command table registration");
	}

	// ---- self-test -------------------------------------------------------------------------------
	// Runs before anything is patched. Nothing here touches engine code: the stubs are exercised with
	// their continuation addresses pointed at local test routines.
	__declspec(naked) void TestPicker()             // stands in for the real picker: proves "run"
	{
		__asm
		{
			mov eax, 0x7777
			ret 0x18
		}
	}

	__declspec(naked) void TestResumeGround()       // ground handler continuation stand-in
	{
		__asm
		{
			add esp, 0x30                       // undo the replayed sub esp,0x30
			ret 0x0C
		}
	}

	__declspec(naked) void TestResumeVtol()
	{
		__asm
		{
			add esp, 0x44
			mov eax, ecx                        // proves the replayed `mov cl,[esp+0xC]` ran: cl = wake mask
			ret 0x0C
		}
	}

	void* g_pGate        = reinterpret_cast<void*>(&VtolPickerGate);
	void* g_pGroundEntry = reinterpret_cast<void*>(&GroundPatrolEntry);
	void* g_pVtolEntry   = reinterpret_cast<void*>(&VtolPatrolEntry);

	// Calls the gate like the air handler does: edi = unit, six stdcall arguments.
	__declspec(naked) DWORD __stdcall CallGate(const void* unit)
	{
		__asm
		{
			push edi
			mov edi, dword ptr [esp + 8]
			push 0
			push 0
			push 0
			push 0
			push 0
			push 0
			call dword ptr [g_pGate]
			pop edi
			ret 4
		}
	}

	// Calls an entry stub like the mission dispatcher does: three stdcall arguments (unit, order,
	// wake mask), then reports the stub's result. eax/ecx/edx are set to known values first so the
	// preservation can be checked by the caller.
	__declspec(naked) DWORD __stdcall CallEntry(void* stub, const void* unit, DWORD mask)
	{
		__asm
		{
			push ebx
			push esi
			push edi
			push ebp
			mov ebx, 0x11111111
			mov esi, 0x22222222
			mov edi, 0x33333333
			mov ebp, 0x44444444
			mov ecx, 0x55555555
			mov edx, 0x66666666
			push dword ptr [esp + 0x1C]         // wake mask (arguments sit above the four saved registers)
			push 0                              // order
			push dword ptr [esp + 0x20]         // unit
			call dword ptr [esp + 0x20]         // stub pointer
			cmp ebx, 0x11111111
			jne bad
			cmp esi, 0x22222222
			jne bad
			cmp edi, 0x33333333
			jne bad
			cmp ebp, 0x44444444
			jne bad
			pop ebp
			pop edi
			pop esi
			pop ebx
			ret 0x0C
		bad:
			pop ebp
			pop edi
			pop esi
			pop ebx
			mov eax, 0xDEADBEEF
			ret 0x0C
		}
	}

	WORD FpuTop()
	{
		WORD sw = 0;
		__asm { fnstsw sw }
		return static_cast<WORD>((sw >> 11) & 7);
	}

	struct GateCase
	{
		const char* name;
		int energyPct, metalPct;
		float curE, maxE, curM, maxM;
		DWORD expect;      // 0 = search skipped, 0x7777 = search runs
	};

	bool RunSelfTest()
	{
		char why[96];
		why[0] = '\0';
		bool ok = true;
		#define PRT_FAIL(text) do { ok = false; snprintf(why, sizeof(why), "%s", text); goto done; } while (0)

		// 1. parser
		{
			struct { const char* in; bool valid; int value; } cases[] =
			{
				{ "",             false, 0 },   { "abc",   false, 0 },   { "-5",  false, 0 },   { "%",   false, 0 },
				{ "50%%",         false, 0 },   { "5 ",    false, 0 },   { "%50", false, 0 },   { "1.5", false, 0 },
				{ "0",            true,  0 },   { "0%",    true,  0 },   { "20",  true, 20 },   { "020", true, 20 },
				{ "85%",          true, 85 },   { "100",   true, 100 },  { "101", true, 100 },
				{ "999999999999", true, 100 },  { "100%",  true, 100 },
			};
			for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i)
			{
				int v = -7;
				const bool r = ParsePercent(cases[i].in, &v);
				if (r != cases[i].valid || (r && v != cases[i].value))
					PRT_FAIL("parser");
			}
			int untouched = 1234;
			if (ParsePercent("abc", &untouched) || untouched != 1234)
				PRT_FAIL("parser leaves output alone on failure");
		}

		// 2. fraction table: default equals the engine constant bit for bit, endpoints exact, strictly increasing
		{
			Frac engineGround, engineAir, f20, f0, f50, f100;
			std::memcpy(&engineGround, reinterpret_cast<const void*>(kConstGround), sizeof(Frac));
			std::memcpy(&engineAir,    reinterpret_cast<const void*>(kConstAir),    sizeof(Frac));
			FractionBits(20, &f20);  FractionBits(0, &f0);  FractionBits(50, &f50);  FractionBits(100, &f100);
			if (!SameBits(engineGround, engineAir))
				PRT_FAIL("engine ground and air constants differ");
			if (!SameBits(f20, engineGround))
				PRT_FAIL("20% is not bit-identical to the engine constant");
			if (f0.w[0] != 0 || f0.w[1] != 0 || f50.w[0] != 0 || f50.w[1] != 0x3FE00000u ||
				f100.w[0] != 0 || f100.w[1] != 0x3FF00000u)
				PRT_FAIL("0/50/100% bits");
			for (int i = 0; i < 100; ++i)
			{
				Frac a, b;
				FractionBits(i, &a);  FractionBits(i + 1, &b);
				// positive doubles order like their bit patterns
				if (!(a.w[1] < b.w[1] || (a.w[1] == b.w[1] && a.w[0] < b.w[0])))
					PRT_FAIL("fraction table not strictly increasing");
				if (kFraction[i] * 100.0 < i - 1e-4 || kFraction[i] * 100.0 > i + 1e-4)
					PRT_FAIL("fraction table value");
			}
			g_vanilla = engineGround;
		}

		// 3. owner -> slot
		{
			static PlayerStruct players[kSlots];
			const void* base = &players[0];
			for (int i = 0; i < kSlots; ++i)
				if (SlotOfOwner(&players[i], base) != i)
					PRT_FAIL("slot of each player");
			if (SlotOfOwner(reinterpret_cast<const BYTE*>(&players[3]) + 1, base) != -1) PRT_FAIL("misaligned owner");
			if (SlotOfOwner(reinterpret_cast<const BYTE*>(&players[0]) - 1, base) != -1) PRT_FAIL("owner before array");
			if (SlotOfOwner(&players[0] + kSlots, base) != -1)                           PRT_FAIL("owner after array");
			if (SlotOfOwner(nullptr, base) != -1 || SlotOfOwner(&players[1], nullptr) != -1) PRT_FAIL("null owner or base");
		}

		// 4. slot selection
		{
			ResetState();
			FractionBits(85, &g_frac[kMetal][3]);   g_pct[kMetal][3] = 85;
			FractionBits(60, &g_frac[kEnergy][3]);  g_pct[kEnergy][3] = 60;
			Frac m85, e60;
			FractionBits(85, &m85);  FractionBits(60, &e60);
			SelectSlot(3);
			if (!SameBits(g_liveMetal, m85) || !SameBits(g_liveEnergy, e60))  PRT_FAIL("slot 3 selection");
			SelectSlot(4);
			if (!SameBits(g_liveMetal, g_vanilla) || !SameBits(g_liveEnergy, g_vanilla))  PRT_FAIL("untouched slot keeps the default");
			SelectSlot(-1);
			if (!SameBits(g_liveMetal, g_vanilla) || !SameBits(g_liveEnergy, g_vanilla))  PRT_FAIL("unresolved slot falls back to the default");
			SelectSlot(kSlots);
			if (!SameBits(g_liveMetal, g_vanilla))  PRT_FAIL("out-of-range slot falls back to the default");
			ResetState();
		}

		// 5. the air gate stub, executed against synthetic owners
		{
			static PlayerStruct owner;
			static BYTE unit[0x200];
			PlayerStruct* op = &owner;
			std::memcpy(unit + 0x96, &op, sizeof(op));
			const DWORD savedPicker = g_pickerAddr;
			g_pickerAddr = reinterpret_cast<DWORD>(&TestPicker);

			const GateCase cases[] =
			{
				{ "both above",               50, 50, 60.f, 100.f, 60.f, 100.f, 0      },
				{ "energy below",             50, 50, 40.f, 100.f, 60.f, 100.f, 0x7777 },
				{ "metal below",              50, 50, 60.f, 100.f, 40.f, 100.f, 0x7777 },
				{ "both below",               50, 50, 40.f, 100.f, 40.f, 100.f, 0x7777 },
				{ "energy exactly at",        50, 50, 50.f, 100.f, 60.f, 100.f, 0      },
				{ "metal exactly at",         50, 50, 60.f, 100.f, 50.f, 100.f, 0      },
				{ "0% never low, empty",       0,  0,  0.f, 100.f,  0.f, 100.f, 0      },
				{ "100% low unless full",    100, 100, 99.f, 100.f, 100.f, 100.f, 0x7777 },
				{ "100% both full",          100, 100, 100.f, 100.f, 100.f, 100.f, 0     },
				{ "energy 0%, metal 100%",     0, 100, 90.f, 100.f, 90.f, 100.f, 0x7777 },
			};
			for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i)
			{
				const GateCase& c = cases[i];
				FractionBits(c.energyPct, &g_liveEnergy);
				FractionBits(c.metalPct,  &g_liveMetal);
				owner.PlayerRes.fCurrentEnergy = c.curE;  owner.PlayerRes.fMaxEnergyStorage = c.maxE;
				owner.PlayerRes.fCurrentMetal  = c.curM;  owner.PlayerRes.fMaxMetalStorage  = c.maxM;
				const WORD top = FpuTop();
				const DWORD got = CallGate(unit);
				if (got != c.expect || FpuTop() != top)
				{
					g_pickerAddr = savedPicker;
					snprintf(why, sizeof(why), "air gate case '%s'", c.name);
					ok = false;
					goto done;
				}
			}
			g_pickerAddr = savedPicker;
		}

		// 6. entry stubs: stack, registers and replayed instructions, continuing into a stand-in
		{
			static BYTE unit[0x200];
			const DWORD savedG = g_groundResume, savedV = g_vtolResume;
			g_groundResume = reinterpret_cast<DWORD>(&TestResumeGround);
			g_vtolResume   = reinterpret_cast<DWORD>(&TestResumeVtol);
			const DWORD g = CallEntry(g_pGroundEntry, unit, 0);
			// the ground handler zeroes eax with the replayed xor; the stand-in returns it untouched
			const DWORD v = CallEntry(g_pVtolEntry, unit, 0x48);
			g_groundResume = savedG;
			g_vtolResume   = savedV;
			if (g != 0)                              PRT_FAIL("ground entry stub");
			if ((v & 0xFF) != 0x48)                  PRT_FAIL("air entry stub");
		}

	done:
		#undef PRT_FAIL
		ResetState();
		if (!ok)
			IDDrawSurface::OutptFmtTxt("[PatrolReclaimThreshold] DISABLED: self-test failed: %s", why);
		return ok;
	}

	// ---- patch bookkeeping ------------------------------------------------------------------------
	// SingleHook keeps the pointer to its replacement bytes, so they live here.
	DWORD g_operandNew[8];
	SingleHook* g_operandHooks[8];
	SingleHook* g_gateHook;
	SingleHook* g_groundHook;
	SingleHook* g_vtolHook;
	SingleHook* g_regHook;

	void RemoveAllHooks()
	{
		// Reverse order of installation; each destructor writes the original bytes back.
		delete g_regHook;    g_regHook = nullptr;
		delete g_vtolHook;   g_vtolHook = nullptr;
		delete g_groundHook; g_groundHook = nullptr;
		delete g_gateHook;   g_gateHook = nullptr;
		for (int i = 7; i >= 0; --i)
		{
			delete g_operandHooks[i];
			g_operandHooks[i] = nullptr;
		}
		Flush(0x00405000u, 0x1000u);
		Flush(0x00415000u, 0x1000u);
		Flush(0x00419000u, 0x1000u);
	}

	// Compares memory against what the hooks were asked to write; SingleHook::Hook() reports nothing.
	bool PatchesReadBack()
	{
		BYTE b[8];
		for (int i = 0; i < 8; ++i)
		{
			DWORD v = 0;
			if (!ReadBytes(kSites[i].addr + 2, b, 4)) return false;
			std::memcpy(&v, b, 4);
			if (v != g_operandNew[i]) return false;
		}
		struct Jmp { DWORD addr; DWORD target; size_t len; };
		const Jmp jmps[] =
		{
			{ kGroundEntry, reinterpret_cast<DWORD>(&GroundPatrolEntry),    5 },
			{ kVtolEntry,   reinterpret_cast<DWORD>(&VtolPatrolEntry),      7 },
			{ kRegSite,     reinterpret_cast<DWORD>(&RegisterCommandsStub), 5 },
		};
		for (const Jmp& j : jmps)
		{
			BYTE code[8];
			if (!ReadBytes(j.addr, code, j.len)) return false;
			DWORD rel = 0;
			std::memcpy(&rel, code + 1, 4);
			if (code[0] != 0xE9 || rel != j.target - (j.addr + 5)) return false;
			for (size_t k = 5; k < j.len; ++k)
				if (code[k] != 0x90) return false;
		}
#if PATROL_RECLAIM_AIR_GATE_ENABLE
		{
			DWORD rel = 0;
			if (!ReadBytes(kPickerCall + 1, b, 4)) return false;
			std::memcpy(&rel, b, 4);
			if (rel != g_gateRel32) return false;
		}
#endif
		return true;
	}
}

namespace PatrolReclaimThreshold
{
	void Install()
	{
		if (g_installed)
			return;

		if (!VerifyPremises())
			return;                        // logged inside; refuse to patch an unexpected build

		std::memcpy(&g_vanilla, reinterpret_cast<const void*>(kConstGround), sizeof(Frac));
		if (!RunSelfTest())
			return;                        // logged inside

		ResetState();                      // before anything is repointed: the operands read valid defaults
		ResetCounters();
		g_lastGameTime = 0;
		g_wasInGame = false;
		g_lastBeat = GetTickCount();

		for (int i = 0; i < 8; ++i)
			g_operandNew[i] = reinterpret_cast<DWORD>(kSites[i].energy ? &g_liveEnergy : &g_liveMetal);
		g_gateRel32 = reinterpret_cast<DWORD>(&VtolPickerGate) - (kPickerCall + 5);

		// Order matters only for consistency while patching: operands and gate first (they read the
		// defaults just set), entry splices next, command registration last.
		for (int i = 0; i < 8; ++i)
		{
			g_operandHooks[i] = new SingleHook(kSites[i].addr + 2, 4, INLINE_UNPROTECTEVINMENT,
				reinterpret_cast<LPBYTE>(&g_operandNew[i]));
			Flush(kSites[i].addr + 2, 4);
		}
#if PATROL_RECLAIM_AIR_GATE_ENABLE
		g_gateHook = new SingleHook(kPickerCall + 1, 4, INLINE_UNPROTECTEVINMENT, reinterpret_cast<LPBYTE>(&g_gateRel32));
		Flush(kPickerCall + 1, 4);
#endif
		g_groundHook = new SingleHook(kGroundEntry, 5, INLINE_SINGLEJMP, reinterpret_cast<LPBYTE>(&GroundPatrolEntry));
		Flush(kGroundEntry, 5);
		g_vtolHook = new SingleHook(kVtolEntry, 7, INLINE_SINGLEJMP, reinterpret_cast<LPBYTE>(&VtolPatrolEntry));
		Flush(kVtolEntry, 7);
		g_regHook = new SingleHook(kRegSite, 5, INLINE_SINGLEJMP, reinterpret_cast<LPBYTE>(&RegisterCommandsStub));
		Flush(kRegSite, 5);

		if (!PatchesReadBack())
		{
			RemoveAllHooks();
			ResetState();
			IDDrawSurface::OutptTxt("[PatrolReclaimThreshold] DISABLED: patched bytes did not read back; everything restored");
			return;
		}

		GameTickHook::GetInstance()->addCallback(OnGameTick);
		g_installed = true;

		IDDrawSurface::OutptFmtTxt(
			"[PatrolReclaimThreshold] installed: 8 operand sites, entries @0x%08X/0x%08X, commands @0x%08X, air gate %s",
			kGroundEntry, kVtolEntry, kRegSite, PATROL_RECLAIM_AIR_GATE_ENABLE ? "on" : "off");
	}

	void Shutdown()
	{
		if (!g_installed)
			return;
		RemoveAllHooks();
		ResetState();
		g_installed = false;
	}
}

#endif // PATROL_RECLAIM_THRESHOLD_ENABLE
