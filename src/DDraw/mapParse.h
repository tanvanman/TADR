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

// ===== shared megamap colour helpers =====
// The megamap terrain image is 8-bit, indexing the live in-game colormap.
// These helpers expose that colormap and the area-average + nearest-palette
// snapping used when downscaling so the terrain downscaler and the feature
// overlay both convert/snap colours identically (no divergent palette path).

// Live colormap base (256 entries, R,G,B,0 layout) from the game struct;
// NULL before the game struct exists (callers then fall back to rqAry).
const BYTE* MegamapColormap();

// RGB of a palette index via the live colormap (falls back to rqAry).
void MegamapIndexRGB(const BYTE* pal, BYTE idx, int* r, int* g, int* b);

// Nearest colormap index for an arbitrary RGB (brute-force Euclidean over the
// 256 entries; cheap). Uses the live colormap, falls back to rqAry.
BYTE MegamapNearestIndex(const BYTE* pal, int r, int g, int b);

// State of the totala.ini [Preferences] "MegamapDither" option (default TRUE):
// TRUE => error-diffuse the downscaled terrain colours back onto the palette,
// FALSE => snap each pixel to its nearest palette entry in OKLab with no
// diffusion. Read once per render; both the megamap and the built-in minimap
// are rendered at map load, so a change takes effect from the next map.
bool MegamapDitherEnabled();

struct MegamapPaletteSubset
{
	int   r[256], g[256], b[256];
	BYTE  index[256];
	float labL[256], labA[256], labB[256];
	int   count;
};

// Build a palette subset from a 256-entry inclusion mask (non-zero => included).
void MegamapBuildPaletteSubsetFromMask(MegamapPaletteSubset* out, const BYTE* pal, const BYTE used[256]);

// Filter an already-built MegamapPaletteSubset by a 256-entry mask without
// recomputing OKLab conversions.
void MegamapFilterPaletteSubset(const MegamapPaletteSubset* src, const BYTE used[256], MegamapPaletteSubset* out);

// Nearest entry in a palette subset (OKLab perceptual distance by default).
BYTE MegamapSnap(const MegamapPaletteSubset* sub, int r, int g, int b);

// Error accumulators are kept in 1/16ths; round-to-nearest, symmetric about 0.
static inline int MegamapErr16(int v)
{
	return (v >= 0) ? ((v + 8) / 16) : (-((-v + 8) / 16));
}

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
	BOOL RenderStoredMinimapAtSize (
		LPBYTE DestPixelBits,
		int DestWidth,
		int DestHeight,
		const BYTE * StoredBits,
		int StoredWidth,
		int StoredHeight,
		int StoredPitch);
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
