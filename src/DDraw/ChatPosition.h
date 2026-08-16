#pragma once

// Relocates the in-game chat message list.
//
// TA hard-codes the chat list at (138, 52) — top-left, just inside the HUD
// frame. This module rewrites the four constants the engine reads for that
// anchor, so the list can be placed anywhere on screen from taesc.ini.
//
// The four sites (verified against Escalation 10.1 GOLD, see ENGINE_NOTES.md
// §26.5) all live in Hud_DrawChatHudRing @ 0x00464060:
//
//   0x004640DA  imm32   0x34 (52)   running Y for the first visible line
//   0x00464187  imm32   0x8A (138)  logo RECT.left, AND the text X of every
//                                   line that has NO logo (mov ebx, 0x8a)
//   0x004641C3  disp32  0x8A (138)  logo RECT.right base (lea edx,[eax+0x8a])
//   0x004FD530  double  138.0       text X of every line that HAS a logo:
//                                   textX = 138.0 + 1.5 * floor(0.8*lineHeight)
//
// The fourth site is not optional. Patch only the three code immediates and
// player chat (which carries a logo) visibly splits away from unit/system
// chat, because the has-logo path recomputes text X in floating point from
// its own constant. All three doubles at 0x4FD520/0x4FD528/0x4FD530 are
// referenced exactly once each, only from this block; 0x4FD520 (0.8) and
// 0x4FD528 (-1.5) are logo shape/offset and are deliberately left alone.
//
// DETERMINISM: this is render-only. Hud_DrawChatHudRing is reached from
// Hud_DrawBattleFrame and touches no simulation state. Two clients with
// different ChatAnchor settings compute byte-identical simulation state.
//
// Configuration (taesc.ini [Preferences]):
//
//   ChatAnchor = topleft | topcenter | bottomcenter | topright | bottomright
//   ChatPosX   = signed pixel offset from the anchor
//   ChatPosY   = signed pixel offset from the anchor
//
// Defaults are topleft / 0 / 0, which resolves to exactly (138, 52) — i.e.
// vanilla, byte for byte. An absent or unparseable ChatAnchor leaves the
// engine completely unpatched.
namespace ChatPosition
{
	// Reads the ini and captures the original bytes. Does NOT patch yet —
	// the screen size is not known this early. Call once during DLL init.
	void Install();

	// Restores the original engine constants. Safe to call more than once,
	// and safe to call when Install() never ran.
	void Shutdown();

	// Resolves the anchor against the current screen size and writes the
	// patch if the result changed. Cheap: a couple of integer ops and an
	// early-out when nothing moved. Called from the chat draw path so the
	// values are always correct for the frame about to be drawn, including
	// after a resolution change.
	void EnsureApplied();

	// Currently applied top-left of the chat list, in absolute screen
	// pixels. Returns the vanilla 138/52 until EnsureApplied() has run.
	// Used by ChatBackdrop so the legibility backdrop tracks the text.
	int X();
	int Y();
}
