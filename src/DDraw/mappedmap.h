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

	// Static base image = terrain background + baked feature sprites. Neither
	// changes during a game, so this is built ONCE (per map) and reused; each
	// tick MappedBits is just refreshed from it before the fog pass. Avoids
	// re-running the expensive per-pixel feature composite every frame/tick.
	LPBYTE m_baseBits;
	LPBYTE m_basePixelSrc;     // terrain source the base was built from
	void*  m_baseFeatureMap;   // feature map the base was built from

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
