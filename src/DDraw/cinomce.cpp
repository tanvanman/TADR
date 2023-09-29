#include "oddraw.h"
#include "cincome.h"
#include "iddrawsurface.h"
#include "font.h"
#include "tamem.h"
#include "tafunctions.h"
#include "TAConfig.h"

#include "fullscreenminimap.h"
#include "GUIExpand.h"

#include "MegamapControl.h"

#include "dialog.h"

#ifdef min
  #undef min
#endif
#include <algorithm>
#include <stdio.h>

int X,Y;

CIncome::CIncome(BOOL VidMem)
{
	LocalShare->Income = this;
	LPDIRECTDRAW TADD = (IDirectDraw*)LocalShare->TADirectDraw;

	DDSURFACEDESC ddsd;
	DDRAW_INIT_STRUCT(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;

	if (VidMem)
	{
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;
		
	}
	else	
	{
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
	}
	
	ddsd.dwWidth = PlayerWidth;
	ddsd.dwHeight = PlayerHight*11;

	TADD->CreateSurface(&ddsd, &lpIncomeSurf, NULL);

	BackgroundType = 0;
	ReadPos();
	First = true;

	CursorX= 0;
	CursorY= 0;
	IDDrawSurface::OutptTxt ( "New CIncome");
}

CIncome::~CIncome()
{
	if(lpIncomeSurf)
		lpIncomeSurf->Release();

	lpIncomeSurf= NULL;
	WritePos();
}

void CIncome::BlitIncome(LPDIRECTDRAWSURFACE DestSurf)
{
	if(lpIncomeSurf->IsLost() != DD_OK)
	{
		if(lpIncomeSurf->Restore() != DD_OK)
			return;
	}
	BlitState++;

	//ShowAllIncome();
	if(DataShare->TAProgress == TAInGame)
	{
		if(BlitState%30 == 1)
		{
			DDSURFACEDESC ddsd;
			DDRAW_INIT_STRUCT(ddsd);
			int PlayerDrawn;
			TAdynmemStruct * Ptr= *(TAdynmemStruct* *)0x00511de8;
			
			if(lpIncomeSurf->Lock(NULL, &ddsd, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL)==DD_OK)
			{
				SurfaceMemory = ddsd.lpSurface;
				lPitch = ddsd.lPitch;

				if((0!=(WATCH& (Ptr->Players[LocalShare->OrgLocalPlayerID].PlayerInfo->PropertyMask)))
					||DataShare->PlayingDemo)
					PlayerDrawn = ShowAllIncome();
				else
					PlayerDrawn = ShowAllyIncome();
				//PlayerDrawn = ShowAllIncome();
			}
			else
				SurfaceMemory = NULL;

			if(First == true && PlayerDrawn>0)
			{
				First = false;
				CorrectPos();
			}

			lpIncomeSurf->Unlock(NULL);
		}

		RECT Dest;
		Dest.left = posX;
		Dest.top = posY;
		Dest.right = posX + PlayerWidth;
		Dest.bottom = posY + LocalShare->Height;
		RECT Source;
		Source.left = 0;
		Source.top = 0;
		Source.right = PlayerWidth;
		Source.bottom = LocalShare->Height;

		DDBLTFX ddbltfx;
		DDRAW_INIT_STRUCT(ddbltfx);
		ddbltfx.ddckSrcColorkey.dwColorSpaceLowValue = 1;
		ddbltfx.ddckSrcColorkey.dwColorSpaceHighValue = 1;
		if(DestSurf->Blt(&Dest, lpIncomeSurf, &Source, DDBLT_ASYNC | DDBLT_KEYSRCOVERRIDE , &ddbltfx)!=DD_OK)
		{
			DestSurf->Blt(&Dest, lpIncomeSurf, &Source, DDBLT_WAIT | DDBLT_KEYSRCOVERRIDE , &ddbltfx);
		}
		
		//BlitCursor
		if ((CursorY!=-1)
			&&(CursorX!=-1))
		{
			((Dialog *)LocalShare->Dialog)->BlitCursor ( DestSurf, CursorX, CursorY);
		}
	}
	
}

int CIncome::ShowAllIncome()
{
	if(BackgroundType==2)
		FillRect(0);
	else
		FillRect(1);

	DataShare->IsRunning = 15;

	LocalShare->Width = PlayerWidth;

	int j = 0;
	for(int i=1; i<10; i++)
	{
		if(strlen(DataShare->PlayerNames[i])>0)
		{
			DataShare->IsRunning = 50;
			ShowPlayerIncome(i, 0, j*PlayerHight);
			j++;
		}
	}

	// watcher's things
	

	ShowMyViewIncome ( 0, j*PlayerHight);

	j++;
	LocalShare->Height = PlayerHight*j;
	return j;
}

int CIncome::ShowAllyIncome()
{
	if(BackgroundType==2)
		FillRect(0);
	else
		FillRect(1);

	DataShare->IsRunning = 15;

	LocalShare->Width = PlayerWidth;

	//int OffsetX = posX;
	//int OffsetY = posY;
	LocalShare->Height = 0;
	int j = 0;
	for(int i=1; i<10; i++)
	{
		if(DataShare->allies[i])
		{
			DataShare->IsRunning = 100;
			ShowPlayerIncome(i, 0, j*PlayerHight);
			j++;
		}
	}

	LocalShare->Height = PlayerHight*j;
	return j;
}

void CIncome::ShowMyViewIncome (int posx, int posy)
{
	if(SurfaceMemory == NULL)
		return;

	char *SurfMem = (char*)SurfaceMemory;


	for(int i=0; i<4; i++)
	{
		memset(&SurfMem[posx+(posy+i)*lPitch+ 44], 90, 100);
	}

	//int Backup= BackgroundType;
	//BackgroundType = 1;
	DrawText ( "My Original Camera", posx+30, posy+8, 208u);
	//DrawText ( "Press Name To Player View", posx+30, posy+18, 208u);

	for(int i=0; i<4; i++)
	{
		memset(&SurfMem[posx+(posy+24 +i)*lPitch+ 44], 90, 100);
	}


	//BackgroundType= Backup;
}

void CIncome::ShowPlayerIncome(int Player, int posx, int posy)
{
	float ValueF;
	char Value[100];

	char C = GetPlayerColor(Player);

	static unsigned char LockColor= 20;
	static unsigned char LosViewColor= 20^ 0x20;
	
	if (Player==DataShare->LockOn)
	{//hignlight the player name that lockon
		if (NULL!=SurfaceMemory)
		{
			LockColor= LockColor^ 0x20;
			char *SurfMem = (char*)SurfaceMemory;

			for(int i=0; i<PlayerHight; i++)
			{
				memset(&SurfMem[posx+(posy+i)*lPitch], LockColor, PlayerWidth);
			}
		}
	}
	else if (Player==DataShare->LosViewOn)
	{//hignlight the player that lockon
		if (NULL!=SurfaceMemory)
		{
			char *SurfMem = (char*)SurfaceMemory;

			for(int i=0; i<PlayerHight; i++)
			{
				memset(&SurfMem[posx+(posy+i)*lPitch], LosViewColor, PlayerWidth);
			}
		}
	}

	//DrawText(DataShare->PlayerNames[Player], posx+45, posy+1, 0);
	DrawPlayerRect(posx+36, posy+1, C);

	DrawText(DataShare->PlayerNames[Player], posx+44, posy, 7);

	DrawStorageText(DataShare->storedM[Player], posx, posy+10);
	DrawStorageText(DataShare->storedE[Player], posx, posy+20);

	PaintStoragebar(posx+44, posy+10, Player , MetalBar);
	ValueF = DataShare->incomeM[Player];
	sprintf_s(Value, 100, "+%.1f", ValueF);
	DrawText(Value, posx+150, posy+11, 0);
	DrawText(Value, posx+149, posy+10, 6);

	PaintStoragebar(posx+44, posy+20, Player , EnergyBar);
	ValueF = DataShare->incomeE[Player];
	sprintf_s(Value, 100, "+%.0f", ValueF);
	DrawText(Value, posx+150, posy+21, 0);
	DrawText(Value, posx+149, posy+20, 6);
}

int CIncome::DrawStorageText(float Storage, int posx, int posy)
{
	char Value[100];
	if(Storage<10000)
	{
		sprintf_s (Value, "%.0f", Storage);
	}
	else if(Storage<100000)
	{
		Storage = Storage/1000;
		sprintf_s (Value, "%.1fK", Storage);
	}
	else
	{
		Storage = Storage/1000;
		sprintf_s(Value, "%.0fK", Storage);
	}
	DrawText(Value, posx+1 + (5-strlen(Value))*8, posy+1, 0);
	DrawText(Value, posx + (5-strlen(Value))*8, posy, 6);

	return 0;
}

void CIncome::DrawText(char *String, int posx, int posy, char Color)
{
	if(SurfaceMemory == NULL)
		return;

	char *SurfMem = (char*)SurfaceMemory;

	if(BackgroundType == 1)
	{
		for(int i=0; i<9; i++)
		{
			memset(&SurfMem[posx+(posy+i)*lPitch], 0, strlen(String)*8+1);
		}
	}

	posx++;
	posy++;
	for(size_t i=0; i<strlen(String); i++)
	{
		for(int j=0; j<8; j++)
		{
			for(int k=0; k<8; k++)
			{
				bool b = 0!=(ThinFont[String[i]*8+j] & (1 << k));//windowsÀïµÄfalse==0
				if(b)
					SurfMem[(posx+(i*8)+(7-k))+(posy+j)*lPitch] = Color;
			}
		}
	}
}

void CIncome::PaintStoragebar(int posx, int posy, int Player, int Type)
{
	if(SurfaceMemory == NULL)
		return;

	char *SurfMem = (char*)SurfaceMemory;
	int StorageBarLength= 0;
	char FillColor;
	for(int i=0; i<100; i++)
	{
		SurfMem[(posx+i)+posy*lPitch] = 0;
		SurfMem[(posx+i)+(posy+1)*lPitch] = 0;
		SurfMem[(posx+i)+(posy+2)*lPitch] = 0;
	}

	if(Type == EnergyBar)
	{
		if(DataShare->storageE[Player]!=0)
			StorageBarLength = static_cast<int>((DataShare->storedE[Player] / DataShare->storageE[Player])*100);
		FillColor = 208U;
	}
	else if(Type == MetalBar)
	{
		if(DataShare->storageM[Player]!=0)
			StorageBarLength = static_cast<int>((DataShare->storedM[Player] / DataShare->storageM[Player])*100);
		FillColor = 224U;
	}
	if ( StorageBarLength>99)
	{
		StorageBarLength= 99;
	}else if (StorageBarLength<0)
	{
		StorageBarLength= 0;
	}
	for(int i=0; i<StorageBarLength; i++)
	{
		SurfMem[(posx+i)+posy*lPitch] = FillColor;
		SurfMem[(posx+i)+(posy+1)*lPitch] = FillColor;
		SurfMem[(posx+i)+(posy+2)*lPitch] = FillColor;
	}
}

void CIncome::FillRect(char Color)
{
	if(SurfaceMemory == NULL)
		return;

	char *SurfMem = (char*)SurfaceMemory;

	for(int i=0; i<LocalShare->Height; i++)
	{
		memset(&SurfMem[i*lPitch], Color, PlayerWidth);
	}
}

void CIncome::Set(int BGType)
{
	BackgroundType = BGType;
}

void CIncome::ReadPos()
{
	HKEY hKey;
	DWORD dwDisposition;
	DWORD Size = sizeof(int);

	std::string SubKey = CompanyName_CCSTR;
	SubKey += "\\Eye";

	RegCreateKeyEx(HKEY_CURRENT_USER, SubKey.c_str(), NULL, TADRCONFIGREGNAME, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition);

	if(RegQueryValueEx(hKey, "IncomePosX", NULL, NULL, (unsigned char*)&posX, &Size) == ERROR_SUCCESS)
	{
		;
	}
	else
	{
		//default pos
		posX = LocalShare->ScreenWidth - 254;
	}

	if(RegQueryValueEx(hKey, "IncomePosY", NULL, NULL, (unsigned char*)&posY, &Size) == ERROR_SUCCESS)
	{
	}
	else
	{
		posY = 50;
	}

	RegCloseKey(hKey);
}

void CIncome::WritePos()
{
	HKEY hKey;
	HKEY hKey1;
	DWORD dwDisposition;

	RegCreateKeyEx(HKEY_CURRENT_USER, CompanyName_CCSTR, NULL, TADRCONFIGREGNAME, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey1, &dwDisposition);

	RegCreateKeyEx(hKey1, "Eye", NULL, TADRCONFIGREGNAME, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition);

	RegSetValueEx(hKey, "IncomePosX", NULL, REG_DWORD, (unsigned char*)&posX, sizeof(int));
	RegSetValueEx(hKey, "IncomePosY", NULL, REG_DWORD, (unsigned char*)&posY, sizeof(int));

	
	RegCloseKey(hKey);

	RegCloseKey(hKey1);
}

bool CIncome::Message(HWND WinProchWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	__try
	{
		if(DataShare->TAProgress != TAInGame)
			return false;
		switch(Msg)
		{
		case WM_MOUSEMOVE:
			if(LOWORD(lParam)>posX && LOWORD(lParam)<(posX+LocalShare->Width) && HIWORD(lParam)>(posY) && HIWORD(lParam)<((posY)+LocalShare->Height))
			{
				CursorX = LOWORD(lParam);
				CursorY = HIWORD(lParam);

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
				CursorX = -1;
				CursorY = -1;
			}

			if((wParam&MK_LBUTTON)>0 && StartedInRect == true)
			{
				posX += LOWORD(lParam)-X;
				posY += HIWORD(lParam)-Y;
                CorrectPos();
				X = LOWORD(lParam);
				Y = HIWORD(lParam);
				return true;
			}
			else
				StartedInRect = false;
			X = LOWORD(lParam);
			Y = HIWORD(lParam);
			break;
		case WM_LBUTTONDOWN:
			if(LOWORD(lParam)>posX && LOWORD(lParam)<(posX+LocalShare->Width) && HIWORD(lParam)>(posY) && HIWORD(lParam)<((posY)+LocalShare->Height))
			{
				StartedInRect = true;
				return true;
			}
			break;
		case WM_LBUTTONDBLCLK:
			if(LOWORD(lParam)>posX && LOWORD(lParam)<(posX+LocalShare->Width) && HIWORD(lParam)>(posY) && HIWORD(lParam)<((posY)+LocalShare->Height))
			{
				TAdynmemStruct * Ptr;
				Ptr	= *(TAdynmemStruct* *)0x00511de8;
				if(DataShare->PlayingDemo
					|| (0!=(WATCH& (Ptr->Players[LocalShare->OrgLocalPlayerID].PlayerInfo->PropertyMask))))
				{
					//((Dialog*)LocalShare->Dialog)->ShowDialog();
					WORD CurtXPos= LOWORD(lParam);
					WORD CurtYPos= HIWORD(lParam);
					int ClickedPlayerID= (CurtYPos- posY)/ PlayerHight+ 1;

					if (CurtYPos>(posY+ LocalShare->Height- PlayerHight))
					{
						DataShare->LockOn= 0;
						DataShare->LosViewOn= 0xa;
						if (LocalShare->OrgLocalPlayerID!=Ptr->LocalHumanPlayer_PlayerID)
						{
							DeselectUnits ( );
							UpdateSelectUnitEffect ( ) ;
							ApplySelectUnitMenu_Wapper ( );
							ViewPlayerLos_Replay ( 0, TRUE);
						}
						else
						{

							DeselectUnits ( );
							UpdateSelectUnitEffect ( ) ;
							ApplySelectUnitMenu_Wapper ( );

							ViewPlayerLos_Replay ( 0);
						}
					}
					else if (ClickedPlayerID==DataShare->LockOn)
					{// now we deal about click in player res bar. first check if already lockon someone
						DataShare->LockOn= 0;
					}
					else if (ClickedPlayerID==(DataShare->LosViewOn))
					{// if not lock on this one, maybe watching this one's res bar
						DataShare->LockOn= DataShare->LosViewOn;
					}
					else
					{// when watching not the one res bar, exchange this this one.
						DataShare->LosViewOn= (CurtYPos- posY)/ PlayerHight+ 1;

						if (DataShare->PlayingDemo)
						{
							DeselectUnits ( );
							UpdateSelectUnitEffect ( ) ;
							ApplySelectUnitMenu_Wapper ( );
							ViewPlayerLos_Replay ( DataShare->LosViewOn, TRUE);
						}
						else
						{
							DeselectUnits ( );

							UpdateSelectUnitEffect ( ) ;
							ApplySelectUnitMenu_Wapper ( );
							ViewPlayerLos_Replay ( DataShare->LosViewOn);
						}
						
						//
						if (LocalShare->OrgLocalPlayerID!=DataShare->LockOn)
						{
							DataShare->LockOn= DataShare->LosViewOn;;
						}
					}

					return true;
				}
			}
			break;
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		;// return LocalShare->TAWndProc(WinProcWnd, Msg, wParam, lParam);
	}
	return false;
}

bool CIncome::IsShow (RECT * Rect_p)
{
	if (Rect_p)

	{
		Rect_p->left= posX;
		Rect_p->right= posX+ LocalShare->Width;
		Rect_p->top= posY;
		Rect_p->bottom= LocalShare->Height;
	}

	GameingState * GameingState_P= (*TAmainStruct_PtrPtr)->GameingState_Ptr;
	if (GameingState_P
		&&(gameingstate::MULTI==GameingState_P->State))
	{
		return true;
	}
	return false;
}

void CIncome::CorrectPos()
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

    if (posX < bounds.left)
        posX = bounds.left;
    if (posX > bounds.right - LocalShare->Width)
        posX = bounds.right - LocalShare->Width;

    if (posY < bounds.top)
        posY = bounds.top;
    if (posY > bounds.bottom - LocalShare->Height)
        posY = bounds.bottom - LocalShare->Height;
}

unsigned char CIncome::GetPlayerColor(int Player)
{
	return DataShare->PlayerDotColors[DataShare->PlayerColors[Player]];
}

void CIncome::DrawPlayerRect(int posx, int posy, char Color)
{
	if(SurfaceMemory == NULL)
		return;

	char *SurfMem = (char*)SurfaceMemory;

	for(int i=0; i<8; i++)
	{
		memset(&SurfMem[posx+(posy+i)*lPitch], Color, 8);
	}

}

