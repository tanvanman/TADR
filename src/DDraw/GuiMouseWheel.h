#pragma once

#include <windows.h>

namespace GuiMouseWheel
{
	// Returns true only when a native GUI scrollbar owns this wheel event.
	// Patch dialogs run first. A matched native control precedes whiteboard,
	// economy panels, construction controls, chat scrollback and gameplay wheel
	// consumers; an unmatched event continues to those existing handlers.
	bool Message(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
}
