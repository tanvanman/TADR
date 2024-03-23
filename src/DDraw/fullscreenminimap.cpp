// .text:00483638 0B4 8B D8                                                           mov     ebx, eax
// 	.text:0048363A 0B4 8B 03                                                           mov     eax, [ebx]
// .text:0048363C 0B4 3D 20 10 00 00                                                  cmp     eax, 4128
#include "oddraw.h"


#include "hook/hook.h"
#include "hook/etc.h"
#include "tamem.h"
#include "tafunctions.h"
#include "mapParse.h"
#include "gameredrawer.h"
#include "UnitDrawer.h"
#include "mappedmap.h"
#include "PCX.H"

#include "megamaptastuff.h "
#include "ProjectilesMap.h"
#include "MegamapControl.h"
#include "fullscreenminimap.h"
#include "dialog.h"
#include "gaf.h"
#include "iddrawsurface.h"

#include <vector>
using namespace std;
#include "TAConfig.h"

#ifdef USEMEGAMAP

int __stdcall LoadMap_Routine (PInlineX86StackBuffer X86StrackBuffer)
{
	TNTHeaderStruct * TNTPtr= (TNTHeaderStruct *)X86StrackBuffer->Eax;
	FullScreenMinimap * thethis= (FullScreenMinimap * )(X86StrackBuffer->myInlineHookClass_Pish->ParamOfHook);

	
	thethis->InitMinimap ( TNTPtr);
	return 0;
}

FullScreenMinimap::FullScreenMinimap(BOOL Doit, int FPSlimit)
{
	IDDrawSurface::OutptTxt ( "FullScreenMinimap init");
	LoadMap_hook= NULL;
	MyMinimap_p= NULL;
	GameDrawer= NULL;
	UnitsMap= NULL;
	Mapped_p= NULL;
	ProjectilesMap_p= NULL;
	TAStuff= NULL;

	KeepActive= NULL;
	KeepActive1= NULL;
	DrawTAScreen_hok= NULL;
	DrawTAScreenEnd_hok= NULL;


	MegamapVirtualKey= VK_TAB;

	Blit_b= FALSE;
	Flipping= FALSE;



	Controler= NULL;


	VidMem= FALSE;

	DrawBackground=  TRUE;
	DrawMapped= TRUE;
	DrawProjectile=  TRUE;
	DrawUnits= TRUE;
	DrawMegamapRect=  TRUE;
	DrawMegamapBlit=  TRUE;
	DrawSelectAndOrder= TRUE;
	DrawMegamapCursor=  TRUE;
	DrawMegamapTAStuff= TRUE;

	DoubleClickMoveMegamap=  FALSE;
	WheelMoveMegaMap= TRUE;
	WheelZoom=  TRUE;

	UseSurfaceCursor= FALSE;

	MaxIconWidth= ICONMAXWIDTH;
	MaxIconHeight= ICONMAXHEIGHT;



	memset ( &MegamapRect, 0, sizeof(RECT) );
	memset ( &MegaMapInscren, 0, sizeof(RECT) );
	memset ( &TAMAPTAPos, 0, sizeof(RECT) );

	MegamapWidth= 0;
	MegamapHeight= 0;

	Do_b= Doit;

	megamapFps = FPSlimit;
	if (Doit)
	{
		
		LoadMap_hook= new InlineSingleHook ( LoadMap_Addr , 5, INLINE_5BYTESLAGGERJMP, LoadMap_Routine);
		
		LoadMap_hook->SetParamOfHook ( (LPVOID)this);
		GameDrawer= new TAGameAreaReDrawer;
		
	}



	
	DrawTAScreen_hok= new InlineSingleHook ( (unsigned int)DrawGameScreen_Addr, 5, 
		INLINE_5BYTESLAGGERJMP, BlockTADraw);

	DrawTAScreen_hok->SetParamOfHook ( reinterpret_cast<LPVOID>(this));


	DrawTAScreenBlit_hok= new InlineSingleHook ( (unsigned int)DrawTAScreenBlitAddr, 5, 
		INLINE_5BYTESLAGGERJMP, ForceTADrawBlit);

	DrawTAScreenBlit_hok->SetParamOfHook ( reinterpret_cast<LPVOID>(this));


	

	//DrawTAScreenEnd_hok= new InlineSingleHook ( (unsigned int)DrawGameScreenEnd_Addr, 5, 
	//	INLINE_5BYTESLAGGERJMP, DischargeTADraw);

	//DrawTAScreenEnd_hok->SetParamOfHook ( reinterpret_cast<LPVOID>(this));


}


FullScreenMinimap::~FullScreenMinimap (void)
{

	if (NULL!=LoadMap_hook)
	{
		delete LoadMap_hook;
	}
	if (NULL!=MyMinimap_p)
	{
		delete MyMinimap_p;
	}
	if (NULL!=GameDrawer)
	{
		delete GameDrawer;
	}

	if (NULL!=Mapped_p)
	{
		delete Mapped_p;
	}

	if (NULL!=ProjectilesMap_p)
	{
		delete ProjectilesMap_p;
	}

	if (NULL!=Controler)
	{
		delete Controler;
	}

	if (TAStuff)
	{
		delete TAStuff;

	}
	if(DrawTAScreen_hok)
	{
		delete DrawTAScreen_hok;
		DrawTAScreen_hok= NULL;
	}
	if(DrawTAScreenBlit_hok)
	{
		delete DrawTAScreenBlit_hok;
		DrawTAScreenBlit_hok= NULL;
	}

	
// 
// 	if (DrawTAScreenEnd_hok)
// 	{
// 		delete DrawTAScreenEnd_hok;
// 		DrawTAScreenEnd_hok= NULL;
// 	}

	if (KeepActive)
	{
		delete KeepActive;
		KeepActive= NULL;

	}
	if (KeepActive1)
	{
		delete KeepActive1;
		KeepActive1= NULL;
	}
}

void FullScreenMinimap::InitMinimap (TNTHeaderStruct * TNTPtr, RECT * GameScreen)
{
	if (!(IDirectDraw*)LocalShare->TADirectDraw)
	{
		return ;
	}

	QuitMegaMap ( );
	if (MyMinimap_p)
	{
		delete MyMinimap_p;
		MyMinimap_p= NULL;
	}

// 	if (NULL==GameDrawer->GameAreaSurfaceFront_ptr)
// 	{
// 		GameDrawer->InitOwnSurface ( (IDirectDraw*)LocalShare->TADirectDraw, ! VsyncOn);
// 	}
// 	
	if (NULL==GameScreen)
	{
		GameScreen= GameDrawer->TAWGameAreaRect ( NULL);
	}

	int GameWidth= GameScreen->right- GameScreen->left;
	int GameHeight= GameScreen->bottom- GameScreen->top;

	POINT MinimapAspect;

	MyMinimap_p= new TNTtoMiniMap ( GameWidth, GameHeight);

	if (MyMinimap_p)
	{
		MyMinimap_p->MapFromTNTInMem ( reinterpret_cast<LPVOID>(TNTPtr));

		MyMinimap_p->PictureInfo ( NULL, &MinimapAspect);

		GameDrawer->TAWGameAreaRect ( &MegaMapInscren);

		MegamapWidth= MinimapAspect.x;
		MegamapHeight= MinimapAspect.y;

		MegamapRect.right= MegaMapInscren.right- MegaMapInscren.left;
		MegamapRect.bottom= MegaMapInscren.bottom- MegaMapInscren.top;
		MegamapRect.top= 0;
		MegamapRect.left= 0;

		if ((MegamapWidth<MegamapRect.right)
			&&(2<(MegamapRect.right- MegamapWidth)))
		{
			MegamapRect.left= (MegamapRect.right- MegamapWidth)/ 2;
			MegamapRect.right= MegamapRect.left+ MegamapWidth;
		}

		if ((MegamapHeight<MegamapRect.bottom)
			&&(2<(MegamapRect.bottom- MegamapHeight)))
		{
			MegamapRect.top= (MegamapRect.bottom- MegamapHeight)/ 2;
			MegamapRect.bottom= MegamapRect.top+ MegamapHeight;
		}

		MegaMapInscren.top= MegaMapInscren.top+ MegamapRect.top;
		MegaMapInscren.bottom= MegaMapInscren.top+ MegamapHeight;
		MegaMapInscren.left= MegaMapInscren.left+ MegamapRect.left;
		MegaMapInscren.right= MegaMapInscren.left+ MegamapWidth;

		TAMAPTAPos.left= 0;
		TAMAPTAPos.top= 0;
		TAMAPTAPos.right= (TNTPtr->Width- 1)* 16;
		TAMAPTAPos.bottom= (TNTPtr->Height- 4)* 16;


		if (UnitsMap)
		{
			delete UnitsMap;
			UnitsMap= NULL; 
		}

		DrawBackground= MyConfig->GetIniBool ( "DrawBackground", TRUE);
		DrawMapped= MyConfig->GetIniBool ( "DrawMapped", TRUE);
		DrawProjectile= MyConfig->GetIniBool ( "DrawProjectile", TRUE);
		DrawUnits= MyConfig->GetIniBool ( "DrawUnits", TRUE);
		DrawMegamapRect= MyConfig->GetIniBool ( "DrawMegamapRect", TRUE);
		DrawMegamapBlit= MyConfig->GetIniBool ( "DrawMegamapBlit", TRUE);
		DrawSelectAndOrder= MyConfig->GetIniBool ( "DrawSelectAndOrder", TRUE);
		DrawMegamapCursor= MyConfig->GetIniBool ( "DrawMegamapCursor", TRUE);

		DrawMegamapTAStuff= MyConfig->GetIniBool ( "DrawMegamapTAStuff", TRUE);

		DoubleClickMoveMegamap=  MyConfig->GetIniBool ( "DoubleClickMoveMegamap", FALSE);
		WheelMoveMegaMap= MyConfig->GetIniBool ( "WheelMoveMegaMap", TRUE);
		WheelZoom= MyConfig->GetIniBool ( "WheelZoom", TRUE);


		UseSurfaceCursor= MyConfig->GetIniBool ( "UseSurfaceCursor", FALSE);


		MaxIconWidth= MyConfig->GetIniInt ( "MaxIconWidth", ICONMAXWIDTH) ;
		MaxIconHeight=  MyConfig->GetIniInt ( "MaxIconHeight", ICONMAXHEIGHT) ;
		
		UnitsMap= new UnitsMinimap (  this, MinimapAspect.x, MinimapAspect.y,
			MyConfig->GetIniInt ( "MegamapRadarMinimum", -1),
			MyConfig->GetIniInt ( "MegamapSonarMinimum", -1),
			MyConfig->GetIniInt ( "MegamapSonarJamMinimum", -1),
			MyConfig->GetIniInt ( "MegamapRadarJamMinimum", -1),
			MyConfig->GetIniInt ( "MegamapAntiNukeMinimum", -1),
			MyConfig->GetIniInt ( "MegamapWeapon1Color", -1),
			MyConfig->GetIniInt ( "MegamapWeapon2Color", -1),
			MyConfig->GetIniInt ( "MegamapWeapon3Color", -1),
			MyConfig->GetIniInt ( "MegamapRadarColor", -1),
			MyConfig->GetIniInt ( "MegamapSonarColor", -1),
			MyConfig->GetIniInt ( "MegamapRadarJamColor", -1),
			MyConfig->GetIniInt ( "MegamapSonarJamColor", -1),
			MyConfig->GetIniInt ( "MegamapAntinukeColor", -1),
			MyConfig->GetIniBool ( "UnderAttackFlash", FALSE),
			DataShare->PlayerDotColors);
		//UnitsMap->InitSurface ( (IDirectDraw*)LocalShare->TADirectDraw, !VsyncOn);


		if (Mapped_p)
		{
			delete Mapped_p;
			Mapped_p= NULL;
		}
		Mapped_p= new MappedMap ( MinimapAspect.x, MinimapAspect.y);

		if (NULL!=Controler)
		{
			Controler->Init ( this, &MegaMapInscren, &TAMAPTAPos, GameScreen, MaxIconWidth, MaxIconHeight, MegamapVirtualKey, WheelMoveMegaMap, DoubleClickMoveMegamap, WheelZoom);
		}
		else
		{
			Controler= new MegaMapControl ( this, &MegaMapInscren, &TAMAPTAPos, GameScreen, MaxIconWidth, MaxIconHeight, MegamapVirtualKey, WheelMoveMegaMap, DoubleClickMoveMegamap, WheelZoom);
		}

		if (TAStuff)
		{
			TAStuff->Init ( this, &MegaMapInscren, &TAMAPTAPos, GameScreen, MaxIconWidth, MaxIconHeight, UseSurfaceCursor);
		}
		else
		{
			TAStuff= new MegamapTAStuff ( this, &MegaMapInscren, &TAMAPTAPos, GameScreen, MaxIconWidth, MaxIconHeight, UseSurfaceCursor);
		}

		if (ProjectilesMap_p)
		{
			delete ProjectilesMap_p;
		}
		if (UnitsMap)
		{
			ProjectilesMap_p= new ProjectileMap ( this, UnitsMap);
		}

		//
		InitSurface ( (IDirectDraw*)LocalShare->TADirectDraw);
	}
}

void FullScreenMinimap::Set (int VirtualKey)
{
	MegamapVirtualKey= VirtualKey;
	//VidMem= VidMem_a;

	if (Controler)
	{
		Controler->Set ( VirtualKey);
	}
}
void FullScreenMinimap::SetVid (BOOL VidMem_a)
{
	
	VidMem= VidMem_a;


}

void FullScreenMinimap::Blit(LPDIRECTDRAWSURFACE DestSurf)
{
	static DWORD LastTick= 0;
	static DWORD CurrentTick= 0;;

	if (Do_b
		&&Blit_b)
	{
		if (gameingstate::EXITGAME==(*TAmainStruct_PtrPtr)->GameStateMask)
		{
			return ;
		}
		if (TAInGame==DataShare->TAProgress)
		{
			//IDDrawSurface::OutptTxt ( "FullScreenMinimap blit");



			CurrentTick= GetTickCount ( );
			//i++;

			if (!megamapFps || (LastTick+ 1000/megamapFps)<CurrentTick)
			{//
				if (!Flipping)
				{
					Flipping= TRUE;

					LastTick= CurrentTick;
					//i= 0;
					POINT Aspect;
					LPBYTE PixelBits;

					if (MyMinimap_p)
					{
						do 
						{
							__try
							{
								if (DrawBackground)
								{
									MyMinimap_p->PictureInfo ( &PixelBits, &Aspect);
								}

								UpdateTAProcess ( );
								if (TAInGame!=DataShare->TAProgress)
								{
									break;
								}

								if (DrawMapped)
								{
									if (! Mapped_p->NowDrawMapped ( PixelBits, &Aspect))
									{
										break;
									}
									
								}
								Mapped_p->PictureInfo (  &PixelBits, &Aspect);

								UpdateTAProcess ( );
								if (TAInGame!=DataShare->TAProgress)
								{
									break;
								}
								if (DrawProjectile)
								{
									ProjectilesMap_p->NowDrawProjectile ( PixelBits, &Aspect);
								}

								UpdateTAProcess ( );

								if (TAInGame!=DataShare->TAProgress)
								{
									break;
								}
								if (DrawUnits)
								{
									UnitsMap->NowDrawUnits ( PixelBits, &Aspect);
								}
							}
							__except (true)
							{
								;
							}
						} while (false);
				
						if (DrawMegamapRect)
						{
							if (DrawUnits)
							{
								GameDrawer->MixDSufInBlit ( &MegamapRect, UnitsMap->GetSurface ( ) , NULL);
							}
							else
							{
								GameDrawer->MixBitsInBlit ( &MegamapRect, PixelBits, &Aspect, NULL);
							}
						}

						//}

						GameDrawer->Flip ( );
					}
			
					Flipping= FALSE;
				}
			}

			if (DrawMegamapBlit)
			{
				GameDrawer->BlitTAGameArea ( DestSurf);
			}
			if (DrawMegamapTAStuff||DrawSelectAndOrder)
			{
				TAStuff->BlitTAGameStuff (DestSurf, DrawMegamapTAStuff, DrawSelectAndOrder);
			}

			if (DrawMegamapCursor)
			{
				DDSURFACEDESC ddsd;
				DDRAW_INIT_STRUCT(ddsd);

				if(UseSurfaceCursor)
				{
					BlitSurfaceCursor ( DestSurf);
				}
				else if (DD_OK==DestSurf->Lock ( NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL))
				{
					unsigned int CursorX= Controler->PubCursorX;
					unsigned int CursorY= Controler->PubCursorY;

					if ((CursorY!=-1)
						&&(CursorX!=-1))
					{
						TAStuff->DrawCursor ( ddsd.lpSurface, ddsd.dwWidth, ddsd.dwHeight, ddsd.lPitch,
							CursorX, CursorY);
					}
				}
				DestSurf->Unlock ( NULL);
			}
		}
	}
}

void FullScreenMinimap::LockBlit_MEGA (LPVOID lpSurfaceMem, int dwWidth,int dwHeight, int lPitch)
{
	if (Do_b
		&&Blit_b)
	{
		if (gameingstate::EXITGAME==(*TAmainStruct_PtrPtr)->GameStateMask)
		{
			return ;
		}

		if (TAInGame==DataShare->TAProgress)
		{
			TAStuff->LockBlit_MEGA ( lpSurfaceMem, dwWidth, dwHeight, lPitch);
		}
	}
}

void FullScreenMinimap::LockBlit_TA (LPVOID lpSurfaceMem, int dwWidth,int dwHeight, int lPitch)
{
	if (Do_b
		&&Blit_b
		&&DrawMegamapTAStuff)
	{
		if (gameingstate::EXITGAME==(*TAmainStruct_PtrPtr)->GameStateMask)
		{
			return ;
		}
		if (TAInGame==DataShare->TAProgress)
		{
			TAStuff->LockBlit_TA ( lpSurfaceMem, dwWidth, dwHeight, lPitch);
		}
	}
}




void FullScreenMinimap::LockBlit (LPVOID lpSurfaceMem, int dwWidth,int dwHeight, int lPitch)
{
	if (Do_b
		&&Blit_b
		&&DrawMegamapTAStuff)
	{
		if (TAInGame==DataShare->TAProgress)
		{
			TAStuff->LockBlit ( lpSurfaceMem, dwWidth, dwHeight, lPitch);
		}
	}
}

void FullScreenMinimap::BlitSurfaceCursor (LPDIRECTDRAWSURFACE DestSurf)
{
	if (! UseSurfaceCursor)
	{
		return ;
	}
	int CursorX= Controler->PubCursorX;
	int CursorY= Controler->PubCursorY;

	if (DrawMegamapCursor)
	{
		if ((CursorY!=-1)
			&&(CursorX!=-1))
		{
			if (Controler==NULL||(! Controler->IsDrawRect ( TRUE)))
			{
					TAStuff->DrawCursor ( DestSurf, CursorX,  CursorY);
			}
		}
	}
}
void FullScreenMinimap::InitSurface (LPDIRECTDRAW TADD, BOOL VidMem)
{
	if (Do_b
		&&(TALobby!=DataShare->TAProgress)
		&&MyMinimap_p)
	{
		if (UnitsMap)
		{
			POINT MinimapAspect;
			MyMinimap_p->PictureInfo ( NULL, &MinimapAspect);
			UnitsMap->ReSet ( MinimapAspect.x, MinimapAspect.y);

			UnitsMap->InitSurface ( TADD, VidMem);
		}

		if (GameDrawer)
		{
			GameDrawer->InitOwnSurface ( TADD, VidMem);
		}


		if (TAStuff)
		{
			TAStuff->InitSurface ( TADD, VidMem);
		}
	}
}
void FullScreenMinimap::InitSurface (LPDIRECTDRAW TADD)
{
	InitSurface ( TADD, VidMem);
}

void FullScreenMinimap::ReleaseSurface (void) 
{
	if (GameDrawer)
	{
		GameDrawer->ReleaseSurface ( );
		
	}
	if (UnitsMap)
	{
		UnitsMap->ReleaseSurface ( );
	}

	if (TAStuff)
	{
		TAStuff->ReleaseSurface();
	}
}

bool FullScreenMinimap::Message(HWND WinProcWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (Do_b)
	{
		if (TAInGame==DataShare->TAProgress)
		{

			if (Controler)
			{
				return Controler->Message ( WinProcWnd,  Msg,  wParam,  lParam);
			}
		}
	}
	return false;
}

void FullScreenMinimap::EnterMegaMap ()
{
	Blit_b= TRUE;


}
void FullScreenMinimap::QuitMegaMap ( )
{
	Blit_b= FALSE;
}

void FullScreenMinimap::BlockGUIState ( )
{
/*
	BYTE KeepActiveBits[]= {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90};
	BYTE Rtn_buf[]= { 0xC2, 0x0C, 0x00 };
	KeepActive= new SingleHook ( (unsigned int)0x4AB0B0, sizeof(Rtn_buf), INLINE_UNPROTECTEVINMENT, Rtn_buf);
	KeepActive1= new SingleHook ( KeepActiveAddr1, sizeof(KeepActiveBits), INLINE_UNPROTECTEVINMENT, KeepActiveBits);*/
}
void FullScreenMinimap::DischargeGUIState ( )
{
/*
	if (KeepActive)
	{
		delete KeepActive;

	}
	if (KeepActive1)
	{
		delete KeepActive1;
	}*/
}

/*
.text:004699D3 224 8B 8C 24 28 02 00 00                                            mov     ecx, [esp+224h+IsDrawUnit_b]
.text:004699DA 224 85 C9                                                           test    ecx, ecx
	.text:004699DC 224 74 27                                                           jz      short loc_469A05


	.text:00469B02 224 8B 9C 24 28 02 00 00                                            mov     ebx, [esp+224h+IsDrawUnit_b]
.text:00469B09 224 85 DB                                                           test    ebx, ebx
	.text:00469B0B 224 0F 84 BF 00 00 00                                               jz      loc_469BD0



	01 224 85 DB                                                           test    ebx, ebx
	.text:00469C03 224 0F 84 2F 01 00 00                                               jz      loc_469D38


xt:00469D6D 224 85 DB                                                           test    ebx, ebx
   .text:00469D6F 224 74 22                                                           jz      short loc_469D93

xt:00469D85 224 85 DB                                                           test    ebx, ebx
   .text:00469D87 224 74 0A                                                           jz      short loc_469D93


ext:00469DB4 224 85 DB                                                           test    ebx, ebx
	.text:00469DB6 224 75 0A                                                           jnz     short loc_469DC2

	test    ebx, ebx
	.text:00469FC4 224 74 0F                                                           jz      short loc_469FD5


	test    ebx, ebx
	.text:00469FE4 224 0F 84 1D 01 00 00                                               jz      loc_46A107


	test    ebx, ebx
	.text:0046A31E 224 0F 84 99 00 00 00                                               jz      loc_46A3BD

	test    ebx, ebx
	.text:0046A3CE 224 74 10                                                           jz      short loc_46A3E0
*/

int __stdcall BlockTADraw (PInlineX86StackBuffer X86StrackBuffer)
{
	FullScreenMinimap * this_me= (FullScreenMinimap *)(X86StrackBuffer->myInlineHookClass_Pish->ParamOfHook);


	if (this_me->IsBliting ( ))
	{
		//()(X86StrackBuffer->Esp+ 4)
		//this_me->BlockGUIState ( );

		*(LPDWORD)(X86StrackBuffer->Esp+4 )= 0;
		//*(LPDWORD)(X86StrackBuffer->Esp+8 )= 0;
		X86StrackBuffer->Esp-= 0x214;
		return X86STRACKBUFFERCHANGE;
	}

	return 0;
}


int __stdcall ForceTADrawBlit (PInlineX86StackBuffer X86StrackBuffer)
{
	FullScreenMinimap * this_me= (FullScreenMinimap *)(X86StrackBuffer->myInlineHookClass_Pish->ParamOfHook);


	if (this_me->IsBliting ( ))
	{
		

		X86StrackBuffer->Eax= *(LPDWORD)(X86StrackBuffer->Esp+ 0x22c);
		//X86StrackBuffer->rtnAddr_Pvoid= ;
		return X86STRACKBUFFERCHANGE;
	}

	return 0;
}

int __stdcall DischargeTADraw (PInlineX86StackBuffer X86StrackBuffer)
{
	FullScreenMinimap * this_me= (FullScreenMinimap *)(X86StrackBuffer->myInlineHookClass_Pish->ParamOfHook);


	if (this_me->IsBliting ( ))
	{
		this_me->DischargeGUIState ( );
		//return X86STRACKBUFFERCHANGE;
	}

	return 0;
}

BOOL FullScreenMinimap::IsBliting()
{
	return Blit_b;
}

#endif