#pragma once

struct _OFFSCREEN;

// Draws a solid black rectangle behind each visible engine chat line in the
// top-left of the game screen, to improve legibility for vision-impaired
// players. It is a no-op unless the ctrl-F2 "Chat Text Backdrop" option is
// enabled (default off).
namespace ChatBackdrop
{
	// Installs an inline hook on the engine's ChatMessageWithLogo (DrawChatText)
	// @ 0x00464060 so the backdrop is drawn immediately before the chat text on
	// every render path (normal gameplay, megamap overlay, victory screen).
	// Call once during DLL init.
	void Install();

	// Fills the backdrop for the current chat lines into the given render
	// context. Called by the installed hook; safe to call directly with a
	// valid OFFSCREEN.
	void Draw(_OFFSCREEN* offscreen);

	// The three below are for ChatLayout, which draws its own per-line backdrop
	// when it owns the chat draw (Draw() assumes the engine's single column).

	// True iff the ctrl-F2 "Chat Text Backdrop" option is on.
	bool BackdropEnabled();

	// Pixel width of one chat line, measured against the SAME font the caller
	// is about to draw it with: ChatFont TTF metrics at `fontHeight` when
	// useChatFont is true (must already be Ensure()'d by the caller),
	// otherwise the engine's native GAF-glyph metrics -- unconditionally, not
	// "TTF if an atlas happens to build". Get this wrong and the backdrop box
	// is sized for a font that isn't the one actually on screen.
	int MeasureLineWidth(const unsigned char* ringEntry, bool useChatFont, int fontHeight);

	// Draws one chat line with the engine's own native font
	// (DrawColorTextInScreen), the same call the pre-existing ctrl-F2 "crisp
	// chat" DrawText hook would otherwise intercept for a live-ring line and
	// silently upgrade to ChatFont -- which it never did for a
	// retained-history line or the scrollback indicator, neither of which is
	// a ring pointer. That hook stands down entirely while ChatLayout owns
	// the draw (see DrawTextHookProc), so calling this from ChatLayout always
	// draws the plain native font, for every line alike; ChatLayout's own
	// ChatFont path (ChatFontColor / ChatFontOutline) is unaffected and
	// unrelated -- this is only ever the `useChatFont == false` half.
	void DrawChatLine(_OFFSCREEN* offscreen, const char* str, int x, int y, int colorIndex);

	// One solid black box, clipped to the surface.
	void FillBehind(_OFFSCREEN* offscreen, int left, int top, int right, int bottom);
}
