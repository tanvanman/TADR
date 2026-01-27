#include "config.h"
#include "oddraw.h"
#include "commanderwarp.h"
#include "iddrawsurface.h"
#include "pcxread.h"
#include "dialog.h"
#include <stdio.h>
#include "fullscreenminimap.h"
#include "GUIExpand.h"

CWarp::CWarp(BOOL VidMem)
{
	lpButton = NULL;
	if(DataShare->ehaOff == 1)
		return;

	LocalShare->CommanderWarp = this;

	int *PTR = (int*)0x00511de8;
	MapX = (int*)(*PTR + 0x1431f);
	MapY = (int*)(*PTR + 0x14323);

	lpButton = CreateSurfPCXResource(12, VidMem);
	ButtonDown = false;
	StartedinButton = false;

	IDDrawSurface::OutptTxt ( "New CWarp");
}

CWarp::~CWarp()
{
	if(lpButton)
		lpButton->Release();

	lpButton= NULL;
}

void CWarp::SetPos(int x, int y)
{
	int *PTR1 = (int*)0x511de8;
	int *Commander = (int*)((*PTR1)+0x1b8e+0x3c);
	unsigned short *XPos = (unsigned short*)((*Commander) + 0x6c);
	unsigned short *YPos = (unsigned short*)(*Commander + 0x74);

	*XPos = (short)x;
	*YPos = (short)y;

}

bool CWarp::Message(HWND WinProcWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	__try
	{

#if USEMEGAMAP
		if (GUIExpander
			&&GUIExpander->myMinimap)
		{
			if (GUIExpander->myMinimap->IsBliting ( ))
			{
				return false;
			}
		}
#endif

		if(DataShare->CommanderWarp == DISABLED || DataShare->CommanderWarp == ALLDONE)
			return false;

		if(DataShare->TAProgress != TAInGame)
			return false;

		switch(Msg)
		{
		case WM_LBUTTONDOWN:
			xPos = LOWORD(lParam);
			yPos = HIWORD(lParam);
			if(WithinButton(xPos, yPos))
			{
				StartedinButton = true;
				ButtonDown = true;
			}
			else if(xPos>128 && yPos>32 && yPos<LocalShare->ScreenHeight-32)
			{
				SetPos((xPos-128) + *MapX, (yPos) + *MapY);
			}
			return true;
			break;

		case WM_LBUTTONUP:
			if(WithinButton(LOWORD(lParam), HIWORD(lParam)) && StartedinButton)
			{
				DataShare->CommanderWarp = CLIENTDONE;
				return true;
			}
			ButtonDown = false;
			StartedinButton = false;
			break;

		case WM_MOUSEMOVE:
			xPos = LOWORD(lParam);
			yPos = HIWORD(lParam);
			break;
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		;// return LocalShare->TAWndProc(WinProcWnd, Msg, wParam, lParam);
	}
	return false;
}

void CWarp::Blit(LPDIRECTDRAWSURFACE DestSurf)
{
	if(DataShare->CommanderWarp == DISABLED || DataShare->CommanderWarp == ALLDONE)
		return;

	if(lpButton->IsLost() != DD_OK)
	{
		lpButton->Restore();
		RestoreFromPCX(12, lpButton);
	}

	if(DataShare->TAProgress == TAInGame)
	{
		if(DataShare->CommanderWarp == ENABLED)
		{
			Dialog *pDialog = (Dialog*)LocalShare->Dialog;
			pDialog->DrawText(DestSurf, LocalShare->ScreenWidth-400, 100, "Place your commander and click done");
			BlitBtn(DestSurf);
			Dialog *DialogPTR = (Dialog*)LocalShare->Dialog;
			DialogPTR->BlitCursor(DestSurf, xPos, yPos);
		}
		if(DataShare->CommanderWarp == CLIENTDONE)
		{
			Dialog *pDialog = (Dialog*)LocalShare->Dialog;
			pDialog->DrawText(DestSurf, LocalShare->ScreenWidth-400, 100, "Waiting for others to finish");
		}
	}
}

bool CWarp::WithinButton(int x, int y)
{
	if(x>ButtonXPos && x<ButtonXPos+ButtonWidth && y>ButtonYPos && y<ButtonYPos+ButtonHeight)
		return true;
	else
		return false;
}

void CWarp::BlitBtn(LPDIRECTDRAWSURFACE DestSurf)
{
	Dialog *DialogPTR = (Dialog*)LocalShare->Dialog;

	RECT Dest;
	RECT Source;
	Dest.left = ButtonXPos;
	Dest.top = ButtonYPos;
	Dest.right = ButtonXPos + ButtonWidth;
	Dest.bottom = ButtonYPos + ButtonHeight;
	Source.left = ButtonDown*ButtonWidth;
	Source.top = 0;
	Source.right = ButtonWidth + ButtonDown*ButtonWidth;
	Source.bottom = ButtonHeight;
	if(DestSurf->Blt(&Dest, lpButton, &Source, DDBLT_ASYNC, NULL)!=DD_OK)
	{
		DestSurf->Blt(&Dest, lpButton, &Source, DDBLT_WAIT , NULL);
	}


	DialogPTR->DrawText(DestSurf, ButtonXPos+3+ButtonDown, ButtonYPos+4+ButtonDown, "Done");
}
