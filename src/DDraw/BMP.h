#pragma once

class TABMP
{
public:
	TABMP (void);
	~TABMP (void);

	void Load (char * FileName);
	void Transformer  (LPBYTE Bits,  LOGPALETTE * Palette_p, LPBITMAPINFO SrcBMPInfo_p, LPBYTE SrcBits);
	LONG Width()
	{
		return  BMPInfo_p->bmiHeader.biWidth;
	}

	LONG Height()
	{
		return abs ( BMPInfo_p->bmiHeader.biHeight);
	}
	LPBYTE Data ()
	{
		return Bits;
	}
	long BufSize ()
	{
		return lSize;
	}
private:
	BITMAPFILEHEADER BMPFileHeader;
	LPBITMAPINFO BMPInfo_p;
	LPBYTE Bits;
	LOGPALETTE * TAPalette_p;
	long lSize;
};