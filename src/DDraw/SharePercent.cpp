#include "SharePercent.h"

#if SHARE_PERCENT_ENABLE

#include "GameTickHook.h"
#include "hook/hook.h"
#include "iddrawsurface.h"
#include "tafunctions.h"
#include "tamem.h"

#include <cstddef>
#include <cstdio>
#include <cstring>

// tamem.h is #pragma pack(1); these anchor the offsets this module depends
// on. If any of these fire, tamem.h moved -- re-derive from a fresh
// disassembly before touching anything below.
static_assert(offsetof(TAdynmemStruct, LocalHumanPlayer_PlayerID) == 0x2A42, "TAdynmemStruct::LocalHumanPlayer_PlayerID moved");
static_assert(offsetof(TAdynmemStruct, WorkStatusMask)            == 0x2A44, "TAdynmemStruct::WorkStatusMask moved");
static_assert(offsetof(TAdynmemStruct, Players)                   == 0x1B63, "TAdynmemStruct::Players moved");
static_assert(offsetof(PlayerStruct, PlayerRes)                   == 0x8C,   "PlayerStruct::PlayerRes moved");
static_assert(offsetof(PlayerStruct, ShareMetal)                  == 0xE4,   "PlayerStruct::ShareMetal moved");
static_assert(offsetof(PlayerStruct, ShareEnergy)                 == 0xE8,   "PlayerStruct::ShareEnergy moved");
static_assert(offsetof(PlayerResourcesStruct, fMaxEnergyStorage)  == 0x18,   "PlayerResourcesStruct::fMaxEnergyStorage moved");
static_assert(offsetof(PlayerResourcesStruct, fMaxMetalStorage)   == 0x1C,   "PlayerResourcesStruct::fMaxMetalStorage moved");

namespace
{
	// Cmd_SetShareMetalThreshold / Cmd_SetShareEnergyThreshold.
	const DWORD kSetShareMetalAddr  = 0x00419340u;
	const DWORD kSetShareEnergyAddr = 0x00419400u;

	// Both prologues are `mov ecx, [0x511DE8]` -- 6 bytes exactly; a 5-byte
	// splice would land mid-instruction.
	const size_t kHookLen = 6;
	const BYTE kExpectedPrologue[kHookLen] = { 0x8B, 0x0D, 0xE8, 0x1D, 0x51, 0x00 };

	// Neither handler has a direct call/jmp reference anywhere in .text --
	// both are reached only through File_DispatchNamedCommand's function
	// table, and no reference lands inside the 6 bytes being replaced. So a
	// plain INLINE_SINGLEJMP splice here needs no trampoline/relocator and
	// nothing can land mid-instruction.
	bool HasExpectedBytes(DWORD address, const BYTE* expected, size_t length)
	{
		if (std::memcmp(reinterpret_cast<const void*>(address), expected, length) == 0)
			return true;

		IDDrawSurface::OutptFmtTxt(
			"[SharePercent] disabled: unexpected TotalA.exe bytes at 0x%08X", address);
		return false;
	}

	// Opaque: TA's pre-tokenized command line. Only touched through the two
	// accessors below.
	struct TaTokenLine;

	// File_TokenLine_GetArgPtr @0x004B73C0 -- bounds-checked raw argument text.
	typedef char* (__thiscall* TokenLine_GetArgPtr_t)(TaTokenLine* self, int index, char* fallback);
	TokenLine_GetArgPtr_t TokenLine_GetArgPtr = reinterpret_cast<TokenLine_GetArgPtr_t>(0x004B73C0u);

	// File_TokenLine_AtoiAt @0x004B73E0 -- same call vanilla's own handlers use.
	typedef int (__thiscall* TokenLine_AtoiAt_t)(TaTokenLine* self, int index, int defaultValue);
	TokenLine_AtoiAt_t TokenLine_AtoiAt = reinterpret_cast<TokenLine_AtoiAt_t>(0x004B73E0u);

	// -1 = absolute mode, 0..100 = armed percentage. Per PLAYER SLOT rather
	// than a bare pair of globals: `+control` repoints LocalHumanPlayer_
	// PlayerID without resetting anything, so a single pair of globals would
	// leak one player's armed percentage onto whichever slot was last local.
	// Indexing by slot keeps each seat's arm tied to that seat's own storage.
	int g_metalPercent[10];
	int g_energyPercent[10];

	int g_lastGameTime = 0;
	bool g_wasInGame = false;   // edge-detects the (lobby/loading) -> TAInGame transition

	SingleHook* g_metalHook = nullptr;
	SingleHook* g_energyHook = nullptr;

	void ResetPercentModes()
	{
		for (int i = 0; i < 10; ++i)
		{
			g_metalPercent[i] = -1;
			g_energyPercent[i] = -1;
		}
	}

	int ClampPercent(long v)
	{
		if (v < 0)   return 0;
		if (v > 100) return 100;
		return static_cast<int>(v);
	}

	// Clamps to a range safely castable to long, for display only.
	long SafeLongForDisplay(float v)
	{
		const float kLimit = 2000000000.0f;   // comfortably inside int32 range
		if (v > kLimit)  return static_cast<long>(kLimit);
		if (v < -kLimit) return static_cast<long>(-kLimit);
		return static_cast<long>(v);
	}

	// Accepts only "<digits>%": at least one digit, exactly one trailing '%',
	// nothing else. Anything ambiguous (bare integer, "%", "50%%", "abc%")
	// falls through to vanilla atoi() instead of being half-interpreted.
	// Digit count capped at 9 so the accumulator can't overflow before
	// ClampPercent runs.
	bool ParsePercent(const char* raw, int* outPercent)
	{
		if (!raw || !*raw)
			return false;

		const char* p = raw;
		if (*p < '0' || *p > '9')
			return false;

		long acc = 0;
		int digitCount = 0;
		while (*p >= '0' && *p <= '9')
		{
			if (digitCount < 9)
				acc = acc * 10 + (*p - '0');
			else
				acc = 1000000000L;   // saturate; ClampPercent bounds it to 100 regardless
			++digitCount;
			++p;
		}

		if (*p != '%' || *(p + 1) != '\0')
			return false;

		*outPercent = ClampPercent(acc);
		return true;
	}

	// threshold = pct% of maxStorage; maxStorage <= 0 (including NaN) yields 0.
	float DerivePercentValue(float maxStorage, int percent)
	{
		if (!(maxStorage > 0.0f))
			return 0.0f;

		float value = maxStorage * (static_cast<float>(percent) / 100.0f);
		if (value > maxStorage)   // defensive; unreachable given percent <= 100
			value = maxStorage;
		if (value < 0.0f)         // defensive; unreachable given percent >= 0
			value = 0.0f;
		return value;
	}

	// Bounds-checked "local player right now" accessor. LocalHumanPlayer_
	// PlayerID has no range check in vanilla; cast to unsigned char before
	// comparing so a sentinel like 0xFF can't slip past as -1 and index
	// Players[-1]. `outId`, when given, receives the resolved slot index.
	PlayerStruct* LocalPlayerOrNull(int* outId = nullptr)
	{
		TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
		if (!ta)
			return nullptr;

		// Vanilla's own first gate, reproduced verbatim.
		if ((ta->WorkStatusMask & 1) == 0)
			return nullptr;

		unsigned char id = static_cast<unsigned char>(ta->LocalHumanPlayer_PlayerID);
		if (id > 9)
			return nullptr;

		PlayerStruct* me = &ta->Players[id];
		if (!me->PlayerActive)
			return nullptr;

		if (outId)
			*outId = id;
		return me;
	}

	// Shared implementation for both replacement handlers; `isMetal` selects
	// which field/format string to use.
	void HandleSetShareThreshold(TaTokenLine* line, bool isMetal)
	{
		int id = -1;
		PlayerStruct* me = LocalPlayerOrNull(&id);
		if (!me)
			return;

		const float maxStorage = isMetal ? me->PlayerRes.fMaxMetalStorage
		                                  : me->PlayerRes.fMaxEnergyStorage;
		const char* resourceName = isMetal ? "metal" : "energy";
		// Arm the slot the command was typed as, not a bare global -- see
		// the state comment above.
		int* pct = isMetal ? &g_metalPercent[id] : &g_energyPercent[id];

		const char* raw = TokenLine_GetArgPtr(line, /*index=*/1, /*fallback=*/const_cast<char*>(""));

		int parsedPercent = 0;
		char msg[128];

		if (ParsePercent(raw, &parsedPercent))
		{
			*pct = parsedPercent;
			const float value = DerivePercentValue(maxStorage, parsedPercent);

			if (isMetal) me->ShareMetal  = value;
			else         me->ShareEnergy = value;

			_snprintf(msg, sizeof(msg) - 1, "OK.  Will share %s if above %d%% (%ld)",
				resourceName, parsedPercent, SafeLongForDisplay(value));
			msg[sizeof(msg) - 1] = '\0';
			NewChatText(msg, 2, 0, 10);
			return;
		}

		// Not a percentage: clear percentage mode and reproduce vanilla's
		// own logic exactly (upper-bound clamp only -- vanilla does not
		// clamp negative values, and neither do we here).
		*pct = -1;
		const int n = TokenLine_AtoiAt(line, /*index=*/1, /*defaultValue=*/0);
		float value = static_cast<float>(n);
		if (value > maxStorage)
			value = maxStorage;

		if (isMetal) me->ShareMetal  = value;
		else         me->ShareEnergy = value;

		_snprintf(msg, sizeof(msg) - 1, "OK.  Will share %s if above %ld",
			resourceName, SafeLongForDisplay(static_cast<float>(n)));
		msg[sizeof(msg) - 1] = '\0';
		NewChatText(msg, 2, 0, 10);
	}

	// Signature/calling convention match the originals exactly:
	// File_DispatchNamedCommand pushes one TokenLine* argument and both
	// original handlers end in `ret 4`.
	void __stdcall SetShareMetalThresholdReplacement(TaTokenLine* line)
	{
		HandleSetShareThreshold(line, /*isMetal=*/true);
	}

	void __stdcall SetShareEnergyThresholdReplacement(TaTokenLine* line)
	{
		HandleSetShareThreshold(line, /*isMetal=*/false);
	}

	void OnGameTick(int gameTime)
	{
		// GameTickHook fires several times per game tick with an unchanged
		// gameTime; this callback only recomputes and stores, so re-entry is
		// wasteful, not wrong -- guard anyway, it's one comparison.
		if (gameTime == g_lastGameTime)
			return;

		// GameTime at or below the last value seen is read as "a new game
		// (or replay) started". Backed by the TAInGame-edge signal below,
		// not relied on alone.
		if (gameTime < g_lastGameTime)
			ResetPercentModes();
		g_lastGameTime = gameTime;

		const bool inGame = (DataShare != nullptr) && (DataShare->TAProgress == TAInGame);
		if (inGame && !g_wasInGame)
			ResetPercentModes();
		g_wasInGame = inGame;

		bool anyArmed = false;
		for (int i = 0; i < 10; ++i)
			if (g_metalPercent[i] >= 0 || g_energyPercent[i] >= 0) { anyArmed = true; break; }
		if (!anyArmed)
			return;

		if (!inGame || DataShare->PlayingDemo)
			return;

		TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
		if (!ta)
			return;
		// Vanilla's own "simulation running" gate, applied once for the pass.
		if ((ta->WorkStatusMask & 1) == 0)
			return;

		// Walk every armed slot regardless of which one is currently local,
		// so switching seats with `+control` can't bleed one player's arm
		// into another's ShareMetal/ShareEnergy.
		for (int i = 0; i < 10; ++i)
		{
			if (g_metalPercent[i] < 0 && g_energyPercent[i] < 0)
				continue;

			PlayerStruct* p = &ta->Players[i];
			if (!p->PlayerActive)
				continue;

			if (g_metalPercent[i] >= 0)
				p->ShareMetal = DerivePercentValue(p->PlayerRes.fMaxMetalStorage, g_metalPercent[i]);
			if (g_energyPercent[i] >= 0)
				p->ShareEnergy = DerivePercentValue(p->PlayerRes.fMaxEnergyStorage, g_energyPercent[i]);
		}
	}
}

namespace SharePercent
{
	void Install()
	{
		if (g_metalHook || g_energyHook)
			return;

		if (!HasExpectedBytes(kSetShareMetalAddr, kExpectedPrologue, kHookLen) ||
			!HasExpectedBytes(kSetShareEnergyAddr, kExpectedPrologue, kHookLen))
		{
			return;   // logged inside HasExpectedBytes; refuse to hook an unexpected build
		}

		ResetPercentModes();
		g_lastGameTime = 0;
		g_wasInGame = false;

		g_metalHook = new SingleHook(kSetShareMetalAddr, kHookLen, INLINE_SINGLEJMP,
			reinterpret_cast<LPBYTE>(&SetShareMetalThresholdReplacement));
		g_energyHook = new SingleHook(kSetShareEnergyAddr, kHookLen, INLINE_SINGLEJMP,
			reinterpret_cast<LPBYTE>(&SetShareEnergyThresholdReplacement));

		GameTickHook::GetInstance()->addCallback(OnGameTick);

		IDDrawSurface::OutptTxt("[SharePercent] installed");
	}

	void Shutdown()
	{
		delete g_metalHook;  g_metalHook = nullptr;
		delete g_energyHook; g_energyHook = nullptr;
		ResetPercentModes();
	}

	// Report the CURRENTLY LOCAL player's armed state. Unused outside this
	// module today; kept for API completeness alongside GetDisplayPercent.
	int GetMetalPercent()
	{
		int id = -1;
		if (!LocalPlayerOrNull(&id))
			return -1;
		return g_metalPercent[id];
	}

	int GetEnergyPercent()
	{
		int id = -1;
		if (!LocalPlayerOrNull(&id))
			return -1;
		return g_energyPercent[id];
	}

	int GetDisplayPercent(bool isMetal)
	{
		int id = -1;
		PlayerStruct* me = LocalPlayerOrNull(&id);
		if (!me)
			return 0;

		const int armed = isMetal ? g_metalPercent[id] : g_energyPercent[id];
		if (armed >= 0)
			return armed;

		const float maxStorage = isMetal ? me->PlayerRes.fMaxMetalStorage
		                                  : me->PlayerRes.fMaxEnergyStorage;
		const float threshold  = isMetal ? me->ShareMetal : me->ShareEnergy;
		if (!(maxStorage > 0.0f))
			return 0;

		const long pct = static_cast<long>((threshold * 100.0f) / maxStorage);
		return ClampPercent(pct);
	}
}

#endif // SHARE_PERCENT_ENABLE
