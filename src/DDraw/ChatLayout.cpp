#include "config.h"
#include "ChatLayout.h"

#include "ChatClassify.h"
#include "ChatPosition.h"
#include "ChatBackdrop.h"
#include "ChatFont.h"
#include "PCX.H"

#include "tamem.h"
#include "tafunctions.h"
#include "iddrawsurface.h"
#include "TAConfig.h"
#include "hook/hook.h"

#include <windows.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>

// TADR owns Hud_DrawChatHudRing @0x00464060 -- int __stdcall(OFFSCREEN*),
// ret 4. First 5 bytes are `sub esp,0x20 / push ebx / push ebp` (3 whole
// instructions), safe for a lagger-jmp to replay or to cancel. The caller
// pushes the OFFSCREEN* and does no post-call cleanup, so at the hook
// [Esp] = return addr and [Esp+4] = OFFSCREEN*.
//
// Fail-safe: the router captures the return address, then runs everything
// under SEH. Any fault or unresolved pointer -> return 0 and the engine draws
// normally. Worst case is a double-drawn frame; the chat can never blank.

namespace
{
	const unsigned CHAT_DRAW_ADDR = 0x00464060;
	// `sub esp,0x20 / push ebx / push ebp` -- checked before installing the
	// hook, same reasoning as PlayerMute.cpp's BytesMatch: an exe where this
	// does not hold is not one this module's hardcoded ring/field offsets
	// apply to either, and InlineSingleHook has no way to know that on its
	// own.
	const unsigned char kExpectedChatDraw[5] = { 0x83, 0xEC, 0x20, 0x53, 0x55 };

	bool BytesMatch(DWORD addr, const unsigned char* expected, size_t len)
	{
		__try
		{
			return memcmp((const void*)addr, expected, len) == 0;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return false;
		}
	}

	// Ring layout (same offsets as ChatBackdrop.cpp).
	const unsigned OFF_CHAT_TEXT     = 0x12EF;
	const unsigned CHAT_ENTRY_STRIDE = 0x48;
	const int      CHAT_ENTRY_COUNT  = 30;
	const unsigned OFF_STR_ALERT     = 0x44;   // u16
	const unsigned OFF_STR_LOGO      = 0x46;   // player idx, 10 = no logo
	const unsigned OFF_STR_FLAGS     = 0x47;   // &0x0F channel, &0x20 highlight
	const unsigned OFF_FREE_INDEX    = 0x2A3E;
	const unsigned OFF_CHAT_NUM      = 0x2A40;
	const unsigned OFF_CHAT_MODE     = 0x37EFE;
	const unsigned OFF_SCREENCHAT    = 0x37F02;
	const unsigned OFF_TEXTLINES     = 0x37F27;
	const unsigned OFF_COMIX_FONT    = 0x391F9;
	const unsigned OFF_REMAP_NORMAL    = 0xDDA;
	const unsigned OFF_REMAP_HIGHLIGHT = 0xDD5;

	// Active GUI panel chain (ta->desktopGUI.TheActive_GUIMEM, walked by
	// per_active -- same mechanism as unitrotate.cpp). Scrollback arms while
	// the chat-entry panel is in this chain; a closed panel is simply absent
	// next frame, so no "chat closed" signal is needed.
	const unsigned OFF_ACTIVE_GUIMEM   = 0x0519 + 0x18; // _GUIInfo.TheActive_GUIMEM
	const unsigned OFF_GUIMEM_PERACTIVE = 0x00;
	const unsigned OFF_GUIMEM_NAME      = 0x41;         // char GUIName[16]

	// Vanilla top-left, the fixed home of the system column when split.
	const int VANILLA_X  = 138;
	const int VANILLA_Y  = 52;
	const int HUD_TOP    = 32;   // top bar; never draw a line above this
	const int HUD_BOTTOM = 32;   // bottom bar; never draw a line below (screenH - this)

	// Backdrop padding -- must match ChatBackdrop.cpp so a tadr-drawn line
	// gets the same box an engine-drawn line would.
	const int PAD_X_LEFT  = 4;
	const int PAD_X_RIGHT = 4;
	const int PAD_Y       = 1;

	// TAPalette[0] is pure black; the ChatFontOutline colour.
	const int TA_BLACK_INDEX = 0;

	// Engine functions this drawer calls (signatures from the disassembly).
	// Font_SetColors / DrawColorTextInScreen are called via
	// ChatBackdrop::DrawChatLine instead of bound here directly.
	typedef void (__stdcall* Fn_FontSetCurrent)(void* fontDataStruct);  // 0x4C1420, ret 4
	typedef void (__stdcall* Fn_DrawPlayerLogo)(void* off, void* player, RECT* r, int flag); // 0x467C00, ret 0x10
	const Fn_FontSetCurrent  Font_SetCurrent  = (Fn_FontSetCurrent) 0x004C1420;
	const Fn_DrawPlayerLogo  DrawPlayerLogo   = (Fn_DrawPlayerLogo) 0x00467C00;

	enum Renderer { R_Engine = 0, R_Tadr };

	Renderer          g_renderer  = R_Engine;
	bool              g_split     = false;
	bool              g_growUp    = false;   // ChatGrow: false = down (oldest on anchor), true = up (newest on anchor)
	unsigned          g_sysGroups = CHATGROUPS_DEFAULT_SYS;
	int               g_plrLines  = 0;       // ChatLines:    0 = auto-fit, >0 = that many newest player-column lines
	int               g_sysLines  = 0;       // ChatSysLines: 0 = auto-fit, >0 = that many newest system-column lines
	int               g_fontSize  = 0;       // ChatFontSize:  0 = native bitmap font, >0 = ChatFont TTF atlas at that pixel height
	bool              g_fontColorSet   = false;
	int               g_fontColorIndex = 0;  // nearest TAPalette[] index to ChatFontColor's RGB, resolved once at Install()
	bool              g_fontOutline    = false;
	bool              g_active    = false;
	InlineSingleHook* g_hook      = nullptr;

	// Retained chat history: the engine ring holds only 30 entries, so every
	// line it gains is copied here (oldest evicted past HIST_CAP) for
	// scrollback. Fed once per frame from WalkChat via HistoryConsume(), which
	// diffs the ring freeIndex against the last one it saw and appends the
	// slots written since (freeIndex advances by 1 per line, wraps at 30). The
	// first call seeds from the visible ring so a mid-game start is not blank.
	// Entries are byte-identical to ring entries (0x48 stride).
	//
	// Per-match, not per-process: when ta->GameTime goes backwards (new match
	// or replay rewind) the buffer is dropped. The ring is not cleared between
	// matches, so the reset must NOT re-seed from it -- it just resyncs the
	// delta to the current freeIndex.
	const int      HIST_CAP = 512;
	unsigned char  g_hist[HIST_CAP][CHAT_ENTRY_STRIDE];
	unsigned       g_histCount        = 0;    // lines appended in the current match
	bool           g_histSeeded       = false;
	int            g_histLastFree     = 0;    // ring freeIndex at the last consume
	int            g_histLastGameTime = -1;   // ta->GameTime at the last consume; -1 = never seen

	// While the chat compose prompt is open (TALK.GUI in the active GUI chain)
	// the player column is drawn from the retained history at g_sbOffset
	// (0 = live tail) instead of the live ring; the system column stays live.
	bool           g_sbArmed  = false;
	int            g_sbOffset = 0;
	int            g_sbPick[HIST_CAP];        // history indices of the player-column lines, newest-first

	void HistAppend(const unsigned char* ringEntry)
	{
		memcpy(g_hist[g_histCount % HIST_CAP], ringEntry, CHAT_ENTRY_STRIDE);
		++g_histCount;
	}

	// Diff the ring against g_histLastFree and append new lines. `oldestVisible`
	// is WalkChat's already-computed walk-back index, reused to seed.
	void HistoryConsume(unsigned char* ta, int oldestVisible, int freeIndex)
	{
		if (freeIndex < 0 || freeIndex >= CHAT_ENTRY_COUNT)
			return;

		// New match / replay rewind: game clock went backwards. Drop the old
		// history; do NOT re-seed (the ring is not cleared between matches) --
		// just resync the delta to the current freeIndex.
		const int gameTime = reinterpret_cast<TAdynmemStruct*>(ta)->GameTime;
		if (gameTime < g_histLastGameTime)
		{
			g_histCount    = 0;
			g_histSeeded   = true;
			g_histLastFree = freeIndex;
		}
		g_histLastGameTime = gameTime;

		if (!g_histSeeded)
		{
			g_histSeeded = true;
			for (int j = oldestVisible; j != freeIndex; j = (j + 1) % CHAT_ENTRY_COUNT)
				HistAppend(ta + OFF_CHAT_TEXT + j * CHAT_ENTRY_STRIDE);
			g_histLastFree = freeIndex;
			return;
		}

		if (freeIndex == g_histLastFree)
			return;

		// freeIndex advances by 1 per line and both indices are in [0,30), so
		// this walk always appends real recent lines -- at worst it repeats one
		// on a (frame-rate-unreachable) >=30-lines-in-one-frame burst, never
		// garbage.
		const int oldFree = g_histLastFree;

		for (int j = oldFree; j != freeIndex; j = (j + 1) % CHAT_ENTRY_COUNT)
			HistAppend(ta + OFF_CHAT_TEXT + j * CHAT_ENTRY_STRIDE);
		g_histLastFree = freeIndex;
	}

	unsigned char* TaBase()
	{
		__try { return *(unsigned char**)0x00511DE8; }
		__except (EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
	}

	bool LooksLikeOffscreen(const OFFSCREEN* p)
	{
		if (!p)
			return false;
		__try
		{
			if (!p->lpSurface)                        return false;
			if (p->Height <= 0 || p->Height > 8192)   return false;
			if (p->lPitch <= 0 || p->lPitch > 65536)  return false;
			if (p->ScreenRect.right <= 0 ||
			    p->ScreenRect.right > 8192)           return false;
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) { return false; }
	}

	// Engine's per-line visibility test (jump table @0x464270).
	bool ChatLineVisible(int mode, int screenchat, int channel)
	{
		switch (mode)
		{
		case 1:  return channel == 2;
		case 2:  return channel != 8;
		case 3:  return screenchat != 0
		              || channel == 1 || channel == 4 || channel == 8;
		default: return false;
		}
	}

	// Lower-cases, trims at the first ';' / whitespace.
	void CleanToken(char* buf)
	{
		_strlwr_s(buf, strlen(buf) + 1);
		for (char* p = buf; *p; ++p)
			if (*p == ';' || *p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
			{ *p = '\0'; break; }
	}

	Renderer ParseRenderer(const char* s)
	{
		if (!s || !*s)
			return R_Engine;
		char buf[32];
		strncpy_s(buf, sizeof(buf), s, _TRUNCATE);
		CleanToken(buf);
		if (0 == strcmp(buf, "tadr"))  return R_Tadr;
		return R_Engine;
	}

	// Comma/space list of kind names -> ChatSysGroups bitmask. Unknown tokens
	// are ignored; an empty/absent value keeps the default system set.
	unsigned ParseSysGroups(const char* s)
	{
		if (!s || !*s)
			return CHATGROUPS_DEFAULT_SYS;

		char buf[128];
		strncpy_s(buf, sizeof(buf), s, _TRUNCATE);
		for (char* p = buf; *p; ++p)
			if (*p == ';') { *p = '\0'; break; }
		_strlwr_s(buf, strlen(buf) + 1);

		unsigned mask = 0;
		bool any = false;
		char* ctx = nullptr;
		for (char* tok = strtok_s(buf, " ,\t", &ctx); tok; tok = strtok_s(nullptr, " ,\t", &ctx))
		{
			ChatKind k;
			if      (0 == strcmp(tok, "chat"))   k = CK_Chat;
			else if (0 == strcmp(tok, "ping"))   k = CK_Ping;
			else if (0 == strcmp(tok, "unit"))   k = CK_Unit;
			else if (0 == strcmp(tok, "event"))  k = CK_Event;
			else if (0 == strcmp(tok, "notice")) k = CK_Notice;
			else if (0 == strcmp(tok, "cmd"))    k = CK_Cmd;
			else if (0 == strcmp(tok, "other"))  k = CK_Other;
			else
			{
				continue;
			}
			mask |= ChatKindBit(k);
			any = true;
		}
		return any ? mask : CHATGROUPS_DEFAULT_SYS;
	}

	// Nearest entry in TA's fixed 256-colour palette (PCX.CPP's TAPalette[],
	// the same table UnitIcon.cpp uses for arbitrary-RGB -> palette). Squared
	// RGB distance.
	int NearestPaletteIndex(int r, int g, int b)
	{
		int  best     = 0;
		long bestDist = 0x7FFFFFFF;
		for (int i = 0; i < 256; ++i)
		{
			const long dr = (long)TAPalette[i].peRed   - r;
			const long dg = (long)TAPalette[i].peGreen - g;
			const long db = (long)TAPalette[i].peBlue  - b;
			const long dist = dr * dr + dg * dg + db * db;
			if (dist < bestDist)
			{
				bestDist = dist;
				best = i;
			}
		}
		return best;
	}

	// "RRGGBB" / "#RRGGBB" -> nearest palette index. Empty or malformed
	// (including the ini default "") -> false: ChatFontColor stays off.
	bool ParseHexColor(const char* s, int& outIndex)
	{
		if (!s || !*s)
			return false;
		if (*s == '#')
			++s;
		if (strlen(s) != 6)
			return false;
		char* end = nullptr;
		const unsigned long v = strtoul(s, &end, 16);
		if (!end || *end != '\0')
			return false;
		outIndex = NearestPaletteIndex((v >> 16) & 0xFF, (v >> 8) & 0xFF, v & 0xFF);
		return true;
	}

	// All three confirmed callers of 0x00464060 push a real OFFSCREEN* as the
	// single stdcall argument, so at this JMP-lagger hook [Esp+4] is always
	// it. There is deliberately no fallback to [Esp] -- that slot holds the
	// return address, not another candidate pointer, and treating it as one
	// would hand FillBehind/ChatFont::DrawString an address chosen by
	// whatever bytes happen to sit at the call site.
	OFFSCREEN* ResolveOffscreen(PInlineX86StackBuffer buf)
	{
		__try
		{
			OFFSCREEN* a = *(OFFSCREEN**)(buf->Esp + 4);
			if (LooksLikeOffscreen(a))
				return a;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {}
		return nullptr;
	}

	// True while the chat compose prompt is open, i.e. the TALK.GUI panel is
	// in the engine's active GUI chain. Read-only; a closed panel is just gone
	// from the chain next frame, so no close hook is needed. SEH-guarded, any
	// fault -> not armed.
	bool ChatPromptOpen(unsigned char* ta)
	{
		__try
		{
			unsigned char* g = *(unsigned char**)(ta + OFF_ACTIVE_GUIMEM);
			for (int hops = 0; g && hops < 32; ++hops)
			{
				const char* nm = (const char*)(g + OFF_GUIMEM_NAME);
				if (nm[0] == 'T' && nm[1] == 'A' && nm[2] == 'L' && nm[3] == 'K' &&
				    (nm[4] == '\0' || nm[4] == '.'))
					return true;
				g = *(unsigned char**)(g + OFF_GUIMEM_PERACTIVE);
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {}
		return false;
	}

	// ChatFontColor / ChatFontOutline, applied only to ChatFont-drawn text.
	// When set, ChatFontColor replaces `remapColor` uniformly. The outline is
	// 8 one-pixel black copies drawn first, so the glyph stays legible over
	// any background.
	void DrawChatFontString(OFFSCREEN* off, const char* str, int x, int y, int remapColor)
	{
		const int color = g_fontColorSet ? g_fontColorIndex : remapColor;
		if (g_fontOutline)
		{
			static const int dx[8] = { -1, 0, 1, -1, 1, -1, 0, 1 };
			static const int dy[8] = { -1,-1,-1,  0, 0,  1, 1, 1 };
			for (int i = 0; i < 8; ++i)
				ChatFont::DrawString(off, str, x + dx[i], y + dy[i], TA_BLACK_INDEX);
		}
		ChatFont::DrawString(off, str, x, y, color);
	}

	// Draw one chat line (logo + optional backdrop + text) at (colX,curY).
	// `entry` is a 0x48-byte ring-shaped record (a live ring slot or a
	// byte-identical history copy). `useChatFont`: draw text with the ChatFont
	// atlas the caller already built for `h`, not the engine's native font.
	void DrawOneLine(OFFSCREEN* off, unsigned char* ta, const unsigned char* entry,
	                 int colX, int textX, int curY, int scaledH, int h,
	                 bool hasLogo, int slot, int highlight, bool backdrop,
	                 bool useChatFont)
	{
		const int remap = highlight ? ta[OFF_REMAP_HIGHLIGHT] : ta[OFF_REMAP_NORMAL];

		if (hasLogo)
		{
			RECT r;
			r.left = colX; r.top = curY;
			r.right = colX + scaledH; r.bottom = curY + scaledH;
			void* player = &reinterpret_cast<TAdynmemStruct*>(ta)->Players[slot];
			DrawPlayerLogo(off, player, &r, 0);
		}

		if (backdrop)
		{
			// Pass g_fontSize (the atlas key ChatFont::Ensure caches on), not
			// `h` (CellHeight()'s tmHeight, which Verdana frequently rounds to
			// a different value -- e.g. requested 15 builds a tmHeight-14
			// atlas). Passing `h` here made this Ensure() a different cache
			// key than WalkChat's, so every backdrop-enabled frame rebuilt
			// the whole glyph atlas twice (GDI CreateFontA + a 236-glyph
			// rasterise pass, each way) instead of hitting the cache.
			const int w = ChatBackdrop::MeasureLineWidth(entry, useChatFont, g_fontSize);
			if (w > 0)
				ChatBackdrop::FillBehind(off,
					colX - PAD_X_LEFT, curY - PAD_Y,
					textX + w + PAD_X_RIGHT, curY + h + PAD_Y);
		}

		if (useChatFont)
			DrawChatFontString(off, (const char*)entry, textX, curY, remap);
		else
			ChatBackdrop::DrawChatLine(off, (const char*)entry, textX, curY, remap);
	}

	// One walk of the chat ring. draw = true issues the engine draw calls (the
	// router then cancels the engine function); draw = false classifies only.
	// Returns the number of visible lines walked.
	int WalkChat(OFFSCREEN* off, bool draw)
	{
		unsigned char* ta = TaBase();
		if (!ta)
		{
			return 0;
		}

		const int textlines = *(int*)(ta + OFF_TEXTLINES);
		if (textlines <= 0)
		{
			return 0;
		}

		unsigned char* comix = *(unsigned char**)(ta + OFF_COMIX_FONT);
		int h = comix ? comix[0] : 0;   // one byte
		if (h <= 0 || h >= 128)
		{
			return 0;
		}

		// ChatFontSize: draw with the ChatFont TTF atlas at a configured pixel
		// height. `h` drives line spacing, the logo square and the fit math
		// for both columns, so overriding it here scales everything uniformly.
		// Any failure falls back to the native height and font.
		bool useChatFont = false;
		if (g_renderer == R_Tadr && g_fontSize > 0 && ChatFont::Ensure(g_fontSize))
		{
			useChatFont = true;
			const int cellH = ChatFont::CellHeight();
			if (cellH > 0)
				h = cellH;
		}

		const int freeIndex  = *(unsigned short*)(ta + OFF_FREE_INDEX);
		const int chatNum     = *(unsigned short*)(ta + OFF_CHAT_NUM);
		const int mode        = *(int*)(ta + OFF_CHAT_MODE);
		const int screenchat  = *(int*)(ta + OFF_SCREENCHAT);
		if (freeIndex < 0 || freeIndex >= CHAT_ENTRY_COUNT ||
		    chatNum   < 0 || chatNum   >= CHAT_ENTRY_COUNT)
		{
			return 0;
		}

		const int anchorX = ChatPosition::X();
		const int anchorY = ChatPosition::Y();
		const int sysX = g_split ? VANILLA_X : anchorX;
		const int sysY = g_split ? VANILLA_Y : anchorY;
		const int scaledH = (int)(0.8 * (double)h);   // == ftol(0.8*h); logo square side

		const bool backdrop = draw && ChatBackdrop::BackdropEnabled();

		// The engine selects the COMIX font once before the loop; do the same
		// so the per-line ChatBackdrop::DrawChatLine calls below act on the
		// right object.
		if (draw && comix)
			Font_SetCurrent(comix);

		// Walk back to the oldest still-visible entry: the whole 30-entry ring
		// (ChatPosition stops throttling `textlines` once we take over); how
		// many of those lines draw is decided per column below.
		const int backSteps = (g_renderer == R_Tadr)
			? CHAT_ENTRY_COUNT
			: (textlines < CHAT_ENTRY_COUNT ? textlines : CHAT_ENTRY_COUNT);
		int idx = freeIndex;
		for (int n = 1; n < backSteps; ++n)
		{
			if (idx == chatNum)
				break;
			if (--idx < 0)
				idx = CHAT_ENTRY_COUNT - 1;
		}

		// Tee new ring lines into the retained history (tadr only, passive
		// read). `idx` is the oldest visible entry, reused to seed.
		if (g_renderer == R_Tadr)
			HistoryConsume(ta, idx, freeIndex);

		// Scrollback: while the prompt is open, gather the player column's
		// mode-visible lines from history, newest-first, into g_sbPick. The
		// system column is unaffected.
		const unsigned retained = (g_histCount < (unsigned)HIST_CAP)
			? g_histCount : (unsigned)HIST_CAP;
		int sbN = 0;
		bool sbArmed = false;
		if (g_renderer == R_Tadr && retained > 0 && ChatPromptOpen(ta))
		{
			for (unsigned k = 0; k < retained && sbN < HIST_CAP; ++k)
			{
				const unsigned si = (unsigned)((g_histCount - 1u - k) % (unsigned)HIST_CAP);
				const unsigned char* e = g_hist[si];
				const int ch = e[OFF_STR_FLAGS] & 0x0F;
				if (!ChatLineVisible(mode, screenchat, ch))
					continue;
				const int al = *(unsigned short*)(e + OFF_STR_ALERT);
				const int sl = e[OFF_STR_LOGO];
				const ChatKind kk = ChatClassify(ch, al, sl, (char)e[0]);
				if (g_split && (g_sysGroups & ChatKindBit(kk)) != 0)
					continue;                       // that line belongs to the system column
				g_sbPick[sbN++] = (int)si;
			}
			sbArmed = (sbN > 0);
		}
		if (!sbArmed)
			g_sbOffset = 0;
		g_sbArmed = sbArmed;

		// Pre-pass: count visible lines per column, so an upward-growing player
		// column can start high enough for its newest line to land on the anchor.
		int plrCount = 0, sysCount = 0;
		{
			int j = idx;
			while (j != freeIndex)
			{
				unsigned char* e = ta + OFF_CHAT_TEXT + j * CHAT_ENTRY_STRIDE;
				const int ch = e[OFF_STR_FLAGS] & 0x0F;
				if (ChatLineVisible(mode, screenchat, ch))
				{
					const int al = *(unsigned short*)(e + OFF_STR_ALERT);
					const int sl = e[OFF_STR_LOGO];
					const ChatKind k = ChatClassify(ch, al, sl, (char)e[0]);
					if (g_split && (g_sysGroups & ChatKindBit(k)) != 0) ++sysCount;
					else ++plrCount;
				}
				if (++j == CHAT_ENTRY_COUNT)
					j = 0;
			}
		}

		if (sbArmed)
			plrCount = sbN;   // player column is fed from history, not the live ring

		// Per-column line budget (tadr only): draw the newest lines of each
		// column, trimming the oldest when it holds more than its ini cap
		// (ChatLines / ChatSysLines) or than physically fits before the screen
		// edge it grows toward. engine / probe leave plrShow == plrCount.
		int plrShow = plrCount, sysShow = sysCount;
		int plrSkip = 0,        sysSkip = 0;
		int capPlr = CHAT_ENTRY_COUNT, fitPlr = CHAT_ENTRY_COUNT;   // hoisted: reused by the scrollback block
		if (g_renderer == R_Tadr)
		{
			int screenH = LocalShare ? LocalShare->ScreenHeight : 0;
			if (screenH <= 0 || screenH > 8192)
				screenH = (off && off->ScreenRect.bottom > 0) ? off->ScreenRect.bottom + 1 : 0;

			int fitSys = CHAT_ENTRY_COUNT;
			if (screenH > 0)
			{
				const int bottomLimit = screenH - HUD_BOTTOM;
				// Whole lines that fit before the column hits the HUD bar it
				// grows toward: grow=up climbs from anchorY to HUD_TOP,
				// grow=down / the system column descend toward bottomLimit.
				fitPlr = g_growUp ? ((anchorY - HUD_TOP) / h + 1)
				                  : ((bottomLimit - anchorY) / h);
				fitSys = (bottomLimit - sysY) / h;
				if (fitPlr < 1) fitPlr = 1;   // always draw at least the newest
				if (fitSys < 1) fitSys = 1;
				if (fitPlr > CHAT_ENTRY_COUNT) fitPlr = CHAT_ENTRY_COUNT;
				if (fitSys > CHAT_ENTRY_COUNT) fitSys = CHAT_ENTRY_COUNT;
			}
			capPlr = g_plrLines > 0 ? g_plrLines : CHAT_ENTRY_COUNT;
			const int capSys = g_sysLines > 0 ? g_sysLines : CHAT_ENTRY_COUNT;

			if (plrShow > capPlr) plrShow = capPlr;
			if (plrShow > fitPlr) plrShow = fitPlr;
			if (sysShow > capSys) sysShow = capSys;
			if (sysShow > fitSys) sysShow = fitSys;

			plrSkip = plrCount - plrShow;
			sysSkip = sysCount - sysShow;
		}

		// System column: pinned top-left, grows down. Player column: grows down
		// from the anchor by default; with ChatGrow=up it starts high enough
		// that the newest line lands on the anchor. Uses plrShow (post-trim).
		int plrY = anchorY;
		if (g_renderer == R_Tadr && g_growUp && plrShow > 1)
		{
			plrY = anchorY - (plrShow - 1) * h;
			if (plrY < HUD_TOP)
				plrY = HUD_TOP;   // never draw under the top HUD bar
		}
		int sysCursorY = sysY;
		int drawnSys = 0, drawnPlr = 0;
		int plrSeen  = 0, sysSeen  = 0;   // visible lines encountered per column (for the oldest-first trim)

		while (idx != freeIndex)
		{
			unsigned char* entry = ta + OFF_CHAT_TEXT + idx * CHAT_ENTRY_STRIDE;
			const int channel   = entry[OFF_STR_FLAGS] & 0x0F;
			const int alert     = *(unsigned short*)(entry + OFF_STR_ALERT);
			const int slot      = entry[OFF_STR_LOGO];
			const int highlight = entry[OFF_STR_FLAGS] & 0x20;

			if (ChatLineVisible(mode, screenchat, channel))
			{
				const ChatKind kind = ChatClassify(channel, alert, slot, (char)entry[0]);
				const bool toSys = g_split && (g_sysGroups & ChatKindBit(kind)) != 0;

				// Oldest-first trim: the first plrSkip / sysSkip visible lines
				// of a capped column are walked but not drawn and do not
				// advance the cursor.
				int& seenCol = toSys ? sysSeen : plrSeen;
				++seenCol;
				const bool trimmed = seenCol <= (toSys ? sysSkip : plrSkip);
				// While scrollback is armed the player column draws from
				// history below, so skip its live-ring lines here.
				const bool skip = trimmed || (sbArmed && !toSys);

				const int colX = toSys ? sysX : anchorX;
				int&      curY = toSys ? sysCursorY : plrY;

				const bool hasLogo = (slot >= 0 && slot < 10);   // 10 = system; >10 = don't blit
				int textX = colX;
				if (hasLogo)
					textX = (int)((double)colX + 1.5 * (double)scaledH);

				if (draw && !skip)
					DrawOneLine(off, ta, entry, colX, textX, curY, scaledH, h,
					            hasLogo, slot, highlight, backdrop, useChatFont);

				if (!skip)
				{
					curY += h;
					if (toSys) ++drawnSys; else ++drawnPlr;
				}
			}

			if (++idx == CHAT_ENTRY_COUNT)
				idx = 0;
		}

		// Scrollback: draw the player column from history. g_sbPick[0..sbN-1]
		// are history indices newest-first; show the newest `show` of them,
		// offset by g_sbOffset (clamped below), oldest-of-window first.
		if (sbArmed && g_renderer == R_Tadr)
		{
			int show = sbN;
			if (show > capPlr) show = capPlr;
			if (show > fitPlr) show = fitPlr;
			if (show < 1) show = 1;

			int maxOff = sbN - show;
			if (maxOff < 0) maxOff = 0;
			if (g_sbOffset > maxOff) g_sbOffset = maxOff;
			if (g_sbOffset < 0)      g_sbOffset = 0;

			// While scrolled away from the live tail, spend the row nearest
			// the anchor on a "-- N newer --" readout instead of a real line,
			// so it is unambiguous that this is history. Costs one line of the
			// column budget.
			const bool showIndicator = g_sbOffset > 0 && show > 1;

			int y = anchorY;
			if (g_growUp && show > 1)
			{
				y = anchorY - (show - 1) * h;
				if (y < HUD_TOP) y = HUD_TOP;
			}

			for (int i = show - 1; i >= 0; --i)
			{
				if (i == 0 && showIndicator)
				{
					if (draw)
					{
						char status[64];
						_snprintf(status, sizeof(status) - 1,
						          "-- %d newer (send/Esc = live) --", g_sbOffset);
						status[sizeof(status) - 1] = '\0';
						const int remap = ta[OFF_REMAP_NORMAL];
						if (useChatFont)
							DrawChatFontString(off, status, anchorX, y, remap);
						else
							ChatBackdrop::DrawChatLine(off, status, anchorX, y, remap);
					}
				}
				else
				{
					const int pick = g_sbOffset + i;
					if (pick < 0 || pick >= sbN) continue;
					const unsigned char* e = g_hist[g_sbPick[pick]];
					const int sl = e[OFF_STR_LOGO];
					const int hl = e[OFF_STR_FLAGS] & 0x20;
					const bool hasLogo = (sl >= 0 && sl < 10);   // 10 = system; >10 = don't blit
					int textX = anchorX;
					if (hasLogo)
						textX = (int)((double)anchorX + 1.5 * (double)scaledH);
					if (draw)
						DrawOneLine(off, ta, e, anchorX, textX, y, scaledH, h,
						            hasLogo, sl, hl, backdrop, useChatFont);
				}
				y += h;
				++drawnPlr;
			}
		}

		return drawnSys + drawnPlr;
	}

	// The 0x00464060 router: draw the list, then cancel the engine function.
	// Only ever installed/active when g_renderer == R_Tadr (see Install()).
	unsigned int ChatLayoutRouter(PInlineX86StackBuffer buf)
	{
		if (!g_active)
			return 0;

		void* rtnAddr = nullptr;
		__try { rtnAddr = *(void**)(buf->Esp); }
		__except (EXCEPTION_EXECUTE_HANDLER) { return 0; }

		OFFSCREEN* off = ResolveOffscreen(buf);
		if (!off)
		{
			return 0;   // engine draws
		}

		ChatPosition::EnsureApplied();

		__try
		{
			WalkChat(off, true);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return 0;   // fail safe -- let the engine draw
		}

		// Suppress the engine function. Return to its caller with the stack
		// cleaned as a `ret 4` would leave it (pop return addr + 1 arg).
		buf->Esp = buf->Esp + 4 + 4;
		buf->rtnAddr_Pvoid = rtnAddr;
		return X86STRACKBUFFERCHANGE;
	}
}

void ChatLayout::Install()
{
	if (g_hook || g_active)
		return;

	if (MyConfig)
	{
		char buf[32] = { 0 };
		MyConfig->GetIniStr("ChatRenderer", buf, sizeof(buf), (LPSTR)"engine");
		g_renderer = ParseRenderer(buf);

		g_split = MyConfig->GetIniInt("ChatSplit", 0) != 0;

		char grp[128] = { 0 };
		MyConfig->GetIniStr("ChatSysGroups", grp, sizeof(grp), (LPSTR)"");
		g_sysGroups = ParseSysGroups(grp);

		char grow[16] = { 0 };
		MyConfig->GetIniStr("ChatGrow", grow, sizeof(grow), (LPSTR)"down");
		CleanToken(grow);
		g_growUp = (0 == strcmp(grow, "up"));

		// Per-column line budgets. 0 = fit as many of the 30 ring entries as
		// the screen allows; a positive value pins that many newest lines.
		g_plrLines = MyConfig->GetIniInt("ChatLines", 0);
		g_sysLines = MyConfig->GetIniInt("ChatSysLines", 0);
		if (g_plrLines < 0) g_plrLines = 0;
		if (g_sysLines < 0) g_sysLines = 0;
		if (g_plrLines > CHAT_ENTRY_COUNT) g_plrLines = CHAT_ENTRY_COUNT;
		if (g_sysLines > CHAT_ENTRY_COUNT) g_sysLines = CHAT_ENTRY_COUNT;

		// 0 = native bitmap font. >0 = ChatFont TTF atlas at that pixel cell
		// height, both columns. Clamped to ChatFont::Ensure's ceiling; a size
		// that fails to rasterise falls back to native per frame.
		g_fontSize = MyConfig->GetIniInt("ChatFontSize", 0);
		if (g_fontSize < 0)   g_fontSize = 0;
		if (g_fontSize > 128) g_fontSize = 128;

		// ChatFontColor resolved once here against the fixed palette (PCX.CPP's
		// TAPalette[], available from process start unlike the live DirectDraw
		// palette). Blank/malformed -> off. ChatFontSize-only.
		char colorBuf[16] = { 0 };
		MyConfig->GetIniStr("ChatFontColor", colorBuf, sizeof(colorBuf), (LPSTR)"");
		CleanToken(colorBuf);   // GetIniStr keeps a trailing ';' comment; every other
		                        // string key here strips it explicitly
		g_fontColorSet = ParseHexColor(colorBuf, g_fontColorIndex);

		g_fontOutline = MyConfig->GetIniInt("ChatFontOutline", 0) != 0;   // ChatFontSize-only
	}

	// Shift the bottomcenter anchor to one line above the bar only when we
	// actually draw upward (tadr); engine draws down and keeps the band.
	ChatPosition::SetGrowUp(g_growUp && g_renderer == R_Tadr);

	if (g_renderer == R_Engine)
	{
		IDDrawSurface::OutptTxt("[ChatLayout] ChatRenderer=engine (default) - not installed");
		return;
	}

	if (!BytesMatch(CHAT_DRAW_ADDR, kExpectedChatDraw, sizeof(kExpectedChatDraw)))
	{
		IDDrawSurface::OutptTxt("[ChatLayout] unexpected bytes at 0x00464060 - not installed");
		return;
	}

	g_hook = new InlineSingleHook(
		CHAT_DRAW_ADDR, 5, INLINE_5BYTESLAGGERJMP, ChatLayoutRouter);
	g_active = true;

	IDDrawSurface::OutptFmtTxt(
		"[ChatLayout] ChatRenderer=tadr split=%d grow=%s groups=0x%X lines=%d/%d fontsize=%d "
		"fontcolor=%d/%d outline=%d - hook on 0x%08X (engine draw cancelled)",
		g_split ? 1 : 0, g_growUp ? "up" : "down",
		g_sysGroups, g_plrLines, g_sysLines, g_fontSize,
		g_fontColorSet ? 1 : 0, g_fontColorIndex, g_fontOutline ? 1 : 0, CHAT_DRAW_ADDR);
}

void ChatLayout::Shutdown()
{
	if (g_hook)
	{
		delete g_hook;
		g_hook = nullptr;
	}
	g_active = false;
}

bool ChatLayout::Active()
{
	return g_active;
}

bool ChatLayout::TakingOver()
{
	return g_active && g_renderer == R_Tadr;
}

bool ChatLayout::ScrollbackWheel(int wheelDeltaRaw)
{
	// Checks the engine directly rather than trusting g_sbArmed, which is
	// only refreshed on a frame where WalkChat actually runs to completion --
	// several early-outs above (bad ta/textlines/h/freeIndex) and the
	// router's own SEH catch can all skip that update, and a stale `true`
	// would keep swallowing the wheel event ahead of WheelZoom /
	// WheelMoveMegaMap / build-rotate for good. Mirrors WalkChat's own
	// "armed" condition (retained history exists and the prompt is open).
	if (g_renderer != R_Tadr || g_histCount == 0)
		return false;

	unsigned char* ta = TaBase();
	if (!ta || !ChatPromptOpen(ta))
		return false;

	// WHEEL_DELTA (120) per notch. WalkChat re-clamps g_sbOffset every frame,
	// so an out-of-range value here corrects itself next frame.
	const int notches = wheelDeltaRaw / 120;
	g_sbOffset += notches;
	if (g_sbOffset < 0)
		g_sbOffset = 0;

	return true;
}

