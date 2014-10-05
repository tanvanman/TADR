#include "oddraw.h"
#include "BMP.h"
#include "PCX.H"
#include "TAMem.h"
#include "UnitDrawer.h"
#include "hook/etc.h"
//BITMAPINFOHEADER 
//BITMAPINFO
extern PALETTEENTRY TAPalette[];


TABMP::TABMP (void)
{
	memset ( &BMPFileHeader, 0, sizeof(BITMAPFILEHEADER));
	
	BMPInfo_p= NULL;
	Bits= NULL;

	TAPalette_p= (LOGPALETTE *)malloc ( sizeof(LOGPALETTE)+ sizeof(PALETTEENTRY)* 256);
	TAPalette_p->palNumEntries= 256;
	TAPalette_p->palVersion=  0x300;
	memcpy (&TAPalette_p->palPalEntry, TAPalette,  sizeof(PALETTEENTRY)* 256);
}

TABMP::~TABMP (void)
{
	if (BMPInfo_p)
	{
		delete [] reinterpret_cast<LPBYTE>(BMPInfo_p);
		BMPInfo_p= NULL;
	}
	
	if (Bits)
	{
		delete []Bits;
		Bits= NULL;
	}
	if (TAPalette_p)
	{
		free (TAPalette_p);
	}
}

void TABMP::Load (char * FileName)
{
	BOOL   bSuccess ;
	DWORD    dwInfoSize, dwBytesRead, dwBitsSize;
	HANDLE   hFile ;
	
	LPBYTE BmpBits;

	hFile = CreateFileA (FileName, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL) ;
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return ;
	}

	bSuccess = ReadFile ( hFile, &BMPFileHeader, sizeof (BITMAPFILEHEADER),
		&dwBytesRead, NULL) ;
	if (!bSuccess || (dwBytesRead != sizeof (BITMAPFILEHEADER))       
		|| (BMPFileHeader.bfType != * (WORD *) "BM"))
	{
		CloseHandle (hFile) ;

		return  ;
	}

	dwInfoSize = BMPFileHeader.bfOffBits - sizeof (BITMAPFILEHEADER) ;
	if (NULL == (BMPInfo_p = reinterpret_cast<LPBITMAPINFO>(new BYTE[dwInfoSize])))
	{
		CloseHandle (hFile) ;
		return ;
	}
	bSuccess = ReadFile (hFile, BMPInfo_p, dwInfoSize, &dwBytesRead, NULL);
	if (!bSuccess || (dwBytesRead != dwInfoSize))
	{
		CloseHandle (hFile) ;
		delete [] reinterpret_cast<LPBYTE>(BMPInfo_p);
		BMPInfo_p= NULL;
		return  ;
	}

	BmpBits= new BYTE[((BMPInfo_p->bmiHeader.biWidth)* (BMPInfo_p->bmiHeader.biHeight)* (BMPInfo_p->bmiHeader.biBitCount))/ 8+ 1];
	dwBitsSize = BMPFileHeader.bfSize - BMPFileHeader.bfOffBits ;
	bSuccess = ReadFile ( hFile, BmpBits, dwBitsSize, &dwBytesRead, NULL);

	Bits= new BYTE[(BMPInfo_p->bmiHeader.biWidth)* (BMPInfo_p->bmiHeader.biHeight)+ 1];

	Transformer ( Bits, TAPalette_p, BMPInfo_p, BmpBits);

	CloseHandle ( hFile);
	delete [] BmpBits;
}

void  TABMP::Transformer (LPBYTE Bits,  LOGPALETTE * Palette_p, LPBITMAPINFO SrcBMPInfo_p, LPBYTE SrcBits)
{
	HDC dc= CreateDCA ( "DISPLAY", NULL, NULL, NULL);
	HBITMAP bitmap_h;
	
	HPALETTE palette= CreatePalette ( Palette_p);

	SelectPalette ( dc, palette, TRUE);
	bitmap_h= CreateDIBitmap ( dc, &SrcBMPInfo_p->bmiHeader, CBM_INIT, SrcBits, SrcBMPInfo_p, DIB_PAL_COLORS);

	HGLOBAL old= SelectObject ( dc, bitmap_h);

	GetDIBits ( dc, bitmap_h, 0, SrcBMPInfo_p->bmiHeader.biHeight, Bits, SrcBMPInfo_p, DIB_PAL_COLORS);

	SelectObject ( dc, old);
	DeleteObject ( bitmap_h);
	DeleteDC ( dc);
}

UnitIcon::UnitIcon (char * FileName)
{
	char FileName_var[0x100];
	
	strcpy_s ( FileName_var, FileName);
	_strlwr_s ( FileName_var, 0x100);
	
	bmp= NULL;
	pcx= NULL;
	Use= USEBAD;

	int NameEnd= strlen ( FileName_var);

// 	if (0==strcmp ( &FileName_var[NameEnd- 3], "bmp"))
// 	{
// 		bmp= new TABMP;
// 		bmp->Load ( FileName_var);
// 		Use= USEBMP;
// 	}
	if (0==strcmp ( &FileName_var[NameEnd- 3], "pcx"))
	{
		pcx= new PCX;
		if (pcx)
		{
			if (pcx->Load ( FileName_var, FALSE))
			{
				Use= USEPCX;
			}
		}
	}
}
UnitIcon::~UnitIcon ()
{
	Use= USEBAD;
	if (bmp	)
	{
		delete bmp;
	}
	if (pcx	)
	{
		delete pcx;
	}
	bmp= NULL;
	pcx= NULL;
}
LONG UnitIcon::Width ()
{
	if (USEBMP==Use)
	{
		return bmp->Width();
	}
	if (USEPCX==Use)
	{
		return pcx->Width();
	}
	return 0;
	
}
LONG UnitIcon::Height ()
{
	if (USEBMP==Use)
	{
		return bmp->Height();
	}
	if (USEPCX==Use)
	{
		return pcx->Height();
	}
	return 0;
}
LPBYTE UnitIcon::Data ()
{
	if (USEBMP==Use)
	{
		return bmp->Data();
	}
	if (USEPCX==Use)
	{
		return pcx->PCXData();
	}

	return NULL;
}
long UnitIcon::BufSize ()
{
	if (USEBMP==Use)
	{
		return bmp->BufSize();
	}
	if (USEPCX==Use)
	{
		return pcx->BufSize();
	}

	return NULL;
}