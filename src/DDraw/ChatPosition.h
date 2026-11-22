#pragma once

// Relocates the in-game chat message list. TA hard-codes it at (138, 52); this
// module rewrites the four constants the engine reads for that anchor, all in
// Hud_DrawChatHudRing @ 0x00464060:
//
//   0x004640DA  imm32   0x34 (52)   running Y of the first visible line
//   0x00464187  imm32   0x8A (138)  logo RECT.left, and text X of no-logo lines
//   0x004641C3  disp32  0x8A (138)  logo RECT.right base (lea edx,[eax+0x8a])
//   0x004FD530  double  138.0       text X of has-logo lines:
//                                   textX = 138.0 + 1.5 * floor(0.8*lineHeight)
//
// The double is not optional -- the has-logo path recomputes text X in float
// from it, so patching only the three immediates splits player chat away from
// unit/system chat. The two neighbouring doubles (0.8, -1.5) are logo
// shape/offset and are left alone.
//
// Render-only: Hud_DrawChatHudRing touches no simulation state, so clients
// with different ChatAnchor settings stay in sync.
//
// taesc.ini [Preferences]: ChatAnchor = topleft | topcenter | bottomcenter,
// ChatPosX / ChatPosY = signed offset. Defaults (topleft / 0 / 0) resolve to
// exactly (138, 52); an absent or unparseable ChatAnchor leaves the engine
// unpatched.
#include "config.h"

namespace ChatPosition
{
	// Reads the ini and captures the original bytes; does not patch yet (the
	// screen size is not known this early). Call once during DLL init.
	void Install();

	// Restores the original engine constants. Safe to call more than once, or
	// when Install() never ran.
	void Shutdown();

	// Resolves the anchor against the current screen size and re-patches if it
	// changed. Called from the chat draw path; early-outs when nothing moved.
	void EnsureApplied();

	// ChatLayout calls this when ChatRenderer=tadr and ChatGrow=up. Shifts the
	// bottomcenter anchor to one line above the bottom bar, since the list is
	// then drawn upward. No effect for other anchors or growth modes.
	void SetGrowUp(bool growUp);

	// Currently applied top-left of the chat list, absolute screen pixels
	// (vanilla 138/52 until EnsureApplied() has run). Used by ChatBackdrop.
	int X();
	int Y();
}
