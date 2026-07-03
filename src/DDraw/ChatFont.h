#pragma once

struct _OFFSCREEN;

// A crisp, accessible replacement font for in-game chat text.
//
// The engine renders chat with a low-res bitmap (GAF) font that looks soft when
// upscaled. This module rasterises a real TrueType face (via GDI, into an
// in-memory bitmap that is independent of the game's DirectDraw surface, so it
// works under cnc-ddraw) into an 8bpp glyph atlas once, then blits the glyphs
// straight into the chat surface. Sharp / no-antialiasing for maximum legibility.
namespace ChatFont
{
	// Build (or rebuild) the glyph atlas for the given pixel cell height.
	// Cheap no-op when already built for that height. Returns false if the
	// font could not be rasterised (caller should fall back to the engine font).
	bool Ensure(int pixelHeight);

	// Pixel advance width of str using the current atlas (0 if none).
	int Measure(const char* str);

	// Blit str into the 8bpp OFFSCREEN so the visible ink's top-left is at
	// (x,y), using palette index colorIndex.
	void DrawString(_OFFSCREEN* offscreen, const char* str, int x, int y, int colorIndex);

	// Metrics of the current atlas (for alignment diagnostics).
	int CellHeight();
	int Ascent();
	int InkTop();
}
