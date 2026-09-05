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

// ---------------------------------------------------------------------------
// Stage 4b -- TADR owns Hud_DrawChatHudRing @0x00464060.
//
// Verified against TotalA.exe (SHA-matched to the shipped game), and against
// the Stage 4a dry run (tdrawlog: [ChatLayout] PLAN == [ChatEngine] DRAW,
// every x / y / hash, 7 lines, wrap included, 23k frames, zero guard trips):
//
//   * 0x464060 = int __stdcall(OFFSCREEN*), `ret 4`. Prologue 83 ec 20 53 55
//     = `sub esp,0x20 / push ebx / push ebp` -- 3 whole instructions in the
//     first 5 bytes, safe for a lagger-jmp to replay OR to cancel.
//   * caller 0x469FCB pushes the OFFSCREEN* and does no cleanup after the
//     call -> at the hooked instruction [Esp]=return addr, [Esp+4]=OFFSCREEN*.
//   * ring walk: idx = freeIndex(word @+0x2A3E); step back up to textlines-1
//     entries (int @+0x37F27), stop at chatNum(word @+0x2A40); then forward
//     to freeIndex, drawing each visible line.
//   * visibility (mode int @+0x37EFE, screenchat int @+0x37F02, channel =
//     entry[0x47]&0x0F): 1 -> channel==2; 2 -> channel!=8; 3 -> screenchat!=0
//     || channel in {1,4,8}; else invisible (jump table @0x464270 decoded).
//   * line height h = COMIX FontDataStruct byte 0 (*(u8*)*(ta+0x391F9)); the
//     engine gets it via Font_SetCurrent(that)/Font_GetLineHeight(), we read
//     the byte directly (proven equal in 4a: h=14).
//   * per line, engine order: Font_SetColors(0xFE, remap) where remap =
//     (entry[0x47]&0x20) ? ta[0xDD5] : ta[0xDDA]; if entry[0x46] != 10 a logo:
//     scaledH = ftol(0.8*h); RECT{colX, y, colX+scaledH, y+scaledH};
//     DrawPlayerLogo(off, &Players[slot], &RECT, 0) @0x467C00; and the line's
//     text X = ftol(colX + 1.5*scaledH). No logo -> text X = colX.
//   * text: DrawColorTextInScreen(off, entry, textX, y, -1, 0) @0x4A50E0.
//   * Y advances by h per visible line, from ChatPosition::Y().
//
// FAIL-SAFE: the router captures the return address, then does everything
// inside SEH. On ANY fault or unresolved pointer it `return 0`s -- the engine
// then draws normally. It can never blank the chat; the worst case is a
// double-drawn frame.
// ---------------------------------------------------------------------------

namespace
{
	const unsigned CHAT_DRAW_ADDR = 0x00464060;

	// Ring layout -- identical to ChatBackdrop.cpp / the disassembly.
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
	const unsigned OFF_GAME_TIME     = 0x38A47;   // int; goes backwards on a new match / replay rewind
	const unsigned OFF_PLAYERS       = 0x1B63;
	const unsigned PLAYER_STRIDE     = 331;
	const unsigned OFF_REMAP_NORMAL    = 0xDDA;
	const unsigned OFF_REMAP_HIGHLIGHT = 0xDD5;

	// Active GUI panel chain -- ta->desktopGUI.TheActive_GUIMEM, walked by
	// per_active (see unitrotate.cpp / tamem.h _GUIInfo, _GUIMEMSTRUCT).
	// Step 2 arms scrollback while the chat-entry panel is in this chain; no
	// hook, no "chat closed" signal needed (ENGINE_NOTES 26.6) -- a panel that
	// closed is simply not in the chain next frame.
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

	// TAPalette index 0 is pure black (verified fact, already relied on by
	// ChatBackdrop.cpp's backdrop fill) -- the outline colour for ChatFontOutline.
	const int TA_BLACK_INDEX = 0;

	// Engine functions this drawer calls. Signatures from the disassembly;
	// none are bound elsewhere in the codebase (DrawColorTextInScreen is, via
	// tafunctions.h).
	typedef void (__stdcall* Fn_FontSetCurrent)(void* fontDataStruct);  // 0x4C1420, ret 4
	typedef void (__stdcall* Fn_FontSetColors)(int a, int b);           // 0x4C13A0, ret 8
	typedef void (__stdcall* Fn_DrawPlayerLogo)(void* off, void* player, RECT* r, int flag); // 0x467C00, ret 0x10
	const Fn_FontSetCurrent  Font_SetCurrent  = (Fn_FontSetCurrent) 0x004C1420;
	const Fn_FontSetColors   Font_SetColors   = (Fn_FontSetColors)  0x004C13A0;
	const Fn_DrawPlayerLogo  DrawPlayerLogo   = (Fn_DrawPlayerLogo) 0x00467C00;

	enum Renderer { R_Engine = 0, R_Probe, R_Tadr };

	Renderer          g_renderer  = R_Engine;
	bool              g_split     = false;
	bool              g_growUp    = false;   // ChatGrow: false=down (oldest on anchor), true=up (newest on anchor)
	unsigned          g_sysGroups = CHATGROUPS_DEFAULT_SYS;
	int               g_plrLines  = 0;       // ChatLines:    0 = auto-fit to screen, >0 = that many NEWEST player-column lines
	int               g_sysLines  = 0;       // ChatSysLines: 0 = auto-fit to screen, >0 = that many NEWEST system-column lines
	int               g_fontSize  = 0;       // ChatFontSize: 0 = engine's native bitmap font, >0 = ChatFont TTF atlas at that pixel height (both columns)
	bool              g_fontColorSet   = false; // ChatFontColor given and parsed OK -- overrides the per-line player-colour remap for ChatFont-drawn text
	int               g_fontColorIndex = 0;     // nearest TAPalette[] index to the requested RGB (resolved once at Install(); see NearestPaletteIndex)
	bool              g_fontOutline    = false; // ChatFontOutline: draw an 8-direction 1px black outline behind ChatFont text (legibility over any background)
	bool              g_active    = false;
	InlineSingleHook* g_hook      = nullptr;
	unsigned          g_frames    = 0;

	// -----------------------------------------------------------------------
	// Stage 4c-b (step 1): retained chat history.
	//
	// The engine ring is only 30 entries. To scroll back further we keep our
	// own copy: every line the ring gains is appended here, oldest evicted
	// once we pass HIST_CAP. This step is PASSIVE -- the buffer is filled and
	// (instrumented builds) dumped, but nothing is drawn from it yet. It is a
	// pure read of memory the engine already owns into a TADR-owned array, so
	// it cannot affect simulation or the picture.
	//
	// Feed: once per tadr frame WalkChat calls HistoryConsume(), which diffs
	// the ring's freeIndex against the last one it saw and appends whatever
	// slots were written in between (the ring advances freeIndex by exactly 1
	// per line, wrapping at 30 -- verified via the mutetest head_before/after
	// fields). The first call seeds from the currently-visible ring so a
	// mid-game start is not blank. Entries are stored byte-identical to ring
	// entries (0x48 stride) so ChatClassify and the draw path will accept
	// them unchanged when step 2 renders from here.
	//
	// PER-MATCH, not per-process. Scrollback is for looking back within the
	// current match; carrying chat across matches is explicitly not wanted.
	// HistoryConsume() watches ta->GameTime and drops the buffer when the clock
	// goes backwards (new match, or replay rewind) -- the same new-game signal
	// AreaDamageOverflow and ShareGuard already use. The engine chat ring is
	// NOT cleared between games (ENGINE_NOTES 26, "ring is not cleared between
	// games"), so the reset must NOT re-seed from the ring -- it resyncs delta
	// tracking to the current freeIndex and lets the fresh match fill in.
	const int      HIST_CAP = 512;
	unsigned char  g_hist[HIST_CAP][CHAT_ENTRY_STRIDE];
	unsigned       g_histCount        = 0;     // lines appended in the current match (monotonic within a match)
	bool           g_histSeeded       = false;
	int            g_histLastFree     = 0;     // ring freeIndex at the last consume
	int            g_histLastGameTime = -1;    // ta->GameTime at the last consume; -1 = never seen
	bool           g_histFrozen       = false; // FreezeHistory()/ThawHistory() -- suite bracket (always false in a stripped build)

	// Stage 4c-b step 2: scrollback. While the chat compose prompt is open
	// (TALK.GUI in the active GUI chain) the player column is drawn from the
	// retained history at g_sbOffset (0 = live tail) instead of the live ring.
	// The system column stays live. g_sbOffset is moved by the wheel in step 3;
	// it resets to 0 whenever the prompt is not open.
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

		// New match / replay rewind: the engine game clock has gone backwards.
		// Drop the previous match's history. The engine ring is NOT cleared
		// between games, so do NOT re-seed from it -- mark seeded, point delta
		// tracking at the current freeIndex, and let real new lines fill in.
		const int gameTime = *(int*)(ta + OFF_GAME_TIME);
		if (gameTime < g_histLastGameTime)
		{
			g_histCount    = 0;
			g_histSeeded   = true;
			g_histLastFree = freeIndex;
			g_histFrozen   = false;
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

		// Frozen (a .mutetest run is churning the ring): stay in sync but
		// append nothing. ThawHistory() jumps g_histLastFree past the batch.
		if (g_histFrozen)
			return;

		if (freeIndex == g_histLastFree)
			return;

		// Both indices are in [0,30) (WalkChat guards freeIndex before the
		// call), so this mod-30 gap is always 1..29 -- it cannot distinguish
		// "advanced 3" from "advanced 33". The distinction does not matter:
		// every ring slot always holds one of the last 30 real lines, so a
		// pathological multi-wrap frame (>=30 lines at once -- not reachable
		// at frame rate) would append real recent content, at worst repeating
		// a line. It never appends stale/garbage bytes.
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

	// Engine's per-line visibility test -- jump table @0x464270 decoded.
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
		if (0 == strcmp(buf, "probe")) return R_Probe;
		return R_Engine;
	}

	// "unit,cmd,event,notice,other" -> bitmask of ChatKindBit(). Unknown
	// tokens are ignored (and, in instrumented builds, named once). An empty
	// / absent value keeps the default system set.
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

	// TA/Escalation's fixed 256-colour palette (PCX.CPP) -- the same table
	// UnitIcon.cpp already relies on to convert an arbitrary RGB bitmap into
	// legitimate indices for this game's real palette. Reused here rather
	// than guessed at: nearest-match search by squared RGB distance.
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

	// "RRGGBB" or "#RRGGBB" -> nearest TAPalette index in *outIndex. Empty or
	// malformed input (including the ini's absent-key default of "") -> false,
	// unchanged: ChatFontColor is off and ChatFont text keeps the per-line
	// player-colour remap it has always used.
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

	OFFSCREEN* ResolveOffscreen(PInlineX86StackBuffer buf)
	{
		__try
		{
			OFFSCREEN* a = *(OFFSCREEN**)(buf->Esp + 4);
			if (LooksLikeOffscreen(a))
				return a;
			OFFSCREEN* b = *(OFFSCREEN**)(buf->Esp);
			if (LooksLikeOffscreen(b))
				return b;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {}
		return nullptr;
	}

	// True while the in-game chat compose prompt is open -- i.e. the TALK.GUI
	// panel is somewhere in the engine's active GUI chain
	// (ta->desktopGUI.TheActive_GUIMEM, walked by per_active). Purely a read:
	// a closed panel is simply gone from the chain next frame, which is why
	// this needs no "chat closed" hook (ENGINE_NOTES 26.6). SEH-guarded; any
	// fault -> not armed -> live ring.
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

	// ChatFontColor / ChatFontOutline -- applied only to ChatFont-drawn text;
	// the native engine font branch is untouched, exactly as before either
	// feature existed. `remapColor` is the per-line player-colour index
	// (ta[OFF_REMAP_NORMAL]/HIGHLIGHT]) that ChatFont text has always used;
	// ChatFontColor, when set, replaces it uniformly (every ChatFont line,
	// highlighted or not, draws in the one configured colour). The outline is
	// 8 one-pixel offset copies in solid black drawn first, so the glyph
	// reads over any background the game happens to be showing underneath --
	// the reason it's asked for at all is that a single flat colour can
	// disappear into a similarly-coloured battlefield tile or unit.
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
	// `entry` is a 0x48-byte ring-shaped record -- a live ring slot or a
	// byte-identical retained-history copy; both render the same way.
	// `useChatFont`: draw text with ChatFont's TTF atlas (already built for
	// `h` by the caller) instead of the engine's native font -- ChatFontSize.
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
			void* player = ta + OFF_PLAYERS + slot * PLAYER_STRIDE;
			DrawPlayerLogo(off, player, &r, 0);
		}

		if (backdrop)
		{
			// lineHOverride = h whenever ChatFont is driving the draw, so the
			// backdrop box is measured against the SAME atlas/height that is
			// about to render the text (see MeasureChatLineWidth's atlas-
			// thrash note -- this is the fix: one shared height, not two).
			const int w = ChatBackdrop::MeasureLineWidth(entry, useChatFont ? h : 0);
			if (w > 0)
				ChatBackdrop::FillBehind(off,
					colX - PAD_X_LEFT, curY - PAD_Y,
					textX + w + PAD_X_RIGHT, curY + h + PAD_Y);
		}

		if (useChatFont)
		{
			DrawChatFontString(off, (const char*)entry, textX, curY, remap);
		}
		else
		{
			Font_SetColors(0xFE, remap);
			DrawColorTextInScreen(off, (const char*)entry, textX, curY, -1, 0);
		}
	}

	// One walk of the chat ring. `draw` true  -> issue the engine draw calls
	// and cancel nothing here (the router cancels). `draw` false -> touch
	// nothing, just classify + (instrumented) log what would be drawn.
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
		int h = comix ? comix[0] : 0;   // ONE byte -- Font_GetLineHeight reads *(u8*)
		if (h <= 0 || h >= 128)
		{
			return 0;
		}

		// ChatFontSize: draw with ChatFont's TrueType atlas at a configured
		// pixel height instead of the engine's native bitmap font. Both
		// columns share one `h`, so this scales line spacing, the logo square,
		// and the per-column "how many lines fit" math for both uniformly --
		// exactly the same knobs that already drive the native-size drawer.
		// Any failure (atlas build fails, ChatFontSize unset) falls back to
		// the native engine height and font, untouched.
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
		// so Font_SetColors / DrawColorTextInScreen act on the right object.
		if (draw && comix)
			Font_SetCurrent(comix);

		// walk-back to the oldest still-visible entry.
		//
		// TADR walks the WHOLE 30-entry ring so no history is lost to the
		// engine's `textlines` window -- ChatPosition stops throttling that
		// value once we take over (see ChatPosition::EnsureApplied). How many
		// of those lines actually get drawn is decided per column, below, by
		// ChatLines / ChatSysLines and by what physically fits on screen.
		//
		// `probe` still mirrors the engine's own window exactly, so the 4a
		// parity trace ([ChatLayout] PLAN == [ChatEngine] DRAW) stays valid.
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

		// Stage 4c-b (step 1): tee new ring lines into the retained history.
		// Passive -- reads only, and only when TADR owns the draw. `idx` here
		// is the oldest still-visible entry, reused to seed on the first call.
		if (g_renderer == R_Tadr)
			HistoryConsume(ta, idx, freeIndex);

		// --- scrollback arm + player-column source (tadr only) --------------
		// While the chat prompt is open, the player column is drawn from the
		// retained history: gather its player-column, mode-visible lines
		// newest-first into g_sbPick. The system column is unaffected.
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

		// Pre-pass: count visible lines per column. Needed so an upward-growing
		// player column can start high enough for the newest line to land on
		// the anchor. Cheap -- <=30 ring reads, no engine calls.
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

		// Scrollback: the player column is fed from history, not the live ring,
		// so its budget math and grow-up start work off the history count.
		if (sbArmed)
			plrCount = sbN;

		// --- per-column line budget (tadr only) ------------------------------
		// Draw the NEWEST lines of each column, trimming the oldest when the
		// column holds more visible lines than (a) its ini cap ChatLines /
		// ChatSysLines, or (b) what physically fits between the column's start
		// and the screen edge it grows toward. `plrSkip` / `sysSkip` oldest
		// visible lines in that column are then walked but not drawn.
		//
		// engine / probe are untouched: plrShow == plrCount, nothing trimmed,
		// so the 4a parity trace still matches the engine 1:1.
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
				// How many whole lines fit before the column runs into the HUD
				// bar it grows toward. grow=up climbs from anchorY (line 0) to
				// HUD_TOP, so it can add (anchorY-HUD_TOP)/h lines above line 0.
				// grow=down / the system column descend from their start toward
				// bottomLimit, so exactly (bottomLimit-start)/h whole lines fit.
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

		// The system column is pinned to the top-left and always grows down.
		// The player column grows down from the anchor by default (oldest drawn
		// line on the anchor); with ChatGrow=up it starts high enough that the
		// NEWEST line lands on the anchor and older lines extend upward. Uses
		// plrShow (post-trim), so a capped column still pins its newest line.
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

				// oldest-first trim: the first plrSkip / sysSkip visible lines
				// of a capped column are still walked and classified (so
				// KIND-FIRST is complete) but are not drawn and do not advance
				// the cursor -- only the NEWEST plrShow / sysShow lines render.
				int& seenCol = toSys ? sysSeen : plrSeen;
				++seenCol;
				const bool trimmed = seenCol <= (toSys ? sysSkip : plrSkip);
				// While scrollback is armed the player column is drawn from
				// history below, so skip its live-ring lines here (the system
				// column is unaffected).
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

		// --- scrollback: draw the player column from retained history --------
		// g_sbPick[0..sbN-1] are history indices newest-first. Show the newest
		// plrShow of them, offset by g_sbOffset (clamped so you cannot scroll
		// past the oldest retained line). Oldest-of-window first so grow-up /
		// grow-down match the live column.
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

			// Position indicator: while scrolled away from the live tail,
			// the single row nearest the anchor (always where the newest
			// line of the current window would land, by construction of the
			// `y` start formula below) is spent on a compact readout instead
			// of a real line, so it is never ambiguous that you are looking
			// at history, not the live feed. Costs exactly one line of the
			// existing per-column budget -- never grows past ChatLines or
			// what fits on screen.
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
						{
							Font_SetColors(0xFE, remap);
							DrawColorTextInScreen(off, status, anchorX, y, -1, 0);
						}
					}
				}
				else
				{
					const int pick = g_sbOffset + i;
					if (pick < 0 || pick >= sbN) continue;
					const unsigned char* e = g_hist[g_sbPick[pick]];
					const int sl = e[OFF_STR_LOGO];
					const int hl = e[OFF_STR_FLAGS] & 0x20;
					const bool hasLogo = (sl >= 0 && sl < 10);
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

	// The 0x00464060 router.
	//  R_Probe -> walk + log, return 0 (engine draws).
	//  R_Tadr  -> draw the list, then cancel the engine function.
	unsigned int ChatLayoutRouter(PInlineX86StackBuffer buf)
	{
		++g_frames;
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

		if (g_renderer == R_Tadr)
			ChatPosition::EnsureApplied();

		bool drew = false;
		__try
		{
			WalkChat(off, g_renderer == R_Tadr);
			drew = (g_renderer == R_Tadr);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return 0;   // fail safe -- let the engine draw
		}

		if (!drew)
			return 0;   // R_Probe: engine still draws

		// R_Tadr: suppress the engine function. Return to its caller with the
		// stack cleaned as a `ret 4` would leave it (pop return addr + 1 arg).
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

		// Per-column line budgets. 0 (default) = show as many of the 30-entry
		// ring as fit on screen; a positive value pins that many NEWEST lines.
		// Supersedes ChatBottomLines, which only ever throttled the engine's
		// own draw (and is left disabled once ChatRenderer=tadr takes over).
		g_plrLines = MyConfig->GetIniInt("ChatLines", 0);
		g_sysLines = MyConfig->GetIniInt("ChatSysLines", 0);
		if (g_plrLines < 0) g_plrLines = 0;
		if (g_sysLines < 0) g_sysLines = 0;
		if (g_plrLines > CHAT_ENTRY_COUNT) g_plrLines = CHAT_ENTRY_COUNT;
		if (g_sysLines > CHAT_ENTRY_COUNT) g_sysLines = CHAT_ENTRY_COUNT;

		// 0 (default) = the engine's native ~14px bitmap font, unchanged.
		// >0 = ChatFont's TrueType atlas at that pixel cell height, applied to
		// BOTH columns (one shared line height drives logo size, line
		// spacing, and the per-column fit math already). Clamped to
		// ChatFont::Ensure's own ceiling; a size that fails to rasterise
		// (e.g. 0 < n but the atlas build fails) falls back to native, per
		// frame, with no separate flag needed.
		g_fontSize = MyConfig->GetIniInt("ChatFontSize", 0);
		if (g_fontSize < 0)   g_fontSize = 0;
		if (g_fontSize > 128) g_fontSize = 128;

		// ChatFontColor: "RRGGBB" hex, or absent/blank/malformed (default) to
		// keep the per-line player-colour remap ChatFont text has always used.
		// Resolved once, here, against TA/Escalation's fixed palette (PCX.CPP's
		// TAPalette[]) -- available from process start, unlike the live
		// DirectDraw palette which isn't created until later. Only affects
		// text drawn by ChatFont (ChatFontSize > 0); the native engine font is
		// untouched.
		char colorBuf[16] = { 0 };
		MyConfig->GetIniStr("ChatFontColor", colorBuf, sizeof(colorBuf), (LPSTR)"");
		CleanToken(colorBuf);   // GetIniStr does not strip a trailing ';' comment -- every
		                        // other string key here does this explicitly (see ParseRenderer)
		g_fontColorSet = ParseHexColor(colorBuf, g_fontColorIndex);

		// ChatFontOutline: 1px 8-direction black outline behind ChatFont text,
		// so it stays legible over any background. Also ChatFontSize-only.
		g_fontOutline = MyConfig->GetIniInt("ChatFontOutline", 0) != 0;
	}

	// Only shift the bottomcenter anchor to band-bottom when we actually draw
	// upward (ChatRenderer=tadr). For probe/engine the engine still draws
	// downward and needs the reserved band above the bar.
	ChatPosition::SetGrowUp(g_growUp && g_renderer == R_Tadr);

	if (g_renderer == R_Engine)
	{
		IDDrawSurface::OutptTxt("[ChatLayout] ChatRenderer=engine (default) - not installed");
		return;
	}

	if (g_split && g_renderer != R_Tadr)
		IDDrawSurface::OutptTxt("[ChatLayout] ChatSplit=1 ignored - needs ChatRenderer=tadr");

	g_hook = new InlineSingleHook(
		CHAT_DRAW_ADDR, 5, INLINE_5BYTESLAGGERJMP, ChatLayoutRouter);
	g_active = true;

	IDDrawSurface::OutptFmtTxt(
		"[ChatLayout] ChatRenderer=%s split=%d grow=%s groups=0x%X lines=%d/%d fontsize=%d "
		"fontcolor=%d/%d outline=%d - hook on 0x%08X (%s)",
		g_renderer == R_Tadr ? "tadr" : "probe",
		(g_renderer == R_Tadr && g_split) ? 1 : 0, g_growUp ? "up" : "down",
		g_sysGroups, g_plrLines, g_sysLines, g_fontSize,
		g_fontColorSet ? 1 : 0, g_fontColorIndex, g_fontOutline ? 1 : 0, CHAT_DRAW_ADDR,
		g_renderer == R_Tadr ? "engine draw cancelled" : "DRY RUN, engine still draws");
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
	// Only while the chat prompt is open (g_sbArmed, refreshed every tadr
	// frame in WalkChat -- see ChatPromptOpen()). Every other wheel consumer
	// (WheelZoom, WheelMoveMegaMap, the SnapOverrideKey build-rotate gesture)
	// is untouched whenever this returns false.
	if (!g_sbArmed)
		return false;

	// 120 units (WHEEL_DELTA) per notch; wheel-up (away from the user, a
	// positive delta) is the conventional "scroll toward older" gesture.
	// WalkChat clamps g_sbOffset to [0, sbN-show] every frame, so an
	// out-of-range value here is harmless until the next frame corrects it.
	const int notches = wheelDeltaRaw / 120;
	g_sbOffset += notches;
	if (g_sbOffset < 0)
		g_sbOffset = 0;

	return true;
}

