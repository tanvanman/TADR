#include "oddraw.h"
#include "iddrawsurface.h"

#include "dialog.h"
#include "tahook.h"
#include "tamem.h"
#include "tafunctions.h"
#include "taHPI.h"
#include "gaf.h"

#include "pcxread.h"
#include "font.h"
#include <stdio.h>
#include "whiteboard.h"
#include "cincome.h"
#include "hook\etc.h"
#include "hook\hook.h"

#include "fullscreenminimap.h"
#include "GUIExpand.h"

#include "MegamapControl.h"

#ifdef min
  #undef min
#endif
#include <algorithm>
#include <vector>

#include "UnicodeSupport.h"
#include "TAConfig.h"


Dialog::Dialog(BOOL Vidmem_a)
{
	lpCursor= NULL;
	CursorBackground= -1;
	CursorPosX = -1;
	CursorPosY = -1;
	Move = false;
	StagedButton3State = 0;
	StagedButton3Pushed= false;

	VSyncPushed= false;
	ShareText[0]= 0;
	AutoClickDelayText[0] = 0;
	ClickSnapRadiusText[0] = 0;

	LocalShare->Dialog = this;
	posX = 0;
	posY = 0;
	ShareHeight = 0;
	First = true;
	DialogVisible = false;
	SetVisiblePushed = false;

	VidMem= Vidmem_a;
	ReadPos();
	ReadSettings();
	
	LPDIRECTDRAW TADD = (IDirectDraw*)LocalShare->TADirectDraw;

	DDSURFACEDESC ddsd;
	DDRAW_INIT_STRUCT(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;

	if (VSyncEnabled)
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
	
	lpOKButton = CreateSurfPCXResource(5, VidMem);
	lpStagedButton3 = CreateSurfPCXResource(6, VidMem);
	lpLCFont = CreateSurfPCXResource(7, VidMem);
	lpSmallUCFont = CreateSurfPCXResource(8, VidMem);
	lpSmallLCFont = CreateSurfPCXResource(9, VidMem);
	lpCheckBox = CreateSurfPCXResource(10, VidMem);
	lpStagedButton1 = CreateSurfPCXResource(11, VidMem);
	lpStandardButton = CreateSurfPCXResource(12, VidMem);


	
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
	WhiteboardKeyFocus= false;
	ClickSnapOverrideKeyFocus= false;
	KeyCodeFocus= false;
	ShareBoxFocus= false;
	MegmapFocus= false;

    posX = 1024 - DialogWidth;
    posY = 30;
	CorrectPos(); //make sure dialog is inside screen

	OKButtonPushed = false;
	StartedIn = None;

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
	if(First)
	{
		SetAll();
		First = false;
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
		if(!DialogVisible)
		{
            if (DataShare->TAProgress == TAInGame &&
                Msg == WM_KEYDOWN && wParam == 113 && (GetAsyncKeyState(17) & 0x8000) > 0 /*ctrl*/) {
                ShowDialog();
                return true;
            }
			return false;
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
			if(KeyCodeFocus)
			{
				KeyCodeFocus = false;
				RenderDialog();
			}
			if(WhiteboardKeyFocus)
			{
				WhiteboardKeyFocus = false;
				RenderDialog();
			}
			if (ClickSnapOverrideKeyFocus)
			{
				ClickSnapOverrideKeyFocus = false;
				RenderDialog();
			}
			if(ShareBoxFocus)
			{
				ShareBoxFocus = false;
				RenderDialog();
			}
			if(AutoClickDelayFocus)
			{
				AutoClickDelayFocus = false;
				RenderDialog();
			}
			if (ClickSnapRadiusFocus)
			{
				ClickSnapRadiusFocus = false;
				RenderDialog();
			}
			if (MegmapFocus)
			{
				MegmapFocus= false;
				RenderDialog();
			}
			if(LOWORD(lParam)>posX && LOWORD(lParam)<(posX+DialogWidth) && HIWORD(lParam)>posY && HIWORD(lParam)<(posY+DialogHeight))
			{
				if(Inside(LOWORD(lParam), HIWORD(lParam), OKButton))
				{
					OKButtonPushed = true;
					StartedIn = OKButton;
					RenderDialog();
				}
				else if(Inside(LOWORD(lParam), HIWORD(lParam), StagedButton3))
				{
					StagedButton3Pushed = true;
					StartedIn = StagedButton3;
					RenderDialog();
				}
				else if(Inside(LOWORD(lParam), HIWORD(lParam), SetVisible))
				{
					SetVisiblePushed = true;
					StartedIn = SetVisible;
					RenderDialog();
				}
				else if(Inside(LOWORD(lParam), HIWORD(lParam), VSync))
				{
					VSyncPushed = true;
					StartedIn = VSync;
					RenderDialog();
				}
				else if(Inside(LOWORD(lParam), HIWORD(lParam), KeyCode))
				{
					KeyCodeFocus = true;
					RenderDialog();
				}
				else if(Inside(LOWORD(lParam), HIWORD(lParam), WhiteboardKey))
				{
					WhiteboardKeyFocus = true;
					RenderDialog();
				}
				else if (Inside(LOWORD(lParam), HIWORD(lParam), ClickSnapOverrideKeyId))
				{
					ClickSnapOverrideKeyFocus = true;
					RenderDialog();
				}
				else if(Inside(LOWORD(lParam), HIWORD(lParam), ShareBox))
				{
					ShareBoxFocus = true;
					RenderDialog();
				}
				/*else if(Inside(LOWORD(lParam), HIWORD(lParam), AutoClickDelay))
				{
				AutoClickDelayFocus = true;
				RenderDialog();
				}*/
				else if (Inside(LOWORD(lParam), HIWORD(lParam), ClickSnapRadiusId))
				{
					ClickSnapRadiusFocus = true;
					RenderDialog();
				}
				else if(Inside(LOWORD(lParam), HIWORD(lParam), OptimizeDT))
				{
					StartedIn = OptimizeDT;
				}
				else if(Inside(LOWORD(lParam), HIWORD(lParam), FullRings))
				{
					StartedIn = FullRings;
				}				
				else if(Inside(LOWORD(lParam), HIWORD(lParam), MegaMapKey))
				{
					MegmapFocus = true;
					RenderDialog();
				}
				else  //only move if outside button
				{
					StartedIn = None;
					Move = true;
				}
				return true;
			}
			break;

		case WM_LBUTTONUP:
			if(Inside(LOWORD(lParam), HIWORD(lParam), OKButton) && StartedIn==OKButton)
			{
				SetAll();
				HideDialog();
			}
			else if(Inside(LOWORD(lParam), HIWORD(lParam), StagedButton3) && StartedIn==StagedButton3)
			{
				StagedButton3State += 1;
				StagedButton3State = StagedButton3State%3;
				StagedButton3Pushed = false;
				RenderDialog();
			}
			else if(Inside(LOWORD(lParam), HIWORD(lParam), SetVisible) && StartedIn==SetVisible)
			{
				SetVisibleList();
				SetVisiblePushed = false;
				RenderDialog();
			}
			else if(Inside(LOWORD(lParam), HIWORD(lParam), VSync) && StartedIn==VSync)
			{
				if(VSyncEnabled)
					VSyncEnabled = false;
				else
					VSyncEnabled = true;
				VSyncPushed = false;
				RenderDialog();
			}
			else if(Inside(LOWORD(lParam), HIWORD(lParam), OptimizeDT) && StartedIn==OptimizeDT)
			{
				if(OptimizeDTEnabled)
					OptimizeDTEnabled = false;
				else
					OptimizeDTEnabled = true;
				RenderDialog();
			}
			else if(Inside(LOWORD(lParam), HIWORD(lParam), FullRings) && StartedIn==FullRings)
			{
				if(FullRingsEnabled)
					FullRingsEnabled = false;
				else
					FullRingsEnabled = true;
				RenderDialog();
			}
			StartedIn = None;
			Move = false;
			break;

		case WM_MOUSEMOVE:
			if(LOWORD(lParam)>=(posX-10) && LOWORD(lParam)<(posX+DialogWidth) && HIWORD(lParam)>=(posY-20) && HIWORD(lParam)<(posY+DialogHeight))
			{
				CursorPosX = LOWORD(lParam);
				CursorPosY = HIWORD(lParam);

#ifdef USEMEGAMAP
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
				posX += LOWORD(lParam)-X;
				posY += HIWORD(lParam)-Y;
                CorrectPos();
				X = LOWORD(lParam);
				Y = HIWORD(lParam);
				return true;
			}
			
			X = LOWORD(lParam);
			Y = HIWORD(lParam);
			if(LOWORD(lParam)>=posX && LOWORD(lParam)<(posX+DialogWidth) && HIWORD(lParam)>=posY && HIWORD(lParam)<(posY+DialogHeight))
			{
				if(!Inside(LOWORD(lParam), HIWORD(lParam), OKButton))
				{
					if(OKButtonPushed == true)
					{
						OKButtonPushed = false;
						RenderDialog();
					}
				}
				else if(StartedIn == OKButton)
				{
					if(OKButtonPushed == false)
					{
						OKButtonPushed = true;
						RenderDialog();
					}
				}
				if(!Inside(LOWORD(lParam), HIWORD(lParam), StagedButton3))
				{
					if(StagedButton3Pushed == true)
					{
						StagedButton3Pushed = false;
						RenderDialog();
					}
				}
				else if(StartedIn == StagedButton3)
				{
					if(StagedButton3Pushed == false)
					{
						StagedButton3Pushed = true;
						RenderDialog();
					}
				}
				if(!Inside(LOWORD(lParam), HIWORD(lParam), SetVisible))
				{
					if(SetVisiblePushed == true)
					{
						SetVisiblePushed = false;
						RenderDialog();
					}
				}
				else if(StartedIn == SetVisible)
				{
					if(SetVisiblePushed == false)
					{
						SetVisiblePushed = true;
						RenderDialog();
					}
				}
				if(!Inside(LOWORD(lParam), HIWORD(lParam), VSync))
				{
					if(VSyncPushed == true)
					{
						VSyncPushed = false;
						RenderDialog();
					}
				}
				else if(StartedIn == VSync)
				{
					if(VSyncPushed == false)
					{
						VSyncPushed = true;
						RenderDialog();
					}
				}
				return true;
			}
			break;
		case WM_KEYDOWN:
			if(KeyCodeFocus)
			{
				VirtualKeyCode = (int)wParam;
				RenderDialog();
				return true;
			}
			if(WhiteboardKeyFocus)
			{
				VirtualWhiteboardKey = (int)wParam;
				RenderDialog();
				return true;
			}
			if (ClickSnapOverrideKeyFocus)
			{
				ClickSnapOverrideKey = (int)wParam;
				RenderDialog();
				return true;
			}
			if(MegmapFocus)
			{
				VirtualMegamap = (int)wParam;
				RenderDialog();
				return true;
			}
			break;
		case WM_CHAR:
			if(KeyCodeFocus)
			{
				return true;
			}
			if(WhiteboardKeyFocus)
			{
				return true;
			}
			if (ClickSnapOverrideKeyFocus)
			{
				return true;
			}
			if(MegmapFocus)
			{
				return true;
			}
			if(ShareBoxFocus)
			{
				if(wParam == 8) //backspace
				{
					if(strlen(ShareText)>0)
						ShareText[strlen(ShareText)-1] = '\0';
				}
				else
				{
					if(Lines!=MaxLines)
					{
						char App[2];
						App[0] = (TCHAR)wParam;
						App[1] = '\0';
						lstrcatA(ShareText, App);
					}
				}
				RenderDialog();
				return true;
			}
			if(AutoClickDelayFocus)
			{
				if(wParam == 8) //backspace
				{
					if(strlen(AutoClickDelayText)>0)
						AutoClickDelayText[strlen(AutoClickDelayText)-1] = '\0';
				}
				else
				{
					if(strlen(AutoClickDelayText)!=3)
					{
						if(wParam>='0' && wParam<='9')
						{
							char App[2];
							App[0] = (TCHAR)wParam;
							App[1] = '\0';
							lstrcatA(AutoClickDelayText, App);
						}
					}
				}
				RenderDialog();
				return true;
			}
			if (ClickSnapRadiusFocus)
			{
				if (wParam >= '0' && wParam <= '0' + MAX_CLICK_SNAP_RADIUS)
				{
					char App[2];
					App[0] = (TCHAR)wParam;
					App[1] = '\0';
					lstrcpyA(ClickSnapRadiusText, App);
				}
				RenderDialog();
				return true;
			}
			break;
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		;// return LocalShare->TAWndProc(WinProcWnd, Msg, wParam, lParam);
	}

	return false;//mesage not handled by this dialog
}

void Dialog::RenderDialog()
{
	RECT Dest;
	RECT Source;

	if(lpDialogSurf->Blt(NULL, lpBackground, NULL, DDBLT_ASYNC, NULL)!=DD_OK)
	{
		lpDialogSurf->Blt(NULL, lpBackground, NULL, DDBLT_WAIT , NULL);
	}

	Dest.left = OKButtonPosX;
	Dest.top = OKButtonPosY;
	Dest.right = OKButtonPosX + OKButtonWidth;
	Dest.bottom = OKButtonPosY + OKButtonHeight;
	Source.left = OKButtonPushed*OKButtonWidth;
	Source.top = 0;
	Source.right = OKButtonWidth + OKButtonPushed*OKButtonWidth;
	Source.bottom = OKButtonHeight;
	if(lpDialogSurf->Blt(&Dest, lpOKButton, &Source, DDBLT_ASYNC, NULL)!=DD_OK)
	{
		lpDialogSurf->Blt(&Dest, lpOKButton, &Source, DDBLT_WAIT , NULL);
	}

	DrawBackgroundButton();
	DrawKeyCode();
	DrawShareBox();
	DrawOptimizeDT();
	DrawVSync();
	DrawFullRings();
	//DrawDelay();
	DrawWhiteboardKey();
	DrawClickSnapRadius();
	DrawClickSnapOverrideKey();
	//DrawVisibleButton();
#ifdef USEMEGAMAP
	DrawMegaMapKey ( );
#endif
	
}

bool Dialog::Inside(int x, int y, int Control)
{
	x = x-posX;
	y = y-posY;

	switch(Control)
	{
	case OKButton:
		if(x>=OKButtonPosX && x<OKButtonPosX+OKButtonWidth && y>=OKButtonPosY && y<OKButtonPosY+OKButtonHeight)
			return true;
		else
			return false;
	case StagedButton3:
		if(x>=StagedButton3PosX && x<StagedButton3PosX+StagedButton3Width && y>=StagedButton3PosY && y<StagedButton3PosY+StagedButton3Height)
			return true;
		else
			return false;
	case KeyCode:
		if(x>=KeyCodePosX && x<KeyCodePosX+KeyCodeWidth && y>=KeyCodePosY && y<KeyCodePosY+KeyCodeHeight)
			return true;
		else
			return false;
	case ShareBox:
		if(x>=ShareBoxPosX && x<ShareBoxPosX+ShareBoxWidth && y>=ShareBoxPosY && y<ShareBoxPosY+ShareBoxHeight)
			return true;
		else
			return false;
	case OptimizeDT:
		if(x>=OptimizeDTPosX && x<OptimizeDTPosX+ShareBoxWidth && y>=OptimizeDTPosY && y<OptimizeDTPosY+OptimizeDTHeight)
			return true;
		else
			return false;
	case VSync:
		if(x>=VSyncPosX && x<VSyncPosX+VSyncWidth && y>=VSyncPosY && y<VSyncPosY+VSyncHeight)
			return true;
		else
			return false;
	case FullRings:
		if(x>=FullRingsPosX && x<FullRingsPosX+FullRingsWidth && y>=FullRingsPosY && y<FullRingsPosY+FullRingsHeight)
			return true;
		else
			return false;
	case AutoClickDelay:
		if(x>=AutoClickDelayPosX && x<AutoClickDelayPosX+AutoClickDelayWidth && y>=AutoClickDelayPosY && y<AutoClickDelayPosY+AutoClickDelayHeight)
			return true;
		else
			return false;
	case ClickSnapRadiusId:
		if (x >= ClickSnapRadiusPosX && x < ClickSnapRadiusPosX + ClickSnapRadiusWidth && y >= ClickSnapRadiusPosY && y < ClickSnapRadiusPosY + ClickSnapRadiusHeight)
			return true;
		else
			return false;
	case WhiteboardKey:
		if(x>=WhiteboardKeyPosX && x<WhiteboardKeyPosX+WhiteboardKeyWidth && y>=WhiteboardKeyPosY && y<WhiteboardKeyPosY+WhiteboardKeyHeight)
			return true;
		else
			return false;
	case ClickSnapOverrideKeyId:
		if (x >= ClickSnapOverrideKeyPosX && x < ClickSnapOverrideKeyPosX + ClickSnapOverrideKeyWidth && y >= ClickSnapOverrideKeyPosY && y < ClickSnapOverrideKeyPosY + ClickSnapOverrideKeyHeight)
			return true;
		else
			return false;
	case SetVisible:
		if(x>=SetVisiblePosX && x<SetVisiblePosX+SetVisibleWidth && y>=SetVisiblePosY && y<SetVisiblePosY+SetVisibleHeight)
			return true;
		else
			return false;
#ifdef USEMEGAMAP
	case MegaMapKey:
		if(x>=MegaMapKeyPosX && x<MegaMapKeyPosX+MegamapKeyWidth && y>=MegaMapKeyPoxY && y<MegaMapKeyPoxY+MegamapKeyHeight)
			return true;
		else
			return false;

#endif
	}

	return false;
}

void Dialog::DrawText(LPDIRECTDRAWSURFACE DestSurf, int x, int y, char *Text)
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

void Dialog::DrawSmallText(LPDIRECTDRAWSURFACE DestSurf, int x, int y, char *Text)
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

void Dialog::SetAll()
{
	CIncome *Income = (CIncome*)LocalShare->Income;
	if (Income)
	{
		Income->Set(StagedButton3State);
	}

	IDDrawSurface *SurfClass = (IDDrawSurface*)LocalShare->DDrawSurfClass;
	if (SurfClass)
	{
		SurfClass->Set ( VSyncEnabled);
	}
	

	CTAHook *TAHook = (CTAHook*)LocalShare->TAHook;

	if (TAHook)
	{
		int Delay = atoi(AutoClickDelayText);
		if(Delay<1)
			Delay = 1;

		int Radius = atoi(ClickSnapRadiusText);
		if (Radius < 0)
			Radius = DEFAULT_CLICK_SNAP_RADIUS;
		if (Radius > MAX_CLICK_SNAP_RADIUS)
			Radius = DEFAULT_CLICK_SNAP_RADIUS;
		ClickSnapRadiusText[0] = '0' + Radius;

		TAHook->Set(VirtualKeyCode, ShareText, OptimizeDTEnabled, FullRingsEnabled, Delay, Radius, ClickSnapOverrideKey);
	}

	AlliesWhiteboard *WB = (AlliesWhiteboard*)LocalShare->Whiteboard;
	if (WB)
	{
		WB->Set(VirtualWhiteboardKey);
	}
	
#ifdef USEMEGAMAP
	if (GUIExpander
		&&GUIExpander->myMinimap)
	{
		GUIExpander->myMinimap->Set ( VirtualMegamap);
	}
#endif

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

	RegSetValueEx(hKey, "BackGround", NULL, REG_DWORD, (unsigned char*)&StagedButton3State, sizeof(int));
	RegSetValueEx(hKey, "VSync", NULL, REG_BINARY, (unsigned char*)&VSyncEnabled, sizeof(bool));
	RegSetValueEx(hKey, "KeyCode", NULL, REG_DWORD, (unsigned char*)&VirtualKeyCode, sizeof(int));
	RegSetValueEx(hKey, "OptimizeDT", NULL, REG_BINARY, (unsigned char*)&OptimizeDTEnabled, sizeof(bool));
	RegSetValueEx(hKey, "FullRings", NULL, REG_BINARY, (unsigned char*)&FullRingsEnabled, sizeof(bool));
	RegSetValueEx(hKey, "ShareText", NULL, REG_SZ, (unsigned char*)ShareText, strlen(ShareText));
	RegSetValueEx(hKey, "Delay", NULL, REG_SZ, (unsigned char*)AutoClickDelayText, strlen(AutoClickDelayText));
	RegSetValueEx(hKey, "ClickSnapRadius", NULL, REG_SZ, (unsigned char*)ClickSnapRadiusText, strlen(ClickSnapRadiusText));
	RegSetValueEx(hKey, "ClickSnapOverrideKey", NULL, REG_DWORD, (unsigned char*)&ClickSnapOverrideKey, sizeof(int));
	RegSetValueEx(hKey, "WhiteboardKey", NULL, REG_DWORD, (unsigned char*)&VirtualWhiteboardKey, sizeof(int));
	RegSetValueEx(hKey, "MegamapKey", NULL, REG_DWORD, (unsigned char*)&VirtualMegamap, sizeof(int));

	RegCloseKey(hKey);
	RegCloseKey(hKey1);
}

void Dialog::ReadSettings()
{
	HKEY hKey;
	DWORD dwDisposition;
	DWORD Size;

	//VSyncEnabled= false;

	std::string SubKey = CompanyName_CCSTR;
	SubKey += "\\Eye";

	RegCreateKeyEx(HKEY_CURRENT_USER, SubKey.c_str(), NULL, TADRCONFIGREGNAME, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition);

	Size = sizeof(int);
	if(RegQueryValueEx(hKey, "BackGround", NULL, NULL, (unsigned char*)&StagedButton3State, &Size) != ERROR_SUCCESS)
	{
		StagedButton3State = 0;
	}
	Size = sizeof(bool);
	if(RegQueryValueEx(hKey, "VSync", NULL, NULL, (unsigned char*)&VSyncEnabled, &Size) != ERROR_SUCCESS)
	{
		VSyncEnabled = true;
	}
	Size = sizeof(int);
	if(RegQueryValueEx(hKey, "KeyCode", NULL, NULL, (unsigned char*)&VirtualKeyCode, &Size) != ERROR_SUCCESS)
	{
		VirtualKeyCode = 88;
	}
	Size = sizeof(bool);
	if(RegQueryValueEx(hKey, "OptimizeDT", NULL, NULL, (unsigned char*)&OptimizeDTEnabled, &Size) != ERROR_SUCCESS)
	{
		OptimizeDTEnabled = true;
	}
	Size = sizeof(bool);
	if(RegQueryValueEx(hKey, "FullRings", NULL, NULL, (unsigned char*)&FullRingsEnabled, &Size) != ERROR_SUCCESS)
	{
		FullRingsEnabled = true;
	}
	Size = sizeof(ShareText);
	if(RegQueryValueEx(hKey, "ShareText", NULL, NULL, (unsigned char*)ShareText, &Size) != ERROR_SUCCESS)
	{
		lstrcpyA(ShareText, "+setshareenergy 1000\255+setsharemetal 1000\255+shareall\255+shootall");
		ShareText[20] = 13;
		ShareText[40] = 13;
		ShareText[50] = 13;
	}
	Size = sizeof(AutoClickDelayText);
	if(RegQueryValueEx(hKey, "Delay", NULL, NULL, (unsigned char*)AutoClickDelayText, &Size) != ERROR_SUCCESS)
	{
		lstrcpyA(AutoClickDelayText, "10");
	}
	Size = sizeof(ClickSnapRadiusText);
	if (RegQueryValueEx(hKey, "ClickSnapRadius", NULL, NULL, (unsigned char*)ClickSnapRadiusText, &Size) != ERROR_SUCCESS)
	{
		ClickSnapRadiusText[0] = '0' + DEFAULT_CLICK_SNAP_RADIUS;
		ClickSnapRadiusText[1] = 0;
	}
	Size = sizeof(int);
	if (RegQueryValueEx(hKey, "ClickSnapOverrideKey", NULL, NULL, (unsigned char*)&ClickSnapOverrideKey, &Size) != ERROR_SUCCESS)
	{
		ClickSnapOverrideKey = VK_MENU;
	}
	Size = sizeof(int);
	if(RegQueryValueEx(hKey, "WhiteboardKey", NULL, NULL, (unsigned char*)&VirtualWhiteboardKey, &Size) != ERROR_SUCCESS)
	{
		VirtualWhiteboardKey = VK_OEM_5;
	}

	Size = sizeof(int);
	if(RegQueryValueEx(hKey, "MegamapKey", NULL, NULL, (unsigned char*)&VirtualMegamap, &Size) != ERROR_SUCCESS)
	{
		VirtualMegamap = VK_TAB;
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

void Dialog::DrawBackgroundButton()
{
	RECT Dest;
	RECT Source;

	DrawSmallText(lpDialogSurf, StagedButton3PosX, StagedButton3PosY-12, "Resource Bar Background");
	Dest.left = StagedButton3PosX;
	Dest.top = StagedButton3PosY;
	Dest.right = StagedButton3PosX + StagedButton3Width;
	Dest.bottom = StagedButton3PosY + StagedButton3Height;
	if(StagedButton3Pushed)
		Source.left = StagedButton3Width*3;
	else
		Source.left = StagedButton3State*StagedButton3Width;
	Source.top = 0;
	Source.right = Source.left + StagedButton3Width;
	Source.bottom = StagedButton3Height;
	if(lpDialogSurf->Blt(&Dest, lpStagedButton3, &Source, DDBLT_ASYNC, NULL)!=DD_OK)
	{
		lpDialogSurf->Blt(&Dest, lpStagedButton3, &Source, DDBLT_WAIT , NULL);
	}
	int y;
	if(StagedButton3Pushed)
		y = StagedButton3PosY+4;
	else
		y = StagedButton3PosY+3;
	int x;
	if(StagedButton3Pushed)
		x = StagedButton3PosX+5;
	else
		x = StagedButton3PosX+4;
	switch(StagedButton3State)
	{
	case 0:
		DrawText(lpDialogSurf, x, y, "None");
		break;
	case 1:
		DrawText(lpDialogSurf, x, y, "Text");
		break;
	case 2:
		DrawText(lpDialogSurf, x, y, "Solid");
		break;
	}
}

void Dialog::DrawKeyCode()
{
	DrawSmallText(lpDialogSurf, KeyCodePosX, KeyCodePosY-13, "Autoclick Key");
	DDSURFACEDESC ddsd;
	DDRAW_INIT_STRUCT(ddsd);
	if(lpDialogSurf->Lock(NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT , NULL)==DD_OK)
	{
		SurfaceMemory = ddsd.lpSurface;
		lPitch = ddsd.lPitch;

		FillRect(KeyCodePosX, KeyCodePosY, KeyCodePosX+KeyCodeWidth, KeyCodePosY+KeyCodeHeight, 0);

		char String[32];
		vkToStr ( VirtualKeyCode, String, sizeof(String));
		//wsprintf(String, "%i", VirtualKeyCode);
		if(KeyCodeFocus)
			DrawTinyText(String, KeyCodePosX + 2, KeyCodePosY + 3, 255U);
		else
			DrawTinyText(String, KeyCodePosX + 2, KeyCodePosY + 3, 208U);

		lpDialogSurf->Unlock(NULL);
	}
}

void Dialog::DrawShareBox()
{
	DrawSmallText(lpDialogSurf, ShareBoxPosX, ShareBoxPosY-13, "Chat Macro (F11)");
	DDSURFACEDESC ddsd;
	DDRAW_INIT_STRUCT(ddsd);
	if(lpDialogSurf->Lock(NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT , NULL)==DD_OK)
	{
		SurfaceMemory = ddsd.lpSurface;
		lPitch = ddsd.lPitch;

		FillRect(ShareBoxPosX, ShareBoxPosY, ShareBoxPosX+ShareBoxWidth, ShareBoxPosY+ShareBoxHeight, 0);

		//DrawTinyText(ShareText, ShareBoxPosX + 2, ShareBoxPosY + 3, 208);
		int CharsPerLine = (ShareBoxWidth-4)/8;
		char Line[100];
		Line[0] = '\0';
		char LineNum = 0;
		char LinePos = 0;
		size_t i;
		bool WasLineBreak = false;
		for(i=0; i<strlen(ShareText); i++)
		{
			if(LinePos > CharsPerLine)
			{
				if(ShareBoxFocus)
					DrawTinyText(Line, static_cast<int>(ShareBoxPosX + 2), static_cast<int>(ShareBoxPosY + 3 + LineNum*9), 255U);
				else
					DrawTinyText(Line, static_cast<int>(ShareBoxPosX + 2), static_cast<int>(ShareBoxPosY + 3 + LineNum*9), 208U);
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
				if(ShareText[i] != 13/*enter*/)
				{
					Line[LinePos] = ShareText[i];
					Line[LinePos+1] = '\0';
					LinePos++;
					WasLineBreak = false;
				}
				else
				{
					if(ShareBoxFocus)
						DrawTinyText(Line, static_cast<int>(ShareBoxPosX + 2), static_cast<int>(ShareBoxPosY + 3 + LineNum*9), 255U);
					else
						DrawTinyText(Line, static_cast<int>(ShareBoxPosX + 2), static_cast<int>(ShareBoxPosY + 3 + LineNum*9), 208U);
					if(!WasLineBreak)
						LineNum++;
					LinePos = 0;
					Line[0] = '\0';
					WasLineBreak = false;
				}
			}
		}
		if(ShareBoxFocus)
			DrawTinyText(Line, static_cast<int>(ShareBoxPosX + 2), static_cast<int>(ShareBoxPosY + 3 + LineNum*9), 255U);
		else
			DrawTinyText(Line, static_cast<int>(ShareBoxPosX + 2), static_cast<int>(ShareBoxPosY + 3 + LineNum*9), 208U);
		Lines = LineNum+1;
		lpDialogSurf->Unlock(NULL);
	}
	MaxLines = (ShareBoxHeight-4)/8;
}

void Dialog::DrawOptimizeDT()
{
	RECT Dest;
	RECT Source;

	DrawSmallText(lpDialogSurf, OptimizeDTPosX+20, OptimizeDTPosY+3, "Optimize DT Rows");
	Dest.left = OptimizeDTPosX;
	Dest.top = OptimizeDTPosY;
	Dest.right = OptimizeDTPosX + OptimizeDTWidth;
	Dest.bottom = OptimizeDTPosY + OptimizeDTHeight;
	Source.left = OptimizeDTEnabled*16;
	Source.top = 0;
	Source.right = Source.left + OptimizeDTWidth;
	Source.bottom = OptimizeDTHeight;
	if(lpDialogSurf->Blt(&Dest, lpCheckBox, &Source, DDBLT_ASYNC, NULL)!=DD_OK)
	{
		lpDialogSurf->Blt(&Dest, lpCheckBox, &Source, DDBLT_WAIT , NULL);
	}
}

void Dialog::DrawVSync()
{
	RECT Dest;
	RECT Source;

	DrawSmallText(lpDialogSurf, VSyncPosX, VSyncPosY-12, "VSync");
	Dest.left = VSyncPosX;
	Dest.top = VSyncPosY;
	Dest.right = VSyncPosX + VSyncWidth;
	Dest.bottom = VSyncPosY + VSyncHeight;
	if(VSyncPushed)
		Source.left = VSyncWidth*2;
	else
		Source.left = VSyncEnabled*VSyncWidth;
	Source.top = 0;
	Source.right = Source.left + VSyncWidth;
	Source.bottom = VSyncHeight;
	if(lpDialogSurf->Blt(&Dest, lpStagedButton1, &Source, DDBLT_ASYNC, NULL)!=DD_OK)
	{
		lpDialogSurf->Blt(&Dest, lpStagedButton1, &Source, DDBLT_WAIT , NULL);
	}
	int y;
	if(VSyncPushed)
		y = VSyncPosY+4;
	else
		y = VSyncPosY+3;
	int x;
	if(VSyncPushed)
		x = VSyncPosX+5;
	else
		x = VSyncPosX+4;
	switch(VSyncEnabled)
	{
	case true:
		DrawText(lpDialogSurf, x, y, "Enabled");
		break;
	case false:
		DrawText(lpDialogSurf, x, y, "Disabled");
		break;
	}
}

void Dialog::DrawFullRings()
{
	RECT Dest;
	RECT Source;

	DrawSmallText(lpDialogSurf, FullRingsPosX+20, FullRingsPosY+3, "Enable FullRings");
	Dest.left = FullRingsPosX;
	Dest.top = FullRingsPosY;
	Dest.right = FullRingsPosX + FullRingsWidth;
	Dest.bottom = FullRingsPosY + FullRingsHeight;
	Source.left = FullRingsEnabled*16;
	Source.top = 0;
	Source.right = Source.left + FullRingsWidth;
	Source.bottom = FullRingsHeight;
	if(lpDialogSurf->Blt(&Dest, lpCheckBox, &Source, DDBLT_ASYNC, NULL)!=DD_OK)
	{
		lpDialogSurf->Blt(&Dest, lpCheckBox, &Source, DDBLT_WAIT , NULL);
	}
}

void Dialog::DrawDelay()
{
	DrawSmallText(lpDialogSurf, AutoClickDelayPosX, AutoClickDelayPosY-13, "Autoclick delay");
	DDSURFACEDESC ddsd;
	DDRAW_INIT_STRUCT(ddsd);
	if(lpDialogSurf->Lock(NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT , NULL)==DD_OK)
	{
		SurfaceMemory = ddsd.lpSurface;
		lPitch = ddsd.lPitch;

		FillRect(AutoClickDelayPosX, AutoClickDelayPosY, (AutoClickDelayPosX)+AutoClickDelayWidth, AutoClickDelayPosY+AutoClickDelayHeight, 0);

		if(AutoClickDelayFocus)
			DrawTinyText(AutoClickDelayText, static_cast<int>(AutoClickDelayPosX+2), static_cast<int>(AutoClickDelayPosY+3), 255U);
		else
			DrawTinyText(AutoClickDelayText, static_cast<int>(AutoClickDelayPosX+2), static_cast<int>(AutoClickDelayPosY+3), 208U);

		lpDialogSurf->Unlock(NULL);
	}
}

void Dialog::DrawClickSnapRadius()
{
	DrawSmallText(lpDialogSurf, ClickSnapRadiusPosX, ClickSnapRadiusPosY - 13, "Click-Snap Radius");
	DDSURFACEDESC ddsd;
	DDRAW_INIT_STRUCT(ddsd);
	if (lpDialogSurf->Lock(NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL) == DD_OK)
	{
		SurfaceMemory = ddsd.lpSurface;
		lPitch = ddsd.lPitch;

		FillRect(ClickSnapRadiusPosX, ClickSnapRadiusPosY, (ClickSnapRadiusPosX)+ClickSnapRadiusWidth, ClickSnapRadiusPosY + ClickSnapRadiusHeight, 0);

		if (ClickSnapRadiusFocus)
			DrawTinyText(ClickSnapRadiusText, static_cast<int>(ClickSnapRadiusPosX + 2), static_cast<int>(ClickSnapRadiusPosY + 3), 255U);
		else
			DrawTinyText(ClickSnapRadiusText, static_cast<int>(ClickSnapRadiusPosX + 2), static_cast<int>(ClickSnapRadiusPosY + 3), 208U);

		lpDialogSurf->Unlock(NULL);
	}
}

void Dialog::DrawMegaMapKey ()
{

	DrawSmallText(lpDialogSurf, MegaMapKeyPosX, MegaMapKeyPoxY- 13, "Megamap Key");

	DDSURFACEDESC ddsd;
	DDRAW_INIT_STRUCT(ddsd);
	if(lpDialogSurf->Lock(NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT , NULL)==DD_OK)
	{
		SurfaceMemory = ddsd.lpSurface;
		lPitch = ddsd.lPitch;

		FillRect ( MegaMapKeyPosX, MegaMapKeyPoxY, MegaMapKeyPosX+MegamapKeyWidth, MegaMapKeyPoxY+MegamapKeyHeight, 0);

		char String[32];
		vkToStr ( VirtualMegamap, String, sizeof(String));

		if(MegmapFocus)
			DrawTinyText (String, static_cast<int>(MegaMapKeyPosX + 2), static_cast<int>(MegaMapKeyPoxY + 3), 255U);
		else
			DrawTinyText (String, static_cast<int>(MegaMapKeyPosX + 2), static_cast<int>(MegaMapKeyPoxY + 3), 208U);

		lpDialogSurf->Unlock(NULL);
	}
}

void Dialog::DrawClickSnapOverrideKey()
{
	DrawSmallText(lpDialogSurf, ClickSnapOverrideKeyPosX, ClickSnapOverrideKeyPosY - 13, "Click-Snap Override Key");
	DDSURFACEDESC ddsd;
	DDRAW_INIT_STRUCT(ddsd);
	if (lpDialogSurf->Lock(NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL) == DD_OK)
	{
		SurfaceMemory = ddsd.lpSurface;
		lPitch = ddsd.lPitch;

		FillRect(ClickSnapOverrideKeyPosX, ClickSnapOverrideKeyPosY, ClickSnapOverrideKeyPosX + ClickSnapOverrideKeyWidth, ClickSnapOverrideKeyPosY + ClickSnapOverrideKeyHeight, 0);

		char String[32];
		vkToStr(ClickSnapOverrideKey, String, sizeof(String));
		if (ClickSnapOverrideKeyFocus)
			DrawTinyText(String, static_cast<int>(ClickSnapOverrideKeyPosX + 2), static_cast<int>(ClickSnapOverrideKeyPosY + 3), 255U);
		else
			DrawTinyText(String, static_cast<int>(ClickSnapOverrideKeyPosX + 2), static_cast<int>(ClickSnapOverrideKeyPosY + 3), 208U);

		lpDialogSurf->Unlock(NULL);
	}
}

void Dialog::DrawWhiteboardKey()
{
	DrawSmallText(lpDialogSurf, WhiteboardKeyPosX, WhiteboardKeyPosY-13, "Whiteboard Key");
	DDSURFACEDESC ddsd;
	DDRAW_INIT_STRUCT(ddsd);
	if(lpDialogSurf->Lock(NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT , NULL)==DD_OK)
	{
		SurfaceMemory = ddsd.lpSurface;
		lPitch = ddsd.lPitch;

		FillRect(WhiteboardKeyPosX, WhiteboardKeyPosY, WhiteboardKeyPosX+WhiteboardKeyWidth, WhiteboardKeyPosY+WhiteboardKeyHeight, 0);

		char String[32];
		vkToStr ( VirtualWhiteboardKey, String, sizeof(String));
		//wsprintf ( String, "%i", VirtualWhiteboardKey);
		if(WhiteboardKeyFocus)
			DrawTinyText(String, static_cast<int>(WhiteboardKeyPosX + 2), static_cast<int>(WhiteboardKeyPosY + 3), 255U);
		else
			DrawTinyText(String, static_cast<int>(WhiteboardKeyPosX + 2), static_cast<int>(WhiteboardKeyPosY + 3), 208U);

		lpDialogSurf->Unlock(NULL);
	}
}

void Dialog::DrawVisibleButton()
{
	DrawSmallText(lpDialogSurf, SetVisiblePosX, SetVisiblePosY-13, "Set Visible list");

	RECT Dest;
	RECT Source;
	Dest.left = SetVisiblePosX;
	Dest.top = SetVisiblePosY;
	Dest.right = SetVisiblePosX + SetVisibleWidth;
	Dest.bottom = SetVisiblePosY + SetVisibleHeight;
	Source.left = SetVisiblePushed*SetVisibleWidth;
	Source.top = 0;
	Source.right = SetVisibleWidth + SetVisiblePushed*SetVisibleWidth;
	Source.bottom = SetVisibleHeight;
	if(lpDialogSurf->Blt(&Dest, lpStandardButton, &Source, DDBLT_ASYNC, NULL)!=DD_OK)
	{
		lpDialogSurf->Blt(&Dest, lpStandardButton, &Source, DDBLT_WAIT , NULL);
	}

	DrawText(lpDialogSurf, SetVisiblePosX+10+SetVisiblePushed, SetVisiblePosY+4+SetVisiblePushed, "Set Visible");
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

void Dialog::SetVisibleList()
{

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
