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
#ifdef max
	#undef max
#endif

#include <algorithm>
#include <stdio.h>

int X,Y;

#define EnergyBar 1
#define MetalBar 2

#define PlayerHight 30
#define PlayerWidth 210

#define MinimiseWidgetPosX PlayerWidth
#define MinimiseWidgetPosY 0
#define MinimiseWidgetColour 208u
#define MinimiseWidgetSize 9

//https://www.tauniverse.com/forum/showthread.php?t=43867
#define PlayerNameColour 81u
#define ResourceValueColour 254u
#define MyOriginalCameraColour 208u

CIncome::CIncome(BOOL VidMem):
	Minimised(false)
{
	std::memset(MinimiseWidgetBoxMin, 0, sizeof(MinimiseWidgetBoxMin));
	std::memset(MinimiseWidgetBoxMax, 0, sizeof(MinimiseWidgetBoxMax));

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
		TAdynmemStruct* Ptr = *(TAdynmemStruct**)0x00511de8;
		int targetPosX = GetMinimiseWidgetXPos()
			? Ptr->GameSreen_Rect.right - MinimiseWidgetSize
			: Ptr->GameSreen_Rect.left;

		if (GetCurrentThreadId() == LocalShare->GuiThreadId &&
			(BlitState % 30 == 1 || Minimised && posX != targetPosX))
		{
			DDSURFACEDESC ddsd;
			DDRAW_INIT_STRUCT(ddsd);
			int PlayerDrawn = 0;

			if (Minimised)
			{
				posX += (targetPosX - posX) / 10;
			}

			if(lpIncomeSurf->Lock(NULL, &ddsd, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL)==DD_OK)
			{
				SurfaceMemory = ddsd.lpSurface;
				lPitch = ddsd.lPitch;

				if((0!=(WATCH& (Ptr->Players[LocalShare->OrgLocalPlayerID].PlayerInfo->PropertyMask)))
					||DataShare->PlayingDemo)
				{
					PlayerDrawn = ShowAllIncome();
				}
				else
				{
					PlayerDrawn = ShowAllyIncome();
				}
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
		if (DestSurf->Blt(&Dest, lpIncomeSurf, &Source, DDBLT_ASYNC | DDBLT_KEYSRCOVERRIDE, &ddbltfx) != DD_OK)
		{
			DestSurf->Blt(&Dest, lpIncomeSurf, &Source, DDBLT_WAIT | DDBLT_KEYSRCOVERRIDE, &ddbltfx);
		}
		
		//BlitCursor
		if ((CursorY!=-1)
			&&(CursorX!=-1))
		{
			((Dialog *)LocalShare->Dialog)->BlitCursor ( DestSurf, CursorX, CursorY);
		}
	}
	
}

// replication of F4 scoreboard background
//
//int CIncome::ShowBackground(OFFSCREEN *offscreen, int nPlayers, const char *title)
//{
//	RECT rect;
//	rect.left = 0;
//	rect.top = 0;
//	rect.right = PlayerWidth;
//	rect.bottom = nPlayers * PlayerHight + TitleHight;
//	TADrawTransparentBox(offscreen, &rect, -24);
//	DrawColorTextInScreen(offscreen, title, 0, TitleHight+8, PlayerWidth, 0);
//	LocalShare->Height = rect.bottom;
//	LocalShare->Width = rect.right;
//	return true;
//}
//
//int CIncome::ShowPlayerCard(OFFSCREEN* offscreen, PlayerStruct* player, int nPlayer, bool highlighted)
//{
//	int ypos = nPlayer * PlayerHight + TitleHight;
//	if (highlighted)
//	{
//		RECT rect;
//		rect.left = 4;
//		rect.top = ypos;
//		rect.right = PlayerWidth - 4;
//		rect.bottom = ypos + PlayerHight;
//		TADrawTransparentBox(offscreen, &rect, 31);
//		TADrawTransparentBox(offscreen, &rect, 20);
//	}
//
//	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
//	_GAFFrame *gafFrame = Index2Frame_InSequence(taPtr->_32xLogos, player->PlayerInfo->PlayerLogoColor);
//	POINT source[4];
//	source[0].x = source[0].y = source[1].y = source[3].x = 1;
//	source[1].x = source[2].x = gafFrame->Width;
//	source[2].y = source[3].y = gafFrame->Height;
//	POINT dest[4];
//	dest[0].x = dest[3].x = 8;
//	dest[0].y = dest[1].y = ypos + 4;
//	dest[1].x = dest[2].x = PlayerWidth - 8;
//	dest[2].y = dest[3].y = ypos + PlayerHight - 4;
//	GAF_DrawTransformed(offscreen, gafFrame, dest, source);
//
//	//DrawColorTextInScreen(offscreen, player->Name, 8, ypos + 8, PlayerWidth-16, 0);
//
//	return 1;
//}

int CIncome::GetMinimiseWidgetXPos()
{
	TAdynmemStruct* Ptr = *(TAdynmemStruct**)0x00511de8;
	
	return posX >= Ptr->ScreenWidth / 2
		? PlayerWidth
		: 0;
}

int CIncome::ShowAllIncome()
{
	DataShare->IsRunning = 15;

	if (Minimised)
	{
		TAdynmemStruct* Ptr = *(TAdynmemStruct**)0x00511de8;
		int targetPosX = GetMinimiseWidgetXPos()
			? Ptr->GameSreen_Rect.right - MinimiseWidgetSize
			: Ptr->GameSreen_Rect.left;

		if (std::abs(targetPosX - posX) < 10)
		{
			FillRect(1);
			LocalShare->Width = LocalShare->Height = DrawMinimiseWidget(0, 0, MinimiseWidgetColour);
			return 1;
		}
	}

	if (BackgroundType == 2)
		FillRect(0);
	else
		FillRect(1);

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
	DrawMinimiseWidget(GetMinimiseWidgetXPos(), MinimiseWidgetPosY, MinimiseWidgetColour);

	j++;
	LocalShare->Height = PlayerHight*j;
	return j;
}

int CIncome::ShowAllyIncome()
{
	DataShare->IsRunning = 15;

	if (Minimised)
	{
		TAdynmemStruct* Ptr = *(TAdynmemStruct**)0x00511de8;
		int targetPosX = GetMinimiseWidgetXPos()
			? Ptr->GameSreen_Rect.right - MinimiseWidgetSize
			: Ptr->GameSreen_Rect.left;

		if (std::abs(targetPosX - posX) < 10)
		{
			FillRect(1);
			LocalShare->Width = LocalShare->Height = DrawMinimiseWidget(0, 0, MinimiseWidgetColour);
			return 1;
		}
	}

	if (BackgroundType == 2)
		FillRect(0);
	else
		FillRect(1);

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

	DrawMinimiseWidget(GetMinimiseWidgetXPos(), MinimiseWidgetPosY, MinimiseWidgetColour);

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

	DrawText ( "My Original Camera", posx+30, posy+8, MyOriginalCameraColour);

	for(int i=0; i<4; i++)
	{
		memset(&SurfMem[posx+(posy+24 +i)*lPitch+ 44], 90, 100);
	}
}

void CIncome::ShowPlayerIncome(int Player, int posx, int posy)
{
	float ValueF;
	char Value[100];

	char C = GetPlayerDotColor(Player);

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

	DrawText(DataShare->PlayerNames[Player], posx+44, posy, PlayerNameColour);

	DrawStorageText(DataShare->storedM[Player], posx, posy+10);
	DrawStorageText(DataShare->storedE[Player], posx, posy+20);

	PaintStoragebar(posx+44, posy+10, Player , MetalBar);
	ValueF = DataShare->incomeM[Player];
	sprintf_s(Value, 100, "+%.1f", ValueF);
	DrawText(Value, posx+149, posy+10, ResourceValueColour);

	PaintStoragebar(posx+44, posy+20, Player , EnergyBar);
	ValueF = DataShare->incomeE[Player];
	sprintf_s(Value, 100, "+%.0f", ValueF);
	DrawText(Value, posx+149, posy+20, ResourceValueColour);
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
	DrawText(Value, posx + (5-strlen(Value))*8, posy, ResourceValueColour);

	return 0;
}

void CIncome::DrawText(char* String, int posx, int posy, char Color)
{
	if (BackgroundType == 0)
	{
		for (int dx = -1; dx <= 1; ++dx)
		{
			for (int dy = -1; dy <= 1; ++dy)
			{
				_DrawText(String, posx + dx, posy + dy, 0);
			}
		}
	}
	_DrawText(String, posx, posy, Color);// BackgroundType > 0 ? Color : 255);
}

void CIncome::_DrawText(char *String, int posx, int posy, char Color)
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
				if (b)
				{
					int idx = (posx + (i * 8) + (7 - k)) + (posy + j) * lPitch;
					if (idx >= 0 && idx < PlayerWidth * 11 * PlayerHight)
					{
						SurfMem[idx] = Color;
					}
				}
			}
		}
	}
}

int CIncome::DrawMinimiseWidget(int posx, int posy, char Color)
{
	if (SurfaceMemory == NULL)
		return 0;

	if (posx + MinimiseWidgetSize > PlayerWidth)
	{
		posx = PlayerWidth - MinimiseWidgetSize;
	}

	bool leftArrow = 
		GetMinimiseWidgetXPos() == 0 && !Minimised ||
		GetMinimiseWidgetXPos() > 0 && Minimised;

	char* SurfMem = (char*)SurfaceMemory;
	for (int col = 0; col < MinimiseWidgetSize; ++col)
	{
		int nRows = leftArrow ? 1 + col : MinimiseWidgetSize - col;
		for (int row = 0; row < nRows; ++row)
		{
			int x = posx + col;
			int y = posy + row + (MinimiseWidgetSize - nRows) / 2;
			SurfMem[x + y * lPitch] = Color;
		}
	}
	MinimiseWidgetBoxMin[0] = posX + posx;
	MinimiseWidgetBoxMin[1] = posY + posy;
	MinimiseWidgetBoxMax[0] = posX + posx + MinimiseWidgetSize;
	MinimiseWidgetBoxMax[1] = posY + posy + MinimiseWidgetSize;
	return MinimiseWidgetSize;
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
		if (BackgroundType == 0)
		{
			SurfMem[(posx + i) + (posy - 1) * lPitch] = 0;
			SurfMem[(posx + i) + (posy + 3) * lPitch] = 0;
		}
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
		posX = LocalShare->ScreenWidth - PlayerWidth;
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
				Minimised = false;
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
		case WM_KEYDOWN:
			if ((int)wParam == VK_F4 && LocalShare->Height > 0)
			{
				TAdynmemStruct* Ptr = *(TAdynmemStruct**)0x00511de8;
				bool scoreboardEnabled = Ptr->GameOptionMask & 0x80;
				if (Minimised && !scoreboardEnabled)
				{
					return false;	// let TA enable scoreboard
				}
				else if (Minimised && scoreboardEnabled)
				{
					Minimised = false;
					First = true;	// cause CorrectPos to be called
					return true;	// don't let TA enable scoreboard
				}
				else if (!Minimised && scoreboardEnabled)
				{
					return false;	// let TA disable scoreboard
				}
				else //if (!Minimised && !scoreboardEnabled)
				{
					Minimised = true;
					//posX = GetMinimiseWidgetXPos() ? 65535 : 0;
					//First = true;	// cause CorrectPos to be called
					return true;	// don't let TA enable scoreboard
				}
			}
			break;
		case WM_LBUTTONDBLCLK:
			if (LOWORD(lParam) >= MinimiseWidgetBoxMin[0] && LOWORD(lParam) < MinimiseWidgetBoxMax[0] &&
				HIWORD(lParam) >= MinimiseWidgetBoxMin[1] && HIWORD(lParam) < MinimiseWidgetBoxMax[1])
			{
				Minimised = !Minimised;
				if (Minimised)
				{
					//posX = GetMinimiseWidgetXPos() ? 65535 : 0;
				}
				First = true;	// cause CorrectPos to be called
				return true;
			}
			else if(LOWORD(lParam)>posX && LOWORD(lParam)<(posX+LocalShare->Width) && HIWORD(lParam)>(posY) && HIWORD(lParam)<((posY)+LocalShare->Height))
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
    if (posX > 1+ bounds.right - LocalShare->Width)
        posX = 1+ bounds.right - LocalShare->Width;

    if (posY < bounds.top)
        posY = bounds.top;
    if (posY > 1+ bounds.bottom - LocalShare->Height)
        posY = 1+ bounds.bottom - LocalShare->Height;
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

void CIncome::BlitWeatherReport(LPVOID lpSurfaceMem, int dwWidth, int dwHeight, int lPitch)
{
	if (GetCurrentThreadId() != LocalShare->GuiThreadId || SurfaceMemory == NULL || DataShare->TAProgress != TAInGame) {
		return;
	}

	TAProgramStruct* programPtr = *(TAProgramStruct**)0x0051fbd0;
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	const int timeSeconds = taPtr->GameTime / 30;

	PlayerStruct* player = &taPtr->Players[taPtr->LocalHumanPlayer_PlayerID];
	int raceSide = player->PlayerInfo->RaceSide;
	if (raceSide >= taPtr->RaceCounter) {
		raceSide = 0;
	}
	RaceSideData* race = &taPtr->RaceSideDataAry[raceSide];

	int solar, wind, windMin, windMax, tidal;
	GetWeatherReport(solar, wind, windMin, windMax, tidal);

	int x1 = 110 + std::max(race->ENERGYPRODUCED.right, race->METALPRODUCED.right);
	int x2 = x1 + 117;
	int y1 = race->ENERGYPRODUCED.top;
	int y2 = race->ENERGYCONSUMED.top;
	int y3 = (y1 + y2) / 2;
	const unsigned char GREY = taPtr->desktopGUI.RadarObjecColor[7];
	const unsigned char GREEN = taPtr->desktopGUI.RadarObjecColor[10];

	void* fontHandleBak = programPtr->fontHandle;
	programPtr->fontHandle = (unsigned char*)taPtr->RaceSideDataAry[raceSide].Font_File;
	programPtr->fontFrontColour = GREY;
	programPtr->fontBackColour = programPtr->fontAlpha;

	OFFSCREEN offScreen;
	memset(&offScreen, 0, sizeof(OFFSCREEN));
	offScreen.Height = dwWidth;
	offScreen.Width = lPitch;
	offScreen.lPitch = lPitch;
	offScreen.lpSurface = lpSurfaceMem;
	offScreen.ScreenRect.left = 0;
	offScreen.ScreenRect.right = dwWidth;
	offScreen.ScreenRect.top = 0;
	offScreen.ScreenRect.bottom = dwHeight;

	char windText[32];
	if (DataShare->PlayingDemo || (0 != (WATCH & (taPtr->Players[LocalShare->OrgLocalPlayerID].PlayerInfo->PropertyMask)))) {
		sprintf(windText, "Wind : (%d-%d)", windMin, windMax);
		DrawTextInScreen(&offScreen, windText, x1 - GetTextExtent(programPtr->fontHandle, "Wind : "), y1, -1);
	}
	else {
		sprintf(windText, "Wind : +%d (%d-%d)", wind, windMin, windMax);
		DrawTextInScreen(&offScreen, windText, x1 - GetTextExtent(programPtr->fontHandle, "Wind : "), y1, -1);

		programPtr->fontFrontColour = GREEN;
		sprintf(windText, "+%d", wind);
		DrawTextInScreen(&offScreen, windText, x1, y1, -1);
	}

	char tideText[32];
	programPtr->fontFrontColour = GREY;
	sprintf(tideText, "Tidal :");
	DrawTextInScreen(&offScreen, tideText, x1 - GetTextExtent(programPtr->fontHandle, "Tidal : "), y2, -1);

	programPtr->fontFrontColour = GREEN;
	sprintf(tideText, "+%d", tidal);
	DrawTextInScreen(&offScreen, tideText, x1, y2, -1);

	char clockText[32];
	programPtr->fontFrontColour = GREY;
	sprintf(clockText, "Game Time : %02d:%02d:%02d", (timeSeconds / 3600), (timeSeconds / 60) % 60, timeSeconds % 60);
	DrawTextInScreen(&offScreen, clockText, x2, y3, -1);

	programPtr->fontHandle = (unsigned char*)fontHandleBak;
}
