#pragma once

class FullScreenMinimap;
class PCX;
struct UnitStruct;
//DWORD WINAPI UnitMapDrawerProc( LPVOID argc);
class TABMP;
class PCX;
#define USEPCX (1)
#define USEBMP (0)
#define USEBAD (-1)
class UnitIcon
{
public:
	UnitIcon (char * FileName);
	~UnitIcon ();
	LONG Width ();
	LONG Height ();
	LPBYTE Data ();
	long BufSize ();
private:
	TABMP * bmp;
	PCX * pcx;
	int Use;
};

class UnitsMinimap 
{
public:
	FullScreenMinimap * parent;
		

	

	
public:
	UnitsMinimap ( FullScreenMinimap * inheritFrom, int Width, int Height,
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
		int * argPlayerDotColor);
	UnitsMinimap ( FullScreenMinimap * inheritFrom,	
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
		int * argPlayerDotColor);

	~UnitsMinimap();
	void Init (int argMegamapRadarMinimum,
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
		int * argPlayerDotColor);
	void ReSet (int Width, int Height);

	LPDIRECTDRAWSURFACE InitSurface (LPDIRECTDRAW TADD, BOOL VidMem);
	void ReleaseSurface (void);

// 	void LockEvent (void);
// 	void UnLockEvent (void);
 	LPBYTE LockOn (LPBYTE * PixelBits_pp, POINT * Aspect);
 	void Unlock ( LPBYTE PixelBits_p);
	LPDIRECTDRAWSURFACE GetSurface (void);
	void NowDrawUnits ( LPBYTE PixelBitsBack, POINT * AspectSrc);

	LPBYTE UnitPicture (UnitStruct * unitPtr, int PlayerID, LPBYTE * PixelBits_pp,  POINT * Aspect);
	LPBYTE NukePicture (int PlayerID, LPBYTE * PixelBits_pp,  POINT * Aspect);
	int GetTransparentColor (void);

	void LoadUnitPicture ( void);
	void FreeUnitPicture ( void);
private:
	void DrawUnit ( LPBYTE PixelBits, POINT * Aspect, UnitStruct * unitPtr);

	
	void InitPicturePlayerColors (int PlayeyColor, int FillColor, LPBYTE * * PPCptr);
	void InitUnSelectedPicturePlayerColors (int PlayeyColor, int FillColor, int UnSelectedColor,int TransparentColor,  
	LPBYTE * * PPCptr);

	void InitHoverPicturePlayerColors  (int PlayeyColor, int FillColor, int UnSelectedColor,int HoverColor,  
		LPBYTE * * PPCptr);
private:
	LPDIRECTDRAWSURFACE UnitsMapSfc;
//	HANDLE Syncetux;
	
	HANDLE Thd_h;
	LPDWORD * MaskAry;
	UnitIcon * * PicturesBits;
	LPBYTE * PicturesPlayerColors[PLAYERNUM];
	LPBYTE * UnSelectedPicturesPlayerColors[PLAYERNUM];
	LPBYTE * HoverPicturesPlayerColors[PLAYERNUM];

	int MaskNum;
	int UnknowMaskIndex;
	int NothingMaskIndex;
	int ProjectileNukeIndex;

	int UnSelectedColor;
	int FillColor;
	int TransparentColor;
	int HoverColor;

	BOOL UseCircleHover;

	BOOL UseDefaultIcon;

	BOOL UnderAttackFlash;
// 	InlineSingleHook * Clean_hook;
// 	InlineSingleHook * DrawUnit_hook;
	int Width_m;
	int Height_m;
	BYTE PlayerColros[PLAYERNUM];
	LPBYTE  PlayerBits[PLAYERNUM];
	POINT PlayerAspect[PLAYERNUM];

	BOOL Inited;

	int MegamapRadarMinimum;
	int MegamapSonarMinimum;
	int MegamapSonarJamMinimum;
	int MegamapRadarJamMinimum;
	int MegamapAntiNukeMinimum;

	int MegamapWeapon1Color;
	int MegamapWeapon2Color;
	int MegamapWeapon3Color;
	int MegamapRadarColor;
	int MegamapSonarColor;
	int MegamapRadarJamColor;
	int MegamapSonarJamColor;
	int MegamapAntinukeColor;

	int PlayerDotColor[PLAYERNUM];
};

#define DEFAULTFILLCOLOR (0)
#define DEFAULTTRANSPARENTCOLOR (9)
#define DEFAULTUNSELECTEDCOLOR (89)
#define DEFAULTHOVERCOLOR (84)



#define COMMICON ( "COMMANDER.PCX")
#define MOBILEICOMB ("MOBILECOMBAT.PCX")
#define CONICON ("CONS.PCX")
#define FCTYICON ("FACTORY.PCX")
#define BLDGICON ("BUILDING.PCX")
#define AIRICON ("AIRCRAFTCOMBAT.PCX")
#define AIRCONICON ( "AIRCONS.PCX")
#define NOTHINGICON  ("NONE.PCX")
#define UNKNOWICON ("UNKNOWN.PCX")

#define PROJECTNUKEICON ("NUKEICON.PCX")

#define ICONMAXWIDTH (22)
#define ICONMAXHEIGHT (22)