#include "config.h"
#include "ChatBackdrop.h"

#include "tamem.h"
#include "tafunctions.h"
#include "iddrawsurface.h"
#include "dialog.h"
#include "gaf.h"
#include "hook/hook.h"
#include "ChatFont.h"
#include "ChatPosition.h"

#include <windows.h>
#include <cstring>
#include <cstdio>

// -----------------------------------------------------------------------
// Chat legibility backdrop.
//
// The engine draws in-game chat messages top-left via ChatMessageWithLogo
// (a.k.a. DrawChatText) @ 0x00464060. The layout below is recovered from that
// function's disassembly:
//
//   * TA main struct base   = *(void**)0x00511de8
//   * ChatText ring buffer   @ +0x12EF, 30 entries of 0x48 bytes, wraps at 30
//   * per-entry str[0x46]     = player index (10 => no logo)
//   * per-entry str[0x47]     = flags: &0x0F channel, &0x20 highlight
//   * ChatTextFreeIndex (word)@ +0x2A3E : ring write head
//   * ChatTextNum      (word) @ +0x2A40
//   * field_37EFE     (dword) @ +0x37EFE: chat display mode (1=team,2=ally,3=all)
//   * screenchat      (dword) @ +0x37F02
//   * textlines       (dword) @ +0x37F27: max visible lines
//   * pCOMIXFont            @ +0x391F9 : FontDataStruct*, [0] = charHeight
//
//   First visible line is drawn at (0x8A, 0x34); each subsequent visible line
//   is one charHeight lower. When a logo is present the text is shifted right
//   by roughly one charHeight to make room for it. GetTextExtent (0x4C1480)
//   measures a line's pixel width. Black is palette index 0.
//
// This routine replicates the engine's visibility filter and line walk, but
// instead of drawing text it fills a solid black rectangle behind each line.
// -----------------------------------------------------------------------

namespace
{
	const unsigned OFF_CHAT_TEXT     = 0x12EF;   // ChatText ring buffer base
	const unsigned CHAT_ENTRY_STRIDE = 0x48;     // bytes per ring entry
	const int      CHAT_ENTRY_COUNT  = 30;       // ring wraps at 0x1E
	const unsigned OFF_STR_LOGO      = 0x46;     // entry byte: player idx, 10 = no logo
	const unsigned OFF_STR_FLAGS     = 0x47;     // entry byte: &0x0F channel
	const unsigned OFF_FREE_INDEX    = 0x2A3E;   // word: ring write head
	const unsigned OFF_CHAT_NUM      = 0x2A40;   // word
	const unsigned OFF_CHAT_MODE     = 0x37EFE;  // dword: 1=team, 2=ally, 3=all
	const unsigned OFF_SCREENCHAT    = 0x37F02;  // dword
	const unsigned OFF_TEXTLINES     = 0x37F27;  // dword: max visible lines
	const unsigned OFF_COMIX_FONT    = 0x391F9;  // FontDataStruct*

	// Line left (no logo; a logo shifts the text right) and the top of the
	// first line. These are no longer constants: ChatPosition may relocate
	// the whole list, and the backdrop has to follow it or it detaches from
	// the text. Both return the vanilla 138 / 52 when the feature is
	// unconfigured, so behaviour is unchanged by default.
	inline int ChatTextX() { return ChatPosition::X(); }
	inline int ChatTopY()  { return ChatPosition::Y(); }
	const int BLACK_INDEX = 0;      // TA palette: pure black
	const int PAD_X_LEFT  = 4;
	const int PAD_X_RIGHT = 4;
	const int PAD_Y       = 1;

	const unsigned G_DESKTOP_GUI = 0x0051FBA4;   // holds a POINTER to the desktop GUI

	// Pixel width of one chat line, measured the SAME way the engine draws it.
	// In-game chat is rendered by DrawText (0x4A50E0) with the desktop GAF font,
	// advancing x by each glyph's GAFFrame::Width (chars <= 0x1f don't advance).
	// The COMIX bitmap font (GetTextExtent) is narrower, which left the backdrop
	// ~20% short, so match DrawText exactly and only fall back to COMIX when the
	// GAF font is inactive.
	//
	// DrawText does: EAX = *(int*)0x51FBA4; if (*(int*)(EAX+0x14) != 0) { font
	// object = that; seq = *(int*)(fontObject + 0xc); ... }. Note 0x51FBA4 is a
	// pointer to the GUI, not the GUI itself.
	int MeasureChatLineWidth(const unsigned char* str, unsigned char* comixFont)
	{
		// When the crisp chat font is active the line is rendered with our own
		// TTF atlas, so the backdrop must be sized to that, not the GAF font.
		const int lineH = comixFont ? comixFont[0] : 0;
		if (lineH > 0 && ChatFont::Ensure(lineH))
			return ChatFont::Measure((const char*)str);

		__try
		{
			const int desktopGui = *(int*)G_DESKTOP_GUI;
			const int fontObj = desktopGui ? *(int*)(desktopGui + 0x14) : 0;
			if (fontObj != 0)
			{
				_GAFSequence* seq = *(_GAFSequence**)(fontObj + 0xc);
				if (seq)
				{
					int width = 0;
					for (const unsigned char* p = str; *p; ++p)
					{
						if (*p > 0x1f)
						{
							_GAFFrame* f = Index2Frame_InSequence(seq, *p);
							if (f)
								width += f->Width;
						}
					}
					return width;
				}
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			// Fall through to the COMIX estimate on any bad pointer.
		}
		return GetTextExtent(comixFont, (const char*)str);
	}

	// Mirrors the engine's per-line visibility test in ChatMessageWithLogo.
	bool ChatLineVisible(int mode, int screenchat, int channel)
	{
		switch (mode)
		{
		case 1:  return channel == 2;                                  // team only
		case 2:  return channel != 8;                                  // ally
		case 3:  return screenchat != 0                                // all
		             || channel == 1 || channel == 4 || channel == 8;
		default: return false;
		}
	}

	void FillBlack(_OFFSCREEN* o, int left, int top, int right, int bottom)
	{
		OFFSCREEN* s = (OFFSCREEN*)o;
		if (!s->lpSurface)
			return;

		const int width  = s->ScreenRect.right > 0 ? s->ScreenRect.right : s->lPitch;
		const int height = s->Height;
		if (left   < 0)      left   = 0;
		if (top    < 0)      top    = 0;
		if (right  > width)  right  = width;
		if (bottom > height) bottom = height;
		if (right <= left || bottom <= top)
			return;

		unsigned char* base = (unsigned char*)s->lpSurface;
		const int pitch = s->lPitch;
		for (int y = top; y < bottom; ++y)
			std::memset(base + y * pitch + left, BLACK_INDEX, right - left);
	}
}

void ChatBackdrop::Draw(_OFFSCREEN* offscreen)
{
	if (!offscreen)
		return;

	// Gate on the ctrl-F2 "Chat Text Backdrop" option (default off).
	Dialog* dlg = LocalShare ? (Dialog*)LocalShare->Dialog : nullptr;
	if (!dlg || !dlg->GetChatBackdropEnabled())
		return;

	unsigned char* ta = *(unsigned char**)0x00511de8;
	if (!ta)
		return;

	const int textlines = *(int*)(ta + OFF_TEXTLINES);
	if (textlines <= 0)
		return;

	unsigned char* font = *(unsigned char**)(ta + OFF_COMIX_FONT);
	if (!font)
		return;
	const int lineHeight = font[0];   // FontDataStruct.charHeight
	if (lineHeight <= 0)
		return;

	const int freeIndex = *(unsigned short*)(ta + OFF_FREE_INDEX);
	const int chatNum   = *(unsigned short*)(ta + OFF_CHAT_NUM);
	const int mode      = *(int*)(ta + OFF_CHAT_MODE);
	const int screenchat= *(int*)(ta + OFF_SCREENCHAT);

	// Walk back from the write head by up to (textlines-1) entries to find the
	// oldest line still on screen, stopping if we reach chatNum.
	int idx = freeIndex;
	for (int n = 1; n < textlines; ++n)
	{
		if (idx == chatNum)
			break;
		if (--idx < 0)
			idx = CHAT_ENTRY_COUNT - 1;
	}

	// Iterate forward to the write head, drawing a backdrop behind each
	// visible line. The y cursor only advances for lines that are drawn.
	int y = ChatTopY();
	while (idx != freeIndex)
	{
		const unsigned char* entry = ta + OFF_CHAT_TEXT + idx * CHAT_ENTRY_STRIDE;
		const int channel = entry[OFF_STR_FLAGS] & 0x0F;
		if (ChatLineVisible(mode, screenchat, channel))
		{
			const bool hasLogo = entry[OFF_STR_LOGO] != 10;
			const int  textX   = ChatTextX() + (hasLogo ? lineHeight : 0);
			const int  textW   = MeasureChatLineWidth(entry, font);
			if (textW > 0)
			{
				FillBlack(offscreen,
					ChatTextX() - PAD_X_LEFT, y - PAD_Y,
					textX + textW + PAD_X_RIGHT, y + lineHeight + PAD_Y);
			}
			y += lineHeight;
		}
		if (++idx == CHAT_ENTRY_COUNT)
			idx = 0;
	}
}

// -----------------------------------------------------------------------
// Engine hook: ChatMessageWithLogo (DrawChatText) @ 0x00464060.
//
// The engine renders in-game chat here (from DrawGameScreen @ 0x469fcb and the
// victory screen). It is a __stdcall taking one argument: an OFFSCREEN* render
// context, which the caller passes as a stack local (LEA EDX,[ESP+0x34]; PUSH;
// CALL). We hook the entry, draw the backdrop into that same context, then let
// the original run so the text lands on top.
// -----------------------------------------------------------------------

namespace
{
	const unsigned CHAT_DRAW_ADDR = 0x00464060;

	InlineSingleHook* g_chatDrawHook = nullptr;

	bool LooksLikeOffscreen(const OFFSCREEN* p)
	{
		if (!p)
			return false;
		__try
		{
			if (!p->lpSurface)
				return false;
			if (p->Height <= 0 || p->Height > 8192)
				return false;
			if (p->lPitch <= 0 || p->lPitch > 65536)
				return false;
			if (p->ScreenRect.right <= 0 || p->ScreenRect.right > 8192)
				return false;
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return false;
		}
	}

	// Inline-hook router. For a JMP-lagger hook at a function's first
	// instruction, Esp points at the return address, so the first stdcall
	// argument is at [Esp+4]. Probe that (falling back to [Esp]) and validate
	// before drawing, so an unexpected stack layout skips rather than crashes.
	unsigned int ChatDrawHookProc(PInlineX86StackBuffer buf)
	{
		// Resolve the configured chat anchor against the current screen size
		// and patch the engine's four position constants if they moved. Done
		// here rather than at Install() because the screen size is not known
		// during DLL init, and because this also picks up resolution changes.
		// Early-outs to a couple of integer compares once settled.
		ChatPosition::EnsureApplied();

		OFFSCREEN* off = nullptr;
		__try
		{
			OFFSCREEN* c1 = *(OFFSCREEN**)(buf->Esp + 4);
			if (LooksLikeOffscreen(c1))
				off = c1;
			if (!off)
			{
				OFFSCREEN* c0 = *(OFFSCREEN**)(buf->Esp);
				if (LooksLikeOffscreen(c0))
					off = c0;
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			off = nullptr;
		}

		if (off)
			ChatBackdrop::Draw((_OFFSCREEN*)off);
		return 0;
	}

	// -------------------------------------------------------------------
	// Engine hook: DrawText @ 0x004A50E0 — the crisp chat font.
	//
	// The engine draws every chat line through this __stdcall
	// (gui, str, x, y, maxWidth, flag). We replace ONLY chat lines with our
	// TTF glyph atlas; a chat line is identified by its string pointer lying
	// inside the ChatText ring buffer. All other UI text passes through.
	// This composes with the backdrop (drawn earlier by the 0x464060 hook)
	// and leaves the engine to draw player logos and position everything.
	// -------------------------------------------------------------------
	const unsigned DRAWTEXT_ADDR = 0x004A50E0;
	InlineSingleHook* g_drawTextHook = nullptr;

	// topGUI PaletteRemapTable slots the engine uses for chat colour:
	//   normal    -> [15] at TAdynmem+0xDDA
	//   highlight -> [10] at TAdynmem+0xDD5   (str[0x47] & 0x20)
	const unsigned OFF_REMAP_NORMAL    = 0xDDA;
	const unsigned OFF_REMAP_HIGHLIGHT = 0xDD5;

	bool CrispChatEnabled()
	{
		Dialog* dlg = LocalShare ? (Dialog*)LocalShare->Dialog : nullptr;
		return dlg && dlg->GetChatBackdropEnabled();
	}

	unsigned int DrawTextHookProc(PInlineX86StackBuffer buf)
	{
		__try
		{
			if (!CrispChatEnabled())
				return 0;

			unsigned char* ta = *(unsigned char**)0x00511de8;
			if (!ta)
				return 0;

			// DrawText(gui, str, x, y, maxWidth, flag): x=param_3 at [Esp+0xC],
			// y=param_4 at [Esp+0x10].
			void*          rtnAddr = *(void**)(buf->Esp);
			OFFSCREEN*     off = *(OFFSCREEN**)(buf->Esp + 4);
			unsigned char* str = *(unsigned char**)(buf->Esp + 8);
			const int      x   = *(int*)(buf->Esp + 0xC);
			const int      y   = *(int*)(buf->Esp + 0x10);

			unsigned char* ringBase = ta + OFF_CHAT_TEXT;
			unsigned char* ringEnd  = ringBase + CHAT_ENTRY_COUNT * CHAT_ENTRY_STRIDE;
			if (str < ringBase || str >= ringEnd)
				return 0;   // not a chat line — leave engine text alone

			unsigned char* font = *(unsigned char**)(ta + OFF_COMIX_FONT);
			const int lineH = font ? font[0] : 0;
			if (lineH <= 0 || !ChatFont::Ensure(lineH))
				return 0;   // atlas unavailable — fall back to engine font

			const int colorIndex = (str[OFF_STR_FLAGS] & 0x20)
				? ta[OFF_REMAP_HIGHLIGHT]
				: ta[OFF_REMAP_NORMAL];

			// Anchor our ink top-left to (x,y) — the same spot the engine's
			// chat text and the backdrop box start from.
			ChatFont::DrawString((_OFFSCREEN*)off, (const char*)str, x, y, colorIndex);

			// Suppress the engine's bitmap draw: return straight to DrawText's
			// caller with the stack cleaned as if the 6-arg __stdcall had run.
			buf->Esp = buf->Esp + 4 + 0x18;
			buf->rtnAddr_Pvoid = rtnAddr;
			return X86STRACKBUFFERCHANGE;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
		}
		return 0;
	}
}

void ChatBackdrop::Install()
{
	if (!g_chatDrawHook)
		g_chatDrawHook = new InlineSingleHook(
			CHAT_DRAW_ADDR, 5, INLINE_5BYTESLAGGERJMP, ChatDrawHookProc);
	if (!g_drawTextHook)
		g_drawTextHook = new InlineSingleHook(
			DRAWTEXT_ADDR, 5, INLINE_5BYTESLAGGERJMP, DrawTextHookProc);
}
