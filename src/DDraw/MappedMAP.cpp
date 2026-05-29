#include "config.h"
#include "oddraw.h"



#include "tamem.h"
#include "tafunctions.h"
#include "gameredrawer.h"
#include "mappedmap.h"

#include "fullscreenminimap.h"
#include "iddrawsurface.h"
#include "Profiler.h"

#include <climits>
#include <cstring>


#if USEMEGAMAP

MappedMap::MappedMap (int Width, int Height)
{
	Width_m= Width;
	Height_m= Height;
	MappedBits= static_cast<LPBYTE>(malloc ( Width_m* Height_m+ 1));
	m_lastDrawGameTime = -1;

	// Sentinel keys so the first NowDrawMapped call always rebuilds the LUT.
	m_losCMapX = -1;
	m_losCMapY = -1;
	m_losCSeaLevel = INT_MIN;
	m_losCSightPlayerID = -1;

	Event_h= CreateMutexA ( NULL, FALSE, NULL);
}

MappedMap::~MappedMap()
{
	WaitForSingleObject ( Event_h, INFINITE);
	

	if (MappedBits)
	{
		LPBYTE MappedBits_v= MappedBits;
		MappedBits= NULL;
		free ( MappedBits_v);
	}

	ReleaseMutex ( Event_h);
	CloseHandle ( Event_h);
	Event_h= NULL;
}

BOOL MappedMap::NowDrawMapped (LPBYTE PixelBits, POINT * AspectSrc)
{
	if (TAInGame!=DataShare->TAProgress)
	{
		return FALSE;
	}
	if (gameingstate::EXITGAME==(*TAmainStruct_PtrPtr)->GameStateMask)
	{
		return FALSE;
	}
	//IDDrawSurface::OutptTxt ( "Draw Mapped");

	if (NULL==Event_h)
	{
		return FALSE;
	}
	if ((WAIT_OBJECT_0	!=WaitForSingleObject ( Event_h, INFINITE))
		||(NULL==MappedBits))
	{
BadEnd:
		ReleaseMutex ( Event_h);

		return FALSE;
	}

	// Sim-tick cache: PlayerLosBits and the input terrain only change at
	// 30 Hz, so re-running the per-pixel pass for every render frame in
	// between produces identical output. Skip when we already drew this tick.
	const int currentGameTime = (*TAmainStruct_PtrPtr)->GameTime;
	if (m_lastDrawGameTime >= 0 && currentGameTime == m_lastDrawGameTime)
	{
		PROFILE_SCOPE("MM.MD_TickHit");
		ReleaseMutex(Event_h);
		return TRUE;
	}

	if (PixelBits)
	{
		PROFILE_SCOPE("MM.MD_FullCopy");
		const int rows = AspectSrc->y;
		const int cols = AspectSrc->x;
		if (Width_m == cols)
		{
			std::memcpy(MappedBits, PixelBits, static_cast<size_t>(cols) * rows);
		}
		else
		{
			for (int i = 0; i < rows; ++i)
			{
				std::memcpy(MappedBits + Width_m * i, PixelBits + cols * i, cols);
			}
		}
	}


	if (NOMAPPING==(NOMAPPING&((*TAmainStruct_PtrPtr)->LosType)))
	{//

		if (Permanent!=(Permanent&((*TAmainStruct_PtrPtr)->LosType)))
		{//
			PROFILE_SCOPE("MM.MD_LosA");
			int PlayerID= (*TAmainStruct_PtrPtr)->LOS_Sight_PlayerID;
			PlayerStruct * Player_p= &((*TAmainStruct_PtrPtr)->Players[PlayerID]);
			int MapX= ((*TAmainStruct_PtrPtr)->FeatureMapSizeX)/ 2;
			int MapY= ((*TAmainStruct_PtrPtr)->FeatureMapSizeY)/ 2;
			LPWORD MappedMemory_p= (*TAmainStruct_PtrPtr)->MAPPED_MEMORY_p;

			if (NULL==MappedMemory_p)
			{//break
				goto BadEnd;
			}
			float XScale= (static_cast<float>(MapX)/ static_cast<float>(Width_m));
			float YScale= (static_cast<float>(MapY)/ static_cast<float>(Height_m));
			float MAPPEDMEM_h, MAPPEDMEM_w;
			int i, j;
			for	( i= 0, MAPPEDMEM_h= 0.0; i<Height_m; ++i, MAPPEDMEM_h= MAPPEDMEM_h+ YScale)
			{
				int YOff= i* Width_m;
				int LosBitYOff=  static_cast<int>(MAPPEDMEM_h)* MapX;

				for	( j= 0, MAPPEDMEM_w= 0.0; j<Width_m; ++j, MAPPEDMEM_w= MAPPEDMEM_w+ XScale)
				{
					if ( 0==(((1<<PlayerID)& MappedMemory_p[LosBitYOff+ static_cast<int>(MAPPEDMEM_w)])>> PlayerID))
					{
						MappedBits[YOff+ j]=0;
					}
				}
			}
		}
		else
		{
			PROFILE_SCOPE("MM.MD_LosB");
			int PlayerID= (*TAmainStruct_PtrPtr)->LOS_Sight_PlayerID;
			PlayerStruct * Player_p= &((*TAmainStruct_PtrPtr)->Players[PlayerID]);
			int MapX= ((*TAmainStruct_PtrPtr)->FeatureMapSizeX)/ 2;
			int MapY= ((*TAmainStruct_PtrPtr)->FeatureMapSizeY)/ 2;
			LPWORD MappedMemory_p= (*TAmainStruct_PtrPtr)->MAPPED_MEMORY_p;

			if (NULL==MappedMemory_p)
			{//break;
				goto BadEnd;
			}
			float XScale= (static_cast<float>(MapX)/ static_cast<float>(Width_m));
			float YScale= (static_cast<float>(MapY)/ static_cast<float>(Height_m));
			float MAPPEDMEM_h, MAPPEDMEM_w;
			int i, j;

			memcpy ( TAGrayTABLE, (*TAmainStruct_PtrPtr)->TAProgramStruct_Ptr->GRAY_TABLE, 256);

			int PlMapX= Player_p->LOS_Tilewidth;
			int PlMapY= Player_p->LOS_Tileheight;
			LPBYTE PlayerLosBits= Player_p->LOS_MEMORY_p;

			//	int LosBitYOff;
			


			for	( i= 0, MAPPEDMEM_h= static_cast<float>(0.0- (*TAmainStruct_PtrPtr)->SeaLevel/ 20); i<Height_m; ++i, MAPPEDMEM_h= MAPPEDMEM_h+ YScale)
			{
				int YOff= i* Width_m;
				int LosBitYOff=  static_cast<int>( MAPPEDMEM_h<0? 0: MAPPEDMEM_h)* MapX;

				for	( j= 0, MAPPEDMEM_w= 0.0; j<Width_m; ++j, MAPPEDMEM_w= MAPPEDMEM_w+ XScale)
				{
					if ( 0==(((1<<PlayerID)& MappedMemory_p[LosBitYOff+ static_cast<int>(MAPPEDMEM_w)])>> PlayerID))
					{
						MappedBits[YOff+ j]=0;
					}
					else
					{
						if (0==PlayerLosBits[LosBitYOff+ static_cast<int>(MAPPEDMEM_w)])
						{
							MappedBits[YOff+ j]= TAGrayTABLE[MappedBits[YOff+ j]];
						}
					}
				}
			}
		}
	}
	else
	{
		if (Permanent!=(Permanent&((*TAmainStruct_PtrPtr)->LosType)))
		{// total visual 
			;
		}
		else
		{
			PROFILE_SCOPE("MM.MD_LosC");
			memcpy ( TAGrayTABLE, (*TAmainStruct_PtrPtr)->TAProgramStruct_Ptr->GRAY_TABLE, 256);

			const int sightPlayerID = (*TAmainStruct_PtrPtr)->LOS_Sight_PlayerID;
			PlayerStruct * Player_p= &((*TAmainStruct_PtrPtr)->Players[sightPlayerID]);
			const int MapX = Player_p->LOS_Tilewidth;
			const int MapY = Player_p->LOS_Tileheight;
			const int seaLevel = (*TAmainStruct_PtrPtr)->SeaLevel;
			LPBYTE PlayerLosBits= Player_p->LOS_MEMORY_p;

			if (NULL==PlayerLosBits)
			{//break
				goto BadEnd;
			}

			// Hoist the per-pixel float arithmetic + fp->int casts out of the
			// inner loop into LUTs; rebuild only on input change.
			if (MapX != m_losCMapX || MapY != m_losCMapY ||
				seaLevel != m_losCSeaLevel || sightPlayerID != m_losCSightPlayerID ||
				static_cast<int>(m_losCYOff.size()) != Height_m ||
				static_cast<int>(m_losCXIdx.size()) != Width_m)
			{
				m_losCYOff.assign(Height_m, 0);
				m_losCXIdx.assign(Width_m, 0);
				const float XScale = static_cast<float>(MapX) / static_cast<float>(Width_m);
				const float YScale = static_cast<float>(MapY) / static_cast<float>(Height_m);
				// Running sum (rather than i*scale) to keep float-truncation
				// bit-identical to the original loop.
				float h = static_cast<float>(0.0 - seaLevel / 20);
				for (int i = 0; i < Height_m; ++i)
				{
					m_losCYOff[i] = static_cast<int>(h < 0 ? 0 : h) * MapX;
					h += YScale;
				}
				float w = 0.0f;
				for (int j = 0; j < Width_m; ++j)
				{
					m_losCXIdx[j] = static_cast<int>(w);
					w += XScale;
				}
				m_losCMapX = MapX;
				m_losCMapY = MapY;
				m_losCSeaLevel = seaLevel;
				m_losCSightPlayerID = sightPlayerID;
			}

			const int* yOff = m_losCYOff.data();
			const int* xIdx = m_losCXIdx.data();
			for (int i = 0; i < Height_m; ++i)
			{
				BYTE* destRow = MappedBits + i * Width_m;
				const BYTE* losRow = PlayerLosBits + yOff[i];
				for (int j = 0; j < Width_m; ++j)
				{
					if (0 == losRow[xIdx[j]])
					{
						destRow[j] = TAGrayTABLE[destRow[j]];
					}
				}
			}
		}
	}

	// Update *after* the work; on goto-BadEnd we leave the cache empty and retry.
	m_lastDrawGameTime = currentGameTime;

	ReleaseMutex ( Event_h);

	return TRUE;
}

LPBYTE MappedMap::PictureInfo (LPBYTE * PixelBits_pp, POINT * Aspect)
{
	if (PixelBits_pp)
	{
		*PixelBits_pp= MappedBits;
	}

	if (Aspect)
	{
		Aspect->x= Width_m;
		Aspect->y= Height_m;
	}

	return MappedBits;
}

#endif