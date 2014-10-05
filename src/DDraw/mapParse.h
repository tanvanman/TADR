#pragma once

#pragma pack(push)
#pragma pack(4)
typedef struct tagTNTHeaderStruct
{
	int		IDversion;
	int		Width;
	int		Height;
	WORD *	PTRmapdata;
	WORD *	PTRmapattr;
	LPBYTE	PTRtilegfx;
	int		tiles;
	int		tileanims;
	int		PTRtileanim;
	int		sealevel;
	LPBYTE		PTRminimap;
	int		unknown1;
	int		pad1,pad2,pad3,pad4;
}TNTHeaderStruct, * PTNTHeaderStruct;
#pragma pack(pop)

//DrawMiniMap return 
#define MMPERROR_NOTVALIDBYTECOUNT (-3)
#define MMPERROR_NOTVALIDDESCSIZE (-2)
#define MMPERROR_NOTVALIDBUF (-1)
#define MMPERROR_MODIFYRECT (0x11)
#define MMPERROR_MODIFYDESCRECT (0x12)

class MiniMapPicture 
{
public:
	MiniMapPicture(int Width, int Height);
	~MiniMapPicture();
	LPBYTE StretchTAMapToMiniMap (LPBYTE MiniMap, RECT * CornerPos);
	LPBYTE StretchTATNTDataToMiniMap (PTNTHeaderStruct TATNT_PTNTH);

	int DrawMiniMap (LPBYTE DescPixelBitsBegin, RECT * DescRect, int DescPixelWidth_I, int DescPixelHeight_I, RECT * MiniMapRect);
	LPBYTE PictureInfo ( LPBYTE * PixelBits_pp,  POINT * Aspect);
private:
	LPBYTE MiniMapPixelBits;
	int Width;
	int Height;
	DWORD WholeBytesInPixelsBits;
};

class TNTtoMiniMap
{
public:
	static LPLOGPALETTE TALogPalette_Ptr;
	static int PaletteRefCount;
public:
	TNTtoMiniMap ();
	TNTtoMiniMap (DWORD Width, DWORD Height);
	~TNTtoMiniMap();

	void MapFromTNTFileA (LPSTR TNTFileName);

	PTNTHeaderStruct ParseMyTNTHeader (PTNTHeaderStruct In_PTNTH);
	void MapFromTNTInMem (LPVOID Argc_PTNTH);
	void MapFromValidTNTHeader (PTNTHeaderStruct Argc_PTNTH);
	int DrawMiniMap (LPBYTE DescPixelBitsBegin, int Width_I, int Height_I);

	LPBYTE PictureInfo ( LPBYTE * PixelBits_pp,  POINT * Aspect);
private:
	TNTHeaderStruct TNTHeader;
	LPVOID MapPixelBytes_PB;
	MiniMapPicture * myMiniMap;
private:

	void inline CopyTileToTAPos_Inline (LPBYTE PixelBitsBuf, POINT * TAPos, __int16 TileIndex, PTNTHeaderStruct ArgcTNTHeader);
	LPBYTE DrawRectMapToBuf (LPBYTE * RectPixelBitsBuf_PtrToPB, PTNTHeaderStruct Argc_PTNTH, RECT * TAPosRect);
};
