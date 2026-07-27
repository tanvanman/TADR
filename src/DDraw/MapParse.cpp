#include "config.h"
#include "oddraw.h"
//one pixel mean 1 byte in TA. 
//use TA tiles Pos in TNT class. use pixel Pos in minimap class.
#include "mapParse.h"
#include <tchar.h>
#include <math.h>
#include <CString>
#include "fullscreenminimap.h"
#include "tafunctions.h"

#if USEMEGAMAP

// === MiniMap colour sampling mode ===
// 0 = Original behaviour: pick a single pixel from the source tile (fast, can look blocky on large maps)
// 1 = Averaged: calculate all source pixels covered by each minimap pixel,
//     average their RGB colours from the live colormap, then error-diffuse the
//     result back onto the palette. Much higher visual quality.
#define USE_AVERAGED_MINIMAP_COLORS 1

// Error diffusion (Floyd-Steinberg, serpentine) when snapping the averaged
// colours back to 8-bit.
//
// This is NOT cosmetic polish - without it the downscale loses the terrain's
// colour outright. TA's palette has no desaturated mid-luminance greens: the
// ramp jumps from neutral olive (idx 72 = 67,67,35) straight to saturated green
// (idx 168 = 43,103,19). Grass is therefore rendered by *dithering* those two
// against each other. Area-averaging collapses that dither to ~(72,78,33),
// whose nearest palette entry is the neutral olive - so every mixed cell snaps
// to brown and only near-pure-green cells stay green. That is the "brown crud
// around patches of green" artifact. Diffusing the quantisation error puts the
// dither back and restores the true average colour.
#define USE_MINIMAP_ERROR_DIFFUSION 1

// Judge "which palette entry is closest" in OKLab rather than in RGB.
//
// Error diffusion still accumulates in gamma RGB, so what is *conserved* does
// not change - only which entry gets picked at each step. RGB distance badly
// misjudges saturated colours, which lets the diffusion occasionally reach for
// an entry that is numerically close but obviously wrong to the eye (a saturated
// orange landing in desaturated tan). OKLab is near perceptually uniform, so
// plain Euclidean distance in it tracks what actually looks closest.
//
// Measured on the TA:Zero reference downscale, identical mean colour fidelity,
// but pixels deviating by more than 0.10 OKLab fell from 2.12% to 0.46% - a
// 4.6x reduction in visible stippling. Confirmed in-game across several maps.
//
// Set to 0 to fall back to RGB distance.
#define MEGAMAP_SNAP_OKLAB 1

using namespace std;
LPLOGPALETTE TNTtoMiniMap::TALogPalette_Ptr= NULL;
int TNTtoMiniMap::PaletteRefCount= 0;

extern RGBQUAD rqAry[256];

// ===== shared megamap colour helpers (see mapParse.h) =====
const BYTE* MegamapColormap()
{
	if (TAProgramStruct_PtrPtr && *TAProgramStruct_PtrPtr)
		return (const BYTE*)(*TAProgramStruct_PtrPtr) + 0x214;   // live colormap, R,G,B,0
	return NULL;
}

void MegamapIndexRGB(const BYTE* pal, BYTE idx, int* r, int* g, int* b)
{
	if (pal) { *r = pal[idx * 4 + 0]; *g = pal[idx * 4 + 1]; *b = pal[idx * 4 + 2]; }
	else     { *r = rqAry[idx].rgbRed; *g = rqAry[idx].rgbGreen; *b = rqAry[idx].rgbBlue; }
}

BYTE MegamapNearestIndex(const BYTE* pal, int R, int G, int B)
{
	int bestDist = 0x7fffffff, bestI = 0;
	for (int i = 0; i < 256; ++i)
	{
		int pr, pg, pb;
		if (pal) { pr = pal[i * 4 + 0]; pg = pal[i * 4 + 1]; pb = pal[i * 4 + 2]; }
		else     { pr = rqAry[i].rgbRed; pg = rqAry[i].rgbGreen; pb = rqAry[i].rgbBlue; }
		int dr = pr - R, dg = pg - G, db = pb - B;
		int d = dr * dr + dg * dg + db * db;
		if (d < bestDist) { bestDist = d; bestI = i; }
	}
	return (BYTE)bestI;
}

#if USE_AVERAGED_MINIMAP_COLORS

// Palette subset used when snapping downscaled terrain colours back to 8-bit.
// Restricting the search to indices that actually occur in this map's tile
// graphics keeps error diffusion from reaching for entries that exist in the
// palette but never in terrain (GUI colours, the player-colour ramps, the pure
// primaries at 248-255) - those would show up as bright speckle. It also
// shortens the per-pixel nearest search, which runs Width*Height times.
// ---- colour-space helpers ----

static float g_srgbToLinear[256];
static bool  g_srgbTablesReady = false;

static void MegamapInitSrgbTables()
{
	if (g_srgbTablesReady) return;
	for (int i = 0; i < 256; ++i)
	{
		float c = (float)i / 255.0f;
		g_srgbToLinear[i] = (c <= 0.04045f) ? (c / 12.92f)
		                                    : powf((c + 0.055f) / 1.055f, 2.4f);
	}
	g_srgbTablesReady = true;
}

// Ottosson's OKLab. Perceptually near-uniform, so plain Euclidean distance in
// it is a far better "which colour looks closest" test than RGB distance.
static void MegamapOKLab(int R, int G, int B, float* outL, float* outA, float* outB)
{
	float r = g_srgbToLinear[R & 0xff];
	float g = g_srgbToLinear[G & 0xff];
	float b = g_srgbToLinear[B & 0xff];

	float l = 0.4122214708f * r + 0.5363325363f * g + 0.0514459929f * b;
	float m = 0.2119034982f * r + 0.6806995451f * g + 0.1073969566f * b;
	float s = 0.0883024619f * r + 0.2817188376f * g + 0.6299787005f * b;

	float l_ = cbrtf(l), m_ = cbrtf(m), s_ = cbrtf(s);

	*outL = 0.2104542553f * l_ + 0.7936177850f * m_ - 0.0040720468f * s_;
	*outA = 1.9779984951f * l_ - 2.4285922050f * m_ + 0.4505937099f * s_;
	*outB = 0.0259040371f * l_ + 0.7827717662f * m_ - 0.8086757660f * s_;
}

struct MegamapPaletteSubset
{
	int   r[256], g[256], b[256];
	BYTE  index[256];
	float labL[256], labA[256], labB[256];
	int   count;
};

static void BuildMegamapPaletteSubset(MegamapPaletteSubset* out, const BYTE* pal, PTNTHeaderStruct tnt)
{
	BYTE used[256];
	memset(used, 0, sizeof(used));

	int nTiles = (tnt != NULL) ? tnt->tiles : 0;
	if (nTiles > 0 && nTiles <= 0xffff && tnt->PTRtilegfx != NULL)
	{
		const BYTE* gfx = tnt->PTRtilegfx;
		for (int i = 0, n = nTiles * 32 * 32; i < n; ++i)
			used[gfx[i]] = 1;
	}
	else
	{
		// Tile count unusable - fall back to the whole palette.
		memset(used, 1, sizeof(used));
	}

	MegamapInitSrgbTables();

	out->count = 0;
	for (int i = 0; i < 256; ++i)
	{
		if (!used[i]) continue;
		int r, g, b;
		MegamapIndexRGB(pal, (BYTE)i, &r, &g, &b);
		int k = out->count;
		out->r[k] = r;
		out->g[k] = g;
		out->b[k] = b;
		out->index[k] = (BYTE)i;
		MegamapOKLab(r, g, b, &out->labL[k], &out->labA[k], &out->labB[k]);
		++out->count;
	}
}

// Mean colour of the source tile-graphics pixels covered by one output pixel.
//
// Averages the gamma-encoded 8-bit values directly, which is also what GIMP's
// default downscale does. A linear-light average was tried and rejected: it is
// more physically correct but reads as simply lighter, with no reduction in
// stippling, and it diverges from the reference downscales the mod authors
// compare against.
static void MegamapAveragePixel(PTNTHeaderStruct tnt, const BYTE* pal, int MapDataPitch_I,
                                int xStart, int xEnd, int yStart, int yEnd,
                                int* outR, int* outG, int* outB)
{
	int sumR = 0, sumG = 0, sumB = 0, count = 0;

	for (int sy = yStart; sy < yEnd; ++sy)
	{
		int tileY = sy / 32;
		int ty    = sy % 32;
		int yTileOff = tileY * MapDataPitch_I;

		for (int sx = xStart; sx < xEnd; ++sx)
		{
			int tileX = sx / 32;
			int tx    = sx % 32;

			int TileIndex_I = tnt->PTRmapdata[yTileOff + tileX];
			if (TileIndex_I < 0) TileIndex_I = 0;

			LPBYTE tileBits = &(tnt->PTRtilegfx[TileIndex_I * 32 * 32]);
			BYTE c = tileBits[ty * 32 + tx];

			int r, g, b;
			MegamapIndexRGB(pal, c, &r, &g, &b);
			sumR += r; sumG += g; sumB += b;
			++count;
		}
	}

	if (count > 0) { *outR = sumR / count; *outG = sumG / count; *outB = sumB / count; }
	else           { *outR = 0; *outG = 0; *outB = 0; }
}

// Error accumulators are kept in 1/16ths; round-to-nearest, symmetric about 0
// (plain integer division truncates towards zero and would bias the dither).
static inline int MegamapErr16(int v)
{
	return (v >= 0) ? ((v + 8) / 16) : (-((-v + 8) / 16));
}

static BYTE MegamapSubsetNearest(const MegamapPaletteSubset* sub, int R, int G, int B)
{
	int bestDist = 0x7fffffff, bestI = 0;
	for (int i = 0; i < sub->count; ++i)
	{
		int dr = sub->r[i] - R, dg = sub->g[i] - G, db = sub->b[i] - B;
		int d = dr * dr + dg * dg + db * db;
		if (d < bestDist) { bestDist = d; bestI = i; }
	}
	return sub->index[bestI];
}

// Nearest by perceptual (OKLab) distance rather than RGB distance. The error
// accumulator stays in gamma RGB - we change which entry is judged closest, not
// what is conserved.
static BYTE MegamapSubsetNearestOKLab(const MegamapPaletteSubset* sub, int R, int G, int B)
{
	float tL, tA, tB;
	MegamapOKLab(R, G, B, &tL, &tA, &tB);

	float bestDist = 3.4e38f;
	int   bestI = 0;
	for (int i = 0; i < sub->count; ++i)
	{
		float dL = sub->labL[i] - tL;
		float dA = sub->labA[i] - tA;
		float dB = sub->labB[i] - tB;
		float d = dL * dL + dA * dA + dB * dB;
		if (d < bestDist) { bestDist = d; bestI = i; }
	}
	return sub->index[bestI];
}

static inline BYTE MegamapSnap(const MegamapPaletteSubset* sub, int R, int G, int B)
{
#if MEGAMAP_SNAP_OKLAB
	return MegamapSubsetNearestOKLab(sub, R, G, B);
#else
	return MegamapSubsetNearest(sub, R, G, B);
#endif
}

#endif // USE_AVERAGED_MINIMAP_COLORS

TNTtoMiniMap::TNTtoMiniMap ()
{
	myMiniMap= new MiniMapPicture ( 3200, 1800);
	memset ( &TNTHeader, 0, sizeof(TNTHeaderStruct));
	MapPixelBytes_PB= NULL;
	
	PaletteRefCount= PaletteRefCount+ 1;
	if (NULL==TALogPalette_Ptr)
	{
		TALogPalette_Ptr= reinterpret_cast<LPLOGPALETTE>(new BYTE[sizeof(LOGPALETTE)+ sizeof(RGBQUAD)* 256]);
		TALogPalette_Ptr->palNumEntries= 256;
		TALogPalette_Ptr->palVersion= 0x300;
		memcpy ( &(TALogPalette_Ptr->palPalEntry), rqAry, sizeof(RGBQUAD)* 256);
	}
}

TNTtoMiniMap::TNTtoMiniMap (DWORD Width, DWORD Height)
{
	myMiniMap= new MiniMapPicture ( Width, Height);
	memset ( &TNTHeader, 0, sizeof(TNTHeaderStruct));
	MapPixelBytes_PB= NULL;

	PaletteRefCount= PaletteRefCount+ 1;
	if (NULL==TALogPalette_Ptr)
	{
		TALogPalette_Ptr= reinterpret_cast<LPLOGPALETTE>(new BYTE[sizeof(LOGPALETTE)+ sizeof(RGBQUAD)* 256+ 1]);
		TALogPalette_Ptr->palNumEntries= 256;
		TALogPalette_Ptr->palVersion= 0x300;
		memcpy ( &(TALogPalette_Ptr->palPalEntry), rqAry, sizeof(RGBQUAD)* 256);
	}
}

TNTtoMiniMap::~TNTtoMiniMap ()
{
	if (NULL!=myMiniMap)
	{
		delete myMiniMap;
	}
	

	if (NULL!=MapPixelBytes_PB)
	{
		delete MapPixelBytes_PB;
	}
	PaletteRefCount= PaletteRefCount- 1;
	if (0==PaletteRefCount)
	{
		if (NULL!=TALogPalette_Ptr)
		{
			delete TALogPalette_Ptr;
			TALogPalette_Ptr= NULL;
		}
	}
}

PTNTHeaderStruct TNTtoMiniMap::ParseMyTNTHeader (PTNTHeaderStruct In_PTNTH)
{
	if (NULL!=In_PTNTH)
	{
		memcpy_s ( &TNTHeader, sizeof(TNTHeaderStruct), In_PTNTH, sizeof(TNTHeaderStruct));
		TNTHeader.Height= TNTHeader.Height/ 2; 
		TNTHeader.Width= TNTHeader.Width/ 2;
		TNTHeader.PTRmapdata= (WORD *)((int)(TNTHeader.PTRmapdata)+ (int)In_PTNTH);
		TNTHeader.PTRmapattr= (WORD *)((int)(TNTHeader.PTRmapattr)+ (int)In_PTNTH);
		TNTHeader.PTRtilegfx= (LPBYTE)((int)(TNTHeader.PTRtilegfx)+ (int)In_PTNTH);
		TNTHeader.PTRtileanim= ((int)(TNTHeader.PTRtileanim)+ (int)In_PTNTH);
		TNTHeader.PTRminimap= (LPBYTE)((int)TNTHeader.PTRminimap+ (int)In_PTNTH);
	}
	return In_PTNTH;
}

void TNTtoMiniMap::MapFromTNTFileA (LPSTR TNTHPIPath)
{//?
	HANDLE File_H;
	DWORD FileLen_I;
	LPVOID Buf_BigMem;
	Buf_BigMem= NULL;
	__try
	{
		File_H= CreateFileA ( TNTHPIPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (INVALID_HANDLE_VALUE==File_H)
		{
			__leave;
		}
		FileLen_I= GetFileSize ( File_H, NULL);
		if (0==FileLen_I)
		{
			__leave;
		}
		Buf_BigMem=  malloc ( FileLen_I);

		ReadFile ( File_H, Buf_BigMem, FileLen_I, &FileLen_I, NULL);
		CloseHandle ( File_H);


		//
		MapFromTNTInMem ( Buf_BigMem);
		free ( Buf_BigMem);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		;
	}
	return ;
}

void TNTtoMiniMap::MapFromTNTInMem (LPVOID Argc_PTNTH)
{
	ParseMyTNTHeader ( static_cast<PTNTHeaderStruct>(Argc_PTNTH));
	MapFromValidTNTHeader ( &TNTHeader);
}


void TNTtoMiniMap::MapFromValidTNTHeader (PTNTHeaderStruct Argc_PTNTH)
{
	if (Argc_PTNTH->Width<=0||Argc_PTNTH->Height<=0)
	{
		return;
	}
	RECT Whole_Rect;
	Whole_Rect.left= 0;
	Whole_Rect.top= 0;
	Whole_Rect.bottom= Argc_PTNTH->Height;
	Whole_Rect.right= Argc_PTNTH->Width;

	myMiniMap->StretchTATNTDataToMiniMap ( Argc_PTNTH);
}


int TNTtoMiniMap::DrawMiniMap (LPBYTE DescPixelBitsBegin, int Width_I, int Height_I)
{
/*
	float Divisor_F= static_cast<float>(Height_I);
	float Dividend_F= static_cast<float>(Width_I);
*/
	RECT DescRect;
	DescRect.left= 0;
	DescRect.top= 0;
	DescRect.right= Width_I;
	DescRect.bottom= Height_I;

	return myMiniMap->DrawMiniMap ( DescPixelBitsBegin, &DescRect, Width_I, Height_I, NULL);
}

void inline TNTtoMiniMap::CopyTileToTAPos_Inline (LPBYTE PixelBitsBuf, POINT * TAPos, __int16 TileIndex, PTNTHeaderStruct ArgcTNTHeader)
{
	int PiexelPerLine_I= ArgcTNTHeader->Width* 32;
	LPBYTE begin_PixelBits= &PixelBitsBuf[(TAPos->y* 32)* PiexelPerLine_I+ TAPos->x* 32];
	LPBYTE begin_TilesPixelBits= &(ArgcTNTHeader->PTRtilegfx[ArgcTNTHeader->PTRmapdata[(TAPos->y)* ArgcTNTHeader->Width+ TAPos->x] * 32* 32]);
	//TAPos和Width都是按tiles作单位的，一个tiles的大小是32 width,32 height,所以得到了tiles的坐标后，还要*32* 32才能得到在实际TilesPixelBits中的位置。
	
	for (int yCount= 0; yCount<32; ++yCount)
	{
		memcpy (  begin_PixelBits+ yCount* PiexelPerLine_I, begin_TilesPixelBits+ yCount* 32, 32);
	}
}

LPBYTE TNTtoMiniMap::DrawRectMapToBuf (LPBYTE * RectPixelBitsBuf_PtrToPB, PTNTHeaderStruct Argc_PTNTH, RECT * TAPosRect)
{//这个是按照TNTHeader中的来绘制的，这个RectPixelBitsBeginBuf_PtrToPB会返回malloc来申请的对应于Rect大小的图片。
	RECT DescRect;
	DescRect.left= 0;
	DescRect.right= (TAPosRect->right- TAPosRect->left)* 32;
	DescRect.top= 0;
	DescRect.bottom= (TAPosRect->bottom- TAPosRect->top)* 32;
	LPBYTE Buf_PB= (LPBYTE)malloc ((DescRect.right)* (DescRect.bottom)+ 1);
	

	myMiniMap->DrawMiniMap ( Buf_PB, &DescRect, DescRect.right, DescRect.bottom, NULL);
	*RectPixelBitsBuf_PtrToPB= Buf_PB;

	return Buf_PB;
}

LPBYTE TNTtoMiniMap::PictureInfo ( LPBYTE * PixelBits_pp,  POINT * Aspect)
{
	return myMiniMap->PictureInfo ( PixelBits_pp, Aspect);
}
///////////-------------

MiniMapPicture::MiniMapPicture (int Width_I, int Height_I)
{
	MiniMapPixelBits= NULL;
	
	WholeBytesInPixelsBits= Width_I* Height_I;
	MiniMapPixelBits= (LPBYTE)malloc ( WholeBytesInPixelsBits);
	
	this->Width= Width_I;
	this->Height= Height_I;
}
MiniMapPicture::~MiniMapPicture ()
{
	if (NULL!=MiniMapPixelBits)
	free ( MiniMapPixelBits);
}

LPBYTE MiniMapPicture::StretchTATNTDataToMiniMap (PTNTHeaderStruct TATNT_PTNTH)
{
	int MapDataPitch_I= TATNT_PTNTH->Width;
	int MapDataWidth_I= MapDataPitch_I- 1;
	int MapDataHeight_I= TATNT_PTNTH->Height- 4;
	float XInterval_I;
	float YInterval_I;
	float MiniMapScale= static_cast<float>(Width)/ static_cast<float>(Height);
	
	if ((MapDataHeight_I* MiniMapScale)<MapDataWidth_I)
	{
		Height= static_cast<int>(static_cast<float>(Width)/ static_cast<float>(MapDataWidth_I) * static_cast<float>(MapDataHeight_I));
	}
	else 
	if ((MapDataHeight_I* MiniMapScale)>MapDataWidth_I)
	{
		Width= static_cast<int>(static_cast<float>(Height)/ static_cast<float>(MapDataHeight_I)* static_cast<float>(MapDataWidth_I) );
	}
	else
	{
		if (Width<Height)
		{
			Height= Width;
		}
		else
		{
			Width= Height;
		}
	}
	XInterval_I= static_cast<float>(MapDataWidth_I)* 32.0f;
	XInterval_I= XInterval_I/ Width;
	YInterval_I= static_cast<float>(MapDataHeight_I)* 32.0f;
	YInterval_I= YInterval_I/ Height;
	if (1>XInterval_I)
	{
		XInterval_I= 1;
	}
	if (1>YInterval_I)
	{
		YInterval_I= 1;
	}
	if ((DWORD)(Width* Height)>WholeBytesInPixelsBits)
	{
		free ( MiniMapPixelBits);
		MiniMapPixelBits= NULL;
		WholeBytesInPixelsBits= Width* Height;
		MiniMapPixelBits= (LPBYTE)malloc ( WholeBytesInPixelsBits);
	}
	
	if (NULL==MiniMapPixelBits)
	{
		return NULL;
	}

#if USE_AVERAGED_MINIMAP_COLORS

	// === High-quality path: area sampling + RGB average + error-diffused snap ===

	// Live colormap from game (R,G,B,0 layout per entry) for accurate RGB averaging of tile pixels.
	// Falls back to rqAry (BGRx) only if no game struct (should not happen for in-game TNT minimap).
	const BYTE* palBase = MegamapColormap();

	MegamapPaletteSubset palSubset;
	BuildMegamapPaletteSubset(&palSubset, palBase, TATNT_PTNTH);

	int srcPixelW = MapDataWidth_I * 32;
	int srcPixelH = MapDataHeight_I * 32;

	// Row of averaged colours, plus this-row / next-row error accumulators for
	// the diffusion pass. The error rows carry a one-pixel guard band at each
	// end (index x is stored at x+1) so the x-1 / x+1 taps never need clamping.
	int* rowAvg  = NULL;
	int* errCur  = NULL;
	int* errNext = NULL;
	bool dither  = false;

#if USE_MINIMAP_ERROR_DIFFUSION
	rowAvg  = (int*)malloc(sizeof(int) * 3 * Width);
	errCur  = (int*)calloc(3 * (Width + 2), sizeof(int));
	errNext = (int*)calloc(3 * (Width + 2), sizeof(int));
	dither  = (rowAvg != NULL && errCur != NULL && errNext != NULL);
	if (!dither)
	{
		// Out of memory - drop to the undithered snap rather than failing.
		free(rowAvg);  rowAvg  = NULL;
		free(errCur);  errCur  = NULL;
		free(errNext); errNext = NULL;
	}
#endif

	int scanDir = 1;   // serpentine: alternate direction each row to break up worms

	for (int YPos= 0; YPos<Height; YPos++)
	{	//Y
		int MiniMapPixelYStart= YPos* Width;

		// Source pixel range in the full tilegfx resolution covered by this output pixel
		int yStart = static_cast<int>(YPos * YInterval_I);
		int yEnd   = static_cast<int>((YPos + 1) * YInterval_I);
		if (yEnd <= yStart) yEnd = yStart + 1;
		if (yStart < 0) yStart = 0;
		if (yEnd > srcPixelH) yEnd = srcPixelH;
		if (yStart >= srcPixelH) yStart = srcPixelH - 1;

		for (int XPos= 0; XPos<Width; XPos++)
		{//X
			int xStart = static_cast<int>(XPos * XInterval_I);
			int xEnd   = static_cast<int>((XPos + 1) * XInterval_I);
			if (xEnd <= xStart) xEnd = xStart + 1;
			if (xStart < 0) xStart = 0;
			if (xEnd > srcPixelW) xEnd = srcPixelW;
			if (xStart >= srcPixelW) xStart = srcPixelW - 1;

			int avgR, avgG, avgB;
			MegamapAveragePixel(TATNT_PTNTH, palBase, MapDataPitch_I,
			                    xStart, xEnd, yStart, yEnd, &avgR, &avgG, &avgB);

			if (dither)
			{
				rowAvg[XPos * 3 + 0] = avgR;
				rowAvg[XPos * 3 + 1] = avgG;
				rowAvg[XPos * 3 + 2] = avgB;
			}
			else
			{
				MiniMapPixelBits[MiniMapPixelYStart + XPos] =
					MegamapSnap(&palSubset, avgR, avgG, avgB);
			}
		}

		if (!dither)
			continue;

		// Floyd-Steinberg across the row (serpentine). Without this the averaged
		// colours snap to whichever single palette entry is nearest, which for TA
		// terrain means the neutral olive ramp - see the note at the top of the file.
		int xFirst = (scanDir > 0) ? 0 : (Width - 1);
		for (int step = 0; step < Width; ++step)
		{
			int x = xFirst + scanDir * step;

			int here = (x + 1) * 3;
			int fwd  = (x + scanDir + 1) * 3;
			int back = (x - scanDir + 1) * 3;

			int tr = rowAvg[x * 3 + 0] + MegamapErr16(errCur[here + 0]);
			int tg = rowAvg[x * 3 + 1] + MegamapErr16(errCur[here + 1]);
			int tb = rowAvg[x * 3 + 2] + MegamapErr16(errCur[here + 2]);
			if (tr < 0) tr = 0; else if (tr > 255) tr = 255;
			if (tg < 0) tg = 0; else if (tg > 255) tg = 255;
			if (tb < 0) tb = 0; else if (tb > 255) tb = 255;

			BYTE idx = MegamapSnap(&palSubset, tr, tg, tb);
			MiniMapPixelBits[MiniMapPixelYStart + x] = idx;

			int pr, pg, pb;
			MegamapIndexRGB(palBase, idx, &pr, &pg, &pb);
			int er = tr - pr, eg = tg - pg, eb = tb - pb;

			errCur [fwd  + 0] += er * 7;  errCur [fwd  + 1] += eg * 7;  errCur [fwd  + 2] += eb * 7;
			errNext[back + 0] += er * 3;  errNext[back + 1] += eg * 3;  errNext[back + 2] += eb * 3;
			errNext[here + 0] += er * 5;  errNext[here + 1] += eg * 5;  errNext[here + 2] += eb * 5;
			errNext[fwd  + 0] += er * 1;  errNext[fwd  + 1] += eg * 1;  errNext[fwd  + 2] += eb * 1;
		}

		int* swap = errCur; errCur = errNext; errNext = swap;
		memset(errNext, 0, sizeof(int) * 3 * (Width + 2));
		scanDir = -scanDir;
	}

	free(rowAvg);
	free(errCur);
	free(errNext);

#else

	// === Original simple sampling path (single pixel per minimap output pixel) ===

	for (int YPos= 0, YInTrue= 0; YPos<Height; YPos++, YInTrue= static_cast<int> (YPos* YInterval_I))
	{	//Y
		int MiniMapPixelYStart= YPos* Width;
		int MiniMapTileIndexYoffset= (YInTrue/ 32)* (MapDataPitch_I);
		int MiniMapTileYOffset= (YInTrue% 32)* 32;

		for (int XPos= 0, XInTrue= 0; XPos<Width; XPos++, XInTrue=  static_cast<int> (XPos* XInterval_I))
		{//X 
			int TileIndex_I= TATNT_PTNTH->PTRmapdata[MiniMapTileIndexYoffset+ XInTrue/ 32];
			if (0>TileIndex_I)
			{
				TileIndex_I= 0;
			}
			LPBYTE begin_TilesPixelBits= &(TATNT_PTNTH->PTRtilegfx[TileIndex_I* 32* 32]);

			int MiniMapByte= begin_TilesPixelBits[MiniMapTileYOffset+ (XInTrue% 32)];
			MiniMapPixelBits[MiniMapPixelYStart+ XPos]= MiniMapByte;
		}
	}

#endif

	return MiniMapPixelBits;
}

LPBYTE MiniMapPicture::StretchTAMapToMiniMap (LPBYTE RectPixelBitsBuf_PB, RECT * TAMapRect)
{
	int MapWidth_I= (TAMapRect->right- TAMapRect->left);
	int MapHeight_I= (TAMapRect->bottom- TAMapRect->top);
	float XInterval_I;
	float YInterval_I;


	if (MapHeight_I<MapWidth_I)
	{
		Height= static_cast<int>(static_cast<float>(Width)/ static_cast<float>(MapWidth_I) * static_cast<float>(MapHeight_I));
		
	}
	else
	if (MapHeight_I>MapWidth_I)
	{
		Width= static_cast<int>(static_cast<float>(Height)/ static_cast<float>(MapHeight_I)* static_cast<float>(MapWidth_I));
	}
	else
	{
		if (Width<Height)
		{
			Height= Width;
			
		}
		else
		{
			Width= Height;
		}
	}

	XInterval_I= static_cast<float>(MapWidth_I)* 32.0f;
	XInterval_I= XInterval_I/ Width;
	YInterval_I= static_cast<float>(MapHeight_I)* 32.0f;
	YInterval_I= YInterval_I/ Height;
	if (1>XInterval_I)
	{
		XInterval_I= 1;
	}
	if (1>YInterval_I)
	{
		YInterval_I= 1;
	}
	if ((DWORD)(Width* Height)>WholeBytesInPixelsBits)
	{
		free ( MiniMapPixelBits);
		MiniMapPixelBits= NULL;
		WholeBytesInPixelsBits= Width* Height;
		MiniMapPixelBits= (LPBYTE)malloc ( WholeBytesInPixelsBits);
	}

	if (NULL==MiniMapPixelBits)
	{
		return NULL;
	}
	int MapPixelWidth_I=  MapWidth_I* 32;
	for (int YPos= 0; YPos<Height; YPos++)
	{	//Y
		int MiniMapPixelYStart= YPos* Width;
		int PixelPerYPosMean_I= static_cast<int>(YPos* YInterval_I)* MapPixelWidth_I;
		for (int XPos= 0; XPos<Width; XPos++)
		{//X 
			MiniMapPixelBits[MiniMapPixelYStart+ XPos]= RectPixelBitsBuf_PB[PixelPerYPosMean_I+ static_cast<int>(XPos* XInterval_I)];
		}
	}
//	
	return MiniMapPixelBits;
}

int MiniMapPicture::DrawMiniMap (LPBYTE DescPixelBitsBegin, RECT * DescRect, int DescPixelWidth_I, int DescPixelHeight_I,  RECT * MiniMapRect)
{	
	if ((NULL==DescPixelBitsBegin)||(NULL==MiniMapPixelBits))
	{
		return MMPERROR_NOTVALIDBUF;
	}

	int Rtn_I= 0;
	RECT FullMiniMap;
	RECT DescRect_Local;
	memcpy ( &DescRect_Local, DescRect, sizeof(RECT));

	if (NULL==MiniMapRect)
	{
		FullMiniMap.right= Width;
		FullMiniMap.bottom= Height;
		FullMiniMap.left= 0;
		FullMiniMap.top= 0;
		MiniMapRect= &FullMiniMap;
	}

	float XInterval_I;
	float YInterval_I;

	int DescWidth_I;
	int DescHeight_I;

	int SrcWidth_I;
	int SrcHeight_I;
	__try	
	{
		if (MiniMapRect->right>Width)
		{
			MiniMapRect->right= Width;
			Rtn_I= MMPERROR_MODIFYRECT;
		}
		if (MiniMapRect->bottom>Height)
		{
			MiniMapRect->bottom=Height;
			Rtn_I= MMPERROR_MODIFYRECT;
		}
		if (MiniMapRect->left<0)
		{
			MiniMapRect->left= 0;
			Rtn_I= MMPERROR_MODIFYRECT;
		}
		if (MiniMapRect->top<0)
		{
			MiniMapRect->top= 0;
			Rtn_I= MMPERROR_MODIFYRECT;
		}

		if (DescRect_Local.left<0)
		{
			DescRect_Local.left= 0;
			Rtn_I= MMPERROR_MODIFYDESCRECT;
		}
		if (DescRect_Local.top<0)
		{
			DescRect_Local.top= 0;
			Rtn_I= MMPERROR_MODIFYDESCRECT;
		}

		DescWidth_I= DescRect_Local.right- DescRect_Local.left;
		DescHeight_I= (DescRect_Local.bottom- DescRect_Local.top);

		SrcWidth_I= MiniMapRect->right- MiniMapRect->left;
		SrcHeight_I= MiniMapRect->bottom- MiniMapRect->top;
		if (DescWidth_I>DescHeight_I)
		{
			if (DescHeight_I>SrcHeight_I)
			{
				int HeightBorder_I= (DescHeight_I- SrcHeight_I)/ 2;
				DescHeight_I= SrcHeight_I;
				DescRect_Local.top+= HeightBorder_I;
				DescRect_Local.bottom= DescRect_Local.top+ DescHeight_I;
			}
			int NewDescWidth_I= SrcWidth_I* DescHeight_I/ SrcHeight_I;
			int WidthBorder_I= (DescWidth_I- NewDescWidth_I)/ 2;
			DescWidth_I= NewDescWidth_I;
			DescRect_Local.left+= WidthBorder_I;
			DescRect_Local.right= DescRect_Local.left+ DescWidth_I;
		}
		else if (DescWidth_I<DescHeight_I)
		{
			if (DescWidth_I>SrcWidth_I)
			{
				int WidthBorder_I= (DescWidth_I- SrcWidth_I)/ 2;
				DescWidth_I= SrcWidth_I;
				DescRect_Local.left+= WidthBorder_I;
				DescRect_Local.right= DescRect_Local.left+ DescWidth_I;
			}
			int NewDescHeight_I= SrcHeight_I* DescWidth_I/ SrcWidth_I;
			int HeightBorder_I= (NewDescHeight_I- DescHeight_I)/ 2;
			DescHeight_I= NewDescHeight_I;
			DescRect_Local.top+= HeightBorder_I;
			DescRect_Local.bottom= DescRect_Local.top+ DescHeight_I;
		}

		// now, the Rect is suitable to mini map. so we can stretch map.

		if (0==DescWidth_I||0==DescHeight_I)
		{
			return MMPERROR_NOTVALIDDESCSIZE;
		}


		XInterval_I= static_cast<float>(Width)/ static_cast<float>(DescWidth_I);
		YInterval_I= static_cast<float>(Height)/ static_cast<float>(DescHeight_I);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER)
	{
		return MMPERROR_NOTVALIDDESCSIZE;
	}

	////
	__try
	{
		for (int YPos= 0; YPos<DescHeight_I; YPos++)
		{	//Y
			int DescPixelYStart= (YPos+ DescRect_Local.top)* (DescPixelWidth_I);
			int MiniMapPixelYStart= static_cast<int>(YPos* YInterval_I)* Width;
			for (int XPos= 0; XPos<DescWidth_I; XPos++)
			{//X 
				DescPixelBitsBegin[DescPixelYStart+ (XPos+ DescRect_Local.left)]= MiniMapPixelBits[MiniMapPixelYStart+ static_cast<int>(XPos* XInterval_I)];
			}
		}
	}
	__except ( EXCEPTION_EXECUTE_HANDLER)
	{
		return MMPERROR_NOTVALIDBUF;
	}
	//SaveToLagBmp ( Height, Width, MiniMapPixelBits);
	return Rtn_I;
}

LPBYTE MiniMapPicture::PictureInfo ( LPBYTE * PixelBits_pp,  POINT * Aspect)
{
	if (PixelBits_pp)
	{
		*PixelBits_pp= MiniMapPixelBits;
	}
	
	if (Aspect)
	{
		Aspect->x= Width;
		Aspect->y= Height;
	}

	return MiniMapPixelBits;
}

RGBQUAD rqAry[256]=
{
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x80, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x80, 0x00, 0x00, 0x80, 0x80, 0x80, 0x00
	, 0xC0, 0xDC, 0xC0, 0x00, 0xFC, 0x54, 0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0xF3, 0xEB, 0xFF, 0x00, 0xD3, 0xC7, 0xEB, 0x00, 0xB3, 0xA3, 0xD7, 0x00, 0x97, 0x87, 0xC3, 0x00, 0x7F, 0x6F, 0xAF, 0x00, 0x63, 0x5B, 0x9B, 0x00, 0x4F, 0x47, 0x8B, 0x00, 0x47, 0x3B, 0x7B, 0x00
	, 0x3B, 0x33, 0x6F, 0x00, 0x33, 0x2B, 0x63, 0x00, 0x2B, 0x23, 0x57, 0x00, 0x27, 0x1B, 0x4B, 0x00, 0x1F, 0x17, 0x3B, 0x00, 0x17, 0x0F, 0x2F, 0x00, 0x0F, 0x0B, 0x23, 0x00, 0x0B, 0x07, 0x17, 0x00
	, 0xDF, 0xFF, 0x73, 0x00, 0xBF, 0xE7, 0x57, 0x00, 0x9F, 0xCF, 0x43, 0x00, 0x83, 0xB7, 0x2F, 0x00, 0x67, 0x9F, 0x1F, 0x00, 0x4F, 0x8B, 0x13, 0x00, 0x3F, 0x77, 0x0F, 0x00, 0x37, 0x6B, 0x0B, 0x00
	, 0x2F, 0x5F, 0x07, 0x00, 0x2B, 0x53, 0x07, 0x00, 0x27, 0x47, 0x00, 0x00, 0x23, 0x3F, 0x00, 0x00, 0x1B, 0x33, 0x00, 0x00, 0x17, 0x27, 0x00, 0x00, 0x0F, 0x1B, 0x00, 0x00, 0x0B, 0x13, 0x00, 0x00
	, 0xFF, 0xEF, 0xE3, 0x00, 0xE7, 0xDF, 0xC7, 0x00, 0xCB, 0xCF, 0xAF, 0x00, 0xA7, 0xB7, 0x93, 0x00, 0x83, 0x9F, 0x7F, 0x00, 0x67, 0x87, 0x6B, 0x00, 0x53, 0x6F, 0x5F, 0x00, 0x47, 0x63, 0x5F, 0x00
	, 0x3B, 0x57, 0x5B, 0x00, 0x33, 0x43, 0x53, 0x00, 0x2B, 0x3B, 0x47, 0x00, 0x23, 0x33, 0x3B, 0x00, 0x1B, 0x2B, 0x2F, 0x00, 0x13, 0x1F, 0x23, 0x00, 0x0F, 0x13, 0x17, 0x00, 0x07, 0x0B, 0x0B, 0x00
	, 0xD7, 0xFB, 0xFB, 0x00, 0xB7, 0xDF, 0xDF, 0x00, 0x9B, 0xC3, 0xC3, 0x00, 0x83, 0xAB, 0xAB, 0x00, 0x6F, 0x93, 0x93, 0x00, 0x57, 0x77, 0x77, 0x00, 0x43, 0x63, 0x63, 0x00, 0x33, 0x53, 0x53, 0x00
	, 0x23, 0x43, 0x43, 0x00, 0x17, 0x33, 0x33, 0x00, 0x0F, 0x23, 0x23, 0x00, 0x07, 0x1B, 0x1B, 0x00, 0x07, 0x17, 0x17, 0x00, 0x00, 0x13, 0x13, 0x00, 0x00, 0x0F, 0x0F, 0x00, 0x00, 0x0B, 0x0B, 0x00
	, 0xFB, 0xFB, 0xFB, 0x00, 0xEB, 0xEB, 0xEB, 0x00, 0xDB, 0xDB, 0xDB, 0x00, 0xCB, 0xCB, 0xCB, 0x00, 0xBB, 0xBB, 0xBB, 0x00, 0xAB, 0xAB, 0xAB, 0x00, 0x9B, 0x9B, 0x9B, 0x00, 0x8B, 0x8B, 0x8B, 0x00
	, 0x7B, 0x7B, 0x7B, 0x00, 0x6B, 0x6B, 0x6B, 0x00, 0x5B, 0x5B, 0x5B, 0x00, 0x4B, 0x4B, 0x4B, 0x00, 0x3B, 0x3B, 0x3B, 0x00, 0x2B, 0x2B, 0x2B, 0x00, 0x1F, 0x1F, 0x1F, 0x00, 0x0F, 0x0F, 0x0F, 0x00
	, 0xFF, 0xF3, 0xEB, 0x00, 0xFF, 0xE3, 0xCB, 0x00, 0xFF, 0xCF, 0xAF, 0x00, 0xFF, 0xB3, 0x97, 0x00, 0xFF, 0x97, 0x7B, 0x00, 0xFF, 0x7F, 0x67, 0x00, 0xEF, 0x6B, 0x53, 0x00, 0xE3, 0x5B, 0x3F, 0x00
	, 0xD7, 0x4B, 0x33, 0x00, 0xCB, 0x3B, 0x23, 0x00, 0xAF, 0x2F, 0x17, 0x00, 0x97, 0x27, 0x0F, 0x00, 0x7B, 0x1F, 0x07, 0x00, 0x63, 0x17, 0x07, 0x00, 0x47, 0x0F, 0x00, 0x00, 0x2F, 0x0B, 0x00, 0x00
	, 0xFF, 0xF7, 0xE3, 0x00, 0xE7, 0xDB, 0xBF, 0x00, 0xCF, 0xBF, 0x9F, 0x00, 0xB7, 0xA7, 0x83, 0x00, 0xA3, 0x8F, 0x6B, 0x00, 0x8B, 0x77, 0x53, 0x00, 0x73, 0x5F, 0x3F, 0x00, 0x5F, 0x4B, 0x2F, 0x00
	, 0x57, 0x3F, 0x27, 0x00, 0x4F, 0x37, 0x23, 0x00, 0x47, 0x2F, 0x1F, 0x00, 0x3F, 0x27, 0x1B, 0x00, 0x37, 0x1F, 0x17, 0x00, 0x2F, 0x1B, 0x13, 0x00, 0x27, 0x13, 0x0F, 0x00, 0x1F, 0x0F, 0x0B, 0x00
	, 0xFF, 0xEF, 0xD7, 0x00, 0xEF, 0xE3, 0xBB, 0x00, 0xDF, 0xCB, 0x9B, 0x00, 0xCF, 0xB7, 0x83, 0x00, 0xC3, 0xA3, 0x6B, 0x00, 0xB3, 0x8F, 0x53, 0x00, 0xA3, 0x7B, 0x3F, 0x00, 0x97, 0x6B, 0x2F, 0x00
	, 0x87, 0x5B, 0x23, 0x00, 0x77, 0x4B, 0x1B, 0x00, 0x67, 0x3F, 0x13, 0x00, 0x57, 0x33, 0x0B, 0x00, 0x47, 0x27, 0x07, 0x00, 0x37, 0x1B, 0x00, 0x00, 0x27, 0x13, 0x00, 0x00, 0x1B, 0x0B, 0x00, 0x00
	, 0xFF, 0xE7, 0xFF, 0x00, 0xEB, 0xC7, 0xE7, 0x00, 0xD7, 0xAB, 0xD3, 0x00, 0xC3, 0x93, 0xBB, 0x00, 0xB3, 0x7B, 0xA7, 0x00, 0x9F, 0x63, 0x8F, 0x00, 0x8F, 0x4B, 0x77, 0x00, 0x7F, 0x3B, 0x63, 0x00
	, 0x6F, 0x2B, 0x4F, 0x00, 0x63, 0x1F, 0x43, 0x00, 0x57, 0x17, 0x37, 0x00, 0x47, 0x0F, 0x2B, 0x00, 0x3B, 0x07, 0x1F, 0x00, 0x2B, 0x00, 0x13, 0x00, 0x1F, 0x00, 0x0B, 0x00, 0x13, 0x00, 0x07, 0x00
	, 0xA7, 0xFF, 0xD7, 0x00, 0x7F, 0xE7, 0xAB, 0x00, 0x5B, 0xD3, 0x83, 0x00, 0x3F, 0xBF, 0x67, 0x00, 0x2B, 0xAB, 0x4B, 0x00, 0x2B, 0x97, 0x43, 0x00, 0x27, 0x87, 0x37, 0x00, 0x1B, 0x77, 0x2F, 0x00
	, 0x13, 0x67, 0x2B, 0x00, 0x0F, 0x5B, 0x23, 0x00, 0x0B, 0x4F, 0x1F, 0x00, 0x07, 0x43, 0x1B, 0x00, 0x00, 0x33, 0x17, 0x00, 0x00, 0x27, 0x0F, 0x00, 0x00, 0x1B, 0x0B, 0x00, 0x00, 0x0F, 0x07, 0x00
	, 0x9F, 0xE3, 0xFF, 0x00, 0x73, 0xC7, 0xE3, 0x00, 0x53, 0xAF, 0xCB, 0x00, 0x3F, 0x97, 0xB3, 0x00, 0x2F, 0x83, 0x9B, 0x00, 0x23, 0x6F, 0x83, 0x00, 0x17, 0x5B, 0x6B, 0x00, 0x0F, 0x47, 0x53, 0x00
	, 0x0B, 0x3B, 0x4B, 0x00, 0x07, 0x33, 0x43, 0x00, 0x07, 0x2B, 0x3B, 0x00, 0x00, 0x23, 0x37, 0x00, 0x00, 0x1B, 0x2F, 0x00, 0x00, 0x13, 0x27, 0x00, 0x00, 0x0F, 0x1F, 0x00, 0x00, 0x0B, 0x1B, 0x00
	, 0xA3, 0xFF, 0xFF, 0x00, 0x83, 0xF3, 0xFB, 0x00, 0x67, 0xE3, 0xF7, 0x00, 0x4F, 0xD3, 0xF3, 0x00, 0x33, 0xBB, 0xEF, 0x00, 0x1B, 0xA7, 0xEF, 0x00, 0x13, 0x8F, 0xEB, 0x00, 0x0F, 0x7B, 0xE7, 0x00
	, 0x07, 0x4F, 0xDF, 0x00, 0x00, 0x23, 0xD7, 0x00, 0x00, 0x1F, 0xBF, 0x00, 0x00, 0x1B, 0xA7, 0x00, 0x00, 0x17, 0x93, 0x00, 0x00, 0x13, 0x7B, 0x00, 0x00, 0x13, 0x63, 0x00, 0x00, 0x0F, 0x4F, 0x00
	, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xBF, 0xFF, 0x00, 0x00, 0x83, 0xFF, 0x00, 0x00, 0x47, 0xFF, 0x00, 0x00, 0x2B, 0xD3, 0x00, 0x00, 0x17, 0xAB, 0x00, 0x00, 0x07, 0x7F, 0x00, 0x00, 0x00, 0x57, 0x00
	, 0xFF, 0xCB, 0xDF, 0x00, 0xDF, 0x9F, 0xBB, 0x00, 0xBF, 0x77, 0x9B, 0x00, 0x9F, 0x57, 0x7F, 0x00, 0x7F, 0x3B, 0x67, 0x00, 0x5F, 0x23, 0x4B, 0x00, 0x3F, 0x13, 0x33, 0x00, 0x1F, 0x07, 0x1B, 0x00
	, 0xFF, 0xDB, 0xD3, 0x00, 0xF7, 0x9F, 0x87, 0x00, 0xEF, 0x6F, 0x43, 0x00, 0xE7, 0x47, 0x17, 0x00, 0xBB, 0x2B, 0x0B, 0x00, 0x8F, 0x17, 0x07, 0x00, 0x63, 0x07, 0x00, 0x00, 0x37, 0x00, 0x00, 0x00
	, 0x77, 0xFF, 0x7B, 0x00, 0x4F, 0xDF, 0x53, 0x00, 0x2B, 0xBF, 0x33, 0x00, 0x13, 0x9F, 0x1B, 0x00, 0x0B, 0x7F, 0x1B, 0x00, 0x07, 0x5F, 0x17, 0x00, 0x00, 0x3F, 0x13, 0x00, 0x00, 0x1F, 0x0B, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xFB, 0xFF, 0x00, 0xA4, 0xA0, 0xA0, 0x00
	, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00
};

#endif