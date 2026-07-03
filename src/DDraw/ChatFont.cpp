#include "config.h"
#include "ChatFont.h"

#include "tamem.h"

#include <windows.h>
#include <vector>

namespace
{
	// Accessible, high-legibility face that ships with Windows. Verdana was
	// designed specifically for on-screen reading at small sizes.
	const char* const kFontFace = "Verdana";

	const int  kFirstChar = 0x20;
	const int  kLastChar  = 0xFF;

	struct Glyph
	{
		int advance = 0;                 // pen advance in pixels
		int w = 0;                       // stored coverage width
		int h = 0;                       // stored coverage height (cell height)
		int inkLeft = 0;                 // first column containing ink
		std::vector<unsigned char> cover;// w*h, 1 = ink
	};

	Glyph g_glyphs[256];
	int   g_builtHeight = 0;             // pixelHeight the atlas was built for
	int   g_cellH       = 0;             // >0 once a usable atlas exists
	int   g_inkTop      = 0;             // topmost inked row across the atlas
	int   g_ascent      = 0;             // font ascent (for diagnostics)

	void ClearAtlas()
	{
		for (int i = 0; i < 256; ++i)
			g_glyphs[i] = Glyph();
		g_cellH = 0;
	}
}

bool ChatFont::Ensure(int pixelHeight)
{
	if (pixelHeight <= 0 || pixelHeight > 128)
		return false;
	if (g_builtHeight == pixelHeight)
		return g_cellH > 0;

	g_builtHeight = pixelHeight;   // remember the attempt even if it fails
	ClearAtlas();

	HDC screenDc = GetDC(NULL);
	HDC hdc = CreateCompatibleDC(screenDc);
	if (screenDc)
		ReleaseDC(NULL, screenDc);
	if (!hdc)
		return false;

	// NONANTIALIASED_QUALITY => sharp, hard-edged glyphs (no grey fringe),
	// which is both what we want and simplest to map into an 8bpp palette.
	HFONT font = CreateFontA(
		pixelHeight, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		NONANTIALIASED_QUALITY, DEFAULT_PITCH, kFontFace);
	if (!font)
	{
		DeleteDC(hdc);
		return false;
	}
	HFONT oldFont = (HFONT)SelectObject(hdc, font);

	TEXTMETRICA tm;
	GetTextMetricsA(hdc, &tm);
	const int cellH = tm.tmHeight > 0 ? tm.tmHeight : pixelHeight;
	g_ascent = tm.tmAscent;
	const int cellW = pixelHeight * 2 + 8;   // generous per-glyph scratch width

	BITMAPINFO bmi;
	ZeroMemory(&bmi, sizeof(bmi));
	bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth       = cellW;
	bmi.bmiHeader.biHeight      = -cellH;    // top-down
	bmi.bmiHeader.biPlanes      = 1;
	bmi.bmiHeader.biBitCount    = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	void* bits = NULL;
	HBITMAP dib = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
	bool ok = false;
	if (dib && bits)
	{
		HBITMAP oldBmp = (HBITMAP)SelectObject(hdc, dib);
		SetTextColor(hdc, RGB(255, 255, 255));
		SetBkColor(hdc, RGB(0, 0, 0));
		SetBkMode(hdc, OPAQUE);

		const unsigned int* px = (const unsigned int*)bits;
		RECT cell = { 0, 0, cellW, cellH };

		for (int c = kFirstChar; c <= kLastChar; ++c)
		{
			char ch = (char)c;
			SIZE sz;
			if (!GetTextExtentPoint32A(hdc, &ch, 1, &sz))
				continue;
			int adv = sz.cx;
			if (adv < 0) adv = 0;
			if (adv > cellW) adv = cellW;

			// ETO_OPAQUE fills the cell with the (black) background, then draws
			// the (white) glyph — one call clears and renders.
			ExtTextOutA(hdc, 0, 0, ETO_OPAQUE, &cell, &ch, 1, NULL);
			GdiFlush();

			Glyph& g = g_glyphs[c];
			g.advance = adv;
			g.w = adv;
			g.h = cellH;
			g.inkLeft = adv;   // sentinel; lowered below if ink found
			g.cover.assign((size_t)adv * cellH, 0);
			for (int y = 0; y < cellH; ++y)
				for (int x = 0; x < adv; ++x)
					if ((px[y * cellW + x] & 0xFFFFFF) > 0x7F7F7F)
						g.cover[(size_t)y * adv + x] = 1;
		}

		SelectObject(hdc, oldBmp);

		// Compute the atlas-wide top inked row and each glyph's left inked
		// column, so DrawString can anchor the visible ink (not the blank cell
		// padding) to the requested position.
		g_inkTop = cellH;
		for (int c = kFirstChar; c <= kLastChar; ++c)
		{
			Glyph& g = g_glyphs[c];
			if (g.cover.empty())
				continue;
			for (int y = 0; y < g.h; ++y)
				for (int x = 0; x < g.w; ++x)
					if (g.cover[(size_t)y * g.w + x])
					{
						if (y < g_inkTop)  g_inkTop = y;
						if (x < g.inkLeft) g.inkLeft = x;
						break;
					}
			if (g.inkLeft > g.w)   // no ink (e.g. space)
				g.inkLeft = 0;
		}
		if (g_inkTop >= cellH)
			g_inkTop = 0;

		g_cellH = cellH;
		ok = true;
	}

	if (dib)
		DeleteObject(dib);
	SelectObject(hdc, oldFont);
	DeleteObject(font);
	DeleteDC(hdc);
	return ok;
}

int ChatFont::Measure(const char* str)
{
	if (g_cellH <= 0 || !str)
		return 0;
	int w = 0;
	for (const unsigned char* p = (const unsigned char*)str; *p; ++p)
	{
		if (*p < kFirstChar)
			continue;
		w += g_glyphs[*p].advance;
	}
	return w;
}

int ChatFont::CellHeight() { return g_cellH; }
int ChatFont::Ascent()     { return g_ascent; }
int ChatFont::InkTop()     { return g_inkTop; }

void ChatFont::DrawString(_OFFSCREEN* offscreen, const char* str, int x, int y, int colorIndex)
{
	OFFSCREEN* s = (OFFSCREEN*)offscreen;
	if (!s || !s->lpSurface || g_cellH <= 0 || !str)
		return;

	unsigned char* base = (unsigned char*)s->lpSurface;
	const int pitch  = s->lPitch;
	const int width  = s->ScreenRect.right > 0 ? s->ScreenRect.right : pitch;
	const int height = s->Height;
	const unsigned char color = (unsigned char)colorIndex;

	// Anchor the visible ink top-left to (x,y): skip the blank rows above the
	// ink (g_inkTop) and the left bearing of the first glyph, so (x,y) is where
	// the text visually begins — matching how the engine anchors its glyphs.
	int firstInkLeft = 0;
	for (const unsigned char* p = (const unsigned char*)str; *p; ++p)
		if (*p >= kFirstChar)
		{
			firstInkLeft = g_glyphs[*p].inkLeft;
			break;
		}

	int cx = x - firstInkLeft;
	for (const unsigned char* p = (const unsigned char*)str; *p; ++p)
	{
		if (*p < kFirstChar)
			continue;
		const Glyph& g = g_glyphs[*p];
		for (int gy = 0; gy < g.h; ++gy)
		{
			const int py = y + gy - g_inkTop;
			if ((unsigned)py >= (unsigned)height)
				continue;
			const unsigned char* row = &g.cover[(size_t)gy * g.w];
			unsigned char* dst = base + (size_t)py * pitch;
			for (int gx = 0; gx < g.w; ++gx)
			{
				if (row[gx])
				{
					const int px = cx + gx;
					if ((unsigned)px < (unsigned)width)
						dst[px] = color;
				}
			}
		}
		cx += g.advance;
	}
}
