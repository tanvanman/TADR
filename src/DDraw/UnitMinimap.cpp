#include "config.h"
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

#include "fullscreenminimap.h"
#include "MegamapControl.h"
#include "dialog.h"
#include "gaf.h"
#include "iddrawsurface.h"

#include <cstring>
#include <vector>
using namespace std;
#include "TAConfig.h"
#include "ExternQuickKey.h"


#if USEMEGAMAP

//hook .text:00466DC0 000 83 EC 1C                                                        sub     esp, 1Ch
//  .text:00466E83 02C 0F BF 43 6C      movsx   eax, word ptr [ebx+(UnitsInGame.UnitPosition.X+2)] ; //0x6A

// 
// int __stdcall ReSet_proc (PInlineX86StackBuffer X86StrackBuffer)
// {
// 	//GUIExpander->myMinimap->Uni
// 	UnitsMinimap * class_ptr= (UnitsMinimap *)X86StrackBuffer->myInlineHookClass_Pish->ParamOfHook;
// 	POINT Aspect;
// 	LPBYTE PixelBits;
// 	if (class_ptr)
// 	{
// 		class_ptr->PictureInfo ( &PixelBits, &Aspect);
// 
// 		if (NULL==PixelBits)
// 		{
// 			RECT * GameRect= &(*TAmainStruct_PtrPtr)->GameSreen_Rect;
// 			Aspect.x= GameRect->right- GameRect->left;
// 			Aspect.y= GameRect->bottom- GameRect->top;
// 			PixelBits= class_ptr->Init ( Aspect.x, Aspect.y);
// 		}
// 		memset ( PixelBits, 0xffffff, Aspect.x* Aspect.y);
// 	}
// 
// 	
// 	return 0;
// }
// 
// int __stdcall drawunit_proc (PInlineX86StackBuffer X86StrackBuffer)
// {
// 	//
// 	UnitsMinimap * class_ptr= (UnitsMinimap *)X86StrackBuffer->myInlineHookClass_Pish->ParamOfHook;
// 
// 	UnitStruct * unitPtr= (UnitStruct *)X86StrackBuffer->Ebx;
// 
// 	if (class_ptr)
// 	{
// 		class_ptr->DrawUnit (  unitPtr->PlayerStruct_index, unitPtr->XPos, unitPtr->YPos);
// 	}
// 	
// 	return 0;
//}
// 
// DWORD WINAPI UnitMapDrawerProc( LPVOID argc)
// {
// 	IDDrawSurface::OutptTxt ( "UnitMapDrawer Thread Start");
// 	UnitsMinimap * class_ptr= (UnitsMinimap *)argc;
// 	if (class_ptr)
// 	{
// 		POINT Aspect;
// 		LPBYTE PixelBits;
// 		while (true)
// 		{
// 			if ((TAInGame==DataShare->TAProgress))
// 			{
// 				class_ptr->LockOn ( &PixelBits, &Aspect);
// 
// 				if (NULL!=PixelBits)
// 				{
// 					memset ( PixelBits, 0xffffff, Aspect.x* Aspect.y);
// 
// 					UnitStruct * unitPtr= (*TAmainStruct_PtrPtr)->OwnUnitunitPtr;
// 					UnitStruct * unitEndPtr= (*TAmainStruct_PtrPtr)->EndOfUnitsArray_p;
// 
// 					for (; unitPtr!=unitEndPtr; ++unitPtr)
// 					{
// 						class_ptr->DrawUnit (  PixelBits, &Aspect, unitPtr->PlayerStruct_index, unitPtr->XPos, unitPtr->YPos);
// 					}
// 				}
// 				class_ptr->Unlock ( PixelBits);
// 			}
// 			Sleep ( 500); 
// 		}
// 	}
// 	return 0;
// }

UnitsMinimap::UnitsMinimap (FullScreenMinimap * inheritFrom, int Width, int Height,	
	int argMegamapRadarMinimum,
	int argMegamapSonarMinimum,
	int argMegamapSonarJamMinimum,
	int argMegamapRadarJamMinimum,
	int argMegamapAntiNukeMinimum,
	int argMegamapWeapon1Color,
	int argMegamapWeapon2Color,
	int argMegamapWeapon3Color,
	int argMegamapRadarColor,
	int argMegamapSonarColor,
	int argMegamapRadarJamColor,
	int argMegamapSonarJamColor,
	int argMegamapAntinukeColor,
	BOOL argUnderAttackFlash, 
	int * argPlayerDotColor
	)
{
	parent= inheritFrom;

	Init ( 		 argMegamapRadarMinimum,
		argMegamapSonarMinimum,
		argMegamapSonarJamMinimum,
		argMegamapRadarJamMinimum,
		argMegamapAntiNukeMinimum,
		argMegamapWeapon1Color,
		argMegamapWeapon2Color,
		argMegamapWeapon3Color,
		argMegamapRadarColor,
		argMegamapSonarColor,
		argMegamapRadarJamColor,
		argMegamapSonarJamColor,
		argMegamapAntinukeColor,
		argUnderAttackFlash,
		argPlayerDotColor);
	ReSet ( Width, Height);
	// LoadUnitPicture ( );
	// 	
	// 	ReSet_hook= new InlineSingleHook ( 0x0466DC0, 0x5, INLINE_5BYTESLAGGERJMP, ReSet_proc);
	// 	DrawUnit_hook= new InlineSingleHook ( 0x0466E83, 0x5, INLINE_5BYTESLAGGERJMP, drawunit_proc);
}

UnitsMinimap::UnitsMinimap (FullScreenMinimap * inheritFrom,	
	int argMegamapRadarMinimum,
	int argMegamapSonarMinimum,
	int argMegamapSonarJamMinimum,
	int argMegamapRadarJamMinimum,
	int argMegamapAntiNukeMinimum,
	int argMegamapWeapon1Color,
	int argMegamapWeapon2Color,
	int argMegamapWeapon3Color,
	int argMegamapRadarColor,
	int argMegamapSonarColor,
	int argMegamapRadarJamColor,
	int argMegamapSonarJamColor,
	int argMegamapAntinukeColor,
	BOOL argUnderAttackFlash, 
	int * argPlayerDotColor)
{
	parent= inheritFrom;
	

	Init ( 		 argMegamapRadarMinimum,
		argMegamapSonarMinimum,
		argMegamapSonarJamMinimum,
		argMegamapRadarJamMinimum,
		argMegamapAntiNukeMinimum,
		argMegamapWeapon1Color,
		argMegamapWeapon2Color,
		argMegamapWeapon3Color,
		argMegamapRadarColor,
		argMegamapSonarColor,
		argMegamapRadarJamColor,
		argMegamapSonarJamColor,
		argMegamapAntinukeColor,
		argUnderAttackFlash,
		argPlayerDotColor);
	ReSet ( 0, 0);
	// LoadUnitPicture ( );
	// 	ReSet_hook= new InlineSingleHook ( 0x0466DC0, 0x5, INLINE_5BYTESLAGGERJMP, ReSet_proc);
	// 	DrawUnit_hook= new InlineSingleHook ( 0x0466E83, 0x5, INLINE_5BYTESLAGGERJMP, drawunit_proc);
	//	ReSet_hook->SetParamOfHook ( (LPVOID)this);
}

UnitsMinimap::~UnitsMinimap()
{
	ReSet ( 0, 0);

	FreeUnitPicture ( );
	//	if (DrawUnit_hook)
	// 	{
	// 		delete DrawUnit_hook;
	// 	}
	// 	if (ReSet_hook)
	// 	{
	// 		delete ReSet_hook;
	// 	}
}

void UnitsMinimap::Init (	
	int argMegamapRadarMinimum,
	int argMegamapSonarMinimum,
	int argMegamapSonarJamMinimum,
	int argMegamapRadarJamMinimum,
	int argMegamapAntiNukeMinimum,
	int argMegamapWeapon1Color,
	int argMegamapWeapon2Color,
	int argMegamapWeapon3Color,
	int argMegamapRadarColor,
	int argMegamapSonarColor,
	int argMegamapRadarJamColor,
	int argMegamapSonarJamColor,
	int argMegamapAntinukeColor,
	BOOL argUnderAttackFlash, 
	int * argPlayerDotColor)
{
	UnitsMapSfc= NULL;
	Inited= FALSE;

	UseCircleHover= FALSE;

	MaskNum= 0;
	UnknowMaskIndex= 0;


	MegamapRadarMinimum= 0;
	MegamapSonarMinimum= 0;
	MegamapSonarJamMinimum= 0;
	MegamapRadarJamMinimum= 0;
	MegamapAntiNukeMinimum= 0x200;

	MegamapWeapon1Color =(*TAmainStruct_PtrPtr)->desktopGUI.RadarObjecColor[6];
	MegamapWeapon2Color =1;
	MegamapWeapon3Color =MegamapWeapon2Color ;
	MegamapRadarColor =(*TAmainStruct_PtrPtr)->desktopGUI.RadarObjecColor[0xa] ;
	MegamapSonarColor =MegamapRadarColor ;
	MegamapRadarJamColor =(*TAmainStruct_PtrPtr)->desktopGUI.RadarObjecColor[0xc] ;
	MegamapSonarJamColor =MegamapRadarJamColor ;
	MegamapAntinukeColor =(*TAmainStruct_PtrPtr)->desktopGUI.RadarObjecColor[0xf];

	if (-1!=argMegamapRadarMinimum)
	{
		MegamapRadarMinimum= argMegamapRadarMinimum;
	}
	if (-1!=argMegamapSonarMinimum)
	{
		MegamapSonarMinimum= argMegamapSonarMinimum;
	}
	if (-1!=argMegamapSonarJamMinimum)
	{
		MegamapSonarJamMinimum= argMegamapSonarJamMinimum;
	}
	if (-1!=argMegamapRadarJamMinimum)
	{
		MegamapRadarJamMinimum= argMegamapRadarJamMinimum;
	}
	if (-1!=argMegamapAntiNukeMinimum)
	{
		MegamapAntiNukeMinimum= argMegamapAntiNukeMinimum;
	}
	
	
	if (-1!=argMegamapWeapon1Color)
	{
		MegamapWeapon1Color =argMegamapWeapon1Color;
	}
	if (-1!=argMegamapWeapon2Color)
	{
		MegamapWeapon2Color =argMegamapWeapon2Color;
	}
	if (-1!=argMegamapWeapon3Color)
	{
		MegamapWeapon3Color =argMegamapWeapon3Color;
	}
	if (-1!=argMegamapRadarColor)
	{
		MegamapRadarColor =argMegamapRadarColor;
	}
	if (-1!=argMegamapSonarColor)
	{
		MegamapSonarColor =argMegamapSonarColor;
	}
	if (-1!=argMegamapRadarJamColor)
	{
		MegamapRadarJamColor =argMegamapRadarJamColor;
	}
	if (-1!=argMegamapSonarJamColor)
	{
		MegamapSonarJamColor =argMegamapSonarJamColor;
	}
	if (-1!=argMegamapAntinukeColor)
	{
		MegamapAntinukeColor =argMegamapAntinukeColor;
	}


	UnderAttackFlash= argUnderAttackFlash;
	for (int i= 0; i<PLAYERNUM; ++i)
	{
		PlayerBits[i]= NULL;

		PlayerAspect[i].x= 0;
		PlayerAspect[i].y= 0;

		PlayerColros[i]= 0xff;
	}


	for (int i= 0; i<MaskNum; ++i)
	{
		PicturesBits[i]= NULL;
	}

	PicturesBits= NULL;

	for (int i= 0; i<PLAYERNUM; ++i)
	{
		PicturesPlayerColors[i]= NULL;
	}

	for (int i= 0; i<PLAYERNUM; ++i)
	{
		UnSelectedPicturesPlayerColors[i]= NULL;
	}

	for (int i= 0; i<PLAYERNUM; ++i)
	{
		HoverPicturesPlayerColors[i]= NULL;
	}
	for (int i= 0; i<PLAYERNUM; ++i)
	{
		PlayerBits[i]= NULL;

		PlayerAspect[i].x= 0;
		PlayerAspect[i].y= 0;

		PlayerColros[i]= 0xff;
	}

	for (int i= 0; i<PLAYERNUM; ++i)
	{
		PlayerDotColor[i]= argPlayerDotColor[i];
	}
		
}
void UnitsMinimap::ReSet (int Width, int Height)
{
	//	DWORD TID;
	Width_m= Width;
	Height_m= Height;

	if (UnitsMapSfc)
	{
		UnitsMapSfc->Release ( );
		UnitsMapSfc= NULL;
	}

	// 	Syncetux= CreateMutexA ( NULL, FALSE, NULL);
	// 	if (INVALID_HANDLE_VALUE!=Syncetux)
	// 	{
	// 		Thd_h= CreateThread ( NULL, 0, UnitMapDrawerProc, (LPVOID)this, 0, &TID);
	// 	}

	return;
}

LPDIRECTDRAWSURFACE UnitsMinimap::InitSurface (LPDIRECTDRAW TADD, BOOL VidMem)
{
	IDDrawSurface::OutptTxt ( "UnitsMap Surface Init");
	if (NULL!=UnitsMapSfc)
	{
		UnitsMapSfc->Release ( );
		UnitsMapSfc= NULL;
	}

	if (TADD)
	{
		if ((0==Width_m)
			||(0==Height_m))
		{
			return NULL;
		}

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
		
		ddsd.dwWidth = Width_m;
		ddsd.dwHeight = Height_m;

		TADD->CreateSurface(&ddsd, &UnitsMapSfc, NULL);
	}

	return UnitsMapSfc;
}


void UnitsMinimap::ReleaseSurface (void)
{
	if (NULL!=UnitsMapSfc)
	{
		UnitsMapSfc->Release ( );
	}
	UnitsMapSfc= NULL;
}

// void UnitsMinimap::LockEvent (void)
// {
// 	WaitForSingleObject ( Syncetux, INFINITE);
// }
// void UnitsMinimap::UnLockEvent (void)
// {
// 	ReleaseMutex ( Syncetux);
// }
// 
LPBYTE UnitsMinimap::LockOn (LPBYTE * PixelBits_pp, POINT * Aspect)
{

	//LockEvent ( );
	DDSURFACEDESC ddsd;
	DDRAW_INIT_STRUCT( ddsd);
	if (UnitsMapSfc)
	{
		if(UnitsMapSfc->IsLost() != DD_OK)
		{
			if(UnitsMapSfc->Restore() != DD_OK)
				return NULL;
		}

		if (DD_OK==UnitsMapSfc->Lock ( NULL, &ddsd, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL))
		{
			if (PixelBits_pp)
			{
				*PixelBits_pp= (LPBYTE )ddsd.lpSurface;
			}

			if (Aspect)
			{
				Aspect->x= ddsd.lPitch;
				Aspect->y= ddsd.dwHeight;
			}
		}
	}
	if (PixelBits_pp)
	{
		return *PixelBits_pp;
	}
	else
	{
		return (LPBYTE )ddsd.lpSurface;
	}

}

void UnitsMinimap::Unlock ( LPBYTE PixelBits_p)
{
	if (UnitsMapSfc)
	{
		UnitsMapSfc->Unlock ( reinterpret_cast<LPVOID>(PixelBits_p));
	}
	//UnLockEvent ( );
}

LPDIRECTDRAWSURFACE UnitsMinimap::GetSurface (void)
{
	if (UnitsMapSfc)
	{
		if (DD_OK!=UnitsMapSfc->IsLost ( ))
		{
			UnitsMapSfc->Restore ( );
		}
	}
	return UnitsMapSfc;
}

void UnitsMinimap::InitPicturePlayerColors (int PlayeyColor, int FillColor, LPBYTE * * PPCptr)
{
	IDDrawSurface::OutptTxt( "InitPictureColors");
	LPBYTE * PPC;
	BOOL Replace;
	LPBYTE ptr;
	if (*PPCptr)
	{
		PPC= *PPCptr;
		Replace= TRUE;
	}
	else
	{
		PPC= (LPBYTE *)malloc ( sizeof(LPBYTE)* MaskNum);
		memset ( PPC, 0, sizeof(LPBYTE)* MaskNum);
		Replace= FALSE;
	}


	for (int i= 0; i<MaskNum; ++i)
	{
		if (PicturesBits[i])
		{
			int Width= PicturesBits[i]->Width();
			int Height= PicturesBits[i]->Height();
			LPBYTE bits= PicturesBits[i]->Data();
			if (!Replace)
			{
				PPC[i]= (LPBYTE)malloc ( PicturesBits[i]->BufSize());
			}

			if (NULL!=PPC[i])
			{
				ptr= PPC[i];
				for (int j= 0; j<Height; ++j)
				{
					int Line= j* Width;
					for (int y= 0; y<Width; ++y)
					{
						register int Color= bits[Line+ y];
						if (FillColor==Color)
						{
							Color= PlayeyColor;
						}

						ptr[Line+ y]= Color;
					}
				}
			}

		}
	}

	*PPCptr= PPC;
}

void UnitsMinimap::InitHoverPicturePlayerColors (int PlayeyColor, int FillColor, int UnSelectedColor,int HoverColor,  
	LPBYTE * * PPCptr)
{
	IDDrawSurface::OutptTxt( "InitUnSelectedPictureColors");
	LPBYTE * PPC;
	BOOL Replace;
	LPBYTE ptr;
	if (*PPCptr)
	{
		PPC= *PPCptr;
		Replace= TRUE;
	}
	else
	{
		PPC= (LPBYTE *)malloc ( sizeof(LPBYTE)* MaskNum);
		memset ( PPC, 0, sizeof(LPBYTE)* MaskNum);
		Replace= FALSE;
	}


	for (int i= 0; i<MaskNum; ++i)
	{
		if (PicturesBits[i])
		{
			int Width= PicturesBits[i]->Width();
			int Height= PicturesBits[i]->Height();
			LPBYTE bits= PicturesBits[i]->Data();
			if (!Replace)
			{
				PPC[i]= (LPBYTE)malloc ( PicturesBits[i]->BufSize());
			}

			if (NULL!=PPC[i])
			{
				ptr= PPC[i];
				for (int j= 0; j<Height; ++j)
				{
					int Line= j* Width;
					for (int y= 0; y<Width; ++y)
					{
						register int Color= bits[Line+ y];

						if (UnSelectedColor==Color)
						{
							Color= HoverColor;
						}
						if (FillColor==Color)
						{
							Color= PlayeyColor;
						}
						ptr[Line+ y]= Color;
					}
				}
			}
		}
	}
	*PPCptr= PPC;
}

void UnitsMinimap::InitUnSelectedPicturePlayerColors (int PlayeyColor, int FillColor, int UnSelectedColor,int TransparentColor,  
	LPBYTE * * PPCptr)
{
	IDDrawSurface::OutptTxt( "InitUnSelectedPictureColors");
	LPBYTE * PPC;
	BOOL Replace;
	LPBYTE ptr;
	if (*PPCptr)
	{
		PPC= *PPCptr;
		Replace= TRUE;
	}
	else
	{
		PPC= (LPBYTE *)malloc ( sizeof(LPBYTE)* MaskNum);
		memset ( PPC, 0, sizeof(LPBYTE)* MaskNum);
		Replace= FALSE;
	}


	for (int i= 0; i<MaskNum; ++i)
	{
		if (PicturesBits[i])
		{
			int Width= PicturesBits[i]->Width();
			int Height= PicturesBits[i]->Height();
			LPBYTE bits= PicturesBits[i]->Data();
			if (!Replace)
			{
				PPC[i]= (LPBYTE)malloc ( PicturesBits[i]->BufSize());
			}

			if (NULL!=PPC[i])
			{
				ptr= PPC[i];
				for (int j= 0; j<Height; ++j)
				{
					int Line= j* Width;
					for (int y= 0; y<Width; ++y)
					{
						register int Color= bits[Line+ y];

						if (UnSelectedColor==Color)
						{
							Color= TransparentColor;
						}
						if (FillColor==Color)
						{
							Color= PlayeyColor;
						}

						ptr[Line+ y]= Color;
					}
				}
			}

		}
	}
	*PPCptr= PPC;
}


void UnitsMinimap::LoadUnitPicture ( void)
{
	if (Inited)
	{
		return ;
	}
	Inited= TRUE;
	

	MaskAry= NULL;
	PicturesBits= NULL;
	for (int i= 0; i<PLAYERNUM; ++i)
	{
		PicturesPlayerColors[i]= NULL;
	}

	for (int i= 0; i<PLAYERNUM; ++i)
	{
		UnSelectedPicturesPlayerColors[i]= NULL;
	}


	for (int i= 0; i<PLAYERNUM; ++i)
	{
		HoverPicturesPlayerColors[i]= NULL;
	}

	MaskNum= 0;

	UseDefaultIcon= FALSE;
	UnknowMaskIndex= 0;
	NothingMaskIndex= 0;
	ProjectileNukeIndex= 0;
	UseCircleHover= FALSE;


	FillColor= DEFAULTFILLCOLOR;
	TransparentColor= DEFAULTTRANSPARENTCOLOR;
	UnSelectedColor= DEFAULTUNSELECTEDCOLOR;
	HoverColor= DEFAULTHOVERCOLOR;

	char ConfigFileName[MAX_PATH]={0};
	char TAPath[MAX_PATH]= {0};
	char ConfigFilePath[MAX_PATH]= {0};
	char * Aftequ;

	MyConfig->GetIniStr ( "MegaMapConfig", ConfigFileName, MAX_PATH, NULL);
	
	if (0!=ConfigFileName[0])
	{
		clean_remark (  ConfigFileName, ';');
		GetCurrentDirectoryA  ( MAX_PATH, TAPath);
		wsprintfA ( ConfigFilePath, "%s\\%s", TAPath, ConfigFileName);
		if (0xffffffff!=GetFileAttributesA ( ConfigFilePath))
		{
			// valid
			FillColor= GetPrivateProfileIntA (  "Option", "FillColor", DEFAULTFILLCOLOR, ConfigFilePath);
			TransparentColor= GetPrivateProfileIntA (  "Option", "TransparentColor", DEFAULTTRANSPARENTCOLOR, ConfigFilePath);
			UnSelectedColor= GetPrivateProfileIntA (  "Option", "SelectedColor", DEFAULTUNSELECTEDCOLOR, ConfigFilePath);
			HoverColor= GetPrivateProfileIntA (  "Option", "HoverColor", DEFAULTHOVERCOLOR, ConfigFilePath);
	
			GetPrivateProfileStringA ( "Option", "UseCircleHover", "false", ConfigFileName , MAX_PATH, ConfigFilePath);
			_strlwr_s ( ConfigFileName , MAX_PATH);
			if (NULL!=strstr ( ConfigFileName, "true"))
			{
				UseCircleHover= TRUE;
			}
			

			GetPrivateProfileStringA ( "Option", "UseDefaultIcon", "true", ConfigFileName , MAX_PATH, ConfigFilePath);
			_strlwr_s ( ConfigFileName , MAX_PATH);
			if (NULL==strstr ( ConfigFileName, "true"))
			{// if not null, use default
				char * CategoryNameAry= (char *)malloc ( 0x10000);

				if (CategoryNameAry)
				{

					int CfgSize= GetPrivateProfileSection ( "Icon", CategoryNameAry, 0x10000, ConfigFilePath);

					for (int i= 0, end= 0; i<CfgSize; i= i+ end+ 1)
					{
						end= strlen ( &CategoryNameAry[i]);
						MaskNum++;
					}
					if (0<MaskNum)
					{
						clean_remark (  ConfigFilePath, '\\');

						MaskAry= (LPDWORD* )malloc ( sizeof(LPDWORD)* (MaskNum+ RACENUMBER));
						memset ( MaskAry, 0, sizeof(LPDWORD)* (MaskNum+ RACENUMBER));
						PicturesBits= (UnitIcon * *)malloc ( sizeof(UnitIcon *)* (MaskNum+ RACENUMBER));
						memset ( PicturesBits, 0, sizeof(UnitIcon *)* (MaskNum+ RACENUMBER));

						MaskNum= 0;


						//------------  Load Commander
						for (int i= 0; i<static_cast<int>((*TAmainStruct_PtrPtr)->RaceCounter); ++i)
						{
							if ('\0'!=myExternQuickKey->Commanders[i][0])
							{
								wsprintfA ( ConfigFileName, "%s\\%s.PCX", ConfigFilePath, &myExternQuickKey->Commanders[i][0]);
								if (0xffffffff!=GetFileAttributesA ( ConfigFileName))
								{
									MaskAry[MaskNum]= myExternQuickKey->CommandersMask[i];
									PicturesBits[MaskNum]= new UnitIcon( ConfigFileName);
									MaskNum++;
								}
							}
						}
						//--------------
						

						for (int i= 0, end= 0; i<CfgSize; i= i+ end+ 1)
						{
							end= strlen ( &CategoryNameAry[i]);

							if (0<end)
							{
								Aftequ= strchr ( &CategoryNameAry[i], '=');
								if (Aftequ)
								{
									*(Aftequ)= '\0';
									Aftequ= &Aftequ[1];

									char key[0x100], value[0x100];
									std::strncpy(key, &CategoryNameAry[i], sizeof(key));
									std::strncpy(value, Aftequ, sizeof(value));
									_strlwr_s(key, sizeof(key));

									if (0==strcmp (key, "nothing"))
									{
										NothingMaskIndex= MaskNum;
									}
									else if (0==strcmp (key, "unknow"))
									{
										UnknowMaskIndex= MaskNum;
									}
									else if (0==strcmp (key, "nukeicon"))
									{
										ProjectileNukeIndex= MaskNum;
									}
									else
									{
										MaskAry[MaskNum]= GetUnitIDMaskAryByCategory ( trim_crlf_(key));
									}
									wsprintfA ( ConfigFileName, "%s\\%s", ConfigFilePath, value);
									clean_remark (  ConfigFileName, ';');
									PicturesBits[MaskNum]= new UnitIcon( ConfigFileName);
									MaskNum++;
								}
							}
						}

					}
					free ( CategoryNameAry);

					
				}
				return ;
			}

		}
	
	}
	//else if (MyConfig->GetIniBool( "MegaMapUseDefaultIconSet", TRUE))
	UseDefaultIcon= TRUE;

	GetCurrentDirectoryA  ( MAX_PATH, TAPath);
	MyConfig->GetIniStr ( "MegaMapConfig", ConfigFileName, MAX_PATH, NULL);

	if (0!=ConfigFileName[0])
	{
		clean_remark (  ConfigFileName, ';');
		GetCurrentDirectoryA  ( MAX_PATH, TAPath);
		wsprintfA ( ConfigFilePath, "%s\\%s", TAPath, ConfigFileName);
		clean_remark (  ConfigFilePath, '\\');
		strcpy_s ( TAPath, MAX_PATH, ConfigFilePath);
	}

	MaskAry= (LPDWORD* )malloc ( sizeof(LPDWORD)* (9+ RACENUMBER));
	memset ( MaskAry, 0, sizeof(LPDWORD)* (9+ RACENUMBER));
	PicturesBits= (UnitIcon * *)malloc ( sizeof(UnitIcon *)* (9+ RACENUMBER));
	memset ( PicturesBits, 0, sizeof(UnitIcon *)* (9+ RACENUMBER));


	//------------  Load Commander
	for (int i= 0; i<RACENUMBER; ++i)
	{
		if ('\0'!=myExternQuickKey->Commanders[i][0])
		{
			wsprintfA ( ConfigFilePath, "%s\\%s.PCX", TAPath, &myExternQuickKey->Commanders[i][0]);
			if (0xffffffff!=GetFileAttributesA ( ConfigFilePath))
			{
				MaskAry[MaskNum]= myExternQuickKey->CommandersMask[i];
				PicturesBits[MaskNum]= new UnitIcon( ConfigFilePath);
				MaskNum++;
			}
		}
	}
	//--------------
	

	wsprintfA ( ConfigFilePath, "%s\\%s", TAPath, COMMICON);
	if (0xffffffff!=GetFileAttributesA ( ConfigFilePath))
	{
		MaskAry[MaskNum]= myExternQuickKey->CommanderMask;
		PicturesBits[MaskNum]= new UnitIcon( ConfigFilePath);
		MaskNum++;
	}


	wsprintfA ( ConfigFilePath, "%s\\%s", TAPath, AIRICON);
	if (0xffffffff!=GetFileAttributesA ( ConfigFilePath))
	{
		MaskAry[MaskNum]= myExternQuickKey->AirWeaponMask;
		PicturesBits[MaskNum]= new UnitIcon( ConfigFilePath);
		MaskNum++;
	}

	wsprintfA ( ConfigFilePath, "%s\\%s", TAPath,AIRCONICON);
	if (0xffffffff!=GetFileAttributesA ( ConfigFilePath))
	{
		MaskAry[MaskNum]= myExternQuickKey->AirConMask;
		PicturesBits[MaskNum]= new UnitIcon( ConfigFilePath);
		MaskNum++;
	}


	wsprintfA ( ConfigFilePath, "%s\\%s", TAPath, CONICON);
	if (0xffffffff!=GetFileAttributesA ( ConfigFilePath))
	{
		MaskAry[MaskNum]= myExternQuickKey->ConstructorMask;
		PicturesBits[MaskNum]= new UnitIcon( ConfigFilePath);
		MaskNum++;
	}

	wsprintfA ( ConfigFilePath, "%s\\%s", TAPath, MOBILEICOMB);
	if (0xffffffff!=GetFileAttributesA ( ConfigFilePath))
	{
		MaskAry[MaskNum]= myExternQuickKey->MobileWeaponMask;
		PicturesBits[MaskNum]= new UnitIcon( ConfigFilePath);
		MaskNum++;
	}



	wsprintfA ( ConfigFilePath, "%s\\%s", TAPath, FCTYICON);
	if (0xffffffff!=GetFileAttributesA ( ConfigFilePath))
	{
		MaskAry[MaskNum]= myExternQuickKey->FactoryMask;
		PicturesBits[MaskNum]= new UnitIcon( ConfigFilePath);
		MaskNum++;
	}


	wsprintfA ( ConfigFilePath, "%s\\%s", TAPath, BLDGICON);
	if (0xffffffff!=GetFileAttributesA ( ConfigFilePath))
	{
		MaskAry[MaskNum]= myExternQuickKey->BuildingMask;
		PicturesBits[MaskNum]= new UnitIcon( ConfigFilePath);
		MaskNum++;
	}


	wsprintfA ( ConfigFilePath, "%s\\%s", TAPath, NOTHINGICON);
	if (0xffffffff!=GetFileAttributesA ( ConfigFilePath))
	{
		PicturesBits[MaskNum]= new UnitIcon( ConfigFilePath);

		NothingMaskIndex= MaskNum;
		MaskNum++;
	}

	wsprintfA ( ConfigFilePath, "%s\\%s", TAPath, UNKNOWICON);
	if (0xffffffff!=GetFileAttributesA ( ConfigFilePath))
	{
		PicturesBits[MaskNum]= new UnitIcon( ConfigFilePath);

		UnknowMaskIndex= MaskNum;
		MaskNum++;
	}

	wsprintfA ( ConfigFilePath, "%s\\%s", TAPath, PROJECTNUKEICON);
	if (0xffffffff!=GetFileAttributesA ( ConfigFilePath))
	{
		PicturesBits[MaskNum]= new UnitIcon( ConfigFilePath);

		ProjectileNukeIndex= MaskNum;
		MaskNum++;
	}
	

}

void UnitsMinimap::FreeUnitPicture ( void)
{	
	if (MaskAry)
	{
		free ( MaskAry);
		MaskAry= NULL;
	}

	if (PicturesBits)
	{
		for (int i= 0; i<MaskNum; ++i)
		{
			delete PicturesBits[i];
			PicturesBits[i]= NULL;
		}

		free  ( PicturesBits);
		PicturesBits= NULL;
	}


	for (int i= 0; i<PLAYERNUM; ++i)
	{
		if (PicturesPlayerColors[i])
		{
			for (int j= 0; j<MaskNum; ++j)
			{
				if (PicturesPlayerColors[i][j])
				{
					free ( PicturesPlayerColors[i][j]);
				}
			}
		}
		PicturesPlayerColors[i]= NULL;
	}



	for (int i= 0; i<PLAYERNUM; ++i)
	{
		if (UnSelectedPicturesPlayerColors[i])
		{
			for (int j= 0; j<MaskNum; ++j)
			{
				if (UnSelectedPicturesPlayerColors[i][j])
				{
					free ( UnSelectedPicturesPlayerColors[i][j]);
				}
			}
		}
		UnSelectedPicturesPlayerColors[i]= NULL;
	}

	for (int i= 0; i<PLAYERNUM; ++i)
	{
		if (HoverPicturesPlayerColors[i])
		{
			for (int j= 0; j<MaskNum; ++j)
			{
				if (HoverPicturesPlayerColors[i][j])
				{
					free ( HoverPicturesPlayerColors[i][j]);
				}
			}
		}
		HoverPicturesPlayerColors[i]= NULL;
	}

	for (int i= 0; i<PLAYERNUM; ++i)
	{
		if (PlayerBits[i])
		{
			free ( PlayerBits[i]);
			PlayerBits[i]= NULL;
		}

		if (0!=PlayerAspect[i].x)
		{
			PlayerAspect[i].x= 0;
			PlayerAspect[i].y= 0;
		}
		PlayerColros[i]= 0xff;
	}

	MaskNum= 0;
	UnknowMaskIndex= 0;

	Inited= FALSE;
}
int UnitsMinimap::GetTransparentColor (void)
{
	return TransparentColor;
}
LPBYTE UnitsMinimap::NukePicture (int PlayerID, LPBYTE * PixelBits_pp,  POINT * Aspect)
{
	if (PixelBits_pp)
	{
		*PixelBits_pp= PicturesBits[ProjectileNukeIndex]->Data ( );
	}
	if (Aspect)
	{
		Aspect->x=  PicturesBits[ProjectileNukeIndex]->Width ( );
		Aspect->y=  PicturesBits[ProjectileNukeIndex]->Height ( );
	}

	if ((NULL==PicturesPlayerColors[PlayerID]))
	{// Init player color
		int CurtColor= (*TAmainStruct_PtrPtr)->Players[PlayerID].PlayerInfo->PlayerLogoColor;

		CurtColor= PlayerDotColor[CurtColor];

		/*switch(CurtColor)
		{
		case 0:
			CurtColor = (char)227;
			break;
		case 1:
			CurtColor = (char)212;
			break;
		case 2:
			CurtColor = (char)80;
			break;
		case 3:
			CurtColor = (char)235;
			break;
		case 4:
			CurtColor = (char)108;
			break;
		case 5:
			CurtColor = (char)219;
			break;
		case 6:
			CurtColor = (char)208;
			break;
		case 7:
			CurtColor = (char)93;
			break;
		case 8:
			CurtColor = (char)130;
			break;
		case 9:
			CurtColor = (char)67;
			break;
		}*/
		InitPicturePlayerColors ( CurtColor, FillColor, &PicturesPlayerColors[PlayerID]);
	}


	return PicturesPlayerColors[PlayerID][ProjectileNukeIndex];
}
LPBYTE UnitsMinimap::UnitPicture(UnitStruct * unitPtr,int PlayerID, LPBYTE * PixelBits_pp, POINT * Aspect)
{
	if ((PLAYERNUM<PlayerID)
		||(PlayerID<0))
	{
		return NULL;
	}
	BOOL Update_b= FALSE;
	int CurtColor= (*TAmainStruct_PtrPtr)->Players[PlayerID].PlayerInfo->PlayerLogoColor;
	if (PlayerColros[PlayerID]!=CurtColor)
	{
		PlayerColros[PlayerID]= CurtColor;
		Update_b= TRUE;
	}

	//
	if (0<MaskNum)
	{
		CurtColor= PlayerDotColor[CurtColor];
		/*switch(CurtColor)
		{
		case 0:
			CurtColor = (char)227;
			break;
		case 1:
			CurtColor = (char)212;
			break;
		case 2:
			CurtColor = (char)80;
			break;
		case 3:
			CurtColor = (char)235;
			break;
		case 4:
			CurtColor = (char)108;
			break;
		case 5:
			CurtColor = (char)219;
			break;
		case 6:
			CurtColor = (char)208;
			break;
		case 7:
			CurtColor = (char)93;
			break;
		case 8:
			CurtColor = (char)130;
			break;
		case 9:
			CurtColor = (char)67;
			break;
		}
*/

		if ((NULL==PicturesPlayerColors[PlayerID])
			||(Update_b))
		{// Init player color

			InitPicturePlayerColors ( CurtColor, FillColor, &PicturesPlayerColors[PlayerID]);
		}

		if ((NULL==UnSelectedPicturesPlayerColors[PlayerID])
			||(Update_b))
		{// Init Unslected player color
			InitUnSelectedPicturePlayerColors ( CurtColor, FillColor, UnSelectedColor, TransparentColor, &UnSelectedPicturesPlayerColors[PlayerID]);
		}
		if ((NULL==HoverPicturesPlayerColors[PlayerID])
			||(Update_b))
		{
			if (! UseCircleHover)
			{
				InitHoverPicturePlayerColors ( CurtColor, FillColor, UnSelectedColor, HoverColor, &HoverPicturesPlayerColors[PlayerID]);
			}
		}

		if (1==CheckUnitInPlayerLOS ( &(*TAmainStruct_PtrPtr)->Players[(*TAmainStruct_PtrPtr)->LOS_Sight_PlayerID], unitPtr))
		{
			if (0!=(0x10& unitPtr->UnitSelected))
			{
				for (int i= 0; i<MaskNum; ++i)
				{
					if ((i!=UnknowMaskIndex)
						&&(i!=NothingMaskIndex)  
						&&(i!=ProjectileNukeIndex)// avoid null mask
						&&(MatchInTypeAry ( unitPtr->UnitID, MaskAry[i])))
					{
						if (PixelBits_pp)
						{
							*PixelBits_pp= PicturesPlayerColors[PlayerID][i];
						}
						if (Aspect)
						{
							Aspect->x=  PicturesBits[i]->Width ( );
							Aspect->y=  PicturesBits[i]->Height ( );
						}
						return PicturesPlayerColors[PlayerID][i];
					}
				}
				if (PixelBits_pp)
				{
					*PixelBits_pp= PicturesPlayerColors[PlayerID][UnknowMaskIndex];
				}
				if (Aspect)
				{
					Aspect->x=  PicturesBits[UnknowMaskIndex]->Width ( );
					Aspect->y=  PicturesBits[UnknowMaskIndex]->Height ( );
				}
				return PicturesPlayerColors[PlayerID][UnknowMaskIndex];
			}
			else if ((! UseCircleHover)
				&&((*TAmainStruct_PtrPtr)->MouseOverUnit ==unitPtr->UnitInGameIndex))
			{
				for (int i= 0; i<MaskNum; ++i)
				{
					if ((i!=UnknowMaskIndex)
						&&(i!=NothingMaskIndex)  
						&&(i!=ProjectileNukeIndex)// avoid null mask
						&&(MatchInTypeAry ( unitPtr->UnitID, MaskAry[i])))
					{
						if (PixelBits_pp)
						{
							*PixelBits_pp= HoverPicturesPlayerColors[PlayerID][i];
						}
						if (Aspect)
						{
							Aspect->x=  PicturesBits[i]->Width ( );
							Aspect->y=  PicturesBits[i]->Height ( );
						}
						return HoverPicturesPlayerColors[PlayerID][i];
					}
				}
				if (PixelBits_pp)
				{
					*PixelBits_pp= HoverPicturesPlayerColors[PlayerID][UnknowMaskIndex];
				}
				if (Aspect)
				{
					Aspect->x=  PicturesBits[UnknowMaskIndex]->Width ( );
					Aspect->y=  PicturesBits[UnknowMaskIndex]->Height ( );
				}
				return HoverPicturesPlayerColors[PlayerID][UnknowMaskIndex];
			}
			else
			{//UnSelectedPicturesPlayerColors
				for (int i= 0; i<MaskNum; ++i)
				{
					if ((i!=UnknowMaskIndex)
						&&(i!=NothingMaskIndex)  
						&&(i!=ProjectileNukeIndex)// avoid null mask
						&&(MatchInTypeAry ( unitPtr->UnitID, MaskAry[i])))
					{
						if (PixelBits_pp)
						{
							*PixelBits_pp= UnSelectedPicturesPlayerColors[PlayerID][i];
						}
						if (Aspect)
						{
							Aspect->x=  PicturesBits[i]->Width ( );
							Aspect->y=  PicturesBits[i]->Height ( );
						}
						return UnSelectedPicturesPlayerColors[PlayerID][i];
					}
				}
				if (PixelBits_pp)
				{
					*PixelBits_pp= UnSelectedPicturesPlayerColors[PlayerID][UnknowMaskIndex];
				}
				if (Aspect)
				{
					Aspect->x=  PicturesBits[UnknowMaskIndex]->Width ( );
					Aspect->y=  PicturesBits[UnknowMaskIndex]->Height ( );
				}
				return UnSelectedPicturesPlayerColors[PlayerID][UnknowMaskIndex];
			}
		}

		if ((! UseCircleHover)
			&&((*TAmainStruct_PtrPtr)->MouseOverUnit ==unitPtr->UnitInGameIndex))
		{
			if (PixelBits_pp)
			{
				*PixelBits_pp= HoverPicturesPlayerColors[PlayerID][NothingMaskIndex];
			}
			if (Aspect)
			{
				Aspect->x=  PicturesBits[NothingMaskIndex]->Width ( );
				Aspect->y=  PicturesBits[NothingMaskIndex]->Height ( );
			}
			return HoverPicturesPlayerColors[PlayerID][NothingMaskIndex];
		}

		if (PixelBits_pp)
		{
			*PixelBits_pp= UnSelectedPicturesPlayerColors[PlayerID][NothingMaskIndex];
		}
		if (Aspect)
		{
			Aspect->x=  PicturesBits[NothingMaskIndex]->Width ( );
			Aspect->y=  PicturesBits[NothingMaskIndex]->Height ( );
		}
		return UnSelectedPicturesPlayerColors[PlayerID][NothingMaskIndex];
	}

	if ((NULL==PlayerBits[PlayerID])
		||Update_b)
	{
		if (NULL!=PlayerBits[PlayerID])
		{
			free ( PlayerBits[PlayerID]);
			PlayerBits[PlayerID]= NULL;
		}


		PlayerAspect[PlayerID].x= 0;
		PlayerAspect[PlayerID].y= 0;

		PGAFFrame UnitPic= Index2Frame_InSequence ( (*TAmainStruct_PtrPtr)->radlogo, PlayerColros[PlayerID]);
		InstanceGAFFrame ( UnitPic, &PlayerBits[PlayerID], &PlayerAspect[PlayerID]);
	}

	if (PixelBits_pp)
	{
		*PixelBits_pp= PlayerBits[PlayerID];
	}
	if (Aspect)
	{
		Aspect->x= PlayerAspect[PlayerID].x;
		Aspect->y= PlayerAspect[PlayerID].y;
	}

	return PlayerBits[PlayerID];

}

void UnitsMinimap::DrawUnit ( LPBYTE PixelBits, POINT * Aspect, UnitStruct * unitPtr)
{
	if ((NULL==PixelBits)
		||(NULL==Aspect))
	{
		return ;
	}
	int PlayerID= unitPtr->Owner_PlayerPtr0->PlayerAryIndex;

	int TAx= unitPtr->XPos- unitPtr->UnitType->FootX/ 2; 
	int TAy= unitPtr->YPos- (unitPtr->ZPos)/ 2- unitPtr->UnitType->FootY/ 2;
	

	POINT GafAspect= {0, 0};
	LPBYTE GafPixelBits= NULL;
	RECT DescRect;
	int DescHeight_I;
	int DescWidth_I;
	int DesclPitch= Aspect->x;

	unsigned int MouseID= (*TAmainStruct_PtrPtr)->MouseOverUnit;
	unsigned int ShowRange=  (*TAmainStruct_PtrPtr)->ShowRangeUnitIndex;

	DWORD Radius;
	if (! IsPlayerAllyUnit ( MouseID, (*TAmainStruct_PtrPtr)->LOS_Sight_PlayerID))
	{
		MouseID= 0;
	}

	if (! IsPlayerAllyUnit ( ShowRange, (*TAmainStruct_PtrPtr)->LOS_Sight_PlayerID))
	{
		ShowRange= 0;
	}
	//

	UnitPicture( unitPtr, PlayerID, &GafPixelBits, &GafAspect);
	int mmFW = (*TAmainStruct_PtrPtr)->FeatureMapSizeX;
	int mmRight = (mmFW - 2) * 16; if (mmRight <= 0) mmRight = parent->TAMAPTAPos.right;
	DescRect.left = static_cast<int>(static_cast<float>(TAx) * (static_cast<float>(Width_m) / static_cast<float>(mmRight))) - GafAspect.x / 2;
	DescRect.right= DescRect.left+ GafAspect.x;
	
	int mmFH = (*TAmainStruct_PtrPtr)->FeatureMapSizeY;
	int mmBottom = (mmFH - 8) * 16; if (mmBottom <= 0) mmBottom = parent->TAMAPTAPos.bottom;
	DescRect.top = static_cast<int>(static_cast<float>(TAy) * (static_cast<float>(Height_m) / static_cast<float>(mmBottom))) - GafAspect.y / 2;
	DescRect.bottom = DescRect.top + GafAspect.y;

	Aspect->x= (Aspect->x)/ 4* 4;// avoid draw out of the surface, this x== pitch
	if ((DescRect.right<0)
		||((Aspect->x)<DescRect.left)
		||(Aspect->y<DescRect.top)
		||(DescRect.bottom<0))
	{// out of map
		return ;
	}

	// do not draw out of the map part
	if (DescRect.left<0)
	{
		DescRect.left= 0;
	}
	if (Aspect->x<DescRect.right)
	{
		DescRect.right= Aspect->x;
	}
	if (DescRect.top<0)
	{
		DescRect.top= 0;
	}
	if (Aspect->y<DescRect.bottom)
	{
		DescRect.bottom= Aspect->y;
	}

	DescHeight_I= DescRect.bottom- DescRect.top;
	DescWidth_I= DescRect.right- DescRect.left;

	try
	{
		if ( (! UnderAttackFlash)
			||0==unitPtr->RecentDamage
			||((0!=unitPtr->RecentDamage)&&((*TAmainStruct_PtrPtr)->AntiWeaponDotte_b& 1))
			)
		{
			for (int YPos= 0; YPos<DescHeight_I; YPos++)
			{	//Y
				int DescPixelYStart= (YPos+ DescRect.top)* (DesclPitch);
				int SrcPixelYStart= YPos* GafAspect.x;
				for (int XPos= 0; XPos<DescWidth_I; XPos++)
				{//X 
					register int Color= GafPixelBits[SrcPixelYStart+ XPos];
					if (Color!=TransparentColor)
					{
						PixelBits[DescPixelYStart+ (XPos+ DescRect.left)]= Color;
					}
				}
			} 
		}
		

		TAx= DescRect.left+ GafAspect.x/ 2;
		TAy= DescRect.top+ GafAspect.y/ 2;
		
		if (UseCircleHover
			&&((*TAmainStruct_PtrPtr)->MouseOverUnit==unitPtr->UnitInGameIndex))
		{
			double X= GafAspect.x/ 2;
			double Y= GafAspect.y/ 2;
			Radius= static_cast<int>(sqrt ((X* X+ Y* Y)));
			DrawRadarCircle ( (LPBYTE)PixelBits, Aspect,
				TAx, TAy, 
				Radius, 
				HoverColor  );
// 			++Radius;
// 			DrawRadarCircle ( (LPBYTE)PixelBits, Aspect,
// 				TAx, TAy, 
// 				Radius, 
// 				HoverColor  );
		}


		if (((unitPtr->UnitInGameIndex==MouseID)
			||(unitPtr->UnitInGameIndex==ShowRange))
			&& (parent->Controler->IsDrawOrder ( )))
		{
			{
				if (2&unitPtr->Weapon3Valid)
				{
					if (unitPtr->Weapon3->Range)
					{
						Radius= (static_cast<int>(unitPtr->Weapon3->Range)* static_cast<int>(Aspect->x))/ parent->TAMAPTAPos.right;

						DrawRadarCircle ( (LPBYTE)PixelBits, Aspect,
							TAx, TAy, 
							Radius, 
							MegamapWeapon3Color );
					}
				}

				if (2&unitPtr->Weapon2Valid)
				{
					if (unitPtr->Weapon2->Range)
					{
						Radius= (static_cast<int>(unitPtr->Weapon2->Range)* static_cast<int>(Aspect->x))/ parent->TAMAPTAPos.right;

						DrawRadarCircle ( (LPBYTE)PixelBits, Aspect,
							TAx, TAy, 
							Radius, 
							MegamapWeapon2Color  );
					}
				}
				if (2&unitPtr->Weapon1Valid)
				{
					if (unitPtr->Weapon1->Range)
					{
						Radius= (static_cast<int>(unitPtr->Weapon1->Range)* static_cast<int>(Aspect->x))/ parent->TAMAPTAPos.right;


						DrawRadarCircle ( (LPBYTE)PixelBits, Aspect,
							TAx, TAy, 
							Radius, 
							MegamapWeapon1Color  );
					}
				}

			}
		}

		if (0!=(0x10&  (unitPtr->UnitSelected))
			&&IsPlayerAllyUnit ( unitPtr->UnitInGameIndex, (*TAmainStruct_PtrPtr)->LOS_Sight_PlayerID))
		{// selected
			//radar and jam
			if (MegamapRadarMinimum<unitPtr->UnitType->nRadarDistance)
			{
				DrawRadarCircle ( PixelBits, Aspect,
					TAx, TAy, 
					(static_cast<int>(unitPtr->UnitType->nRadarDistance)* static_cast<int>(Aspect->x))/ parent->TAMAPTAPos.right, 
					MegamapRadarColor );
			}

			if (MegamapSonarMinimum<unitPtr->UnitType->nSonarDistance)
			{

				DrawRadarCircle ( PixelBits, Aspect,
					TAx, TAy, 
					(static_cast<int>(unitPtr->UnitType->nSonarDistance)* static_cast<int>(Aspect->x))/ parent->TAMAPTAPos.right, 
					MegamapSonarColor );
			}
	
			if (MegamapRadarMinimum<unitPtr->UnitType->radardistancejam)
			{
				DrawRadarCircle ( PixelBits, Aspect,
					TAx, TAy, 
					(static_cast<int>(unitPtr->UnitType->radardistancejam)* static_cast<int>(Aspect->x))/ parent->TAMAPTAPos.right, 
					MegamapRadarJamColor  );
			}

			if (MegamapSonarJamMinimum<unitPtr->UnitType->sonardistancejam)
			{
				DrawRadarCircle ( PixelBits, Aspect,
					TAx, TAy, 
					(static_cast<int>(unitPtr->UnitType->sonardistancejam)* static_cast<int>(Aspect->x))/ parent->TAMAPTAPos.right, 
					MegamapSonarJamColor  );
			}

			// anti nuke
			if (0!=(antiweapons& unitPtr->UnitType->UnitTypeMask_0))
			{//
				
				if (0!=(WTM_Interceptor &unitPtr->Weapon1->WeaponTypeMask))
				{
					if (MegamapAntiNukeMinimum<static_cast<int>(unitPtr->Weapon1->coverage))
					{
						int Radius= (static_cast<int>(unitPtr->Weapon1->coverage- 0x200)* (static_cast<int>(Aspect->x))/ parent->TAMAPTAPos.right);

						if (unitPtr->Weapon1Dotte)
						{
							DrawDotteCircle ( PixelBits, Aspect,
								TAx, TAy, 
								Radius,
								MegamapAntinukeColor ,
								0x20, (*TAmainStruct_PtrPtr)->AntiWeaponDotte_b);
						}
						else
						{
							DrawRadarCircle ( PixelBits, Aspect,
								TAx, TAy, 
								Radius,
								MegamapAntinukeColor  );
						}
					}
				}

				if (0!=(WTM_Interceptor &unitPtr->Weapon2->WeaponTypeMask))
				{
					if (MegamapAntiNukeMinimum<static_cast<int>(unitPtr->Weapon2->coverage))
					{
						int Radius= (static_cast<int>(unitPtr->Weapon2->coverage- 0x200)* (static_cast<int>(Aspect->x))/ parent->TAMAPTAPos.right);
						//int Radius= static_cast<int>(static_cast<float>(unitPtr->Weapon2->coverage- 0x200)* (static_cast<float>(Aspect->x)/ static_cast<float>(parent->TAMAPTAPos.right)));
						if (unitPtr->Weapon2Dotte)
						{
							DrawDotteCircle ( PixelBits, Aspect,
								TAx, TAy, 
								Radius,
								MegamapAntinukeColor ,
								0x20, (*TAmainStruct_PtrPtr)->AntiWeaponDotte_b);
						}
						else
						{
							DrawRadarCircle ( PixelBits, Aspect,
								TAx, TAy, 
								Radius,
								MegamapAntinukeColor  );
						}
					}
				}

				if (0!=(WTM_Interceptor &unitPtr->Weapon3->WeaponTypeMask))
				{
					if (MegamapAntiNukeMinimum<static_cast<int>(unitPtr->Weapon3->coverage))
					{
						int Radius= (static_cast<int>(unitPtr->Weapon3->coverage- 0x200)* (static_cast<int>(Aspect->x))/ parent->TAMAPTAPos.right);
						if (unitPtr->Weapon3Dotte)
						{
							DrawDotteCircle ( PixelBits, Aspect,
								TAx, TAy, 
								Radius,
								MegamapAntinukeColor, 
								0x20, (*TAmainStruct_PtrPtr)->AntiWeaponDotte_b);
						}
						else
						{
							DrawRadarCircle ( PixelBits, Aspect,
								TAx, TAy, 
								Radius,
								MegamapAntinukeColor  );
						}
					}
				}
			}

		}
	}
	catch (...)
	{

	}
}

// ===================== ProTA mex-sprite cache =====================

struct MexSprite { LPBYTE bits; int w, h; BYTE trans; int builtPx; };
static MexSprite gMexCache[1024];
static void* gMexCacheMap = NULL;
static bool      gMexCacheInit = false;
static const char* Def_Desc(FeatureDefStruct* d) { return (const char*)((char*)d + 0x80); }

static void MexCacheFlush()
{
	for (int i = 0; i < 1024; ++i) {
		if (gMexCache[i].bits) { delete[] gMexCache[i].bits; gMexCache[i].bits = NULL; }
		gMexCache[i].builtPx = 0;
	}
}

static int NameEqI(const char* a, const char* b)
{
	if (!a || !b) return 0;
	while (*a && *b) {
		char ca = *a++, cb = *b++;
		if (ca >= 'a' && ca <= 'z') ca -= 32;
		if (cb >= 'a' && cb <= 'z') cb -= 32;
		if (ca != cb) return 0;
	}
	return *a == 0 && *b == 0;
}

// struct accessors by confirmed raw offset (the header struct mislabels these)
static const char* Def_Filename(FeatureDefStruct* d) { return (const char*)((char*)d + 0x98); }
static void* Def_Anims(FeatureDefStruct* d) { return *(void**)((char*)d + 0xA8); }
static PGAFSequence Def_RSeq(FeatureDefStruct* d) { return *(PGAFSequence*)((char*)d + 0xAC); }

static bool SeqValid(PGAFSequence s)
{
	return s && s->Signature == 0x00000001 && s->Frames > 0 && s->Frames <= 256
		&& s->PtrFrameAry[0].PtrFrame;
}

// The real GAF filename for a def. If its own filename is "REUSE", borrow the
// filename from a def that shares its description (its family) and names a real
// file -- REUSE features in a TDF share the family's GAF.
static const char* RealFilename(FeatureDefStruct* def, FeatureDefStruct* defs, int ndef)
{
	if (!def) return NULL;
	const char* fn = Def_Filename(def);
	if (fn && fn[0] && !NameEqI(fn, "REUSE")) return fn;   // has its own file
	const char* myDesc = Def_Desc(def);
	if (!myDesc || !myDesc[0]) return NULL;                // nothing to group on
	for (int i = 0; i < ndef; ++i) {
		const char* f = Def_Filename(&defs[i]);
		if (!f || !f[0] || NameEqI(f, "REUSE")) continue;  // need a real filename
		if (NameEqI(Def_Desc(&defs[i]), myDesc)) return f; // same family
	}
	return NULL;
}

// find a parsed GAF already in memory for this def's (REUSE-resolved) filename
static void* FindLoadedGafByFilename(FeatureDefStruct* def, FeatureDefStruct* defs, int ndef)
{
	if (!def || !defs) return NULL;
	const char* want = RealFilename(def, defs, ndef);
	if (!want) return NULL;
	for (int i = 0; i < ndef; ++i) {
		void* a = Def_Anims(&defs[i]);
		if (!a) continue;
		if (((unsigned int*)a)[0] != 0x00010100) continue;
		const char* f = RealFilename(&defs[i], defs, ndef);   // resolve candidate's REUSE too
		if (f && NameEqI(f, want)) return a;
	}
	return NULL;
}

static PGAFSequence FeatureDefToSequence(FeatureDefStruct* def, FeatureDefStruct* defs, int ndef)
{
	if (!def) return NULL;

	// 1) engine already resolved the exact sequence -> use it
	PGAFSequence r = Def_RSeq(def);
	if (SeqValid(r)) return r;

	// 2) borrow from a loaded GAF of the same filename
	void* gaf = FindLoadedGafByFilename(def, defs, ndef);
	if (!gaf) return NULL;
	unsigned int* h = (unsigned int*)gaf;
	if (h[0] != 0x00010100) return NULL;
	unsigned n = h[1]; if (n < 1 || n > 4096) return NULL;
	PGAFSequence* arr = (PGAFSequence*)((char*)gaf + 12);
	PGAFSequence firstValid = NULL;
	for (unsigned i = 0; i < n; ++i) {
		PGAFSequence s = arr[i];
		if (!SeqValid(s)) continue;
		if (!firstValid) firstValid = s;
		if (NameEqI(s->Name, def->Name)) return s;   // best-effort exact match
	}
	return firstValid;   // same-family fallback
}

static MexSprite* GetMexSprite(int defIndex, FeatureDefStruct* def, int targetPx,
	FeatureDefStruct* defs, int ndef)
{
	if (defIndex < 0 || defIndex >= 1024) return NULL;
	MexSprite* s = &gMexCache[defIndex];
	if (s->bits && s->builtPx == targetPx) return s;
	if (s->bits) { delete[] s->bits; s->bits = NULL; }

	PGAFSequence seq = FeatureDefToSequence(def, defs, ndef);
	if (!seq) return NULL; 

	PGAFFrame fr = seq->PtrFrameAry[0].PtrFrame;
	LPBYTE src = NULL; POINT srcA = { 0,0 };
	InstanceGAFFrame(fr, &src, &srcA);
	if (!src || srcA.x <= 0 || srcA.y <= 0) { if (src) free(src); return NULL; }

	int maj = (srcA.x > srcA.y) ? srcA.x : srcA.y;
	int dw = (int)((float)srcA.x * targetPx / maj + 0.5f);
	int dh = (int)((float)srcA.y * targetPx / maj + 0.5f);
	if (dw < 1) dw = 1; if (dh < 1) dh = 1;

	BYTE trans = fr->Background;
	LPBYTE dst = new BYTE[dw * dh];
	for (int y = 0; y < dh; ++y) {
		int sy = y * srcA.y / dh;
		for (int x = 0; x < dw; ++x) {
			int sx = x * srcA.x / dw;
			dst[y * dw + x] = src[sy * srcA.x + sx];
		}
	}
	free(src);

	s->bits = dst; s->w = dw; s->h = dh; s->trans = trans; s->builtPx = targetPx;
	return s;
}

// ===================================================================
// ProTA: draw placed indestructible features (mexes, geos, spires,
// walls) into the megamap TERRAIN image (MappedBits) as their real GAF
// sprites, so they inherit the existing terrain fog/grey pass and layer
// correctly under the unit blips.
//
// Called from MappedMap::NowDrawMapped, AFTER the terrain is copied in
// and BEFORE the LOS/fog passes run. Runs once per sim-tick (tick-cached
// there), not per render frame.
//
//   bits          - the MappedBits terrain buffer (Width*Height, 8-bit)
//   Width, Height - dimensions of that buffer (== megamap image size)
//
// Positioning matches the +makeposter (true map) view across all map
// sizes:
//   - scale: playable extent (fw-2)*16 / (fh-8)*16 (strips the map's
//     32px-right / 128px-bottom dead-zone), == the true map size.
//   - centring: each feature centres on its OWN footprint (footX/footZ),
//     per the Mappy editor's projection. A fixed offset only suits ~1x1
//     features, which is why a single offset left spires scattered.
//   - sizing: one ruler for all types (art major dim * Width/mapRight);
//     no per-type clamps.
//   - anchor: GAF yPosition handle for vertical placement.
//
// Indestructible-and-not-reclaimable features are kept; reclaimable junk
// (rocks/wrecks/trees) is skipped. Coloured-dot fallback only when a
// feature's GAF art can't be resolved.
// ===================================================================

void DrawFeaturesIntoMappedBits(LPBYTE bits, int Width, int Height)
{
	if (!bits || Width <= 0 || Height <= 0)
		return;

	int fw = (*TAmainStruct_PtrPtr)->FeatureMapSizeX;
	int fh = (*TAmainStruct_PtrPtr)->FeatureMapSizeY;
	FeatureStruct* fmap = (*TAmainStruct_PtrPtr)->FeatureMap;
	FeatureDefStruct* fdef = (*TAmainStruct_PtrPtr)->FeatureDef;
	int               ndef = (*TAmainStruct_PtrPtr)->NumFeatureDefs;
	if (fmap == NULL || fdef == NULL || fw <= 0 || fh <= 0)
		return;
	if (!gMexCacheInit) { MexCacheFlush(); gMexCacheInit = true; }
	if (gMexCacheMap != (void*)fmap) { MexCacheFlush(); gMexCacheMap = (void*)fmap; }
	// Playable map extent (strips the 32px-right / 128px-bottom dead-zone):
	// (fw-2)*16 / (fh-8)*16 == the true map size, matching +makeposter.
	int mapRight = (fw - 2) * 16;   // true world width  (320 tiles * 16 on a 5120 map)
	int mapBottom = (fh - 8) * 16;   // true world height
	if (mapRight <= 0)  mapRight = fw * 16;   // guard tiny maps
	if (mapBottom <= 0) mapBottom = fh * 16;

	const int   SPOT_COLOR = 254;  // mex dot (fallback only)
	const int   SPIRE_COLOR = 211;  // spire dot (fallback only)
	const int   OTHER_COLOR = 165;  // other indestructible (fallback only)
	const float HEIGHT_Z = 0.5f; // iso-lift (same as the overlay used)

	for (int gy = 0; gy < fh; ++gy)
		for (int gx = 0; gx < fw; ++gx)
		{
			FeatureStruct* cell = &fmap[gy * fw + gx];
			unsigned short di = cell->FeatureDefIndex;
			if (di == 0xffff || di == 0xfffe) continue;
			if (di >= (unsigned short)ndef)   continue;

			// keep: indestructible AND not reclaimable
			bool indestruct = (fdef[di].FeatureMask & (unsigned short)FeatureMasks::indestructible) != 0;
			bool reclaim = (fdef[di].FeatureMask & (unsigned short)FeatureMasks::reclaimable) != 0;
			if (!(indestruct && !reclaim)) continue;

			bool isSpire = indestruct && NameEqI(Def_Desc(&fdef[di]), "Spire");

			int dotColor = (fdef[di].Metal > 0.0f) ? SPOT_COLOR
				: isSpire ? SPIRE_COLOR
				: OTHER_COLOR;

			// AF centering: each feature centres on its OWN footprint (footX/footZ),
			// not a fixed offset. footprint at +0x94 (two shorts: X, Z).
			short* fpXZ = (short*)((char*)&fdef[di] + 0x94);
			int footX = fpXZ[0];
			int footZ = fpXZ[1];
			if (footX < 1) footX = 1;     // guard bad/zero footprints
			if (footZ < 1) footZ = 1;
			int worldX = gx * 16 + footX * 8;
			int worldY = gy * 16 + footZ * 8 - (int)((float)cell->height * HEIGHT_Z);

			int px = (int)((float)worldX * (float)Width / (float)mapRight);
			int py = (int)((float)worldY * (float)Height / (float)mapBottom);

			// ----- unified sizing: one scale for ALL features (like the poster) -----
			int artMaj = 0, frameH = 0, frameW = 0;
			{
				PGAFSequence sq = FeatureDefToSequence(&fdef[di], fdef, ndef);
				if (sq && SeqValid(sq)) {
					PGAFFrame f0 = sq->PtrFrameAry[0].PtrFrame;
					if (f0) { frameW = f0->Width; frameH = f0->Height; }
				}
			}
			artMaj = (frameW > frameH) ? frameW : frameH;
			if (artMaj < 1) artMaj = 32;   // fallback if art unreadable

			// One ruler: art pixels -> world -> megamap px, same scale as position.
			float pxPerWorld = (float)Width / (float)mapRight;
			const float ARTWORLD = 1.0f;    // art-pixel : world-unit ratio (sizes vs +makeposter)
			int target = (int)((float)artMaj * ARTWORLD * pxPerWorld + 0.5f);
			if (target < 2) target = 2;     // sanity floor only (avoid zero-size)

			MexSprite* spr = GetMexSprite(di, &fdef[di], target, fdef, ndef);

			if (spr && spr->bits) {
				
				// Anchor: X = geometric centre; Y = GAF yPosition handle (scaled
				// to the downscaled sprite) so tall art sits on its base.
				int anchorX = spr->w / 2; // X: geometric centre
				int anchorY = spr->h;     // Y: base (overridden by GAF handle below)
				{
					PGAFSequence sq = FeatureDefToSequence(&fdef[di], fdef, ndef);
					if (sq && SeqValid(sq)) {
						PGAFFrame f0 = sq->PtrFrameAry[0].PtrFrame;
						if (f0 && f0->Height > 0) {
							anchorY = (int)((float)f0->yPosition * (float)spr->h / (float)f0->Height + 0.5f);
						}
					}
				}
				int left = px - anchorX;
				int top = py - anchorY;
								
				for (int yy = 0; yy < spr->h; ++yy) {
					int Y = top + yy;
					if (Y < 0 || Y >= Height) continue;
					int rowS = yy * spr->w;
					int rowD = Y * Width;
					for (int xx = 0; xx < spr->w; ++xx) {
						int X = left + xx;
						if (X < 0 || X >= Width) continue;
						BYTE c = spr->bits[rowS + xx];
						if (c != spr->trans) bits[rowD + X] = c;   // fog pass greys this later
					}
				}
			}
			else {
				// fallback dot (rarely hit art unresolvable)
				for (int dyy = -1; dyy <= 1; ++dyy)
					for (int dxx = -1; dxx <= 1; ++dxx) {
						int X = px + dxx, Y = py + dyy;
						if (X >= 0 && Y >= 0 && X < Width && Y < Height)
							bits[Y * Width + X] = (BYTE)dotColor;
					}
			}
		}
}
// ================== end ProTA mex-sprite cache ====================


void UnitsMinimap::NowDrawUnits(LPBYTE PixelBitsBack, POINT* AspectSrc)
{
	if (TAInGame != DataShare->TAProgress)
	{
		return;
	}
	//IDDrawSurface::OutptTxt ( "Draw units");
	POINT Aspect = { 0, 0 };
	LPBYTE PixelBits = NULL;

	LockOn(&PixelBits, &Aspect);
	try
	{
		do
		{
			if ((TAInGame != DataShare->TAProgress))
			{
				break;
			}
			if (NULL == PixelBits)
			{
				break;
			}
			{
				// memcpy delegates to vectorised CRT; the hand-rolled
				// indexed loop was not auto-vectorising.
				const int rows = AspectSrc->y;
				const int cols = AspectSrc->x;
				if (Aspect.x == cols)
				{
					std::memcpy(PixelBits, PixelBitsBack, static_cast<size_t>(cols) * rows);
				}
				else
				{
					for (int i = 0; i < rows; ++i)
					{
						std::memcpy(PixelBits + Aspect.x * i, PixelBitsBack + cols * i, cols);
					}
				}
			}

			UnitStruct* Begin = (*TAmainStruct_PtrPtr)->BeginUnitsArray_p;
			UnitStruct* unitPtr;

			if (NULL == Begin)
			{
				break;
			}

			// Feature sprites are now baked into the terrain image in
			// MappedMap::NowDrawMapped (DrawFeaturesIntoMappedBits), so they
			// fog/layer for free. Here we only draw the unit blips, which
			// composite on top of the (already feature-bearing) terrain.
			int NumHotRadarUnits = (*TAmainStruct_PtrPtr)->NumHotRadarUnits;
			RadarUnit_* RadarUnits_v = (*TAmainStruct_PtrPtr)->RadarUnits;
			for (int i = 0; i < NumHotRadarUnits; ++i)
			{
				unitPtr = &Begin[RadarUnits_v[i].ID];
				if (0 != unitPtr->UnitID)
				{
					DrawUnit(PixelBits, &Aspect, unitPtr);
				}
			}

		} while (false);

	}
	catch (...)
	{
		;
	}

	Unlock(PixelBits);
}


#endif