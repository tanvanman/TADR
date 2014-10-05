#include "oddraw.h"
#include "pcxread.h"
#include "iddrawsurface.h"


extern HINSTANCE HInstance;

LPDIRECTDRAWSURFACE CreateSurfPCXResource(WORD PCXNum, bool VidMem)
{
	HRSRC FResource = FindResource(HInstance, MAKEINTRESOURCE(PCXNum), RT_RCDATA);
	if(FResource == NULL)
		IDDrawSurface::OutptTxt("FResource NULL");

	HGLOBAL LResource = LoadResource(HInstance, FResource);
	if(LResource == NULL)
		IDDrawSurface::OutptTxt("LResource NULL");

	LPVOID PCXBuf = LockResource(LResource);
	if(PCXBuf == NULL)
		IDDrawSurface::OutptTxt("PCXBuf NULL");

	pcx_picture_typ PCXPic;

	//PCXPic.buffer= (unsigned char *)malloc (	SizeofResource ( HInstance, FResource));
	PCX2BitMap((unsigned char*)PCXBuf, &PCXPic, 0);
	int Height = PCXPic.header.height-PCXPic.header.y+1;
	int Width = PCXPic.header.width-PCXPic.header.x+1;
	LPDIRECTDRAWSURFACE RetSurf;

	DDSURFACEDESC ddsd;
	DDRAW_INIT_STRUCT(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
	if(VidMem)
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;
	else
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN| DDSCAPS_SYSTEMMEMORY;
	ddsd.dwWidth = Width;
	ddsd.dwHeight = Height;

	if( DD_OK!=((LPDIRECTDRAW)LocalShare->TADirectDraw)->CreateSurface(&ddsd, &RetSurf, NULL))
	{
		return NULL;
	}
	DDRAW_INIT_STRUCT(ddsd);

	RetSurf->Lock(NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL);

	unsigned char *SurfPTR = (unsigned char*)ddsd.lpSurface;
	if (NULL!=PCXPic.buffer)
	{
		if (NULL!=SurfPTR)
		{
			for(int i=0; i<Height; i++)
			{
				memcpy(&SurfPTR[i*ddsd.lPitch], &PCXPic.buffer[i*Width], Width);
			}
		}

		delete [] PCXPic.buffer;
	}

	RetSurf->Unlock(NULL);
	return RetSurf;
}

//loads the PCX picture onto an existing surface
void RestoreFromPCX(WORD PCXNum, LPDIRECTDRAWSURFACE lpSurf)
{
	HRSRC FResource = FindResource(HInstance, MAKEINTRESOURCE(PCXNum), RT_RCDATA);
	if(FResource == NULL)
		IDDrawSurface::OutptTxt("FResource NULL");

	HGLOBAL LResource = LoadResource(HInstance, FResource);
	if(LResource == NULL)
		IDDrawSurface::OutptTxt("LResource NULL");

	LPVOID PCXBuf = LockResource(LResource);
	if(PCXBuf == NULL)
		IDDrawSurface::OutptTxt("PCXBuf NULL");

	pcx_picture_typ PCXPic;

	PCX2BitMap((unsigned char*)PCXBuf, &PCXPic, 0);
	int Height = PCXPic.header.height-PCXPic.header.y+1;
	int Width = PCXPic.header.width-PCXPic.header.x+1;

	DDSURFACEDESC ddsd;
	DDRAW_INIT_STRUCT(ddsd);

	lpSurf->Lock(NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL);

	unsigned char *SurfPTR = (unsigned char*)ddsd.lpSurface;

	if ((NULL!=PCXPic.buffer))
	{
		if ((NULL!=SurfPTR)&&(NULL!=PCXPic.buffer))
		{
			for(int i=0; i<Height; i++)
			{
				memcpy(&SurfPTR[i*ddsd.lPitch], &PCXPic.buffer[i*Width], Width);
			}
		}
		delete [] PCXPic.buffer;
	}



	lpSurf->Unlock(NULL);
}

void RestoreFromPCXFile(LPSTR FileName, LPDIRECTDRAWSURFACE lpSurf)
{
	unsigned char* PCXBuf;
	
	HANDLE file = CreateFileA( FileName, GENERIC_READ, 0, NULL, OPEN_EXISTING	, 0, NULL);

	PCXBuf= (unsigned char*)malloc ( GetFileSize ( file, NULL));
	DWORD tempRead;
	ReadFile ( file, PCXBuf, GetFileSize ( file, NULL), &tempRead, NULL);
	CloseHandle ( file);

	pcx_picture_typ PCXPic;

	PCX2BitMap((unsigned char*)PCXBuf, &PCXPic, 0);
	int Height = PCXPic.header.height-PCXPic.header.y+1;
	int Width = PCXPic.header.width-PCXPic.header.x+1;

	DDSURFACEDESC ddsd;
	DDRAW_INIT_STRUCT(ddsd);

	lpSurf->Lock(NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL);

	unsigned char *SurfPTR = (unsigned char*)ddsd.lpSurface;

	if ((NULL!=PCXPic.buffer))
	{
		if ((NULL!=SurfPTR)&&(NULL!=PCXPic.buffer))
		{
			for(int i=0; i<Height; i++)
			{
				memcpy(&SurfPTR[i*ddsd.lPitch], &PCXPic.buffer[i*Width], Width);
			}
		}
		delete [] PCXPic.buffer;
	}



	lpSurf->Unlock(NULL);

	if (PCXBuf)
	{
		free ( PCXBuf);
	}
}


void PCX2BitMap(unsigned char *inbuff, pcx_picture_typ *imgout, long length)
{
	int num_bytes,index;
	long count;
	unsigned char data;
	int pos=0;
	char *imgpointer;
	imgpointer = (char *)imgout;

	for (index=0; index<128; index++)
	{
		imgpointer[index] = inbuff[index];
		pos++;
	}


	count=0;

	int Height = imgout->header.height-imgout->header.y+1;
	int Width = imgout->header.width-imgout->header.x+1;

	imgout->buffer = (UCHAR*) new char[((Height*Width)/ 1000+ 1)* 1000];


	while(count<=(Height*Width))
	{
		// get the first piece of data

		data = inbuff[pos];
		pos++;

		// is this a rle?

		if (data>=192 && data<=255)
		{
			// how many bytes in run?

			num_bytes = data-192;

			// get the actual data for the run

			data  = inbuff[pos];
			pos++;

			// replicate data in buffer num_bytes times

			while(num_bytes-->0)
			{
				imgout->buffer[count++] = data;

			} // end while

		} // end if rle
		else
		{
			// actual data, just copy it into buffer at next location

			imgout->buffer[count++] = data;

		} // end else not rle

	} // end while

	// move to end of file then back up 768 bytes i.e. to begining of palette

	//pos=length-768;
	//fseek(fp,-768L,SEEK_END);

	// load the pallete into the palette

	for (index=0; index<256; index++)
	{
		// get the red component

		imgout->palette[index].red   = (inbuff[pos] >> 2);
		pos++;

		// get the green component

		imgout->palette[index].green = (inbuff[pos] >> 2);
		pos++;

		// get the blue component

		imgout->palette[index].blue  = (inbuff[pos] >> 2);
		pos++;

	} // end for index

	// change the palette to newly loaded palette if commanded to do so


}

