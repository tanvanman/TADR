#include "config.h"
#include "ChatPosition.h"

#include <windows.h>       // TAConfig.h assumes the Win32 typedefs are already in

#include "TAConfig.h"
#include "iddrawsurface.h"

#include <cstring>

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

	// HUD frame margins. The engine draws the game viewport inside a fixed
	// 128px left sidebar and 32px top/bottom bars, with no scaling
	// (ENGINE_NOTES.md §26.8).
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
	Anchor g_anchor     = AnchorNone;
	int   g_offsetX     = 0;
	int   g_offsetY     = 0;

	// Last values actually written, so EnsureApplied() can early-out.
	int   g_appliedX    = VANILLA_X;
	int   g_appliedY    = VANILLA_Y;

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
			x = HUD_LEFT + (w - HUD_LEFT) / 2;
			y = h - HUD_BOTTOM;
			break;
		default:
			return false;
		}

		x += g_offsetX;
		y += g_offsetY;

		// Keep the anchor inside the game viewport. Drawing left of the
		// sidebar or above the top bar puts text under HUD chrome, and the
		// engine's clipper is not prepared for negative coordinates here.
		outX = Clamp(x, HUD_LEFT, w - 16);
		outY = Clamp(y, HUD_TOP,  h - 16);
		return true;
	}

	void WritePosition(int x, int y)
	{
		const DWORD  xi = (DWORD)x;
		const DWORD  yi = (DWORD)y;
		const double xd = (double)x;

		PokeBytes(ADDR_Y_IMM32,  &yi, sizeof(yi));
		PokeBytes(ADDR_X_IMM32,  &xi, sizeof(xi));
		PokeBytes(ADDR_X_DISP32, &xi, sizeof(xi));
		PokeBytes(ADDR_X_DOUBLE, &xd, sizeof(xd));

		g_appliedX = x;
		g_appliedY = y;
		g_patched  = true;
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
		g_anchor  = ParseAnchor(anchorBuf);
		g_offsetX = MyConfig->GetIniInt("ChatPosX", 0);
		g_offsetY = MyConfig->GetIniInt("ChatPosY", 0);
	}

	// Sanity-clamp the offsets so a typo cannot fling chat off-screen; the
	// resolved value is clamped to the viewport anyway, this just keeps the
	// arithmetic in a sane range.
	g_offsetX = Clamp(g_offsetX, -4096, 4096);
	g_offsetY = Clamp(g_offsetY, -4096, 4096);

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

	IDDrawSurface::OutptFmtTxt("[ChatPosition] anchor=%d offset=(%d,%d)",
		(int)g_anchor, g_offsetX, g_offsetY);
	g_installed = true;
}

void ChatPosition::Shutdown()
{
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

	if (g_patched && x == g_appliedX && y == g_appliedY)
		return;                       // nothing moved

	WritePosition(x, y);
}

int ChatPosition::X()
{
	return g_appliedX;
}

int ChatPosition::Y()
{
	return g_appliedY;
}
