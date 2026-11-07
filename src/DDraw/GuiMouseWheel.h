#pragma once

#include <windows.h>

namespace GuiMouseWheel
{
	// Returns true only when a native GUI scrollbar owns this wheel event.
	bool Message(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
}
