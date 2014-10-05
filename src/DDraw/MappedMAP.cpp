#include "oddraw.h"



#include "tamem.h"
#include "tafunctions.h"
#include "gameredrawer.h"
#include "mappedmap.h"

#include "fullscreenminimap.h"
#include "iddrawsurface.h"


#ifdef USEMEGAMAP

MappedMap::MappedMap (int Width, int Height)
{
	Width_m= Width;
	Height_m= Height;
	MappedBits= static_cast<LPBYTE>(malloc ( Width_m* Height_m+ 1));
	
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
	IDDrawSurface::OutptTxt ( "Draw Mapped");

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

	if (PixelBits)
	{
		for (int i= 0; i<AspectSrc->y; ++i)
		{
			int Line= Width_m* i;
			int SrcLine= AspectSrc->x* i;
			for (int j= 0; j<AspectSrc->x; ++j)
			{
				MappedBits[Line+ j]= PixelBits[SrcLine+ j];
			}
		}
	}


	if (NOMAPPING==(NOMAPPING&((*TAmainStruct_PtrPtr)->LosType)))
	{//

		if (Permanent!=(Permanent&((*TAmainStruct_PtrPtr)->LosType)))
		{//
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
			memcpy ( TAGrayTABLE, (*TAmainStruct_PtrPtr)->TAProgramStruct_Ptr->GRAY_TABLE, 256);


			PlayerStruct * Player_p= &((*TAmainStruct_PtrPtr)->Players[(*TAmainStruct_PtrPtr)->LOS_Sight_PlayerID]);
			int MapX= Player_p->LOS_Tilewidth;
			int MapY= Player_p->LOS_Tileheight;
			LPBYTE PlayerLosBits= Player_p->LOS_MEMORY_p;

			if (NULL==PlayerLosBits)
			{//break
				goto BadEnd;
			}

			float XScale= static_cast<float>(MapX)/ static_cast<float>(Width_m);
			float YScale= static_cast<float>(MapY)/ static_cast<float>(Height_m);
			float Los_w, Los_h;
			int i, j;
			int LosBitYOff;

			for	( i= 0, Los_h= static_cast<float>(0.0- (*TAmainStruct_PtrPtr)->SeaLevel/ 20); i<Height_m; ++i, Los_h= Los_h+ YScale)
			{
				int YOff= i* Width_m;

				LosBitYOff=  static_cast<int>( Los_h<0? 0: Los_h)* MapX;

				for	( j= 0, Los_w= 0.0; j<Width_m; ++j, Los_w= Los_w+ XScale)
				{
					if (0==PlayerLosBits[LosBitYOff+ static_cast<int>(Los_w)])
					{
						MappedBits[YOff+ j]= TAGrayTABLE[MappedBits[YOff+ j]];
					}
				}
			}
		}
	}

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