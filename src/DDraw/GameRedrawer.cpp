//用来从TNT地图上创建一个随意大小的小地图出来。
#include "oddraw.h"
//#include "ddraw.h"
#include "tamem.h"
#include "tafunctions.h"
#include "iddrawsurface.h"
#include "gameredrawer.h"
#include "fullscreenminimap.h"

#include <initializer_list>

#ifdef USEMEGAMAP

TAGameAreaReDrawer::TAGameAreaReDrawer()
{
	GameAreaSurfaceFront_ptr= NULL;
	GameAreaSurfaceBack_ptr= NULL;
	
}

LPDIRECTDRAWSURFACE TAGameAreaReDrawer::InitOwnSurface (LPDIRECTDRAW TADD, BOOL VidMem)
{
	if (NULL!=GameAreaSurfaceFront_ptr)
	{
		GameAreaSurfaceFront_ptr->Release ( );
		GameAreaSurfaceFront_ptr= NULL;
	}

	if (NULL!=GameAreaSurfaceBack_ptr)
	{
		GameAreaSurfaceBack_ptr->Release ( );
		GameAreaSurfaceBack_ptr= NULL;
	}

	RECT GameAreaRect;

	if (TADD)
	{
		TAWGameAreaRect ( &GameAreaRect);

		DDSURFACEDESC ddsd;
		DDRAW_INIT_STRUCT(ddsd);
		ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
		if (VidMem)
		{
			ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;
		}
		else
		{
			ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
		}
		ddsd.dwWidth = GameAreaRect.right- GameAreaRect.left;
		ddsd.dwHeight = GameAreaRect.bottom- GameAreaRect.top;

		TADD->CreateSurface( &ddsd, &GameAreaSurfaceFront_ptr, NULL);
		TADD->CreateSurface( &ddsd, &GameAreaSurfaceBack_ptr, NULL);
		
	
		Cls ( );
	}
	
	return GameAreaSurfaceFront_ptr;
}
TAGameAreaReDrawer::~TAGameAreaReDrawer()
{
	if (NULL!=GameAreaSurfaceFront_ptr)
	{
		GameAreaSurfaceFront_ptr->Release ( );
		GameAreaSurfaceFront_ptr= NULL;
	}

	if (NULL!=GameAreaSurfaceBack_ptr)
	{
		GameAreaSurfaceBack_ptr->Release ( );
		GameAreaSurfaceBack_ptr= NULL;
	}
}

void TAGameAreaReDrawer::Cls (void)
{
    for (LPDIRECTDRAWSURFACE GameAreaSurface_ptr : {GameAreaSurfaceBack_ptr, GameAreaSurfaceFront_ptr})
    {
        if (NULL != GameAreaSurface_ptr)
        {
            if (DD_OK != GameAreaSurface_ptr->IsLost())
            {
                GameAreaSurface_ptr->Restore();
            }

            DDBLTFX ddbltfx;
            DDRAW_INIT_STRUCT(ddbltfx);
            ddbltfx.dwFillColor = 95;

            if (GameAreaSurface_ptr->Blt(NULL, NULL, NULL, DDBLT_ASYNC | DDBLT_COLORFILL, &ddbltfx) != DD_OK)
            {
                GameAreaSurface_ptr->Blt(NULL, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &ddbltfx);
            }
        }
    }
}


void TAGameAreaReDrawer::BlitTAGameArea(LPDIRECTDRAWSURFACE DestSurf)
{
	if (NULL!=GameAreaSurfaceFront_ptr)
	{
		if ( DD_OK!=GameAreaSurfaceFront_ptr->IsLost ( ))
		{
			GameAreaSurfaceFront_ptr->Restore ( );

			Cls ( );
		}
// 		DDBLTFX ddbltfx;
// 		DDRAW_INIT_STRUCT(ddbltfx);


		RECT GameScreen;
		TAWGameAreaRect ( &GameScreen);

		if(DestSurf->Blt ( &GameScreen, GameAreaSurfaceFront_ptr, NULL, DDBLT_ASYNC  , NULL)!=DD_OK)
		{
			DestSurf->Blt ( &GameScreen, GameAreaSurfaceFront_ptr, NULL, DDBLT_WAIT  , NULL);
		}
	}
}

HRESULT TAGameAreaReDrawer::Lock (  LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{
	if (GameAreaSurfaceBack_ptr)
	{
		return GameAreaSurfaceBack_ptr->Lock ( lpDestRect, lpDDSurfaceDesc, dwFlags, hEvent);
	}

	return DDERR_SURFACELOST;
}

HRESULT TAGameAreaReDrawer::Unlock(  LPVOID lpSurfaceData)
{
	if (GameAreaSurfaceBack_ptr)
	{
		return GameAreaSurfaceBack_ptr->Unlock ( lpSurfaceData);
	}

	return DDERR_SURFACELOST;
}


BOOL TAGameAreaReDrawer::MixBitsInBlit (LPRECT DescRect, LPBYTE SrcBits, LPPOINT SrcAspect, LPRECT SrcScope)
{
	BOOL Rtn_B= FALSE;
	if (NULL!=GameAreaSurfaceBack_ptr)
	{
		if ( DD_OK!=GameAreaSurfaceBack_ptr->IsLost ( ))
		{
			GameAreaSurfaceBack_ptr->Restore ( );
		}
		DDSURFACEDESC ddsd;
		DDRAW_INIT_STRUCT ( ddsd);


		if (DD_OK==GameAreaSurfaceBack_ptr->Lock ( NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR| DDLOCK_WAIT, NULL))
		{
			DWORD DescYStart;
			DWORD DescXStart;
			DWORD SrcXStart;
			DWORD SrcYStart;
			DWORD CopyHeight;
			DWORD CopyWidth;
			DWORD DescPitch;
			DWORD DescHeight;
			DWORD DescWidth;
			LPBYTE DescMem;

			DescMem= reinterpret_cast<LPBYTE>(ddsd.lpSurface);
			DescPitch= ddsd.lPitch;

			if (NULL==DescRect)
			{
				DescXStart= 0;
				DescYStart= 0;

				DescWidth= ddsd.dwWidth;
				DescHeight= ddsd.dwHeight;
			}
			else
			{
				DescXStart= DescRect->left;
				DescYStart= DescRect->top;
				DescWidth= DescRect->right;
				DescHeight= DescRect->bottom;
			}

			if (NULL==SrcScope)
			{
				SrcXStart= 0;
				SrcYStart= 0;
				CopyHeight= SrcAspect->y;
				CopyWidth= SrcAspect->x;
			}
			else
			{
				SrcXStart= SrcScope->left;
				SrcYStart= SrcScope->right;
				CopyHeight= SrcScope->bottom;
				CopyWidth= SrcScope->right;
			}



			if ((ddsd.dwWidth<DescXStart)
				||(DescHeight<0)
				||(ddsd.dwHeight<DescYStart)
				||(DescWidth<0)
				||(CopyHeight<0)
				||(CopyWidth<0))
			{
				return Rtn_B;
			}

			if (DescXStart<0)
			{
				DescXStart= 0;
			}
			if (DescYStart<0)
			{
				DescYStart= 0;
			}
			if (ddsd.dwWidth<DescWidth)
			{
				DescWidth= ddsd.dwWidth;
			}
			if (ddsd.dwHeight<DescHeight)
			{
				DescHeight= ddsd.dwHeight;
			}
			//
			if (SrcXStart<0)
			{
				SrcXStart= 0;
			}
			if (SrcYStart<0)
			{
				SrcYStart= 0;
			}

			if (CopyHeight>static_cast<DWORD>(SrcAspect->y))
			{
				CopyHeight= static_cast<DWORD>(SrcAspect->y);
			}
			if (CopyWidth>static_cast<DWORD>(SrcAspect->x))
			{
				CopyWidth= static_cast<DWORD>(SrcAspect->x);
			}

			if ((CopyHeight)>DescHeight)
			{
				CopyHeight= DescHeight;
			}
			if ((CopyWidth)>DescWidth)
			{
				CopyWidth= DescWidth;
			}
			
			DWORD TailCopyWidth= CopyWidth% 4;
			DWORD i, i_1, i_2, i_3;
			CopyWidth= CopyWidth- TailCopyWidth;

			for ( i= DescYStart, i_1= SrcYStart; i< CopyHeight; ++i, ++i_1)
			{
				int DescYOffset= i* DescPitch;
				int SrcYOffset= i_1* SrcAspect->x;
				for ( i_2= DescXStart, i_3= SrcXStart; i_2< CopyWidth; i_2= i_2+ 4, i_3= i_3+ 4)
				{
					*reinterpret_cast<LPDWORD>( &(DescMem[DescYOffset+ i_2]))= *reinterpret_cast<LPDWORD>( &(SrcBits[SrcYOffset+ i_3]));
				}

				for (DWORD temp_counter= 0; temp_counter<TailCopyWidth; ++temp_counter)
				{
					DescMem[DescYOffset+ i_2+ temp_counter]= SrcBits[SrcYOffset+ i_3+ temp_counter];
				}
			}
		
			GameAreaSurfaceBack_ptr->Unlock ( ddsd.lpSurface);
			Rtn_B= TRUE;
		}
	}
	return Rtn_B;
}

BOOL TAGameAreaReDrawer::MixDSufInBlit (LPRECT DescRect, LPDIRECTDRAWSURFACE Src_DDrawSurface, LPRECT SrcScope)
{
	BOOL Rtn_B= TRUE;

	if (NULL!=GameAreaSurfaceBack_ptr)
	{
		if ( DD_OK!=GameAreaSurfaceBack_ptr->IsLost ( ))
		{
			GameAreaSurfaceBack_ptr->Restore ( );
		}

		if(GameAreaSurfaceBack_ptr->Blt ( DescRect, Src_DDrawSurface, SrcScope, DDBLT_ASYNC,   NULL)!=DD_OK)
		{
			if (GameAreaSurfaceBack_ptr->Blt ( DescRect, Src_DDrawSurface, SrcScope, DDBLT_WAIT ,  NULL)!=DD_OK)
			{
				Rtn_B= FALSE;
			}
		}
	}
	return Rtn_B;
}

BOOL TAGameAreaReDrawer::GrayBlitOfBits (LPRECT DescRect, LPBYTE SrcBits, LPPOINT SrcAspect, LPRECT SrcScope, BOOL NoMapped)
{
	BOOL Rtn_B= FALSE;
	LPBYTE TAGrayTABLE= (*TAmainStruct_PtrPtr)->TAProgramStruct_Ptr->GRAY_TABLE;


	if (NULL!=GameAreaSurfaceBack_ptr)
	{
		if ( DD_OK!=GameAreaSurfaceBack_ptr->IsLost ( ))
		{
			GameAreaSurfaceBack_ptr->Restore ( );
		}
		DDSURFACEDESC ddsd;
		DDRAW_INIT_STRUCT ( ddsd);


		if (DD_OK==GameAreaSurfaceBack_ptr->Lock ( NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR| DDLOCK_WAIT, NULL))
		{
			DWORD DescYStart;
			DWORD DescXStart;
			DWORD SrcXStart;
			DWORD SrcYStart;
			DWORD CopyHeight;
			DWORD CopyWidth;
			DWORD DescPitch;
			DWORD DescHeight;
			DWORD DescWidth;
			LPBYTE DescMem;

			DescMem= reinterpret_cast<LPBYTE>(ddsd.lpSurface);
			DescPitch= ddsd.lPitch;

			if (NULL==DescRect)
			{
				DescXStart= 0;
				DescYStart= 0;

				DescWidth= ddsd.dwWidth;
				DescHeight= ddsd.dwHeight;
			}
			else
			{
				DescXStart= DescRect->left;
				DescYStart= DescRect->top;
				DescWidth= DescRect->right;
				DescHeight= DescRect->bottom;
			}

			if (NULL==SrcScope)
			{
				SrcXStart= 0;
				SrcYStart= 0;
				CopyHeight= SrcAspect->y;
				CopyWidth= SrcAspect->x;
			}
			else
			{
				SrcXStart= SrcScope->left;
				SrcYStart= SrcScope->right;
				CopyHeight= SrcScope->bottom;
				CopyWidth= SrcScope->right;
			}

			if ((ddsd.dwWidth<DescXStart)
				||(DescHeight<0)
				||(ddsd.dwHeight<DescYStart)
				||(DescWidth<0)
				||(CopyHeight<0)
				||(CopyWidth<0))
			{
				return Rtn_B;
			}

			if (DescXStart<0)
			{
				DescXStart= 0;
			}
			if (DescYStart<0)
			{
				DescYStart= 0;
			}
			if (ddsd.dwWidth<DescWidth)
			{
				DescWidth= ddsd.dwWidth;
			}
			if (ddsd.dwHeight<DescHeight)
			{
				DescHeight= ddsd.dwHeight;
			}
			//
			if (SrcXStart<0)
			{
				SrcXStart= 0;
			}
			if (SrcYStart<0)
			{
				SrcYStart= 0;
			}

			if (CopyHeight>static_cast<DWORD>(SrcAspect->y))
			{
				CopyHeight= static_cast<DWORD>(SrcAspect->y);
			}
			if (CopyWidth>static_cast<DWORD>(SrcAspect->x))
			{
				CopyWidth= static_cast<DWORD>(SrcAspect->x);
			}

			if ((CopyHeight)>DescHeight)
			{
				CopyHeight= DescHeight;
			}
			if ((CopyWidth)>DescWidth)
			{
				CopyWidth= DescWidth;
			}

			DWORD i, i_1, i_2, i_3;

			if (NoMapped)
			{
				for ( i= DescYStart, i_1= SrcYStart; i< CopyHeight; ++i, ++i_1)
				{
					int DescYOffset= i* DescPitch;
					int SrcYOffset= i_1* SrcAspect->x;
					for ( i_2= DescXStart, i_3= SrcXStart; i_2< CopyWidth; i_2++, i_3++)
					{
						if (0==SrcBits[SrcYOffset+ i_3])
						{ // black
							DescMem[DescYOffset+ i_2]= 0;
						}
					}
				}
			}
			else
			{	
				for ( i= DescYStart, i_1= SrcYStart; i< CopyHeight; ++i, ++i_1)
				{
					int DescYOffset= i* DescPitch;
					int SrcYOffset= i_1* SrcAspect->x;
					for ( i_2= DescXStart, i_3= SrcXStart; i_2< CopyWidth; i_2++, i_3++)
					{
						if (0==SrcBits[SrcYOffset+ i_3])
						{
							DescMem[DescYOffset+ i_2]= TAGrayTABLE[SrcBits[SrcYOffset+ i_3]];//TAGrayTABLE[DescMem[DescYOffset+ i_2]];
						}
					}
				}
			}
			GameAreaSurfaceBack_ptr->Unlock ( ddsd.lpSurface);
			Rtn_B= TRUE;
		}
	}
	return Rtn_B;
}
void TAGameAreaReDrawer::ReleaseSurface (void)
{
	if (NULL!=GameAreaSurfaceFront_ptr)
	{
		GameAreaSurfaceFront_ptr->Release ( );
	}
	GameAreaSurfaceFront_ptr= NULL;


	if (NULL!=GameAreaSurfaceBack_ptr)
	{
		GameAreaSurfaceBack_ptr->Release ( );
	}
	GameAreaSurfaceBack_ptr= NULL;
}

LPRECT TAGameAreaReDrawer::TAWGameAreaRect (LPRECT Out_Rect)
{
	static RECT TAGameAera;
	if (NULL==Out_Rect)
	{
		Out_Rect= &TAGameAera;
	}

	memcpy ( Out_Rect, &(*TAmainStruct_PtrPtr)->GameSreen_Rect, sizeof(RECT));

	Out_Rect->right= Out_Rect->left+ (*TAmainStruct_PtrPtr)->GameScreenWidth;
	Out_Rect->bottom= Out_Rect->top+ (*TAmainStruct_PtrPtr)->GameScreenHeight;

	return Out_Rect;
}

LPDIRECTDRAWSURFACE TAGameAreaReDrawer::Flip (void)
{
 

	LPDIRECTDRAWSURFACE GameAreaSurface_ptr= GameAreaSurfaceFront_ptr;
	GameAreaSurfaceFront_ptr= GameAreaSurfaceBack_ptr;
	GameAreaSurfaceBack_ptr= GameAreaSurface_ptr;

// 	if(GameAreaSurfaceFront_ptr->Blt ( NULL, GameAreaSurfaceBack_ptr, NULL, DDBLT_ASYNC, NULL)!=DD_OK)
// 	{
// 		GameAreaSurfaceFront_ptr->Blt ( NULL, GameAreaSurfaceBack_ptr, NULL, DDBLT_WAIT, NULL);
// 	}

	return GameAreaSurfaceFront_ptr;
}

#endif