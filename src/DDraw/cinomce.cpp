#include "config.h"
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
	m_weatherCacheW(0),
	m_weatherCacheH(0),
	m_weatherCacheBlitX(0),
	m_weatherCacheBlitY(0),
	m_weatherCacheAlpha(0),
	m_weatherCacheValid(false),
	BlitState(0),
	SurfaceMemory(NULL),
	lPitch(0),
	StartedInRect(false),
	Minimised(false)
{
	std::memset(&m_weatherCacheKey, 0, sizeof(m_weatherCacheKey));
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
				bool b = 0!=(ThinFont[String[i]*8+j] & (1 << k));//windows���false==0
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
	// NOTE: was checking the CIncome::SurfaceMemory member here, but that
	// tracks lpIncomeSurf's lock state (set inside BlitIncome) and is never
	// touched in skirmish — so an uninitialized non-NULL value would let the
	// weather render in MP and a NULL would block it forever in skirmish.
	// The right thing to gate on is the back-buffer pointer we were handed.
	if (GetCurrentThreadId() != LocalShare->GuiThreadId || lpSurfaceMem == NULL || DataShare->TAProgress != TAInGame) {
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

	// Prefer the production-text rects for vertical alignment — they put the
	// weather rows on the same baseline as the income readouts.  Use the
	// bar rects as a fallback for sides that leave the text rects zeroed
	// (which is what used to shove the weather behind the panel artwork).
	int xRef = std::max(race->Energy_rect.right, race->METALBAR_rect.right);
	if (xRef <= 0)
		xRef = std::max(race->ENERGYPRODUCED.right, race->METALPRODUCED.right);
	if (xRef <= 0)
		xRef = 256;

	int yEnergy = race->ENERGYPRODUCED.top;
	int yMetal  = race->ENERGYCONSUMED.top;
	if (yEnergy <= 0)
		yEnergy = race->Energy_rect.top;
	if (yMetal <= 0)
		yMetal = race->METALBAR_rect.top;
	if (yEnergy < 0) yEnergy = 0;
	if (yMetal  <= yEnergy) yMetal = yEnergy + 16;

	int x1 = 110 + xRef;
	int x2 = x1 + 117;
	int y1 = yEnergy;
	int y2 = yMetal;
	int y3 = (y1 + y2) / 2;
	const unsigned char GREY = taPtr->desktopGUI.RadarObjecColor[7];
	const unsigned char GREEN = taPtr->desktopGUI.RadarObjecColor[10];

	void* fontHandleBak = programPtr->fontHandle;
	programPtr->fontHandle = (unsigned char*)taPtr->RaceSideDataAry[raceSide].Font_File;
	programPtr->fontFrontColour = GREY;
	programPtr->fontBackColour = programPtr->fontAlpha;

	// GetTextExtent of constant strings — recompute on font change only.
	static void* s_cachedFontHandle = NULL;
	static int   s_clockWidth      = 0;
	static int   s_labelWidth      = 0;
	static int   s_windPrefixWidth = 0;
	static int   s_tidalPrefixWidth = 0;
	if (s_cachedFontHandle != programPtr->fontHandle)
	{
		s_clockWidth        = GetTextExtent(programPtr->fontHandle, "Game Time : 00:00:00") + 4;
		s_labelWidth        = GetTextExtent(programPtr->fontHandle, "Wind : +99 (-99-+99)") + 4;
		s_windPrefixWidth   = GetTextExtent(programPtr->fontHandle, "Wind : ");
		s_tidalPrefixWidth  = GetTextExtent(programPtr->fontHandle, "Tidal : ");
		s_cachedFontHandle  = programPtr->fontHandle;
	}

	// Screen-bounds clamp: if a side's bar rects sit too far right, or the
	// resolution is small, shift the columns left so they stay visible.
	const int fontHeight = (programPtr->fontHandle != NULL) ? programPtr->fontHandle[0] : 12;
	const int clockWidth = s_clockWidth;
	const int labelWidth = s_labelWidth;
	const int xMaxClock = (dwWidth > clockWidth) ? (dwWidth - clockWidth) : 0;
	if (x2 > xMaxClock) x2 = xMaxClock;
	const int xMaxLabel = (x2 > labelWidth) ? (x2 - labelWidth) : 0;
	if (x1 > xMaxLabel) x1 = xMaxLabel;
	if (x1 < 0) x1 = 0;
	if (x2 < x1 + 1) x2 = x1 + 1;
	const int yMax = (dwHeight > fontHeight) ? (dwHeight - fontHeight) : 0;
	if (y1 < 0) y1 = 0; else if (y1 > yMax) y1 = yMax;
	if (y2 < 0) y2 = 0; else if (y2 > yMax) y2 = yMax;
	y3 = (y1 + y2) / 2;

	const int watchMode = (DataShare->PlayingDemo
		|| (0 != (WATCH & (taPtr->Players[LocalShare->OrgLocalPlayerID].PlayerInfo->PropertyMask))))
		? 1 : 0;
	const int fontAlpha = programPtr->fontAlpha;

	const bool cacheHit =
		m_weatherCacheValid &&
		m_weatherCacheKey.timeSeconds == timeSeconds &&
		m_weatherCacheKey.wind        == wind &&
		m_weatherCacheKey.windMin     == windMin &&
		m_weatherCacheKey.windMax     == windMax &&
		m_weatherCacheKey.tidal       == tidal &&
		m_weatherCacheKey.raceSide    == raceSide &&
		m_weatherCacheKey.fontHandle  == programPtr->fontHandle &&
		m_weatherCacheKey.watchMode   == watchMode &&
		m_weatherCacheKey.x1          == x1 &&
		m_weatherCacheKey.x2          == x2 &&
		m_weatherCacheKey.y1          == y1 &&
		m_weatherCacheKey.y2          == y2 &&
		m_weatherCacheKey.fontAlpha   == fontAlpha;

	if (!cacheHit)
	{
		const int leftWind  = x1 - s_windPrefixWidth;
		const int leftTidal = x1 - s_tidalPrefixWidth;
		int boxLeft   = leftWind;   if (leftTidal < boxLeft) boxLeft = leftTidal;
		if (x2 < boxLeft) boxLeft = x2;
		if (boxLeft < 0) boxLeft = 0;
		int boxRight  = x1 + s_labelWidth;
		const int clockEnd = x2 + s_clockWidth;
		if (clockEnd > boxRight) boxRight = clockEnd;
		if (boxRight > dwWidth) boxRight = dwWidth;
		int boxTop    = y1; if (y2 < boxTop) boxTop = y2;
		if (boxTop < 0) boxTop = 0;
		int boxBottom = y1 + fontHeight;
		const int y2End = y2 + fontHeight;
		const int y3End = y3 + fontHeight;
		if (y2End > boxBottom) boxBottom = y2End;
		if (y3End > boxBottom) boxBottom = y3End;
		if (boxBottom > dwHeight) boxBottom = dwHeight;

		const int W = (boxRight  > boxLeft) ? (boxRight  - boxLeft) : 0;
		const int H = (boxBottom > boxTop)  ? (boxBottom - boxTop)  : 0;

		if (W > 0 && H > 0)
		{
			m_weatherCache.assign(static_cast<size_t>(W) * H, static_cast<unsigned char>(fontAlpha));
			m_weatherCacheW       = W;
			m_weatherCacheH       = H;
			m_weatherCacheBlitX   = boxLeft;
			m_weatherCacheBlitY   = boxTop;
			m_weatherCacheAlpha   = static_cast<unsigned char>(fontAlpha);

			OFFSCREEN cacheOff;
			memset(&cacheOff, 0, sizeof(OFFSCREEN));
			cacheOff.Height       = W;
			cacheOff.Width        = W;
			cacheOff.lPitch       = W;
			cacheOff.lpSurface    = m_weatherCache.data();
			cacheOff.ScreenRect.left   = 0;
			cacheOff.ScreenRect.right  = W;
			cacheOff.ScreenRect.top    = 0;
			cacheOff.ScreenRect.bottom = H;

			const int rx1 = x1 - boxLeft;
			const int rx2 = x2 - boxLeft;
			const int ry1 = y1 - boxTop;
			const int ry2 = y2 - boxTop;
			const int ry3 = y3 - boxTop;

			char buf[32];
			programPtr->fontFrontColour = GREY;
			if (watchMode)
			{
				sprintf(buf, "Wind : (%d-%d)", windMin, windMax);
				DrawTextInScreen(&cacheOff, buf, rx1 - s_windPrefixWidth, ry1, -1);
			}
			else
			{
				sprintf(buf, "Wind : +%d (%d-%d)", wind, windMin, windMax);
				DrawTextInScreen(&cacheOff, buf, rx1 - s_windPrefixWidth, ry1, -1);
				programPtr->fontFrontColour = GREEN;
				sprintf(buf, "+%d", wind);
				DrawTextInScreen(&cacheOff, buf, rx1, ry1, -1);
			}

			programPtr->fontFrontColour = GREY;
			DrawTextInScreen(&cacheOff, "Tidal :", rx1 - s_tidalPrefixWidth, ry2, -1);

			programPtr->fontFrontColour = GREEN;
			sprintf(buf, "+%d", tidal);
			DrawTextInScreen(&cacheOff, buf, rx1, ry2, -1);

			programPtr->fontFrontColour = GREY;
			sprintf(buf, "Game Time : %02d:%02d:%02d",
				(timeSeconds / 3600), (timeSeconds / 60) % 60, timeSeconds % 60);
			DrawTextInScreen(&cacheOff, buf, rx2, ry3, -1);

			m_weatherCacheKey.timeSeconds = timeSeconds;
			m_weatherCacheKey.wind        = wind;
			m_weatherCacheKey.windMin     = windMin;
			m_weatherCacheKey.windMax     = windMax;
			m_weatherCacheKey.tidal       = tidal;
			m_weatherCacheKey.raceSide    = raceSide;
			m_weatherCacheKey.fontHandle  = programPtr->fontHandle;
			m_weatherCacheKey.watchMode   = watchMode;
			m_weatherCacheKey.x1          = x1;
			m_weatherCacheKey.x2          = x2;
			m_weatherCacheKey.y1          = y1;
			m_weatherCacheKey.y2          = y2;
			m_weatherCacheKey.fontAlpha   = fontAlpha;
			m_weatherCacheValid = true;

			m_weatherRuns.clear();
			m_weatherRuns.reserve(64);
			const unsigned char alpha = m_weatherCacheAlpha;
			const unsigned char* cacheData = m_weatherCache.data();
			for (int y = 0; y < H; ++y)
			{
				const unsigned char* sRow = cacheData + y * W;
				int x = 0;
				while (x < W)
				{
					while (x < W && sRow[x] == alpha) ++x;
					if (x >= W) break;
					const int runStart = x;
					while (x < W && sRow[x] != alpha) ++x;
					WeatherRun run = {
						static_cast<short>(y),
						static_cast<short>(runStart),
						static_cast<short>(x - runStart),
						0
					};
					m_weatherRuns.push_back(run);
				}
			}
		}
		else
		{
			m_weatherCacheValid = false;
			m_weatherRuns.clear();
		}
	}

	// Per-frame: memcpy each opaque run. Bounds-checks are defensive in case
	// the back buffer shrinks before the cache invalidates.
	if (m_weatherCacheValid && !m_weatherRuns.empty())
	{
		const unsigned char* src = m_weatherCache.data();
		unsigned char* dst       = static_cast<unsigned char*>(lpSurfaceMem);
		const int W  = m_weatherCacheW;
		const int dx = m_weatherCacheBlitX;
		const int dy = m_weatherCacheBlitY;
		const size_t runCount = m_weatherRuns.size();
		const WeatherRun* runs = m_weatherRuns.data();
		for (size_t i = 0; i < runCount; ++i)
		{
			const WeatherRun& run = runs[i];
			const int dstY = dy + run.row;
			if (dstY < 0 || dstY >= dwHeight) continue;
			int dstX = dx + run.startX;
			int len  = run.length;
			const unsigned char* sPtr = src + run.row * W + run.startX;
			if (dstX < 0)
			{
				const int skip = -dstX;
				if (skip >= len) continue;
				sPtr += skip;
				len  -= skip;
				dstX = 0;
			}
			if (dstX + len > dwWidth)
			{
				len = dwWidth - dstX;
				if (len <= 0) continue;
			}
			std::memcpy(dst + dstY * lPitch + dstX, sPtr, static_cast<size_t>(len));
		}
	}

	programPtr->fontHandle = (unsigned char*)fontHandleBak;
}
