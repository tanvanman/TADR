#pragma once

namespace TeamColorNanolathe {
	void Install();
	void Shutdown();
	bool IsEnabled();
	unsigned char MapNanoframeColor(unsigned char playerColor, unsigned char stockColor);
}
