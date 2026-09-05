#pragma once

#include "config.h"

struct _OFFSCREEN;

// Stage 4b -- TADR owns the in-game chat drawer, and can split it into two
// columns (system/unit messages stay top-left, player chat + pings follow the
// ChatAnchor position).
//
// taesc.ini `ChatRenderer`:
//   engine   (default) -- the game draws the chat list, exactly as always.
//                         This module is completely inert.
//   tadr               -- this module hooks Hud_DrawChatHudRing @0x00464060
//                         (INLINE_5BYTESLAGGERJMP, chains with ChatBackdrop's
//                         hook on the same address, see ENGINE_NOTES X-17),
//                         draws the whole list itself and CANCELS the engine
//                         function. With `ChatSplit = 1` the list is drawn as
//                         two columns; otherwise it is one column, pixel-for-
//                         pixel where the engine drew it.
//   probe              -- Stage 4a dry run: walk + classify + log
//                         `[ChatLayout] PLAN`, but return 0 so the ENGINE
//                         still draws. Verification aid, instrumented builds
//                         only; a stripped build treats it as `engine`.
//
// Companion keys (only meaningful with `ChatRenderer = tadr`):
//   ChatSplit     = 0 | 1                 -- 0 (default) = single column.
//   ChatSysGroups = unit,cmd,event,notice,other
//                                         -- which kinds go to the top-left
//                                            system column. Anything not
//                                            listed follows ChatAnchor.
//   ChatGrow      = down | up             -- down (default): oldest line on
//                                            the anchor, list grows down (the
//                                            engine's behaviour). up: newest
//                                            line on the anchor, older lines
//                                            stack upward. Player column only;
//                                            the system column always grows
//                                            down from the top-left.
//   ChatFontSize  = 0..128                -- 0 (default): the engine's native
//                                            bitmap chat font. 1+: draw with
//                                            ChatFont's TrueType atlas at that
//                                            pixel cell height instead (both
//                                            columns).
//   ChatFontColor = "RRGGBB" hex          -- ChatFontSize-only. Absent/blank
//                                            (default): keep the per-line
//                                            player-colour remap. Set: every
//                                            ChatFont-drawn line, highlighted
//                                            or not, uses this one colour
//                                            instead (nearest match in TA's
//                                            fixed palette -- see PCX.CPP's
//                                            TAPalette[] and NearestPaletteIndex
//                                            in the .cpp).
//   ChatFontOutline = 0 | 1               -- ChatFontSize-only. 0 (default):
//                                            no outline. 1: an 8-direction 1px
//                                            black outline behind every
//                                            ChatFont-drawn line, so it stays
//                                            legible over any background.
//
// The system column is fixed at the vanilla top-left (138, 52) in this build;
// giving it its own anchor is a later step.
//
// DETERMINISM: render-only. Hud_DrawChatHudRing touches no simulation state,
// and this module reads the chat ring and screen geometry but writes neither.
// Two clients with different ChatRenderer / ChatSplit settings compute
// byte-identical simulation. The takeover fails safe: on any bad pointer or
// SEH fault it returns 0 and lets the engine draw -- it can never blank the
// chat, only (in the worst case) double-draw a frame.

namespace ChatLayout
{
	// Reads `ChatRenderer` / `ChatSplit` / `ChatSysGroups`. Installs the
	// 0x00464060 hook unless the renderer is `engine`. Call once during DLL
	// init, after ChatPosition::Install() and before ChatBackdrop::Install().
	void Install();

	// Removes the hook. Safe to call when Install() never ran.
	void Shutdown();

	// True iff the hook is installed (renderer is `tadr` or `probe`).
	bool Active();

	// True iff the renderer is `tadr` -- i.e. this module draws the list and
	// cancels the engine function. ChatBackdrop calls this to stand down its
	// own 0x00464060 backdrop pass (ChatLayout draws the backdrop instead).
	// False for `engine` and `probe`. Callable in every build.
	bool TakingOver();

	// Stage 4c-b step 3: called from CTAHook::Message on WM_MOUSEWHEEL, before
	// any other wheel consumer (WheelZoom / WheelMoveMegaMap / the build-rotate
	// SnapOverrideKey gesture). `wheelDeltaRaw` is HIWORD(wParam) read as a
	// signed 16-bit value (multiples of WHEEL_DELTA = 120). Moves the
	// scrollback offset and returns true (consumed) only while scrollback is
	// armed -- i.e. the chat prompt is open (see ChatPromptOpen() in the
	// .cpp). Returns false, untouched, otherwise: every other wheel consumer
	// keeps working exactly as before whenever the prompt is closed.
	bool ScrollbackWheel(int wheelDeltaRaw);
}
