#pragma once

#include <vector>

class MappedMap
{
public:
	MappedMap (int Width, int Height);
	~MappedMap();

	BOOL NowDrawMapped (LPBYTE PixelBits,  POINT * AspectSrc);
	LPBYTE PictureInfo (LPBYTE * PixelBits_pp, POINT * Aspect);

private:
	LPBYTE MappedBits;
	int Width_m;
	int Height_m;

	BYTE TAGrayTABLE[256];

	HANDLE Event_h;

	// LosC branch LUTs (pixel-space → LOS-tile-space). Rebuilt only when
	// inputs change — typically once per game start.
	std::vector<int> m_losCYOff;
	std::vector<int> m_losCXIdx;
	int m_losCMapX;
	int m_losCMapY;
	int m_losCSeaLevel;
	int m_losCSightPlayerID;

	// GameTime of last successful pass; -1 = never. Skip the per-pixel work
	// when called more than once in the same sim tick.
	int m_lastDrawGameTime;
};
