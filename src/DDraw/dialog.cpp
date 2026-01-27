#include "config.h"
#include "Dialog.h"

#include "cincome.h"
#include "font.h"
#include "fullscreenminimap.h"
#include "gaf.h"
#include "GUIExpand.h"
#include "hook\hook.h"
#include "iddrawsurface.h"
#include "MegamapControl.h"
#include "whiteboard.h"
#include "pcxread.h"
#include "tahook.h"
#include "tafunctions.h"
#include "Widgets/Button.h"
#include "Widgets/IntegerField.h"
#include "Widgets/Label.h"
#include "Widgets/TextField.h"
#include "Widgets/VirtualKeyField.h"
#include "Widgets/Widget.h"

#include <stdio.h>

#define COL0 16
#define COL0b 22
#define COL0c 74
#define COL1 126
#define COL2 200
#define COL3 310
#define COL4 340

Dialog::Dialog(BOOL Vidmem_a)
{
	lpOKButton = CreateSurfPCXResource(5, Vidmem_a);
	lpStagedButton3 = CreateSurfPCXResource(6, Vidmem_a);
	lpLCFont = CreateSurfPCXResource(7, Vidmem_a);
	lpSmallUCFont = CreateSurfPCXResource(8, Vidmem_a);
	lpSmallLCFont = CreateSurfPCXResource(9, Vidmem_a);
	lpCheckBox = CreateSurfPCXResource(10, Vidmem_a);
	lpStagedButton1 = CreateSurfPCXResource(11, Vidmem_a);
	lpStandardButton = CreateSurfPCXResource(12, Vidmem_a);

	// column 0, 1

#if MAX_MEX_SNAP_RADIUS || MAX_WRECK_SNAP_RADIUS
	m_widgets.push_back(std::make_shared<Label>(COL0, ROW(0), "Snap Override Key"));
	m_widgets.push_back(m_clickSnapOverrideVirtualKeyField = std::make_shared<VirtualKeyField>(COL1, ROW(0), 50, ROW_HEIGHT, VK_MENU, "ClickSnapOverrideKey"));
#endif

#if TA_HOOK_ENABLE
	m_widgets.push_back(std::make_shared<Label>(COL0, ROW(1), "Autoclick Key"));
	m_widgets.push_back(m_autoClickVirtualKeyField = std::make_shared<VirtualKeyField>(COL1, ROW(1), 50, ROW_HEIGHT, 88, "KeyCode"));
#endif

#if USEWHITEBOARD
	m_widgets.push_back(std::make_shared<Label>(COL0, ROW(2), "Whiteboard Key"));
	m_widgets.push_back(m_whiteboardVirtualKeyField = std::make_shared<VirtualKeyField>(COL1, ROW(2), 50, ROW_HEIGHT, VK_OEM_5, "WhiteboardKey"));
#endif

#if USEMEGAMAP
	m_widgets.push_back(std::make_shared<Label>(COL0, ROW(3), "Megamap Key"));
	m_widgets.push_back(m_megaMapVirtualKeyField = std::make_shared<VirtualKeyField>(COL1, ROW(3), 50, ROW_HEIGHT, VK_TAB, "MegamapKey"));
#endif

	m_widgets.push_back(std::make_shared<Label>(COL0, ROW(4), "PATROLLING CONSTRUCTION UNITS"));
#if PATROLING_CONS_RECLAIM_OR_ASSIST_ENABLE
	m_widgets.push_back(std::make_shared<Label>(COL0b, ROW(5), "Hold Pos"));
	m_widgets.push_back(std::make_shared<Label>(COL0b, ROW(6), "Maneuver"));
	m_widgets.push_back(std::make_shared<Label>(COL0b, ROW(7), "Roam"));
	static const std::vector<std::string> patrolButtonLabels({ "Reclaim Only", "Both", "Assist Only" });
	m_widgets.push_back(m_conUnitPatrolHoldPosButton = std::make_shared<Button>(COL0c, ROW(5), lpStagedButton3,
		RECLAIM_ONLY, 3, true, patrolButtonLabels, "ConUnitsPatrolHoldPosOption"));
	m_widgets.push_back(m_conUnitPatrolManeuverButton = std::make_shared<Button>(COL0c, ROW(6), lpStagedButton3,
		RECLAIM_AND_ASSIST, 3, true, patrolButtonLabels, "ConUnitsPatrolManeuverOption"));
	m_widgets.push_back(m_conUnitPatrolRoamButton = std::make_shared<Button>(COL0c, ROW(7), lpStagedButton3,
		RECLAIM_AND_ASSIST, 3, true, patrolButtonLabels, "ConUnitsPatrolRoamOption"));
#else
	m_widgets.push_back(std::make_shared<Label>(COL0b, ROW(5), "Options not available"));
#endif

	m_widgets.push_back(std::make_shared<Label>(COL0, ROW(8), "GUARDING CONSTRUCTION UNITS"));
#if FIXED_POSN_GUARDING_CONS_ENABLE
	m_widgets.push_back(std::make_shared<Label>(COL0b, ROW(9), "Hold Pos"));
	m_widgets.push_back(std::make_shared<Label>(COL0b, ROW(10), "Maneuver"));
	m_widgets.push_back(std::make_shared<Label>(COL0b, ROW(11), "Roam"));
	static const std::vector<std::string> guardButtonLabels({ "Stay", "Cavedog", "Scatter" });
	m_widgets.push_back(m_conUnitGuardHoldPosButton = std::make_shared<Button>(COL0c, ROW(9), lpStagedButton3,
		CAVEDOG, 3, true, guardButtonLabels, "ConUnitsGuardHoldPosOption"));
	m_widgets.push_back(m_conUnitGuardManeuverButton = std::make_shared<Button>(COL0c, ROW(10), lpStagedButton3,
		CAVEDOG, 3, true, guardButtonLabels, "ConUnitsGuardManeuverOption"));
	m_widgets.push_back(m_conUnitGuardRoamButton = std::make_shared<Button>(COL0c, ROW(11), lpStagedButton3,
		CAVEDOG, 3, true, guardButtonLabels, "ConUnitsGuardRoamOption"));
#else
	m_widgets.push_back(std::make_shared<Label>(COL0b, ROW(9), "Options not available"));
#endif

	// column 2, 3, 4
	m_widgets.push_back(std::make_shared<Label>(COL2, ROW(0), "Mex-Snap Radius"));
	m_widgets.push_back(m_mexSnapRadiusIntegerField = std::make_shared<IntegerField>(COL3, ROW(0), 20, ROW_HEIGHT,
		CTAHook::GetDefaultMexSnapRadius(), 0, CTAHook::GetMaxMexSnapRadius(), "MexSnapRadius_"));
	m_mexSnapRadiusIntegerField->m_disabled = CTAHook::GetMaxMexSnapRadius() == 0;
	if (!m_mexSnapRadiusIntegerField->m_disabled)
	{
		m_widgets.push_back(std::make_shared<Label>(COL4, ROW(0), "(0-" + std::to_string(CTAHook::GetMaxMexSnapRadius()) + ")"));
	}

	m_widgets.push_back(std::make_shared<Label>(COL2, ROW(1), "Wreck-Snap Radius"));
	m_widgets.push_back(m_wreckSnapRadiusIntegerField = std::make_shared<IntegerField>(COL3, ROW(1), 20, ROW_HEIGHT,
		CTAHook::GetDefaultWreckSnapRadius(), 0, CTAHook::GetMaxWreckSnapRadius(), "WreckSnapRadius_"));
	m_wreckSnapRadiusIntegerField->m_disabled = CTAHook::GetMaxWreckSnapRadius() == 0;
	if (!m_wreckSnapRadiusIntegerField->m_disabled)
	{
		m_widgets.push_back(std::make_shared<Label>(COL4, ROW(1), "(0-" + std::to_string(CTAHook::GetMaxWreckSnapRadius()) + ")"));
	}

	m_widgets.push_back(std::make_shared<Label>(COL2, ROW(2), "Chat Macro F11"));
	m_widgets.push_back(m_chatMacroTextField = std::make_shared<TextField>(COL2, ROW(3), 170, 5 * ROW_HEIGHT,
		"+setshareenergy 1000\x0d+setsharemetal 1000\x0d+shareall\x0d+shootall", "ShareText"));

	m_widgets.push_back(std::make_shared<Label>(COL2, ROW(8), "Resource Bar Background"));
	m_widgets.push_back(m_resourceBarBackgroundButton = std::make_shared<Button>(COL2, ROW(9), lpStagedButton3,
		1, 3, true, std::vector<std::string>({ "None", "Text", "Solid" }), "BackGround", std::function<void(int)>()));

#if TA_HOOK_ENABLE
	m_widgets.push_back(m_optimiseDtRowsButton = std::make_shared<Button>(COL2, ROW(10), lpCheckBox,
		1, 2, false, std::vector<std::string>(), "OptimizeDT", std::function<void(int)>()));
	m_widgets.push_back(std::make_shared<Label>(COL2 + m_optimiseDtRowsButton->m_width + 4, ROW(10), "Optimize DT Rows"));

	m_widgets.push_back(m_enableFullRingsButton = std::make_shared<Button>(COL2, ROW(11), lpCheckBox,
		1, 2, false, std::vector<std::string>(), "FullRings", std::function<void(int)>()));
	m_widgets.push_back(std::make_shared<Label>(COL2 + m_enableFullRingsButton->m_width + 4, ROW(11), "Enable FullRings"));
#endif

	m_widgets.push_back(m_vsyncButton = std::make_shared<Button>(COL2, ROW(12), lpCheckBox,
		0, 2, false, std::vector<std::string>(), "VSync"));
	m_widgets.push_back(std::make_shared<Label>(COL2 + m_vsyncButton->m_width + 4, ROW(12), "VSync"));

	m_widgets.push_back(m_okButton = std::make_shared<Button>(COL3-16, ROW(12), lpOKButton,
		0, 1, true, std::vector<std::string>(), "",
		[this](int)
	{
		SetAll();
		HideDialog();
	}));

	lpCursor= NULL;
	CursorBackground= -1;
	CursorPosX = -1;
	CursorPosY = -1;
	Move = false;

	LocalShare->Dialog = this;
	posX = 0;
	posY = 0;
	FirstBlit = true;
	DialogVisible = false;

	VidMem= Vidmem_a;
	ReadPos();
	ReadSettings();
	
	LPDIRECTDRAW TADD = (IDirectDraw*)LocalShare->TADirectDraw;

	DDSURFACEDESC ddsd;
	DDRAW_INIT_STRUCT(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;

	if (m_vsyncButton->GetState())
	{
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
	}
	else	
	{
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;
	}
	
	ddsd.dwWidth = DialogWidth;
	ddsd.dwHeight = DialogHeight;
	TADD->CreateSurface(&ddsd, &lpDialogSurf, NULL);

	lpBackground = CreateSurfPCXResource(2, VidMem);
	lpUCFont = CreateSurfPCXResource(3, VidMem);
	//RestoreCursor ( );
	PGAFSequence CursorSequence= (*TAmainStruct_PtrPtr)->cursor_ary[cursornormal];
	if (NULL!=CursorSequence)
	{
		PGAFFrame GafFrame= CursorSequence->PtrFrameAry[0].PtrFrame;
		lpCursor= CreateSurfByGafFrame ( (LPDIRECTDRAW)LocalShare->TADirectDraw, GafFrame, VidMem);
		CursorBackground= GafFrame->Background;
	}
// 	else
// 	{
// 		lpCursor=  CreateSurfPCXResource(4, VidMem);
// 	}
	
	EnterOption_hook= new InlineSingleHook ( EnterOption_Address, 5, INLINE_5BYTESLAGGERJMP, EnterOption);
	PressInOption_hook= new InlineSingleHook ( PressInOption_Address, 5, INLINE_5BYTESLAGGERJMP, PressInOption);
	
	IDDrawSurface::OutptTxt ( "New Dialog");
}

Dialog::~Dialog()
{
	if(lpDialogSurf)
		lpDialogSurf->Release();
	if(lpBackground)
		lpBackground->Release();
	if(lpUCFont)
		lpUCFont->Release();
	if(lpCursor)
		lpCursor->Release();
	if(lpOKButton)
		lpOKButton->Release();
	if(lpStagedButton3)
		lpStagedButton3->Release();
	if(lpLCFont)
		lpLCFont->Release();
	if(lpSmallUCFont)
		lpSmallUCFont->Release();
	if(lpSmallLCFont)
		lpSmallLCFont->Release();
	if(lpCheckBox)
		lpCheckBox->Release();
	if(lpStagedButton1)
		lpStagedButton1->Release();
	if(lpStandardButton)
		lpStandardButton->Release();
	if (EnterOption_hook)
	{
		delete EnterOption_hook;
	}
	if (PressInOption_hook)
	{
		delete PressInOption_hook;
	}
	
	WritePos();
	WriteSettings();
	LocalShare->Dialog = NULL;
}

void Dialog::ShowDialog()
{
	for (auto w : m_widgets)
	{
		w->m_focused = false;
	}

    posX = 1024 - DialogWidth;
    posY = 30;
	CorrectPos(); //make sure dialog is inside screen
	RestoreAll ( );
	RenderDialog();
	DialogVisible = true;
}

void Dialog::HideDialog()
{
	DialogVisible = false;
	CursorPosX = -1;
	CursorPosY = -1;

	WritePos();
	WriteSettings();
}

void Dialog::BlitDialog(LPDIRECTDRAWSURFACE DestSurf)
{
	if(FirstBlit)
	{
		SetAll();
		FirstBlit = false;
	}
	if (DialogVisible)
	{
		if(lpDialogSurf->IsLost() != DD_OK)
		{
			RestoreAll();
		}

		if(!DialogVisible)
			return;

		RECT Dest;
		Dest.left = posX;
		Dest.top = posY;
		Dest.right = posX + DialogWidth;
		Dest.bottom = posY + DialogHeight;

		if(DestSurf->Blt(&Dest, lpDialogSurf, NULL, DDBLT_ASYNC, NULL)!=DD_OK)
		{
			DestSurf->Blt(&Dest, lpDialogSurf, NULL, DDBLT_WAIT, NULL);
		}

		if(CursorPosX!=-1 && CursorPosY!=-1)
		{
			BlitCursor(DestSurf, CursorPosX, CursorPosY);
		}
	}
}

void Dialog::RestoreCursor ()
{
	if (NULL!=lpCursor)
	{
		if (DD_OK!=lpCursor->IsLost ( ))
		{
			lpCursor->Restore ( );
		}

		PGAFSequence CursorSequence= (*TAmainStruct_PtrPtr)->cursor_ary[cursornormal];

		if (NULL!=CursorSequence)
		{
			PGAFFrame GafFrame= CursorSequence->PtrFrameAry[0].PtrFrame;

			DDSURFACEDESC ddsd;
			DDRAW_INIT_STRUCT(ddsd);

			lpCursor->Lock(NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL);

			unsigned char *SurfPTR = (unsigned char*)ddsd.lpSurface;
			CursorBackground= GafFrame->Background;
			POINT Aspect= { ddsd.lPitch, ddsd.dwHeight};
			memset ( SurfPTR, CursorBackground, ddsd.lPitch* ddsd.dwHeight );
			CopyGafToBits ( SurfPTR, &Aspect, 0, 0, GafFrame);

			lpCursor->Unlock ( NULL);
		}
		else
		{
			RestoreFromPCX(4, lpCursor);
		}
	}
	else
	{
		PGAFSequence CursorSequence= (*TAmainStruct_PtrPtr)->cursor_ary[cursornormal];

		if (NULL!=CursorSequence)
		{
			PGAFFrame GafFrame= CursorSequence->PtrFrameAry[0].PtrFrame;
			lpCursor= CreateSurfByGafFrame ( (LPDIRECTDRAW)LocalShare->TADirectDraw, GafFrame, VidMem);
			CursorBackground= GafFrame->Background;
		}
	}
}

void Dialog::RestoreAll()
{
	lpDialogSurf->Restore();
	lpBackground->Restore();

	lpUCFont->Restore();
	lpLCFont->Restore();
	lpSmallUCFont->Restore();
	lpSmallLCFont->Restore();
	lpOKButton->Restore();
	lpStagedButton3->Restore();
	lpCheckBox->Restore();
	lpStagedButton1->Restore();
	lpStandardButton->Restore();

	RestoreCursor ();
	RestoreFromPCX(2, lpBackground);
	RestoreFromPCX(3, lpUCFont);

	RestoreFromPCX(5, lpOKButton);
	RestoreFromPCX(6, lpStagedButton3);
	RestoreFromPCX(7, lpLCFont);
	RestoreFromPCX(8, lpSmallUCFont);
	RestoreFromPCX(9, lpSmallLCFont);
	RestoreFromPCX(10, lpCheckBox);
	RestoreFromPCX(11, lpStagedButton1);
	RestoreFromPCX(12, lpStandardButton);

	RenderDialog();
}

bool Dialog::Message(HWND WinProchWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	__try
	{
		return _Message(WinProchWnd, Msg, wParam, lParam);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		;// return LocalShare->TAWndProc(WinProcWnd, Msg, wParam, lParam);
	}
	return false;
}

bool Dialog::_Message(HWND WinProchWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
		if(!DialogVisible)
		{
            if (DataShare->TAProgress == TAInGame &&
                Msg == WM_KEYDOWN && wParam == 113 && (GetAsyncKeyState(17) & 0x8000) > 0 /*ctrl*/) {
                ShowDialog();
                return true;
            }
			return false;
		}

		if (Msg == WM_LBUTTONDOWN)
		{
			int x = LOWORD(lParam) - posX;
			int y = HIWORD(lParam) - posY;
			for (auto w : m_widgets)
			{
				bool isInside = !w->m_hidden && !w->m_disabled && w->IsInside(x, y);
				if (isInside != w->m_focused)
				{
					w->m_focused = isInside;
					w->Draw(this);
				}
			}
		}

		for (auto w : m_widgets)
		{
			if (w->Message(this, WinProchWnd, Msg, wParam, lParam))
			{
				w->Draw(this);
				return true;
			}
		}

		switch(Msg)
		{
		case WM_LBUTTONDBLCLK:
			if(LOWORD(lParam)>posX && LOWORD(lParam)<(posX+DialogWidth) && HIWORD(lParam)>posY && HIWORD(lParam)<(posY+DialogHeight))
			{
				return true;
			}
			break;

		case WM_LBUTTONDOWN:
			if(LOWORD(lParam)>posX && LOWORD(lParam)<(posX+DialogWidth) && HIWORD(lParam)>posY && HIWORD(lParam)<(posY+DialogHeight))
			{
				Move = true;
				return true;
			}
			break;

		case WM_LBUTTONUP:
			Move = false;
			break;

		case WM_MOUSEMOVE:
			if(LOWORD(lParam)>=(posX-10) && LOWORD(lParam)<(posX+DialogWidth) && HIWORD(lParam)>=(posY-20) && HIWORD(lParam)<(posY+DialogHeight))
			{
				CursorPosX = LOWORD(lParam);
				CursorPosY = HIWORD(lParam);

#if USEMEGAMAP
				if ((GUIExpander)
					&&(GUIExpander->myMinimap)
					&&(GUIExpander->myMinimap->Controler))
				{
					GUIExpander->myMinimap->Controler->PubCursorX= -1;
					GUIExpander->myMinimap->Controler->PubCursorY= -1;
				}
#endif
			}
			else
			{
				CursorPosX = -1;
				CursorPosY = -1;
			}
			if(Move)
			{
				posX += LOWORD(lParam)-posXPrev;
				posY += HIWORD(lParam)-posYPrev;
                CorrectPos();
				posXPrev = LOWORD(lParam);
				posYPrev = HIWORD(lParam);
				return true;
			}
			
			posXPrev = LOWORD(lParam);
			posYPrev = HIWORD(lParam);
			if(LOWORD(lParam)>=posX && LOWORD(lParam)<(posX+DialogWidth) && HIWORD(lParam)>=posY && HIWORD(lParam)<(posY+DialogHeight))
			{
				return true;
			}
			break;
		case WM_KEYDOWN:
			break;
		case WM_CHAR:
			break;
		}

	return false;//mesage not handled by this dialog
}

void Dialog::RenderDialog()
{
	if(lpDialogSurf->Blt(NULL, lpBackground, NULL, DDBLT_ASYNC, NULL)!=DD_OK)
	{
		lpDialogSurf->Blt(NULL, lpBackground, NULL, DDBLT_WAIT , NULL);
	}

	for (auto w : m_widgets)
	{
		w->Draw(this);
	}
}

int Dialog::DrawTextField(int posX, int posY, int width, int height, const std::string& text, char color)
{
	DDSURFACEDESC ddsd;
	DDRAW_INIT_STRUCT(ddsd);
	if (lpDialogSurf->Lock(NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL) == DD_OK)
	{
		SurfaceMemory = ddsd.lpSurface;
		lPitch = ddsd.lPitch;

		FillRect(posX, posY, posX + width, posY + height, 0);
		posY += 7;

		int CharsPerLine = (width - 4) / 8;
		char Line[100];
		Line[0] = '\0';
		char LineNum = 0;
		char LinePos = 0;
		size_t i;
		bool WasLineBreak = false;
		for (i = 0; i < text.size(); i++)
		{
			if (LinePos > CharsPerLine)
			{
				DrawTinyText(Line, static_cast<int>(posX + 2), static_cast<int>(posY + LineNum * 9), color);
				LineNum++;
				LinePos = 0;
				Line[0] = ' ';
				Line[1] = '\0';
				LinePos++;
				i--;
				WasLineBreak = true;
			}
			else
			{
				if (text[i] != 13/*enter*/)
				{
					Line[LinePos] = text[i];
					Line[LinePos + 1] = '\0';
					LinePos++;
					WasLineBreak = false;
				}
				else
				{
					DrawTinyText(Line, static_cast<int>(posX + 2), static_cast<int>(posY + LineNum * 9), color);
					if (!WasLineBreak)
						LineNum++;
					LinePos = 0;
					Line[0] = '\0';
					WasLineBreak = false;
				}
			}
		}
		DrawTinyText(Line, static_cast<int>(posX + 2), static_cast<int>(posY + LineNum * 9), color);
		lpDialogSurf->Unlock(NULL);
		return LineNum + (LinePos == CharsPerLine);
	}
	return 0;
}

void Dialog::DrawText(LPDIRECTDRAWSURFACE DestSurf, int x, int y, const char *Text)
{
	RECT Dest;
	Dest.left = x;
	Dest.top = y;
	Dest.bottom = Dest.top + 14;
	RECT Source;
	Source.left = 0;
	Source.top = 0;
	Source.bottom = 14;
	DDBLTFX ddbltfx;
	DDRAW_INIT_STRUCT(ddbltfx);
	ddbltfx.ddckSrcColorkey.dwColorSpaceLowValue = 102;
	ddbltfx.ddckSrcColorkey.dwColorSpaceHighValue = 102;

	for(size_t i=0; i<strlen(Text); i++)
	{
		if(Text[i]<91 && Text[i]>=33) //upper case or special character
		{
			Dest.right = Dest.left + FontOffsetUC[Text[i]-33][0];
			Source.left = FontOffsetUC[Text[i]-33][1];
			Source.right = Source.left + FontOffsetUC[Text[i]-33][0];
			if(DestSurf->Blt(&Dest, lpUCFont, &Source, DDBLT_ASYNC | DDBLT_KEYSRCOVERRIDE, &ddbltfx)!=DD_OK)
			{
				DestSurf->Blt(&Dest, lpUCFont, &Source, DDBLT_WAIT | DDBLT_KEYSRCOVERRIDE, &ddbltfx);
			}
			Dest.left += FontOffsetUC[Text[i]-33][0];
		}
		else if(Text[i]<123 && Text[i]>=97)
		{
			Dest.right = Dest.left + FontOffsetLC[Text[i]-97][0];
			Source.left = FontOffsetLC[Text[i]-97][1];
			Source.right = Source.left + FontOffsetLC[Text[i]-97][0];
			if(DestSurf->Blt(&Dest, lpLCFont, &Source, DDBLT_ASYNC | DDBLT_KEYSRCOVERRIDE, &ddbltfx)!=DD_OK)
			{
				DestSurf->Blt(&Dest, lpLCFont, &Source, DDBLT_WAIT | DDBLT_KEYSRCOVERRIDE, &ddbltfx);
			}
			Dest.left += FontOffsetLC[Text[i]-97][0];
		}
		if(Text[i] == ' ')
			Dest.left += 7;
	}
}

void Dialog::DrawSmallText(LPDIRECTDRAWSURFACE DestSurf, int x, int y, const char *Text)
{
	RECT Dest;
	Dest.left = x;
	Dest.top = y;
	Dest.bottom = Dest.top + 12;
	RECT Source;
	Source.left = 0;
	Source.top = 0;
	Source.bottom = 12;
	DDBLTFX ddbltfx;
	DDRAW_INIT_STRUCT(ddbltfx);
	ddbltfx.ddckSrcColorkey.dwColorSpaceLowValue = 102;
	ddbltfx.ddckSrcColorkey.dwColorSpaceHighValue = 102;

	for(size_t i=0; i<strlen(Text); i++)
	{
		if(Text[i]<91 && Text[i]>=33) //upper case
		{
			Dest.right = Dest.left + SmallFontOffsetUC[Text[i]-33][0];
			Source.left = SmallFontOffsetUC[Text[i]-33][1];
			Source.right = Source.left + SmallFontOffsetUC[Text[i]-33][0];
			if(DestSurf->Blt(&Dest, lpSmallUCFont, &Source, DDBLT_ASYNC | DDBLT_KEYSRCOVERRIDE, &ddbltfx)!=DD_OK)
			{
				DestSurf->Blt(&Dest, lpSmallUCFont, &Source, DDBLT_WAIT | DDBLT_KEYSRCOVERRIDE, &ddbltfx);
			}
			Dest.left += SmallFontOffsetUC[Text[i]-33][0];
		}
		else if(Text[i]<123 && Text[i]>=97)
		{
			Dest.right = Dest.left + SmallFontOffsetLC[Text[i]-97][0];
			Source.left = SmallFontOffsetLC[Text[i]-97][1];
			Source.right = Source.left + SmallFontOffsetLC[Text[i]-97][0];
			if(DestSurf->Blt(&Dest, lpSmallLCFont, &Source, DDBLT_ASYNC | DDBLT_KEYSRCOVERRIDE, &ddbltfx)!=DD_OK)
			{
				DestSurf->Blt(&Dest, lpSmallLCFont, &Source, DDBLT_WAIT | DDBLT_KEYSRCOVERRIDE, &ddbltfx);
			}
			Dest.left += SmallFontOffsetLC[Text[i]-97][0];
		}
		if(Text[i] == ' ')
			Dest.left += 6;
	}
}

int Dialog::GetClickSnapOverrideKey()
{
	return m_clickSnapOverrideVirtualKeyField->m_vk;
}

void Dialog::SetAll()
{
	CIncome *Income = (CIncome*)LocalShare->Income;
	if (Income)
	{
		Income->Set(m_resourceBarBackgroundButton->GetState());
	}

	IDDrawSurface *SurfClass = (IDDrawSurface*)LocalShare->DDrawSurfClass;
	if (SurfClass)
	{
		SurfClass->Set (m_vsyncButton->GetState());
	}
	
#if TA_HOOK_ENABLE
	CTAHook *TAHook = (CTAHook*)LocalShare->TAHook;
	if (TAHook)
	{
		TAHook->Set(
			m_autoClickVirtualKeyField ? m_autoClickVirtualKeyField->m_vk : 0,
			m_chatMacroTextField ? m_chatMacroTextField->m_text.c_str() : "",
			m_optimiseDtRowsButton ? m_optimiseDtRowsButton->GetState() : 1,
			m_enableFullRingsButton ? m_enableFullRingsButton->GetState() : 1,
			10,
			m_mexSnapRadiusIntegerField ? m_mexSnapRadiusIntegerField->m_value : 0,
			m_wreckSnapRadiusIntegerField ? m_wreckSnapRadiusIntegerField->m_value : 0,
			m_clickSnapOverrideVirtualKeyField ? m_clickSnapOverrideVirtualKeyField->m_vk : 0
		);
	}
#endif

#if USEWHITEBOARD
	AlliesWhiteboard *WB = (AlliesWhiteboard*)LocalShare->Whiteboard;
	if (WB)
	{
		WB->Set(m_whiteboardVirtualKeyField->m_vk);
	}
#endif
	
#if USEMEGAMAP
	if (GUIExpander
		&&GUIExpander->myMinimap)
	{
		GUIExpander->myMinimap->Set (m_megaMapVirtualKeyField->m_vk);
	}
#endif

}

int Dialog::GetConUnitPatrolHoldPosOption()
{
	return m_conUnitPatrolHoldPosButton
		? m_conUnitPatrolHoldPosButton->GetState()
		: RECLAIM_AND_ASSIST;
}

int Dialog::GetConUnitPatrolManeuverOption()
{
	return m_conUnitPatrolManeuverButton
		? m_conUnitPatrolManeuverButton->GetState()
		: RECLAIM_AND_ASSIST;
}

int Dialog::GetConUnitPatrolRoamOption()
{
	return m_conUnitPatrolRoamButton
		? m_conUnitPatrolRoamButton->GetState()
		: RECLAIM_AND_ASSIST;
}

int Dialog::GetConUnitPatrolOption(int unitMovementSetting)
{
	switch (unitMovementSetting)
	{
	case 0: return GetConUnitPatrolHoldPosOption();
	case 1: return GetConUnitPatrolManeuverOption();
	case 2: return GetConUnitPatrolRoamOption();
	default: return RECLAIM_AND_ASSIST;
	};
}

int Dialog::GetConUnitGuardHoldPosOption()
{
	return m_conUnitGuardHoldPosButton
		? m_conUnitGuardHoldPosButton->GetState()
		: CAVEDOG;
}

int Dialog::GetConUnitGuardManeuverOption()
{
	return m_conUnitGuardManeuverButton
		? m_conUnitGuardManeuverButton->GetState()
		: CAVEDOG;
}

int Dialog::GetConUnitGuardRoamOption()
{
	return m_conUnitGuardRoamButton
		? m_conUnitGuardRoamButton->GetState()
		: CAVEDOG;
}

int Dialog::GetConUnitGuardOption(int unitMovementSetting)
{
	switch (unitMovementSetting)
	{
	case 0: return GetConUnitGuardHoldPosOption();
	case 1: return GetConUnitGuardManeuverOption();
	case 2: return GetConUnitGuardRoamOption();
	default: return RECLAIM_AND_ASSIST;
	};
}

//reads dialog position from registry
void Dialog::ReadPos()
{
	HKEY hKey;
	DWORD dwDisposition;
	DWORD Size = sizeof(int);

	std::string SubKey = CompanyName_CCSTR;
	SubKey += "\\Eye";

	RegCreateKeyEx(HKEY_CURRENT_USER, SubKey.c_str(), NULL, TADRCONFIGREGNAME, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition);

	RegQueryValueEx(hKey, "DialogPosX", NULL, NULL, (unsigned char*)&posX, &Size);
	RegQueryValueEx(hKey, "DialogPosY", NULL, NULL, (unsigned char*)&posY, &Size);

	RegCloseKey(hKey);
}

void Dialog::WriteSettings()
{
	HKEY hKey;
	HKEY hKey1;
	DWORD dwDisposition;

	RegCreateKeyEx(HKEY_CURRENT_USER, CompanyName_CCSTR, NULL, TADRCONFIGREGNAME, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey1, &dwDisposition);
	RegCreateKeyEx(hKey1, "Eye", NULL, TADRCONFIGREGNAME, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition);

	for (auto w : m_widgets)
	{
		w->RegistryWrite(hKey);
	}

	RegCloseKey(hKey);
	RegCloseKey(hKey1);
}

void Dialog::ReadSettings()
{
	HKEY hKey;
	DWORD dwDisposition;

	std::string SubKey = CompanyName_CCSTR;
	SubKey += "\\Eye";

	RegCreateKeyEx(HKEY_CURRENT_USER, SubKey.c_str(), NULL, TADRCONFIGREGNAME, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition);

	for (auto w : m_widgets)
	{
		if (!w->m_disabled)
		{
			w->RegistryRead(hKey);
		}
	}

	RegCloseKey(hKey);
}

void Dialog::WritePos()
{
	HKEY hKey;
	HKEY hKey1;
	DWORD dwDisposition;

	RegCreateKeyEx(HKEY_CURRENT_USER, CompanyName_CCSTR, NULL, TADRCONFIGREGNAME, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey1, &dwDisposition);

	RegCreateKeyEx(hKey1, "Eye", NULL, TADRCONFIGREGNAME, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition);

	RegSetValueEx(hKey, "DialogPosX", NULL, REG_DWORD, (unsigned char*)&posX, sizeof(int));
	RegSetValueEx(hKey, "DialogPosY", NULL, REG_DWORD, (unsigned char*)&posY, sizeof(int));

	RegCloseKey(hKey);
	RegCloseKey(hKey1);
}

void Dialog::CorrectPos()
{
    RECT bounds;
    if (DataShare->TAProgress == TAInGame) {
        std::memcpy(&bounds, &(*TAmainStruct_PtrPtr)->GameSreen_Rect, sizeof(bounds));
    }
    else {
        bounds.left = bounds.top = 0;
        bounds.right = (*TAProgramStruct_PtrPtr)->ScreenWidth;
        bounds.bottom = (*TAProgramStruct_PtrPtr)->ScreenHeight;
    }

    if(posX < bounds.left)
		posX = bounds.left;
	if(posX > 1+ bounds.right - DialogWidth)
		posX = 1+ bounds.right - DialogWidth;

	if(posY < bounds.top)
		posY = bounds.top;
	if(posY > 1+ bounds.bottom - DialogHeight)
		posY = 1+ bounds.bottom - DialogHeight;
}

void Dialog::DrawTinyText(char *String, int posx, int posy, char Color)
{
	if(SurfaceMemory == NULL)
		return;

	char *SurfMem = (char*)SurfaceMemory;

	for(size_t i=0; i<strlen(String); i++)
	{
		for(int j=0; j<8; j++)
		{
			for(int k=0; k<8; k++)
			{
				bool b = 0!=(ThinFont[String[i]*8+j] & (1 << k));
				if(b)
					SurfMem[(posx+(i*8)+(7-k))+(posy+j)*lPitch] = Color;
			}
		}
	}
}

void Dialog::FillRect(int x, int y, int x2, int y2, char Color)
{
	if(SurfaceMemory == NULL)
		return;

	char *SurfMem = (char*)SurfaceMemory;

	for(int i=y; i<y2; i++)
	{
		memset(&SurfMem[x+i*lPitch], Color, x2-x);
	}
}

void Dialog::DrawTexture(int x, int y, int width, int height, LPDIRECTDRAWSURFACE texture, int texturePosX, int texturePosY)
{
	RECT Dest;
	RECT Source;

	Dest.left = x;
	Dest.top = y;
	Dest.right = x + width;
	Dest.bottom = y + height;
	Source.left = texturePosX;
	Source.top = texturePosY;
	Source.right = Source.left + width;
	Source.bottom = Source.top + height;
	if (lpDialogSurf->Blt(&Dest, texture, &Source, DDBLT_ASYNC, NULL) != DD_OK)
	{
		lpDialogSurf->Blt(&Dest, texture, &Source, DDBLT_WAIT, NULL);
	}
}

void Dialog::BlitCursor(LPDIRECTDRAWSURFACE DestSurf, int x, int y)
{
	if (NULL==lpCursor
		||lpCursor->IsLost ( ))
	{
		RestoreCursor ( );
	}
	//blit cursor
	DDSURFACEDESC ddsc;
	DDRAW_INIT_STRUCT(ddsc);

	DDBLTFX ddbltfx;
	DDRAW_INIT_STRUCT(ddbltfx);


	lpCursor->GetSurfaceDesc ( &ddsc);
// 0xc0dcc0
	ddbltfx.ddckSrcColorkey.dwColorSpaceLowValue = CursorBackground& 0xffff;
	ddbltfx.ddckSrcColorkey.dwColorSpaceHighValue = CursorBackground>> 16;

	RECT Dest;
	Dest.left = x;
	Dest.top = y;
	Dest.right = x + ddsc.dwWidth;
	Dest.bottom = y + ddsc.dwHeight;
	RECT Src;
	Src.left= 0;
	Src.top= 0;
	Src.right= ddsc.dwWidth;
	Src.bottom= ddsc.dwHeight;
	if(DestSurf->Blt(&Dest, lpCursor, &Src, DDBLT_ASYNC | DDBLT_KEYSRCOVERRIDE, &ddbltfx)!=DD_OK)
	{
		DestSurf->Blt(&Dest, lpCursor, &Src, DDBLT_WAIT | DDBLT_KEYSRCOVERRIDE, &ddbltfx);
	}
}

bool Dialog::IsShow (LPRECT rect_p)
{
	if (NULL!=rect_p)
	{
		rect_p->left = posX;
		rect_p->top = posY;
		rect_p->right = posX + DialogWidth;
		rect_p->bottom = posY + DialogHeight;
	}
	return DialogVisible;
}
int __stdcall EnterOption (PInlineX86StackBuffer X86StrackBuffer)
{
    ((Dialog*)LocalShare->Dialog)->ShowDialog();
	return 0;
}

int __stdcall PressInOption (PInlineX86StackBuffer X86StrackBuffer)
{
    ((Dialog*)LocalShare->Dialog)->SetAll();
    ((Dialog*)LocalShare->Dialog)->HideDialog();
	return 0;
}
