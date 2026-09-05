#include "config.h"
#include "PlayerMute.h"

#include "tamem.h"
#include "tafunctions.h"
#include "iddrawsurface.h"
#include "hook/hook.h"

#include <windows.h>
#include <cstring>
#include <cstdio>

using namespace PlayerMute;

namespace
{
	// ---- engine sites ----------------------------------------------------
	//
	// Net_PushChatHudMessage @0x00463CA0 — TADR declares it as
	//   void __stdcall NewChatText(char* Text, char channel, short alert, char playerIndex)
	// Verified from disassembly (arg slots read at 3-push depth):
	//   [Esp+0x04] text        -> copied into ring entry +0x00
	//   [Esp+0x08] channel     -> low nibble of ring entry +0x47   (@0x463D83)
	//   [Esp+0x0C] alert       -> ring entry +0x44 as a word       (@0x463D94)
	//   [Esp+0x10] playerIndex -> ring entry +0x46 as a byte       (@0x463DB3)
	// It ends `ret 0x10` @0x463E3E, i.e. 4 stdcall args.
	//
	// The arrival sound is played at 0x463DF9 and is skipped when
	// playerIndex == 10 (`cmp dl,0xa / je` @0x463DED). Because the sound call
	// lives INSIDE this function, cancelling the call suppresses the text and
	// the sound together — one hook covers both.
	const DWORD ADDR_PUSH_CHAT   = 0x00463CA0;
	// push ebp(1) + push esi(1) + mov esi,[esp+0xC](4) = 6. Five bytes would
	// split the mov.
	const DWORD LEN_PUSH_CHAT    = 6;
	const DWORD ARGBYTES_PUSH_CHAT = 0x10;

	// Chat_FormatAndSend @0x00463E50 — __stdcall(int arg1, const char* rawText,
	// int scope, PlayerInfoStruct* overrideName), `ret 0x10` @0x463EDF.
	//
	// CORRECTED [live] 2026-08-16: the mute command was first implemented by
	// hooking Chat_SendOutgoingMessage @0x453360 instead, on the assumption
	// that (a) it receives the player's raw typed text and (b) cancelling it
	// suppresses the local echo too. Both assumptions were wrong, and a live
	// two-player test caught it: typed ".mute chat X" appeared as literal
	// text in the sender's own chat, and the mute never took effect.
	//
	// The real architecture, found by tracing 0x453360's ONLY caller
	// (confirmed via a call-site scan — exactly one xref, right here):
	//
	//   sprintf(buf, "<%s%s%s> %s", ..names.., rawText)   ; format string is
	//                                                      ; literally "<%s%s%s> %s"
	//                                                      ; at 0x5072C8
	//   call 0x453360(buf)          <- the OLD hook. Receives the FORMATTED
	//                                   "<Name> text" string, never the raw
	//                                   text, so a leading-'.' check here can
	//                                   never match.
	//   call 0x463CA0(buf, scope, 0, 10)   <- local echo. UNCONDITIONAL: runs
	//                                          whether or not 0x453360's send
	//                                          actually happened. Cancelling
	//                                          0x453360 alone can never stop
	//                                          your own client from showing
	//                                          the raw command text.
	//
	// Hooking THIS function instead fixes both problems at once: arg2 here is
	// the player's raw, unformatted, un-prefixed text, and cancelling the
	// whole call skips formatting, the network send AND the local echo
	// together — the command truly never becomes a chat line anywhere.
	//
	// Confirmed the ONLY entry point for player-typed chat: 0x453360 has
	// exactly one caller (this function); this function itself has six
	// callers, one of which (0x493FAF) sits right next to the known chat-
	// dialog code (Gui_LoadChatDialog @0x494050, ENGINE_NOTES.md §26.6).
	//
	// Length: first instruction is `mov eax,[esp+0x10]` (4 bytes), second is
	// `sub esp,0xC8` (6 bytes). A 5-byte hook would split the second
	// instruction after its opcode byte, so the hook must consume both
	// instructions whole: 4 + 6 = 10 bytes.
	const DWORD ADDR_SEND_CHAT   = 0x00463E50;
	const DWORD LEN_SEND_CHAT    = 10;
	const DWORD ARGBYTES_SEND_CHAT = 0x10;

	InlineSingleHook* g_chatHook = nullptr;
	InlineSingleHook* g_sendHook = nullptr;

	// ---- mute state ------------------------------------------------------
	unsigned char g_mask[kMaxPlayers] = { 0 };

	// Session fingerprint: names + DirectPlayIDs of all slots. When this
	// changes we are in a different game and slot N is a different human, so
	// the mask must not carry over.
	struct SessionSnapshot
	{
		char names[kMaxPlayers][30];
		int  dpids[kMaxPlayers];
	};
	SessionSnapshot g_session;
	bool g_sessionValid = false;

	TAdynmemStruct* GetTA()
	{
		return TAmainStruct_PtrPtr ? *TAmainStruct_PtrPtr : nullptr;
	}

	bool CaptureSession(SessionSnapshot& out)
	{
		TAdynmemStruct* ta = GetTA();
		if (!ta)
			return false;

		__try
		{
			for (int n = 0; n < kMaxPlayers; ++n)
			{
				const PlayerStruct& p = ta->Players[n];
				memcpy(out.names[n], p.Name, sizeof(out.names[n]));
				out.names[n][sizeof(out.names[n]) - 1] = '\0';
				out.dpids[n] = p.PlayerActive ? p.DirectPlayID : 0;
			}
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return false;
		}
	}

	// ---- text helpers ----------------------------------------------------
	const char* SkipSpace(const char* s)
	{
		while (*s == ' ' || *s == '\t') ++s;
		return s;
	}

	// Matches `word` at the start of `s` as a whole token. Returns the
	// remainder (past the token and any following spaces), or nullptr.
	const char* MatchToken(const char* s, const char* word)
	{
		const size_t n = strlen(word);
		if (_strnicmp(s, word, n) != 0)
			return nullptr;
		const char c = s[n];
		if (c != '\0' && c != ' ' && c != '\t')
			return nullptr;      // e.g. ".muted" must not match ".mute"
		return SkipSpace(s + n);
	}

	// Copies up to `cap-1` chars, dropping trailing whitespace.
	void CopyTrimmed(char* dst, size_t cap, const char* src)
	{
		size_t i = 0;
		while (src[i] && i < cap - 1) { dst[i] = src[i]; ++i; }
		while (i > 0 && (dst[i - 1] == ' ' || dst[i - 1] == '\t' ||
		                 dst[i - 1] == '\r' || dst[i - 1] == '\n')) --i;
		dst[i] = '\0';
	}

	const char* CategoryName(Category cat)
	{
		switch (cat)
		{
		case CatChat:  return "chat";
		case CatPings: return "pings";
		case CatDraw:  return "drawings";
		default:       return "everything";
		}
	}

	// ---- command handling ------------------------------------------------
	void ReportMuteState(int slot)
	{
		TAdynmemStruct* ta = GetTA();
		const char* name = (ta && slot >= 0 && slot < kMaxPlayers)
			? ta->Players[slot].Name : "?";

		const unsigned char m = (slot >= 0 && slot < kMaxPlayers) ? g_mask[slot] : 0;
		char msg[160];
		if (m == 0)
		{
			_snprintf(msg, sizeof(msg) - 1, "[mute] %s: nothing muted", name);
		}
		else
		{
			_snprintf(msg, sizeof(msg) - 1, "[mute] %s: %s%s%s", name,
				(m & CatChat)  ? "chat "  : "",
				(m & CatPings) ? "pings " : "",
				(m & CatDraw)  ? "drawings" : "");
		}
		msg[sizeof(msg) - 1] = '\0';
		PlayerMute::LocalNotice(msg);
	}

	void ListMutes()
	{
		TAdynmemStruct* ta = GetTA();
		if (!ta)
			return;

		bool any = false;
		for (int n = 0; n < kMaxPlayers; ++n)
		{
			if (g_mask[n] == 0)
				continue;
			any = true;
			ReportMuteState(n);
		}
		if (!any)
			PlayerMute::LocalNotice("[mute] nobody is muted");
	}

	void ApplyToNamed(const char* nameArg, Category cat, bool toggle, bool onWhenNotToggling)
	{
		if (!nameArg || !*nameArg)
		{
			PlayerMute::LocalNotice("[mute] usage: .mute <name>  |  .mute chat|ping|draw <name>");
			return;
		}

		const int slot = PlayerMute::FindSlotByName(nameArg);
		if (slot == -2)
		{
			PlayerMute::LocalNotice("[mute] that name matches more than one player");
			return;
		}
		if (slot < 0)
		{
			char msg[128];
			_snprintf(msg, sizeof(msg) - 1, "[mute] no player called '%s'", nameArg);
			msg[sizeof(msg) - 1] = '\0';
			PlayerMute::LocalNotice(msg);
			return;
		}

		if (toggle)
			PlayerMute::Toggle(slot, cat);
		else
			PlayerMute::SetMuted(slot, cat, onWhenNotToggling);

		ReportMuteState(slot);
	}

	// Returns true when the text was a local command and must NOT be sent.
	bool HandleLocalCommand(const char* raw)
	{
		if (!raw)
			return false;

		const char* s = SkipSpace(raw);
		if (*s != '.')
			return false;
		++s;

		PlayerMute::SyncSession();

		// ---- .unmute [all|<name>]
		if (const char* rest = MatchToken(s, "unmute"))
		{
			char arg[64];
			CopyTrimmed(arg, sizeof(arg), rest);

			if (arg[0] == '\0' || _stricmp(arg, "all") == 0)
			{
				PlayerMute::ResetAll();
				PlayerMute::LocalNotice("[mute] all mutes cleared");
				return true;
			}
			ApplyToNamed(arg, CatAll, false, false);
			return true;
		}

		// ---- .mute [chat|ping|draw] [<name>]
		if (const char* rest = MatchToken(s, "mute"))
		{
			if (*rest == '\0')
			{
				ListMutes();
				return true;
			}

			Category cat = CatAll;
			const char* after = nullptr;
			if      ((after = MatchToken(rest, "chat")))      cat = CatChat;
			else if ((after = MatchToken(rest, "ping")))      cat = CatPings;
			else if ((after = MatchToken(rest, "pings")))     cat = CatPings;
			else if ((after = MatchToken(rest, "draw")))      cat = CatDraw;
			else if ((after = MatchToken(rest, "drawings")))  cat = CatDraw;

			char arg[64];
			CopyTrimmed(arg, sizeof(arg), after ? after : rest);
			ApplyToNamed(arg, cat, true, true);
			return true;
		}

		return false;
	}

	// ---- hook routers ----------------------------------------------------

	// Net_PushChatHudMessage entry. Cancel the whole call for muted senders,
	// which suppresses both the HUD line and the arrival sound.
	int __stdcall PushChatProc(PInlineX86StackBuffer buf)
	{
		__try
		{
			const DWORD* args = (const DWORD*)(buf->Esp);
			const char* text  = (const char*)args[1];
			const int   chan  = (int)(args[2] & 0x0F);
			const int   alert = (int)(args[3] & 0xFFFF);
			const int   slot  = (int)(args[4] & 0xFF);

			// Slot 10 is the system sentinel: unit alerts, TADR's own local
			// messages and self-typed chat. Never muted.
			if (slot < 0 || slot >= kMaxPlayers)
				return 0;

			if (g_mask[slot] == 0)
				return 0;                     // fast path: nothing muted here

			// Only now, on the rare path where a mute would actually fire, pay
			// for the session check. A slot number means a different human in
			// the next game, and the engine does NOT clear the chat ring or
			// the roster between games, so a carried-over mask would silence
			// the wrong person.
			SyncSession();
			if (g_mask[slot] == 0)
				return 0;                     // mask was stale and got cleared

			if (!text)
				return 0;

			bool drop = false;
			if (chan == 8)
			{
				// Channel 8 carries player chat AND pause/ready notices.
				// Only real chat is formatted "<Name> ..." / "<Name->Allies>",
				// so use that to keep game-state notices visible.
				// HEURISTIC: verified against live replay dumps, not against
				// the formatter's disassembly.
				if (text[0] == '<' && (g_mask[slot] & CatChat))
					drop = true;
			}
			else if (chan == 1 && alert == 0)
			{
				// Marker / ping echo ("*Name added a new marker",
				// "*Name: text"). Channel 1 with a non-zero alert is a unit
				// alert and must survive — the alert camera reads it.
				if (g_mask[slot] & CatPings)
					drop = true;
			}
			// Channel 4 (eliminations, leaves) and channel 2 always pass.

			if (!drop)
				return 0;

			// Cancel: return straight to the caller with the stack cleaned
			// as if the 4-arg __stdcall had run and returned (ret 0x10).
			void* rtnAddr = *(void**)(buf->Esp);
			buf->Esp = buf->Esp + 4 + ARGBYTES_PUSH_CHAT;
			buf->rtnAddr_Pvoid = rtnAddr;
			return X86STRACKBUFFERCHANGE;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
		}
		return 0;
	}

	// Chat_FormatAndSend entry. Swallow `.mute` / `.unmute` before the engine
	// formats, broadcasts OR locally echoes them. Everything else passes
	// through untouched, including the server-bot commands (.time, .ready,
	// ...) and the '+' internal commands. arg2 (this function's raw text
	// argument, [Esp+8] at a JMP-lagger hook site) is the player's actual
	// typed text, unlike the old hook site which only ever saw the
	// pre-formatted "<Name> text" string.
	int __stdcall SendChatProc(PInlineX86StackBuffer buf)
	{
		__try
		{
			const DWORD* args = (const DWORD*)(buf->Esp);
			const char* text  = (const char*)args[2];
			if (!text || !HandleLocalCommand(text))
				return 0;

			void* rtnAddr = *(void**)(buf->Esp);
			buf->Esp = buf->Esp + 4 + ARGBYTES_SEND_CHAT;
			buf->rtnAddr_Pvoid = rtnAddr;
			return X86STRACKBUFFERCHANGE;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
		}
		return 0;
	}
}

// ---------------------------------------------------------------------------

void PlayerMute::Install()
{
	if (!g_chatHook)
		g_chatHook = new InlineSingleHook(
			ADDR_PUSH_CHAT, LEN_PUSH_CHAT, INLINE_5BYTESLAGGERJMP, PushChatProc);

	if (!g_sendHook)
		g_sendHook = new InlineSingleHook(
			ADDR_SEND_CHAT, LEN_SEND_CHAT, INLINE_5BYTESLAGGERJMP, SendChatProc);

	ResetAll();
	IDDrawSurface::OutptTxt("[PlayerMute] installed (.mute / .unmute are local only)");
}

void PlayerMute::Shutdown()
{
	delete g_chatHook;  g_chatHook = nullptr;
	delete g_sendHook;  g_sendHook = nullptr;
	ResetAll();
}

bool PlayerMute::IsMuted(int slot, Category cat)
{
	if (slot < 0 || slot >= kMaxPlayers)
		return false;
	return (g_mask[slot] & (unsigned char)cat) != 0;
}

bool PlayerMute::IsAnyMuted(int slot)
{
	if (slot < 0 || slot >= kMaxPlayers)
		return false;
	return g_mask[slot] != 0;
}

void PlayerMute::SetMuted(int slot, Category cat, bool on)
{
	if (slot < 0 || slot >= kMaxPlayers)
		return;
	if (on)
		g_mask[slot] |= (unsigned char)cat;
	else
		g_mask[slot] &= (unsigned char)~cat;
}

bool PlayerMute::Toggle(int slot, Category cat)
{
	if (slot < 0 || slot >= kMaxPlayers)
		return false;
	const bool nowOn = (g_mask[slot] & (unsigned char)cat) != (unsigned char)cat;
	SetMuted(slot, cat, nowOn);
	return nowOn;
}

void PlayerMute::ResetAll()
{
	memset(g_mask, 0, sizeof(g_mask));
}

void PlayerMute::SyncSession()
{
	SessionSnapshot now;
	if (!CaptureSession(now))
		return;

	if (!g_sessionValid || memcmp(&now, &g_session, sizeof(now)) != 0)
	{
		memcpy(&g_session, &now, sizeof(now));
		g_sessionValid = true;
		ResetAll();
	}
}

int PlayerMute::FindSlotByName(const char* name)
{
	if (!name || !*name)
		return -1;

	TAdynmemStruct* ta = GetTA();
	if (!ta)
		return -1;

	const size_t n = strlen(name);
	int prefixHit = -1;
	int prefixCount = 0;

	for (int i = 0; i < kMaxPlayers; ++i)
	{
		const PlayerStruct& p = ta->Players[i];
		if (!p.PlayerActive || !p.Name[0])
			continue;

		if (_stricmp(p.Name, name) == 0)
			return i;                          // exact match wins outright

		if (_strnicmp(p.Name, name, n) == 0)
		{
			prefixHit = i;
			++prefixCount;
		}
	}

	if (prefixCount == 1)
		return prefixHit;
	if (prefixCount > 1)
		return -2;                             // ambiguous
	return -1;
}

void PlayerMute::LocalNotice(const char* msg)
{
	if (!msg || !NewChatText)
		return;
	// playerIndex 10 = system sentinel: no logo, no arrival sound, and our
	// own filter never touches it.
	NewChatText(const_cast<char*>(msg), 1, 0, (char)kSystemSlot);
}
