#include "oddraw.h"
#include "iddrawsurface.h"
#include "gaf.h"
#include "tamem.h"
#include "tafunctions.h"

LPDIRECTDRAWSURFACE CreateSurfByGafSequence (LPDIRECTDRAW LpDD, PGAFSequence Cursor_P, bool VidMem)
{
	IDDrawSurface::OutptTxt ( "CreateSurfByGafSequence");
	if (NULL==Cursor_P)
	{
		return NULL;
	}

	LPDIRECTDRAWSURFACE RetSurf= NULL;

	LPBYTE GafBits;
	POINT GafSize={0, 0};
	int Width= 0;
	int Height= 0;

	Width= Cursor_P->PtrFrameAry[0].PtrFrame->Width;
	Height= Cursor_P->PtrFrameAry[0].PtrFrame->Height;

	for (int i= 0; i<Cursor_P->Frames; ++i)
	{
		GafSize.x= Cursor_P->PtrFrameAry[i].PtrFrame->Width;
		GafSize.y= Cursor_P->PtrFrameAry[i].PtrFrame->Height;

		if (Width<GafSize.x)
		{
			Width= GafSize.x;
		}

		if (Height<GafSize.y)
		{
			Height= GafSize.y;
		}
	}
	GafBits= (LPBYTE)malloc ( Width* Height); 
	GafSize.x= Width;
	GafSize.y= Height;

	CopyGafSequenceToBits ( GafBits, &GafSize, Cursor_P, 0);
	
	DDSURFACEDESC ddsd;
	DDRAW_INIT_STRUCT(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
	if(VidMem)
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;
	else
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN| DDSCAPS_SYSTEMMEMORY;
	ddsd.dwWidth = Width;
	ddsd.dwHeight = Height;

	LpDD->CreateSurface ( &ddsd, &RetSurf, NULL);

	DDRAW_INIT_STRUCT(ddsd);

	RetSurf->Lock(NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL);

	unsigned char *SurfPTR = (unsigned char*)ddsd.lpSurface;
	if (NULL!=SurfPTR)
	{
		for(int i=0; i<GafSize.y; i++)
		{
			memcpy(&SurfPTR[i*ddsd.lPitch], &GafBits[i* Width], Width);
		}
	}
	free ( GafBits);
	RetSurf->Unlock(NULL);

	return RetSurf;
}

LPBYTE CopyGafSequenceToBits (LPBYTE GafBits, POINT * GafSize,PGAFSequence Cursor_P, int FrameIndex)
{
	memset ( GafBits, Cursor_P->PtrFrameAry[FrameIndex].PtrFrame->Background, GafSize->x* GafSize->y);

	CopyGafToBits ( GafBits, GafSize, Cursor_P->PtrFrameAry[FrameIndex].PtrFrame->xPosition,Cursor_P->PtrFrameAry[FrameIndex].PtrFrame->yPosition, Cursor_P->PtrFrameAry[FrameIndex].PtrFrame);
	return GafBits;
}

LPDIRECTDRAWSURFACE CreateSurfByGafFrame (LPDIRECTDRAW LpDD, PGAFFrame Cursor_P, bool VidMem)
{
	//PCXPic.buffer= (unsigned char *)malloc (	SizeofResource ( HInstance, FResource));
	if (NULL==Cursor_P)
	{
		return NULL;
	}
	PGAFFrame GafFrame= Cursor_P;
	
	LPDIRECTDRAWSURFACE RetSurf= NULL;

	LPBYTE GafBits;
	POINT GafSize;
	InstanceGAFFrame ( GafFrame, &GafBits, &GafSize);

	DDSURFACEDESC ddsd;
	DDRAW_INIT_STRUCT(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
	if(VidMem)
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;
	else
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN| DDSCAPS_SYSTEMMEMORY;
	ddsd.dwWidth = GafSize.x;
	ddsd.dwHeight = GafSize.y;

	LpDD->CreateSurface ( &ddsd, &RetSurf, NULL);

	DDRAW_INIT_STRUCT(ddsd);

	RetSurf->Lock(NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL);

	unsigned char *SurfPTR = (unsigned char*)ddsd.lpSurface;
	if (NULL!=SurfPTR)
	{
		for(int i=0; i<GafSize.y; i++)
		{
			memcpy(&SurfPTR[i*ddsd.lPitch], &GafBits[i*GafSize.x], GafSize.x);
		}
	}
	free ( GafBits);
	RetSurf->Unlock(NULL);
	

	
	return RetSurf;
}

LPBYTE InstanceGAFFrame (PGAFFrame GafFrame, LPBYTE * FrameBits, POINT * Aspect)// need free
{
	if (NULL==GafFrame)
	{
		return NULL;
	}
	LPBYTE lpSuface= reinterpret_cast<LPBYTE>( malloc( GafFrame->Height* GafFrame->Width));
	POINT GafAspect;
	GafAspect.x= GafFrame->Width;
	GafAspect.y= GafFrame->Height;
	//GafFrame->

	memset ( lpSuface, GafFrame->Background, GafAspect.x* GafAspect.y);
	CopyGafToBits ( lpSuface, &GafAspect, 0, 0, GafFrame);

	if (Aspect)
	{
		Aspect->y = GafFrame->Height;
		Aspect->x = GafFrame->Width;
	}
	if (FrameBits)
	{
		*FrameBits=  (lpSuface);
	}
	
	
	return (lpSuface);
}

void CopyGafToBits (LPBYTE PixelBits, POINT * Aspect, unsigned int X, unsigned int Y, PGAFFrame GafFrame, LPRECT Desc_p)
{
	OFFSCREEN OffScreen;
	memset ( &OffScreen, 0, sizeof(OFFSCREEN));
	OffScreen.Height=Aspect->y;
	OffScreen.Width= Aspect->x;
	OffScreen.lPitch= Aspect->x;
	OffScreen.lpSurface= PixelBits;

	if (NULL!=Desc_p)
	{
		memcpy ( &OffScreen.ScreenRect, Desc_p, sizeof(RECT));
	}
	else
	{
		OffScreen.ScreenRect.left= 0;
		OffScreen.ScreenRect.right=  Aspect->x;;

		OffScreen.ScreenRect.top= 0;
		OffScreen.ScreenRect.bottom= Aspect->y;
	}


	CopyGafToContext ( &OffScreen, GafFrame, X, Y);
}
/*

void CopyGafToBits (LPBYTE PixelBits, POINT * Aspect, unsigned int X, unsigned int Y, PGAFFrame GafFrame)
{
		unsigned char *frame;
		int bcount;
		int count;
		int repeat;
		unsigned char mask;
		int xcount;
		int ycount;
		int y;

		int xofs = X - GafFrame->xPosition;
		int yofs = Y - GafFrame->yPosition;

		frame = GafFrame->PtrFrameBits;

		if (GafFrame->Compressed) 
		{
			for (ycount = 0; ycount < GafFrame->Height; ycount++) 
			{
				bcount = *((short *) frame);
				frame += sizeof(short);
				y = yofs + ycount;
				xcount = xofs;
				count = 0;

				int Line= y* Aspect->x;

				while (count < bcount) 
				{
					mask =  frame[count++];

					if ((mask & 0x01) == 0x01) 
					{
						// transparent
						xcount += (mask >> 1);
					}
					else if ((mask & 0x02) == 0x02) 
					{
						// repeat next byte
						repeat = (mask >> 2) + 1;
						while (repeat--) 
						{
							PixelBits[Line+ xcount++]= frame[count];
						}
						count++;
					}
					else 
					{
						repeat = (mask >> 2) + 1;
						while (repeat--) 
						{
							PixelBits[Line+ xcount++]= frame[count++];
						}
					}
				}
				frame += bcount;
			}
		}
		else 
		{
			for (ycount = 0; ycount < GafFrame->Height; ycount++) 
			{
				int Line= Aspect->x* (ycount+ yofs);
				for (xcount = 0; xcount < GafFrame->Width; xcount++) 
				{
					PixelBits[Line+ xcount+ xofs]= *frame++;
				}
			}
		}
}*/