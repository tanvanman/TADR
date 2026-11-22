#pragma once

#include "config.h"

struct _OFFSCREEN;

// TADR-owned chat drawer. With ChatRenderer=tadr this replaces
// Hud_DrawChatHudRing @0x00464060 (INLINE_5BYTESLAGGERJMP, chains with
// ChatBackdrop's hook on the same address) and cancels the engine function,
// which enables the channel split, retained scrollback, and the ChatFont*
// keys. ChatRenderer=engine (default) leaves this module inert. Keys are
// documented in totala.ini.
//
// Render-only: reads the chat ring and screen geometry, writes neither, and
// touches no simulation state, so clients with different settings stay in
// sync. Fails safe -- any bad pointer or SEH fault returns 0 and the engine
// draws.

namespace ChatLayout
{
	// Call once during DLL init, after ChatPosition::Install() and before
	// ChatBackdrop::Install().
	void Install();
	void Shutdown();

	// True iff the hook is installed (renderer is `tadr` or `probe`).
	bool Active();

	// True iff the renderer is `tadr`. ChatBackdrop calls this to stand down
	// its own 0x00464060 backdrop pass. Callable in every build.
	bool TakingOver();

	// Called from CTAHook::Message on WM_MOUSEWHEEL, ahead of every other wheel
	// consumer. `wheelDeltaRaw` is signed HIWORD(wParam) (multiples of 120).
	// Consumes the event and moves the scrollback offset only while scrollback
	// is armed (the chat prompt is open); returns false untouched otherwise.
	bool ScrollbackWheel(int wheelDeltaRaw);
}
