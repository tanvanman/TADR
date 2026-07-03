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
}
