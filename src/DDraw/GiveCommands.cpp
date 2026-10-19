#include "GiveCommands.h"

#if GIVE_COMMANDS_ENABLE

#include <windows.h>

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "GameTickHook.h"
#include "Profiler.h"
#include "hook/hook.h"
#include "iddrawsurface.h"
#include "tafunctions.h"
#include "tamem.h"

// tamem.h is #pragma pack(1). These anchor every offset the module depends on.
static_assert(offsetof(TAdynmemStruct, LocalHumanPlayer_PlayerID) == 0x2A42, "TAdynmemStruct::LocalHumanPlayer_PlayerID moved");
static_assert(offsetof(TAdynmemStruct, Players)                   == 0x1B63, "TAdynmemStruct::Players moved");
static_assert(sizeof(PlayerStruct)                                == 0x14B,  "PlayerStruct size changed (slot = byte offset / 0x14B)");
static_assert(offsetof(PlayerStruct, PlayerActive)                == 0x00,   "PlayerStruct::PlayerActive moved");
static_assert(offsetof(PlayerStruct, PlayerInfo)                  == 0x27,   "PlayerStruct::PlayerInfo moved");
static_assert(offsetof(PlayerStruct, Name)                        == 0x2B,   "PlayerStruct::Name moved");
static_assert(offsetof(PlayerStruct, SecondName)                  == 0x49,   "PlayerStruct::SecondName moved");
static_assert(offsetof(PlayerStruct, My_PlayerType)               == 0x73,   "PlayerStruct::My_PlayerType moved");
static_assert(offsetof(PlayerStruct, PlayerRes)                   == 0x8C,   "PlayerStruct::PlayerRes moved");
static_assert(offsetof(PlayerStruct, resourcesShared)             == 0xEC,   "PlayerStruct::resourcesShared moved");
static_assert(offsetof(PlayerStruct, AllyFlagAry)                 == 0x108,  "PlayerStruct::AllyFlagAry moved");
static_assert(offsetof(PlayerStruct, WholeUnitsCounters)          == 0x140,  "PlayerStruct::WholeUnitsCounters moved");
static_assert(offsetof(PlayerStruct, UnitsNumber)                 == 0x144,  "PlayerStruct::UnitsNumber moved");
static_assert(offsetof(PlayerStruct, PlayerAryIndex)              == 0x146,  "PlayerStruct::PlayerAryIndex moved");
static_assert(offsetof(PlayerResourcesStruct, fCurrentEnergy)     == 0x00,   "PlayerResourcesStruct::fCurrentEnergy moved");
static_assert(offsetof(PlayerResourcesStruct, fCurrentMetal)      == 0x0C,   "PlayerResourcesStruct::fCurrentMetal moved");
static_assert(offsetof(PlayerResourcesStruct, fMaxEnergyStorage)  == 0x18,   "PlayerResourcesStruct::fMaxEnergyStorage moved");
static_assert(offsetof(PlayerResourcesStruct, fMaxMetalStorage)   == 0x1C,   "PlayerResourcesStruct::fMaxMetalStorage moved");
static_assert(offsetof(PlayerInfoStruct, PlayerType)              == 0x94,   "PlayerInfoStruct::PlayerType moved");
static_assert(offsetof(PlayerInfoStruct, PropertyMask)            == 0x9B,   "PlayerInfoStruct::PropertyMask moved");

namespace
{
	const int kSlots = 10;
	const int kMaxTokens = 20;       // the engine's tokenizer stores at most 20 (ENGINE_NOTES 17.4)
	enum { kMetal = 0, kEnergy = 1 };
	enum Mode { kModeGive = 0, kModeMetal = 1, kModeEnergy = 2 };
	const double kMinGive = 1.0;

	// ---- TotalA.exe addresses (Escalation GOLD 10.2) -----------------------------------------------------
	// Inside the routine that registers the built-in command tables on every map load: the second
	// `push 0x501F48 ; call registrar` pair. The registrar is at 0x4B7760.
	const DWORD kRegSite     = 0x004195C9u;
	const DWORD kRegResume   = 0x004195CEu;   // the vanilla `call registrar`
	const DWORD kGiveRecord  = 0x00501D68u;   // vanilla {"Give", 0x416BD0, level 1}
	const DWORD kXferMetal   = 0x00464C60u;   // Player_TransferMetalResource  __stdcall(src, dst, float, sendPacket)
	const DWORD kXferEnergy  = 0x00464B30u;   // Player_TransferEnergyResource
	const DWORD kCmdVecBegin = 0x0051FC9Du;   // the engine's sorted command vector (12-byte records), unaligned
	const DWORD kCmdVecEnd   = 0x0051FCA1u;

	typedef void (__stdcall* TransferFn)(unsigned int src, unsigned int dst, float amount, int sendPacket);
	TransferFn g_transfer[2] = { reinterpret_cast<TransferFn>(kXferMetal), reinterpret_cast<TransferFn>(kXferEnergy) };

	// Opaque: TA's pre-tokenized command line, only touched through the engine's own accessor. Token
	// count is the int at +0xD0 (File_TokenLine); index 0 is the command word.
	struct TaTokenLine;
	typedef char* (__thiscall* TokenLine_GetArgPtr_t)(TaTokenLine* self, int index, char* fallback);
	TokenLine_GetArgPtr_t TokenLine_GetArgPtr = reinterpret_cast<TokenLine_GetArgPtr_t>(0x004B73C0u);

	DWORD g_regResume = kRegResume;
	bool  g_installed;
	int   g_lastGameTime;
	bool  g_wasInGame;

	void Flush(DWORD addr, size_t len)
	{
		FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<LPCVOID>(addr), len);
	}

	bool GameRunning()
	{
		return DataShare != nullptr && DataShare->TAProgress == TAInGame;
	}

	// ---- player helpers (pure over a Players[10] array) -----------------------------------------------------
	bool Alive(const PlayerStruct* p) { return p->UnitsNumber != 0 || p->WholeUnitsCounters == 0; }
	bool Spectator(const PlayerInfoStruct* info) { return (info->PropertyMask & WATCH) != 0; }
	BYTE Byte22(const PlayerStruct* p) { return reinterpret_cast<const BYTE*>(p)[0x22]; }
	float Stock(const PlayerStruct* p, int r) { return r == kMetal ? p->PlayerRes.fCurrentMetal : p->PlayerRes.fCurrentEnergy; }
	float Max(const PlayerStruct* p, int r)   { return r == kMetal ? p->PlayerRes.fMaxMetalStorage : p->PlayerRes.fMaxEnergyStorage; }
	const char* ResName(int r) { return r == kMetal ? "metal" : "energy"; }

	// Name and SecondName are 30-byte fields that need not be NUL-terminated.
	void CopyName(const char* src, char* out31)
	{
		size_t n = 0;
		while (n < 30 && src[n])
			++n;
		std::memcpy(out31, src, n);
		out31[n] = '\0';
	}

	// ---- argument parsing ------------------------------------------------------------------------------------
	enum Status
	{
		kOk, kUsage, kBadAmount, kNoMatch, kAmbiguous, kSelf, kNotAlly, kCannotReceive, kSenderBlocked, kNoRoom, kNoStock
	};

	struct Parsed { int res; double amount; char name[128]; };

	// Digits only, at least one; longer runs saturate at 1e9 instead of overflowing. 0 is a valid number
	// (the caller rejects it as an amount).
	bool ParseNumber(const char* raw, double* out)
	{
		if (!raw || *raw < '0' || *raw > '9')
			return false;
		double acc = 0.0;
		int digits = 0;
		const char* p = raw;
		for (; *p >= '0' && *p <= '9'; ++p, ++digits)
			acc = (digits < 9) ? acc * 10.0 + (*p - '0') : 1000000000.0;
		if (*p != '\0')
			return false;
		*out = acc;
		return true;
	}

	// tok[0] is the command word. `mode` selects the argument order. Joins the player tokens with single
	// spaces (a name with spaces arrives as several tokens).
	Status ParseArgs(const char* const* tok, int count, Mode mode, Parsed* out)
	{
		std::memset(out, 0, sizeof(*out));
		if (count > kMaxTokens + 1)
			count = kMaxTokens + 1;
		int amountIdx, nameEnd;           // name = tok[1 .. nameEnd)
		if (mode == kModeGive)
		{
			if (count < 4)
				return kUsage;
			const char* word = tok[count - 1];
			if (_stricmp(word, "metal") == 0)       out->res = kMetal;
			else if (_stricmp(word, "energy") == 0) out->res = kEnergy;
			else                                    return kUsage;
			amountIdx = count - 2;
		}
		else
		{
			if (count < 3)
				return kUsage;
			out->res = (mode == kModeMetal) ? kMetal : kEnergy;
			amountIdx = count - 1;
		}
		nameEnd = amountIdx;
		size_t used = 0;
		for (int i = 1; i < nameEnd; ++i)
		{
			const size_t len = std::strlen(tok[i]);
			if (len == 0)
				continue;
			if (used + len + 2 > sizeof(out->name))
				return kUsage;
			if (used)
				out->name[used++] = ' ';
			std::memcpy(out->name + used, tok[i], len);
			used += len;
		}
		out->name[used] = '\0';
		if (used == 0)
			return kUsage;
		double amount = 0.0;
		if (!ParseNumber(tok[amountIdx], &amount) || amount < kMinGive)
			return kBadAmount;
		out->amount = amount;
		return kOk;
	}

	// ---- player resolution ---------------------------------------------------------------------------------------
	struct Resolve
	{
		int  slot;
		int  total;               // players matched (exact or prefix)
		int  shown;               // how many names are filled in
		char names[3][32];
	};

	bool IsAllDigits(const char* s)
	{
		if (!*s)
			return false;
		for (; *s; ++s)
			if (*s < '0' || *s > '9')
				return false;
		return true;
	}

	// 1. exact name (Name or SecondName), 2. all digits = slot number, 3. unique prefix of Name; all
	// case-insensitive, over active players only.
	Status ResolveTarget(const PlayerStruct* pl, const char* name, Resolve* r)
	{
		std::memset(r, 0, sizeof(*r));
		r->slot = -1;
		char n1[31], n2[31];
		int hit[kSlots];
		int nh = 0;

		for (int i = 0; i < kSlots; ++i)
		{
			if (!pl[i].PlayerActive)
				continue;
			CopyName(pl[i].Name, n1);
			CopyName(pl[i].SecondName, n2);
			if ((n1[0] && _stricmp(n1, name) == 0) || (n2[0] && _stricmp(n2, name) == 0))
				hit[nh++] = i;
		}
		if (nh == 0 && IsAllDigits(name) && std::strlen(name) <= 2)
		{
			const int v = std::atoi(name);
			if (v >= 0 && v < kSlots && pl[v].PlayerActive)
			{
				r->slot = v;
				return kOk;
			}
		}
		if (nh == 0)
		{
			const size_t len = std::strlen(name);
			for (int i = 0; i < kSlots; ++i)
			{
				if (!pl[i].PlayerActive)
					continue;
				CopyName(pl[i].Name, n1);
				if (n1[0] && _strnicmp(n1, name, len) == 0)
					hit[nh++] = i;
			}
		}
		r->total = nh;
		if (nh == 0)
			return kNoMatch;
		if (nh == 1)
		{
			r->slot = hit[0];
			return kOk;
		}
		for (int k = 0; k < nh && k < 3; ++k)
		{
			CopyName(pl[hit[k]].Name, r->names[k]);
			++r->shown;
		}
		return kAmbiguous;
	}

	// ---- the decision ------------------------------------------------------------------------------------------------
	struct Result
	{
		Status status;
		int    slot;
		int    res;
		float  amount;            // what to send (kOk)
		int    cap;               // 0 none, 1 limited by the receiver's room, 2 limited by the giver's stock
		char   who[32];           // the receiver's name (kOk and refusals about the receiver)
		Resolve resolved;
	};

	// Everything the command decides, from the array and the parsed arguments: no engine call, no side
	// effect, so the self-test and the offline harness run exactly this.
	void Evaluate(const PlayerStruct* pl, int me, const Parsed& in, Result* out)
	{
		std::memset(out, 0, sizeof(*out));
		out->res = in.res;
		out->slot = -1;
		Status st = ResolveTarget(pl, in.name, &out->resolved);
		if (st != kOk)
		{
			out->status = st;
			return;
		}
		const int slot = out->resolved.slot;
		out->slot = slot;
		const PlayerStruct* d = &pl[slot];
		CopyName(d->Name, out->who);
		if (slot == me)
		{
			out->status = kSelf;
			return;
		}
		const PlayerStruct* m = &pl[me];
		if (m->AllyFlagAry[slot] == 0)
		{
			out->status = kNotAlly;                         // the giver's own flag, as for the auto-share
			return;
		}
		const PlayerInfoStruct* dinfo = d->PlayerInfo;
		const int dtype = d->My_PlayerType;
		if (!d->PlayerActive || !dinfo || dtype < 1 || dtype > 3 || static_cast<int>(static_cast<unsigned char>(d->PlayerAryIndex)) != slot ||
			Spectator(dinfo) || !Alive(d) || d->resourcesShared == nullptr || (dtype == 3 && Byte22(d) != 0))
		{
			out->status = kCannotReceive;
			return;
		}
		const PlayerInfoStruct* minfo = m->PlayerInfo;
		if (!m->PlayerActive || !minfo || m->My_PlayerType < 1 || m->My_PlayerType > 3 || m->resourcesShared == nullptr ||
			static_cast<int>(static_cast<unsigned char>(m->PlayerAryIndex)) != me ||
			(dtype == 3 && (!Alive(m) || Spectator(minfo) || Byte22(m) != 0)))
		{
			out->status = kSenderBlocked;                   // the packet path would refuse after the debit
			return;
		}
		const double room = static_cast<double>(Max(d, in.res)) - static_cast<double>(Stock(d, in.res));
		const double mine = static_cast<double>(Stock(m, in.res));
		if (!(room >= kMinGive))
		{
			out->status = kNoRoom;
			return;
		}
		if (!(mine >= kMinGive))
		{
			out->status = kNoStock;
			return;
		}
		double sent = in.amount;
		int cap = 0;
		if (sent > room && room <= mine)
		{
			sent = room;
			cap = 1;
		}
		else if (sent > mine)
		{
			sent = mine;
			cap = 2;
		}
		float f = static_cast<float>(sent);
		if (static_cast<double>(f) > sent)
			f = std::nextafter(f, 0.0f);                    // never above the receiver's room or the giver's stock
		if (!(f >= static_cast<float>(kMinGive)))
		{
			out->status = kNoRoom;
			return;
		}
		out->status = kOk;
		out->amount = f;
		out->cap = cap;
	}

	// ---- messages (no gendered pronouns, no addresses) -------------------------------------------------------------
	void BuildText(const Result& r, const Parsed& in, Mode mode, char* out, size_t n)
	{
		const char* res = ResName(in.res);
		switch (r.status)
		{
		case kOk:
			if (r.cap == 1)
				snprintf(out, n, "Gave %.0f %s to %s (%s's %s storage is now full).", r.amount, res, r.who, r.who, res);
			else if (r.cap == 2)
				snprintf(out, n, "Gave %.0f %s to %s (all you had).", r.amount, res, r.who);
			else
				snprintf(out, n, "Gave %.0f %s to %s.", r.amount, res, r.who);
			break;
		case kBadAmount:
			snprintf(out, n, "The amount must be a whole number of at least 1 (digits only).");
			break;
		case kNoMatch:
			snprintf(out, n, "No player matches \"%s\".", in.name);
			break;
		case kAmbiguous:
			if (r.resolved.shown == 2)
				snprintf(out, n, "\"%s\" matches %s, %s: type more of the name.", in.name, r.resolved.names[0], r.resolved.names[1]);
			else
				snprintf(out, n, "\"%s\" matches %s, %s, %s%s: type more of the name.", in.name, r.resolved.names[0], r.resolved.names[1],
					r.resolved.names[2], r.resolved.total > 3 ? ", ..." : "");
			break;
		case kSelf:
			snprintf(out, n, "You cannot give resources to yourself.");
			break;
		case kNotAlly:
			snprintf(out, n, "You can only give to allies.");
			break;
		case kCannotReceive:
			snprintf(out, n, "%s cannot receive resources right now.", r.who);
			break;
		case kSenderBlocked:
			snprintf(out, n, "You cannot give resources right now.");
			break;
		case kNoRoom:
			snprintf(out, n, "%s has no free %s storage.", r.who, res);
			break;
		case kNoStock:
			snprintf(out, n, "You have no %s to give.", res);
			break;
		case kUsage:
		default:
			if (mode == kModeGive)
				snprintf(out, n, "Usage: +give <player> <amount> metal|energy");
			else
				snprintf(out, n, "Usage: +give%s <player> <amount>", mode == kModeMetal ? "metal" : "energy");
			break;
		}
	}

	// ---- the commands ------------------------------------------------------------------------------------------------
	// The copy only bounds what is handed to NewChatText (191 characters).
	void Say(const char* text)
	{
		char buf[192];
		snprintf(buf, sizeof(buf), "%s", text);
		IDDrawSurface::OutptFmtTxt("[GiveCommands] chat: %s", buf);
		NewChatText(buf, 2, 0, 10);
	}

	void HandleGive(TaTokenLine* line, Mode mode)
	{
		PROFILE_SCOPE("Cmd.Give");
		TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
		if (!ta || !GameRunning())
		{
			Say("Giving resources is only available while a game is running.");
			return;
		}
		if (DataShare && DataShare->PlayingDemo)
		{
			Say("Giving resources has no effect while watching a replay.");
			return;
		}
		const int me = static_cast<unsigned char>(ta->LocalHumanPlayer_PlayerID);
		if (me >= kSlots || !ta->Players[me].PlayerActive)
		{
			Say("No local player right now, nothing sent.");
			return;
		}

		const char* tok[kMaxTokens + 1];
		int count = *reinterpret_cast<const int*>(reinterpret_cast<const BYTE*>(line) + 0xD0);
		if (count < 0)
			count = 0;
		if (count > kMaxTokens + 1)
			count = kMaxTokens + 1;
		for (int i = 0; i < count; ++i)
			tok[i] = TokenLine_GetArgPtr(line, i, const_cast<char*>(""));

		Parsed in;
		Result r;
		char msg[192];
		const Status ps = ParseArgs(tok, count, mode, &in);
		if (ps != kOk)
		{
			std::memset(&r, 0, sizeof(r));
			r.status = ps;
			BuildText(r, in, mode, msg, sizeof(msg));
			Say(msg);
			return;
		}
		Evaluate(&ta->Players[0], me, in, &r);
		BuildText(r, in, mode, msg, sizeof(msg));
		if (r.status == kOk)
		{
			const PlayerStruct* d = &ta->Players[r.slot];
			g_transfer[r.res](static_cast<unsigned char>(ta->Players[me].PlayerAryIndex), static_cast<unsigned char>(d->PlayerAryIndex), r.amount, 1);
		}
		Say(msg);
	}

	// File_DispatchNamedCommand pushes one token-line pointer; the argument array is that same object.
	void __stdcall CmdGive(char* argv[])       { HandleGive(reinterpret_cast<TaTokenLine*>(argv), kModeGive); }
	void __stdcall CmdGiveMetal(char* argv[])  { HandleGive(reinterpret_cast<TaTokenLine*>(argv), kModeMetal); }
	void __stdcall CmdGiveEnergy(char* argv[]) { HandleGive(reinterpret_cast<TaTokenLine*>(argv), kModeEnergy); }

	// The registrar keeps the name pointers and the table entries' contents, so this must stay alive. `Give`
	// matches vanilla's record exactly (case included), so the registrar overwrites that entry's handler.
	InternalCommandTableEntryStruct g_commandTable[] =
	{
		{ "Give",       &CmdGive,       InternalCommandRunLevel::CMD_LEVEL_NORMAL },
		{ "GiveMetal",  &CmdGiveMetal,  InternalCommandRunLevel::CMD_LEVEL_NORMAL },
		{ "GiveEnergy", &CmdGiveEnergy, InternalCommandRunLevel::CMD_LEVEL_NORMAL },
		{ NULL, NULL, InternalCommandRunLevel::CMD_LEVEL_NULL }
	};

	// Runs before the vanilla `push 0x501F48 ; call registrar`: registers this module's table, replays the
	// push and resumes. eax/ecx/edx are dead here (the registrar writes them before reading); flags are dead
	// across the call.
	__declspec(naked) void RegisterCommandsStub()
	{
		__asm
		{
			push offset g_commandTable
			call dword ptr [InitInternalCommand]
			push 0x00501F48                     // replaced: 68 48 1F 50 00
			jmp dword ptr [g_regResume]
		}
	}

#if GIVE_RECEIVE_GUARD_ENABLE
	// ---- receive guard (Part C) -------------------------------------------------------------------------------------------
	// A received 0x16 resource packet is applied by the engine with no sign check: a zero is a no-op, but a NEGATIVE or
	// NaN amount credits the recipient with garbage (and, through the engine's debit, can raise a stock), and infinity
	// fills a storage. The two arms of the receive switch (metal 0x4558C2, energy 0x4558E0) read the amount from
	// [packet+0xD] and call the transfer with sendPacket 0. Each arm's first two instructions are replaced by a jump to a
	// stub that tests the amount's float bits with integer arithmetic only (no FPU, no flags that matter: the arm's next
	// consumer of eflags is far below): a valid amount is positive and finite, 0 < bits < 0x7F800000. Anything else is
	// counted and the arm is left through its own exit (0x455F50), exactly as if the packet had been ignored.
	// Honest limit: this protects the receiving client's view only; a cheater's own client still credits itself.
	const DWORD kArmMetal      = 0x004558C2u;   // 8B 54 24 10  8B 4C 24 20   mov edx,[esp+0x10] ; mov ecx,[esp+0x20]
	const DWORD kArmEnergy     = 0x004558E0u;   // 8B 44 24 10  8B 54 24 20   mov eax,[esp+0x10] ; mov edx,[esp+0x20]
	const DWORD kArmLen        = 8u;
	DWORD g_guardMetalResume   = 0x004558CAu;   // the instruction after the replaced pair (push 0)
	DWORD g_guardEnergyResume  = 0x004558E8u;
	DWORD g_caseExit           = 0x00455F50u;
	DWORD g_guardDrops;                         // read by OnGameTick, written by the stubs

	// The test the stubs implement, for the self-test: 0 < bits < 0x7F800000.
	bool AmountBitsValid(DWORD bits) { return bits - 1u <= 0x7F7FFFFEu; }

	__declspec(naked) void ReceiveGuardMetal()
	{
		__asm
		{
			mov eax, dword ptr [esp + 0x10]         // the packet
			mov eax, dword ptr [eax + 0x0D]         // the amount's float bits
			dec eax
			cmp eax, 0x7F7FFFFE
			ja drop
			mov edx, dword ptr [esp + 0x10]         // replaced: 8B 54 24 10
			mov ecx, dword ptr [esp + 0x20]         // replaced: 8B 4C 24 20
			jmp dword ptr [g_guardMetalResume]
		drop:
			inc dword ptr [g_guardDrops]
			jmp dword ptr [g_caseExit]
		}
	}

	__declspec(naked) void ReceiveGuardEnergy()
	{
		__asm
		{
			mov eax, dword ptr [esp + 0x10]
			mov eax, dword ptr [eax + 0x0D]
			dec eax
			cmp eax, 0x7F7FFFFE
			ja drop
			mov eax, dword ptr [esp + 0x10]         // replaced: 8B 44 24 10
			mov edx, dword ptr [esp + 0x20]         // replaced: 8B 54 24 20
			jmp dword ptr [g_guardEnergyResume]
		drop:
			inc dword ptr [g_guardDrops]
			jmp dword ptr [g_caseExit]
		}
	}

	DWORD g_guardLogged;
	DWORD g_guardLastLog;

	// A forged packet stream must not flood the log: at most one line per ten seconds.
	void MaybeLogGuard()
	{
		if (g_guardDrops == g_guardLogged)
			return;
		const DWORD now = GetTickCount();
		if (now - g_guardLastLog < 10000u)
			return;
		g_guardLastLog = now;
		g_guardLogged = g_guardDrops;
		IDDrawSurface::OutptFmtTxt("[GiveCommands] receive guard: %lu resource packets with a zero, negative or non-finite amount dropped so far", g_guardDrops);
	}
#endif // GIVE_RECEIVE_GUARD_ENABLE

	// Looks the three commands up in the engine's live command vector (12-byte records: name, handler,
	// permission level). "registered" means the name is there with this module's handler.
	struct CommandProbe { bool ok; int mine; int foreign; int total; };
	CommandProbe ProbeCommands()
	{
		CommandProbe r = { false, 0, 0, 0 };
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
				DWORD want = 0;
				if (_stricmp(name, "give") == 0)            want = reinterpret_cast<DWORD>(&CmdGive);
				else if (_stricmp(name, "givemetal") == 0)  want = reinterpret_cast<DWORD>(&CmdGiveMetal);
				else if (_stricmp(name, "giveenergy") == 0) want = reinterpret_cast<DWORD>(&CmdGiveEnergy);
				if (want)
				{
					if (handler == want) ++r.mine; else ++r.foreign;
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

	void LogGameStart()
	{
		const CommandProbe c = ProbeCommands();
		if (!c.ok)
			IDDrawSurface::OutptTxt("[GiveCommands] commands: engine command table unreadable");
		else
			IDDrawSurface::OutptFmtTxt("[GiveCommands] commands: %d of 3 registered with this module's handler%s; %d engine commands",
				c.mine, c.foreign ? ", FOREIGN HANDLER present" : "", c.total);
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
			LogGameStart();
#if GIVE_RECEIVE_GUARD_ENABLE
		MaybeLogGuard();
#endif
	}

	// ---- install-time checks -------------------------------------------------------------------------------------------
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
		IDDrawSurface::OutptFmtTxt("[GiveCommands] DISABLED: unexpected TotalA.exe bytes at 0x%08X (%s)", addr, what);
		return false;
	}

	// ---- premises: every byte this module relies on, verified before anything is patched ---------------------------------
	// A table so the offline harness can flip each byte in turn and prove the install refuses (and writes nothing).
	struct Premise { DWORD addr; const BYTE* bytes; size_t len; const char* what; };

	// the second table's push and the registrar call after it; the first table's call just before; the
	// neighbours (PRT's third-table push, AutoTeam's `push 4`) are other modules' sites
	const BYTE kRegPair[10]   = { 0x68, 0x48, 0x1F, 0x50, 0x00, 0xE8, 0x8D, 0xE1, 0x09, 0x00 };
	const BYTE kFirstPush[5]  = { 0x68, 0x38, 0x1D, 0x50, 0x00 };     // push 0x501D38, at 0x4195A7
	const BYTE kFirstCall[5]  = { 0xE8, 0x97, 0xE1, 0x09, 0x00 };     // call 0x4B7760, at 0x4195C4
	const BYTE kRegistrar[4]  = { 0x8B, 0x4C, 0x24, 0x04 };
	const BYTE kRegistrarRet[3] = { 0xC2, 0x04, 0x00 };
	const BYTE kDedupe[14]    = { 0x85, 0xC0, 0x0F, 0x94, 0xC1, 0xF6, 0xD9, 0x1B, 0xC9, 0x41, 0x84, 0xC9, 0x75, 0x05 };
	const BYTE kGiveRec[12]   = { 0x20, 0x24, 0x50, 0x00, 0xD0, 0x6B, 0x41, 0x00, 0x01, 0x00, 0x00, 0x00 };
	const BYTE kGiveStr[5]    = { 'G', 'i', 'v', 'e', 0 };
	const BYTE kGetArgPtr[29] = { 0x8B, 0x44, 0x24, 0x04, 0x85, 0xC0, 0x7C, 0x0E, 0x3B, 0x81, 0xD0, 0x00, 0x00, 0x00, 0x7D, 0x06,
	                                     0x8B, 0x04, 0x81, 0xC2, 0x08, 0x00, 0x8B, 0x44, 0x24, 0x08, 0xC2, 0x08, 0x00 };
	const BYTE kXferHead[12]  = { 0x8B, 0x44, 0x24, 0x04, 0x53, 0x55, 0x56, 0x3C, 0x0A, 0x57, 0x0F, 0x84 };
	const BYTE kRet10[3]      = { 0xC2, 0x10, 0x00 };
#if GIVE_RECEIVE_GUARD_ENABLE
	// kind switch: mov eax,[edx+1] ; dec eax ; je energy arm ; dec eax ; je metal arm ; dec eax ; jne exit
	const BYTE kRecvSwitch[16] = { 0x8B, 0x42, 0x01, 0x48, 0x74, 0x3C, 0x48, 0x74, 0x1B, 0x48, 0x0F, 0x85, 0xA2, 0x06, 0x00, 0x00 };
	// metal arm: the two replaced movs, push 0, mov eax,[edx+0xD] (the amount), ... call 0x464C60 ... jmp exit
	const BYTE kArmMetalB[26]  = { 0x8B, 0x54, 0x24, 0x10, 0x8B, 0x4C, 0x24, 0x20, 0x6A, 0x00, 0x8B, 0x42, 0x0D, 0x8B, 0x54, 0x24, 0x28,
	                               0x50, 0x51, 0x52, 0xE8, 0x85, 0xF3, 0x00, 0x00, 0xE9 };
	const BYTE kArmEnergyB[26] = { 0x8B, 0x44, 0x24, 0x10, 0x8B, 0x54, 0x24, 0x20, 0x6A, 0x00, 0x8B, 0x48, 0x0D, 0x8B, 0x44, 0x24, 0x28,
	                               0x51, 0x52, 0x50, 0xE8, 0x37, 0xF2, 0x00, 0x00, 0xE9 };
	const BYTE kCaseExitB[7]   = { 0x8B, 0x84, 0x24, 0xF0, 0x00, 0x00, 0x00 };
#endif

	const Premise kPremises[] =
	{
		{ 0x004195A7u, kFirstPush, sizeof(kFirstPush), "first command table push" },
		{ 0x004195C4u, kFirstCall, sizeof(kFirstCall), "first command table registrar call" },
		{ kRegSite, kRegPair, sizeof(kRegPair), "command table registration" },
		{ 0x004B7760u, kRegistrar, sizeof(kRegistrar), "registrar entry" },
		{ 0x004B78CFu, kRegistrarRet, sizeof(kRegistrarRet), "registrar exit" },
		{ 0x004B782Eu, kDedupe, sizeof(kDedupe), "registrar same-name branch" },
		{ kGiveRecord, kGiveRec, sizeof(kGiveRec), "vanilla Give record" },
		{ 0x00502420u, kGiveStr, sizeof(kGiveStr), "\"Give\" string" },
		{ 0x004B73C0u, kGetArgPtr, sizeof(kGetArgPtr), "token accessor" },
		{ kXferMetal, kXferHead, sizeof(kXferHead), "metal transfer entry" },
		{ kXferEnergy, kXferHead, sizeof(kXferHead), "energy transfer entry" },
#if GIVE_RECEIVE_GUARD_ENABLE
		{ 0x0045589Eu, kRecvSwitch, sizeof(kRecvSwitch), "receive switch on the packet kind" },
		{ 0x004558C2u, kArmMetalB, sizeof(kArmMetalB), "metal receive arm" },
		{ 0x004558E0u, kArmEnergyB, sizeof(kArmEnergyB), "energy receive arm" },
		{ 0x00455F50u, kCaseExitB, sizeof(kCaseExitB), "receive case exit" },
#endif
		{ 0x00464D8Eu, kRet10, sizeof(kRet10), "metal transfer ret 0x10" },
		{ 0x00464C5Au, kRet10, sizeof(kRet10), "energy transfer ret 0x10" },
	};
	const size_t kPremiseCount = sizeof(kPremises) / sizeof(kPremises[0]);

	bool VerifyPremises()
	{
		for (size_t i = 0; i < kPremiseCount; ++i)
			if (!Expect(kPremises[i].addr, kPremises[i].bytes, kPremises[i].len, kPremises[i].what))
				return false;
		return true;
	}

	// ---- self-test -------------------------------------------------------------------------------------------------------
	struct World
	{
		PlayerStruct                p[kSlots];
		PlayerInfoStruct            info[kSlots];
		PlayerSharedResourcesStruct eco[kSlots];
	};

	void SetP(World& w, int i, const char* name, float stock, float max, int type = 3)
	{
		PlayerStruct& p = w.p[i];
		p.PlayerActive = 1;
		p.PlayerInfo = &w.info[i];
		p.My_PlayerType = static_cast<char>(type);
		p.PlayerAryIndex = static_cast<char>(i);
		p.resourcesShared = &w.eco[i];
		const size_t len = std::strlen(name) < sizeof(p.Name) ? std::strlen(name) : sizeof(p.Name);
		std::memcpy(p.Name, name, len);                                     // Name is not required to be NUL-terminated
		p.PlayerRes.fCurrentMetal = stock;
		p.PlayerRes.fMaxMetalStorage = max;
		p.PlayerRes.fCurrentEnergy = stock;
		p.PlayerRes.fMaxEnergyStorage = max;
		w.info[i].PlayerType = 1;
		p.UnitsNumber = 5;
		p.WholeUnitsCounters = 5;
	}

	void Reset(World& w)
	{
		std::memset(&w, 0, sizeof(w));
	}

	Status Run(World& w, int me, const char* line, Mode mode, Result* r)
	{
		// split `line` on single spaces like the engine's tokenizer (the command word is token 0)
		static char buf[256];
		const char* tok[kMaxTokens + 1];
		std::snprintf(buf, sizeof(buf), "%s", line);
		int n = 0;
		for (char* s = buf; *s && n <= kMaxTokens; )
		{
			tok[n++] = s;
			while (*s && *s != ' ')
				++s;
			if (*s)
				*s++ = '\0';
		}
		Parsed in;
		const Status ps = ParseArgs(tok, n, mode, &in);
		std::memset(r, 0, sizeof(*r));
		if (ps != kOk)
		{
			r->status = ps;
			return ps;
		}
		Evaluate(w.p, me, in, r);
		return r->status;
	}

	bool RunSelfTest()
	{
		static World w;
		char why[96];
		why[0] = '\0';
		bool ok = true;
		#define GC_FAIL(text) do { ok = false; snprintf(why, sizeof(why), "%s", text); goto done; } while (0)
		Result r;

		// 1. number parsing
		{
			struct { const char* in; bool valid; double value; } cases[] =
			{
				{ "",   false, 0 },  { "abc", false, 0 },  { "-5", false, 0 },  { "+5", false, 0 },  { "1.5", false, 0 },  { "5%", false, 0 },
				{ "1e3", false, 0 }, { "0x10", false, 0 }, { "5 ", false, 0 },  { "0", true, 0 },    { "007", true, 7 },   { "500", true, 500 },
				{ "999999999", true, 999999999.0 },  { "99999999999", true, 1000000000.0 },  { "256", true, 256 },
			};
			for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i)
			{
				double v = -7.0;
				const bool b = ParseNumber(cases[i].in, &v);
				if (b != cases[i].valid || (b && v != cases[i].value))
					GC_FAIL("number parser");
			}
		}

		// 2. a game of 4: me = slot 2 "Alice"; allies Bob (1), "Big Bob" (3 -> slot 3), enemy Eve (0)
		Reset(w);
		SetP(w, 0, "Eve", 100.0f, 1000.0f);
		SetP(w, 1, "Bob", 100.0f, 1000.0f);
		SetP(w, 2, "Alice", 900.0f, 1000.0f);
		SetP(w, 3, "Big Bob", 500.0f, 600.0f);
		SetP(w, 4, "3rd", 10.0f, 1000.0f);
		w.p[2].AllyFlagAry[1] = 1;  w.p[2].AllyFlagAry[3] = 1;  w.p[2].AllyFlagAry[4] = 1;  w.p[2].AllyFlagAry[2] = 1;

		if (Run(w, 2, "givemetal Bob 500", kModeMetal, &r) != kOk || r.slot != 1 || r.amount != 500.0f || r.cap != 0)  GC_FAIL("plain give");
		if (Run(w, 2, "give bob 500 metal", kModeGive, &r) != kOk || r.slot != 1 || r.res != kMetal)                    GC_FAIL("vanilla order, case");
		if (Run(w, 2, "giveenergy 1 50", kModeEnergy, &r) != kOk || r.slot != 1 || r.res != kEnergy)                    GC_FAIL("slot number");
		if (Run(w, 2, "givemetal Big Bob 50", kModeMetal, &r) != kOk || r.slot != 3)                                    GC_FAIL("name with a space");
		if (Run(w, 2, "givemetal 3rd 5", kModeMetal, &r) != kOk || r.slot != 4)                                         GC_FAIL("exact name beats the slot number");
		if (Run(w, 2, "givemetal 3 5", kModeMetal, &r) != kOk || r.slot != 3)                                           GC_FAIL("all digits is the slot");
		if (Run(w, 2, "givemetal Big 5", kModeMetal, &r) != kOk || r.slot != 3)                                         GC_FAIL("unique prefix");
		if (Run(w, 2, "givemetal B 5", kModeMetal, &r) != kAmbiguous || r.resolved.total != 2)                          GC_FAIL("ambiguous prefix");
		if (Run(w, 2, "givemetal Nobody 5", kModeMetal, &r) != kNoMatch)                                                GC_FAIL("unknown name");
		if (Run(w, 2, "givemetal Eve 5", kModeMetal, &r) != kNotAlly)                                                   GC_FAIL("not allied");
		if (Run(w, 2, "givemetal Alice 5", kModeMetal, &r) != kSelf)                                                    GC_FAIL("self");
		if (Run(w, 2, "givemetal 9 5", kModeMetal, &r) != kNoMatch)                                                     GC_FAIL("inactive slot");
		if (Run(w, 2, "givemetal 256 5", kModeMetal, &r) != kNoMatch)                                                   GC_FAIL("slot 256 is not slot 0");
		if (Run(w, 2, "givemetal Bob -5", kModeMetal, &r) != kBadAmount)                                                GC_FAIL("negative amount");
		if (Run(w, 2, "givemetal Bob 0", kModeMetal, &r) != kBadAmount)                                                 GC_FAIL("zero amount");
		if (Run(w, 2, "givemetal Bob 1.5", kModeMetal, &r) != kBadAmount)                                               GC_FAIL("fractional amount");
		if (Run(w, 2, "givemetal Bob", kModeMetal, &r) != kUsage || Run(w, 2, "givemetal", kModeMetal, &r) != kUsage)  GC_FAIL("missing arguments");
		if (Run(w, 2, "give Bob 5", kModeGive, &r) != kUsage || Run(w, 2, "give Bob 5 gold", kModeGive, &r) != kUsage) GC_FAIL("give usage");

		// 3. caps: receiver room, giver stock, huge amounts
		if (Run(w, 2, "givemetal Big Bob 500", kModeMetal, &r) != kOk || r.cap != 1 || std::fabs(r.amount - 100.0f) > 0.01f) GC_FAIL("room cap");
		w.p[2].PlayerRes.fCurrentMetal = 800.0f;                            // below Bob's room (900): the giver's stock binds
		if (Run(w, 2, "givemetal Bob 99999999999", kModeMetal, &r) != kOk || r.cap != 2 || r.amount > 800.0f || r.amount < 799.0f) GC_FAIL("stock cap");
		w.p[2].PlayerRes.fCurrentMetal = 0.5f;
		if (Run(w, 2, "givemetal Bob 5", kModeMetal, &r) != kNoStock)                                                   GC_FAIL("no stock");
		w.p[2].PlayerRes.fCurrentMetal = 900.0f;
		w.p[1].PlayerRes.fCurrentMetal = 1000.0f;
		if (Run(w, 2, "givemetal Bob 5", kModeMetal, &r) != kNoRoom)                                                    GC_FAIL("no room");
		w.p[1].PlayerRes.fCurrentMetal = 100.0f;

		// 4. the transfer path would debit and lose it: receiver dead / +0x22 / spectator / no economy object /
		//    index mismatch; giver dead / spectator / +0x22
		w.p[1].UnitsNumber = 0;  w.p[1].WholeUnitsCounters = 2;
		if (Run(w, 2, "givemetal Bob 5", kModeMetal, &r) != kCannotReceive)                                             GC_FAIL("dead receiver");
		w.p[1].UnitsNumber = 5;  reinterpret_cast<BYTE*>(&w.p[1])[0x22] = 1;
		if (Run(w, 2, "givemetal Bob 5", kModeMetal, &r) != kCannotReceive)                                             GC_FAIL("receiver +0x22");
		reinterpret_cast<BYTE*>(&w.p[1])[0x22] = 0;  w.info[1].PropertyMask = WATCH;
		if (Run(w, 2, "givemetal Bob 5", kModeMetal, &r) != kCannotReceive)                                             GC_FAIL("receiver spectator");
		w.info[1].PropertyMask = 0;  w.p[1].resourcesShared = nullptr;
		if (Run(w, 2, "givemetal Bob 5", kModeMetal, &r) != kCannotReceive)                                             GC_FAIL("receiver without economy object");
		w.p[1].resourcesShared = &w.eco[1];  w.p[1].PlayerAryIndex = 7;
		if (Run(w, 2, "givemetal Bob 5", kModeMetal, &r) != kCannotReceive)                                             GC_FAIL("receiver index mismatch");
		w.p[1].PlayerAryIndex = 1;
		reinterpret_cast<BYTE*>(&w.p[2])[0x22] = 1;
		if (Run(w, 2, "givemetal Bob 5", kModeMetal, &r) != kSenderBlocked)                                             GC_FAIL("giver +0x22 to a remote");
		w.p[1].My_PlayerType = 2;                                           // a local AI needs no packet
		if (Run(w, 2, "givemetal Bob 5", kModeMetal, &r) != kOk)                                                        GC_FAIL("giver +0x22 to a local AI");
		w.p[1].My_PlayerType = 3;  reinterpret_cast<BYTE*>(&w.p[2])[0x22] = 0;
		w.p[2].UnitsNumber = 0;  w.p[2].WholeUnitsCounters = 1;
		if (Run(w, 2, "givemetal Bob 5", kModeMetal, &r) != kSenderBlocked)                                             GC_FAIL("dead giver");
		w.p[2].UnitsNumber = 5;
		w.p[2].resourcesShared = nullptr;                                   // the giver's own economy object: the debit would dereference null
		if (Run(w, 2, "givemetal Bob 5", kModeMetal, &r) != kSenderBlocked)                                             GC_FAIL("giver without an economy object");
		w.p[2].resourcesShared = &w.eco[2];

		// 5. a name that is a prefix of two players, exact name wins over prefix
		Reset(w);
		SetP(w, 0, "Bob", 100.0f, 1000.0f);  SetP(w, 1, "Bobby", 100.0f, 1000.0f);  SetP(w, 2, "Me", 900.0f, 1000.0f);
		w.p[2].AllyFlagAry[0] = 1;  w.p[2].AllyFlagAry[1] = 1;
		if (Run(w, 2, "givemetal Bob 5", kModeMetal, &r) != kOk || r.slot != 0)                                         GC_FAIL("exact name beats prefix");
		if (Run(w, 2, "givemetal bo 5", kModeMetal, &r) != kAmbiguous || r.resolved.total != 2)                         GC_FAIL("ambiguous two");

	done:
		#undef GC_FAIL
		if (!ok)
			IDDrawSurface::OutptFmtTxt("[GiveCommands] DISABLED: self-test failed: %s", why);
		return ok;
	}

	// ---- patch bookkeeping ----------------------------------------------------------------------------------------------
	SingleHook* g_regHook;
#if GIVE_RECEIVE_GUARD_ENABLE
	SingleHook* g_guardMetalHook;
	SingleHook* g_guardEnergyHook;
#endif

	void RemoveAllHooks()
	{
#if GIVE_RECEIVE_GUARD_ENABLE
		delete g_guardEnergyHook;  g_guardEnergyHook = nullptr;
		delete g_guardMetalHook;   g_guardMetalHook = nullptr;
		Flush(0x00455000u, 0x1000u);
#endif
		delete g_regHook;
		g_regHook = nullptr;
		Flush(0x00419000u, 0x1000u);
	}

	bool PatchesReadBack()
	{
		BYTE code[8];
		if (!ReadBytes(kRegSite, code, 5))
			return false;
		DWORD rel = 0;
		std::memcpy(&rel, code + 1, 4);
		if (code[0] != 0xE9 || rel != reinterpret_cast<DWORD>(&RegisterCommandsStub) - (kRegSite + 5))
			return false;
#if GIVE_RECEIVE_GUARD_ENABLE
		const struct { DWORD at; DWORD target; } arms[2] = { { kArmMetal, reinterpret_cast<DWORD>(&ReceiveGuardMetal) }, { kArmEnergy, reinterpret_cast<DWORD>(&ReceiveGuardEnergy) } };
		for (int i = 0; i < 2; ++i)
		{
			BYTE b[8];
			if (!ReadBytes(arms[i].at, b, kArmLen))
				return false;
			std::memcpy(&rel, b + 1, 4);
			if (b[0] != 0xE9 || rel != arms[i].target - (arms[i].at + 5) || b[5] != 0x90 || b[6] != 0x90 || b[7] != 0x90)
				return false;
		}
#endif
		return true;
	}
}

namespace GiveCommands
{
	void Install()
	{
		if (g_installed)
			return;

		if (!VerifyPremises())
			return;                        // logged inside; refuse to patch an unexpected build
		if (!RunSelfTest())
			return;                        // logged inside

		g_lastGameTime = 0;
		g_wasInGame = false;
		g_regHook = new SingleHook(kRegSite, 5, INLINE_SINGLEJMP, reinterpret_cast<LPBYTE>(&RegisterCommandsStub));
		Flush(kRegSite, 5);
#if GIVE_RECEIVE_GUARD_ENABLE
		g_guardDrops = g_guardLogged = 0;
		g_guardLastLog = GetTickCount() - 10000u;
		g_guardMetalHook = new SingleHook(kArmMetal, kArmLen, INLINE_SINGLEJMP, reinterpret_cast<LPBYTE>(&ReceiveGuardMetal));
		Flush(kArmMetal, kArmLen);
		g_guardEnergyHook = new SingleHook(kArmEnergy, kArmLen, INLINE_SINGLEJMP, reinterpret_cast<LPBYTE>(&ReceiveGuardEnergy));
		Flush(kArmEnergy, kArmLen);
#endif

		if (!PatchesReadBack())
		{
			RemoveAllHooks();
			IDDrawSurface::OutptTxt("[GiveCommands] DISABLED: patched bytes did not read back; everything restored");
			return;
		}

		GameTickHook::GetInstance()->addCallback(OnGameTick);
		g_installed = true;
		IDDrawSurface::OutptFmtTxt("[GiveCommands] installed: Give, GiveMetal, GiveEnergy registered through @0x%08X, receive guard %s", kRegSite,
			GIVE_RECEIVE_GUARD_ENABLE ? "on" : "off");
	}

	void Shutdown()
	{
		if (!g_installed)
			return;
		RemoveAllHooks();
		g_installed = false;
	}
}

#endif // GIVE_COMMANDS_ENABLE
