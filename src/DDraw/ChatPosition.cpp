#include "config.h"
#include "ChatPosition.h"
#include "ChatLayout.h"    // ChatLayout::TakingOver() -- gates the textlines cap below

#include <windows.h>       // TAConfig.h assumes the Win32 typedefs are already in

#include "TAConfig.h"
#include "iddrawsurface.h"

#include <cstring>
#include <cstdlib>

namespace
{
	// ---- patch sites (see ChatPosition.h for the disassembly they came from)
	const DWORD ADDR_Y_IMM32   = 0x004640DA;   // mov dword ptr [esp+0x14], 0x34
	const DWORD ADDR_X_IMM32   = 0x00464187;   // mov ebx, 0x8a
	const DWORD ADDR_X_DISP32  = 0x004641C3;   // lea edx, [eax+0x8a]
	const DWORD ADDR_X_DOUBLE  = 0x004FD530;   // fsubr qword ptr [0x4fd530]  (= 138.0)

	// Vanilla values, used both as the restore image and as the topleft base.
	const int VANILLA_X = 138;   // 0x8A
	const int VANILLA_Y = 52;    // 0x34

	// HUD frame margins: a fixed 128px left sidebar and 32px top/bottom bars,
	// no scaling.
	const int HUD_LEFT   = 128;
	const int HUD_TOP    = 32;
	const int HUD_BOTTOM = 32;

	enum Anchor
	{
		AnchorNone = 0,      // module inert, engine left untouched
		AnchorTopLeft,
		AnchorTopCenter,
		AnchorBottomCenter
	};

	bool  g_installed   = false;
	bool  g_patched     = false;
	bool  g_growUp      = false;   // set by ChatPosition::SetGrowUp() -- see the bottomcenter branch
	Anchor g_anchor     = AnchorNone;
	int   g_offsetX     = 0;
	int   g_offsetY     = 0;
	bool  g_offsetXPct  = false;   // g_offsetX is a % of screen width, not pixels
	bool  g_offsetYPct  = false;   // g_offsetY is a % of screen height, not pixels

	// Lines the bottomcenter anchor keeps clear above the bottom HUD bar. The
	// engine draws the list top-down from the anchor, so anchoring at the bar
	// pushes everything past the first line or two off-screen; reserving this
	// many lines' height is the workaround. Overridable via ChatBottomLines.
	int   g_bottomLines = 8;

	// Last values actually written, so EnsureApplied() can early-out.
	int   g_appliedX    = VANILLA_X;
	int   g_appliedY    = VANILLA_Y;

	// Set by SetGrowUp() to force one re-resolve+re-patch on the next
	// EnsureApplied(), without corrupting g_appliedX/Y (which ChatPosition::X()/
	// Y() hand out directly) in the meantime.
	bool  g_needsReapply = false;

	// Non-zero once we have forced the engine's `textlines` down for the
	// bottomcenter anchor (see EnsureApplied). Shutdown restores the real
	// pre-patch value (g_origTextlines), captured the first time we write it.
	int   g_appliedTextlines = 0;
	int   g_origTextlines    = -1;   // -1 = not yet captured

	const unsigned OFF_TEXTLINES = 0x37F27;   // ta+0x37F27, dword

	// Original bytes, captured before the first write.
	DWORD  g_origYImm   = 0;
	DWORD  g_origXImm   = 0;
	DWORD  g_origXDisp  = 0;
	double g_origXDbl   = 0.0;

	// Write `len` bytes over read-only image memory and flush the icache.
	// Returns false and leaves the target untouched if the page cannot be
	// unprotected.
	bool PokeBytes(DWORD addr, const void* src, SIZE_T len)
	{
		DWORD oldProtect = 0;
		if (!VirtualProtect((LPVOID)addr, len, PAGE_EXECUTE_READWRITE, &oldProtect))
			return false;
		memcpy((void*)addr, src, len);
		VirtualProtect((LPVOID)addr, len, oldProtect, &oldProtect);
		FlushInstructionCache(GetCurrentProcess(), (LPCVOID)addr, len);
		return true;
	}

	bool PeekBytes(DWORD addr, void* dst, SIZE_T len)
	{
		__try
		{
			memcpy(dst, (const void*)addr, len);
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return false;
		}
	}

	unsigned char* TaBase()
	{
		__try
		{
			return *(unsigned char**)0x00511DE8;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return nullptr;
		}
	}

	// COMIX chat-font cell height, read from the live TA struct
	// (ta+0x391F9 -> FontDataStruct*, [0] = charHeight). 0 if not resolvable
	// yet -- callers fall back to a nominal value and self-correct next frame.
	int LineHeight()
	{
		__try
		{
			unsigned char* ta = TaBase();
			if (!ta)
				return 0;
			unsigned char* font = *(unsigned char**)(ta + 0x391F9);
			if (!font)
				return 0;
			const int lh = *(int*)font;
			return (lh > 0 && lh < 128) ? lh : 0;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return 0;
		}
	}

	Anchor ParseAnchor(const char* s)
	{
		if (!s || !*s)
			return AnchorNone;

		char buf[64];
		strncpy_s(buf, sizeof(buf), s, _TRUNCATE);
		_strlwr_s(buf, sizeof(buf));

		// Tolerate the trailing ';' the shipped ini uses on every entry, and
		// any stray whitespace around it.
		for (char* p = buf; *p; ++p)
		{
			if (*p == ';' || *p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
			{
				*p = '\0';
				break;
			}
		}

		if (0 == strcmp(buf, "topleft"))      return AnchorTopLeft;
		if (0 == strcmp(buf, "topcenter"))    return AnchorTopCenter;
		if (0 == strcmp(buf, "bottomcenter")) return AnchorBottomCenter;
		return AnchorNone;
	}

	// "138" / "-110" -> pixel offset; "12%" / "-8 %" -> percent of the screen
	// dimension, resolved per frame. Trailing ';' and spaces tolerated.
	// Unparseable -> 0.
	void ParseOffset(const char* s, int* val, bool* isPct)
	{
		*val   = 0;
		*isPct = false;
		if (!s)
			return;

		char buf[32];
		strncpy_s(buf, sizeof(buf), s, _TRUNCATE);

		for (char* p = buf; *p; ++p)
		{
			if (*p == ';') { *p = '\0'; break; }
			if (*p == '%') { *isPct = true; *p = ' '; }
		}

		*val = atoi(buf);   // tolerates leading spaces and a sign
	}

	int Clamp(int v, int lo, int hi)
	{
		if (hi < lo) return lo;
		return v < lo ? lo : (v > hi ? hi : v);
	}

	// Resolve the configured anchor against a screen size. Returns false when
	// the screen size is not yet known, in which case nothing should be
	// written.
	bool Resolve(int& outX, int& outY)
	{
		if (!LocalShare)
			return false;

		const int w = LocalShare->ScreenWidth;
		const int h = LocalShare->ScreenHeight;
		if (w <= HUD_LEFT || h <= HUD_TOP + HUD_BOTTOM)
			return false;

		int x = VANILLA_X;
		int y = VANILLA_Y;
		int maxY = h - 16;   // clamp ceiling for the anchor (top of the list)

		switch (g_anchor)
		{
		case AnchorTopLeft:
			x = VANILLA_X;
			y = VANILLA_Y;
			break;
		case AnchorTopCenter:
			x = HUD_LEFT + (w - HUD_LEFT) / 2;
			y = VANILLA_Y;
			break;
		case AnchorBottomCenter:
		{
			const int lh = (LineHeight() > 0 ? LineHeight() : 14);
			x = HUD_LEFT + (w - HUD_LEFT) / 2;
			if (g_growUp)
				y = h - HUD_BOTTOM - lh;                      // list climbs from one line above the bar
			else
				y = h - HUD_BOTTOM - lh * g_bottomLines;      // reserve g_bottomLines above the bar, list descends
			maxY = y;
			break;
		}
		default:
			return false;
		}

		x += g_offsetXPct ? (g_offsetX * w / 100) : g_offsetX;
		y += g_offsetYPct ? (g_offsetY * h / 100) : g_offsetY;

		// Keep the anchor inside the game viewport. Drawing left of the
		// sidebar or above the top bar puts text under HUD chrome, and the
		// engine's clipper is not prepared for negative coordinates here.
		outX = Clamp(x, HUD_LEFT, w - 16);
		outY = Clamp(y, HUD_TOP,  maxY);
		return true;
	}

	void WritePosition(int x, int y)
	{
		const DWORD  xi = (DWORD)x;
		const DWORD  yi = (DWORD)y;
		const double xd = (double)x;

		const bool okY = PokeBytes(ADDR_Y_IMM32,  &yi, sizeof(yi));
		const bool okX = PokeBytes(ADDR_X_IMM32,  &xi, sizeof(xi));
		const bool okD = PokeBytes(ADDR_X_DISP32, &xi, sizeof(xi));
		const bool okF = PokeBytes(ADDR_X_DOUBLE, &xd, sizeof(xd));

		g_appliedX = x;
		g_appliedY = y;
		g_patched  = true;

		(void)okY; (void)okX; (void)okD; (void)okF;
	}
}

void ChatPosition::Install()
{
	if (g_installed)
		return;

	char anchorBuf[64] = { 0 };
	if (MyConfig)
	{
		MyConfig->GetIniStr("ChatAnchor", anchorBuf, sizeof(anchorBuf), (LPSTR)"");
		g_anchor = ParseAnchor(anchorBuf);

		char xBuf[32] = { 0 };
		char yBuf[32] = { 0 };
		MyConfig->GetIniStr("ChatPosX", xBuf, sizeof(xBuf), (LPSTR)"0");
		MyConfig->GetIniStr("ChatPosY", yBuf, sizeof(yBuf), (LPSTR)"0");
		ParseOffset(xBuf, &g_offsetX, &g_offsetXPct);
		ParseOffset(yBuf, &g_offsetY, &g_offsetYPct);

		g_bottomLines = MyConfig->GetIniInt("ChatBottomLines", 8);
	}

	// Sanity-clamp so a typo cannot fling chat off-screen; the resolved value
	// is clamped to the viewport anyway, this just keeps the arithmetic sane.
	g_offsetX = Clamp(g_offsetX, g_offsetXPct ? -100 : -4096, g_offsetXPct ? 100 : 4096);
	g_offsetY = Clamp(g_offsetY, g_offsetYPct ? -100 : -4096, g_offsetYPct ? 100 : 4096);
	g_bottomLines = Clamp(g_bottomLines, 1, 20);

	if (g_anchor == AnchorNone)
	{
		// No configuration, or an unrecognised anchor: stay completely inert.
		// The engine keeps its own constants and this module never writes.
		IDDrawSurface::OutptTxt("[ChatPosition] no ChatAnchor set - engine chat position unchanged");
		g_installed = true;
		return;
	}

	// Capture the restore image before anything is written. If any read
	// fails the exe is not what we think it is; refuse to patch.
	if (!PeekBytes(ADDR_Y_IMM32,  &g_origYImm,  sizeof(g_origYImm))  ||
	    !PeekBytes(ADDR_X_IMM32,  &g_origXImm,  sizeof(g_origXImm))  ||
	    !PeekBytes(ADDR_X_DISP32, &g_origXDisp, sizeof(g_origXDisp)) ||
	    !PeekBytes(ADDR_X_DOUBLE, &g_origXDbl,  sizeof(g_origXDbl)))
	{
		IDDrawSurface::OutptTxt("[ChatPosition] could not read patch sites - disabled");
		g_anchor    = AnchorNone;
		g_installed = true;
		return;
	}

	// Signature check: refuse to patch an exe whose chat constants are not
	// the ones this module was built against. Silently rewriting unknown
	// bytes at these addresses would corrupt code.
	if (g_origYImm  != (DWORD)VANILLA_Y ||
	    g_origXImm  != (DWORD)VANILLA_X ||
	    g_origXDisp != (DWORD)VANILLA_X ||
	    g_origXDbl  != (double)VANILLA_X)
	{
		IDDrawSurface::OutptFmtTxt(
			"[ChatPosition] unexpected chat constants (Y=%u X=%u/%u dbl=%f) - disabled",
			g_origYImm, g_origXImm, g_origXDisp, g_origXDbl);
		g_anchor    = AnchorNone;
		g_installed = true;
		return;
	}

	IDDrawSurface::OutptFmtTxt("[ChatPosition] anchor=%d offset=(%d%s,%d%s) bottom_lines=%d",
		(int)g_anchor,
		g_offsetX, g_offsetXPct ? "%" : "px",
		g_offsetY, g_offsetYPct ? "%" : "px",
		g_bottomLines);
	g_installed = true;
}

void ChatPosition::Shutdown()
{
	if (g_appliedTextlines)
	{
		unsigned char* ta = TaBase();
		if (ta)
			*(int*)(ta + OFF_TEXTLINES) = (g_origTextlines >= 0) ? g_origTextlines : 30;
		g_appliedTextlines = 0;
	}

	if (!g_patched)
		return;

	PokeBytes(ADDR_Y_IMM32,  &g_origYImm,  sizeof(g_origYImm));
	PokeBytes(ADDR_X_IMM32,  &g_origXImm,  sizeof(g_origXImm));
	PokeBytes(ADDR_X_DISP32, &g_origXDisp, sizeof(g_origXDisp));
	PokeBytes(ADDR_X_DOUBLE, &g_origXDbl,  sizeof(g_origXDbl));

	g_appliedX = VANILLA_X;
	g_appliedY = VANILLA_Y;
	g_patched  = false;
}

void ChatPosition::EnsureApplied()
{
	if (!g_installed || g_anchor == AnchorNone)
		return;

	int x = 0, y = 0;
	if (!Resolve(x, y))
		return;                       // screen size not known yet

	if (!g_patched || g_needsReapply || x != g_appliedX || y != g_appliedY)
	{
		WritePosition(x, y);
		g_needsReapply = false;
	}

	// bottomcenter only: cap the engine's line count. Hud_DrawChatHudRing
	// draws downward from the anchor up to `textlines` lines, so with a bottom
	// anchor and the stock textlines=30 the newest lines fall past the bottom
	// edge. Writing `textlines` normally ratchets `chatNum` one way and eats
	// history, but this is a deliberately small fixed window (like the ctrl-F2
	// "chat lines" option), not a collapsible one, so the ratchet costs
	// nothing visible here. Restored to 30 by Shutdown().
	//
	// Skipped entirely when ChatLayout owns the draw: it walks the full ring
	// and clamps per column itself, so throttling `textlines` would only hide
	// history from it.
	if (g_anchor == AnchorBottomCenter && !ChatLayout::TakingOver())
	{
		unsigned char* ta = TaBase();
		if (ta)
		{
			int* tl = (int*)(ta + OFF_TEXTLINES);
			if (g_origTextlines < 0)
				g_origTextlines = *tl;   // capture the real pre-patch value once
			const int want = Clamp(g_bottomLines, 1, 30);
			if (*tl != want)
			{
				*tl = want;
				g_appliedTextlines = want;
			}
		}
	}
}

void ChatPosition::SetGrowUp(bool growUp)
{
	// ChatLayout calls this when ChatRenderer=tadr and ChatGrow=up. Only the
	// bottomcenter anchor cares: the list draws upward, so the anchor moves to
	// one line above the bottom bar. EnsureApplied() re-resolves next frame.
	if (g_growUp == growUp)
		return;
	g_growUp = growUp;
	g_needsReapply = true;
}

int ChatPosition::X()
{
	return g_appliedX;
}

int ChatPosition::Y()
{
	return g_appliedY;
}
