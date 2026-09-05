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

	// True iff the ctrl-F2 "Chat Text Backdrop" option is on. When ChatLayout
	// owns the draw it queries this and, if set, fills the box per line itself
	// (Draw() walks the engine's single-column layout, which the split breaks).
	bool BackdropEnabled();

	// Pixel width of one chat-ring line, measured the same way it will be
	// drawn. `lineHOverride` > 0 measures against a ChatFont atlas built for
	// that exact pixel height (ChatLayout passes its own ChatFontSize so the
	// backdrop box matches whatever size actually gets drawn); 0 (default)
	// measures the engine's own native size (crisp-font atlas at the native
	// height if the ctrl-F2 option is on, else the desktop GAF font) -- the
	// original behaviour for every caller that predates ChatFontSize.
	int MeasureLineWidth(const unsigned char* ringEntry, int lineHOverride = 0);

	// Fill one solid black box, clipped to the surface. For ChatLayout's use
	// when it draws the list and the backdrop option is on.
	void FillBehind(_OFFSCREEN* offscreen, int left, int top, int right, int bottom);
}
