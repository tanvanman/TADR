#include "oddraw.h"

#include "hook/hook.h"
#include "hook/etc.h"
#include "tamem.h"
#include "tafunctions.h"
#include "mapParse.h"
#include "Gaf.h"
#include "UnitDrawer.h"

#include "ProjectilesMap.h"
#include "fullscreenminimap.h"


#ifdef USEMEGAMAP

ProjectileMap::ProjectileMap (FullScreenMinimap * par_arg, UnitsMinimap * UnitMap_p)
{
	TAmainStruct_Ptr= (*TAmainStruct_PtrPtr);
	
	myUnitMap= UnitMap_p;

	parent= par_arg;
}

ProjectileMap::~ProjectileMap ()
{
	;
}

void ProjectileMap::NowDrawProjectile  (LPBYTE PixelBits, POINT * AspectSrc)
{
	ProjectColor= TAmainStruct_Ptr->desktopGUI.RadarObjecColor[0xe];
	int ProjectileCount= TAmainStruct_Ptr->NumProjectiles;

	ProjectileStruct* Begin= TAmainStruct_Ptr->Projectiles;
	for (int i= 0; i<ProjectileCount; ++i)
	{
		DrawProjectile ( PixelBits, AspectSrc, &Begin[i]);
	}
}

BOOL  ProjectileMap::IsPosInPlayerLos (unsigned int XPos_Tile, unsigned int YPos_Tile, ProjectileStruct * Projectile_p)
{
	if ((XPos_Tile<0)
		||(YPos_Tile<0))
	{
		return FALSE;
	}
	PlayerStruct * LosPlayer;

	LosPlayer= &TAmainStruct_Ptr->Players[TAmainStruct_Ptr->LOS_Sight_PlayerID];
	
	
	unsigned int LosWidth= LosPlayer->LOS_Tilewidth;
	unsigned int LosHeight= LosPlayer->LOS_Tileheight;

	if ((LosWidth<XPos_Tile)
		||(LosHeight<YPos_Tile))
	{
		return FALSE;
	}


	if (TAmainStruct_Ptr->LOS_Sight_PlayerID==Projectile_p->myLos_PlayerID)
	{
		return TRUE;
	}


	if (LosPlayer->AllyFlagAry[Projectile_p->myLos_PlayerID])
	{
		return TRUE;
	}

	if (Permanent!=(Permanent&TAmainStruct_Ptr->LosType))
	{
		if (NOMAPPING==(NOMAPPING&TAmainStruct_Ptr->LosType))
		{//
			return TRUE;
		}
		// check NOMAPPED
		unsigned short *  LOSMEMORY_pw= TAmainStruct_Ptr->MAPPED_MEMORY_p;
		short int LosBits= LOSMEMORY_pw[LosWidth* YPos_Tile+ XPos_Tile];
		int LosPlayerID= TAmainStruct_Ptr->LOS_Sight_PlayerID;

		if (0!=((1<<LosPlayerID)& LosBits))
		{
			return TRUE;
		}
	}
	else
	{//check player los
		unsigned char * LOSMEMORY_pb= LosPlayer->LOS_MEMORY_p;

		if (0!=LOSMEMORY_pb[LosWidth* YPos_Tile+ XPos_Tile])
		{
			return TRUE;
		}
	}

	return FALSE;
}

void  ProjectileMap::DrawProjectile (LPBYTE PixelBits, POINT * Aspect, ProjectileStruct * Projectile_p)
{
	int X= Projectile_p->XPos;
	int Y= Projectile_p->YPos- Projectile_p->ZPos/ 2;


	if (IsPosInPlayerLos ( X/ 32, Y/ 32,  Projectile_p))
	{
		int X_Screen= static_cast<int>(static_cast<float>(X)* (static_cast<float>(Aspect->x)/ static_cast<float>(parent->TAMAPTAPos.right)));
		int Y_Screen= static_cast<int>(static_cast<float>(Y)* (static_cast<float>(Aspect->y)/ static_cast<float>(parent->TAMAPTAPos.bottom)));

		if ((cruise| targetable_mask | stockpile_mask)==((cruise| targetable_mask | stockpile_mask)& Projectile_p->Weapon->WeaponTypeMask))
		{//
			DrawNuke ( PixelBits, Aspect, X_Screen, Y_Screen, Projectile_p);
		}
		else
		{
			DrawWeapon ( PixelBits, Aspect, X_Screen, Y_Screen);
		}
	}
}

void  ProjectileMap::DrawNuke (LPBYTE PixelBits, POINT * Aspect, int X_screen,  int Y_screen, ProjectileStruct * Projectile_p)
{
	POINT NukeAspect;
	LPBYTE Bits;
	if ((Bits= myUnitMap->NukePicture ( Projectile_p->myLos_PlayerID,  &Bits, &NukeAspect)))
	{
		int TransparentColor= myUnitMap->GetTransparentColor ( );
		X_screen= X_screen- NukeAspect.x/ 2;
		Y_screen= Y_screen- NukeAspect.y/ 2;

		int Width= NukeAspect.x;
		int Height=NukeAspect.y;

		if (X_screen<0)
		{
			Width= Width+ X_screen;
			X_screen= 0;
		}
		if (Y_screen<0)
		{
			Height= Height+ Y_screen;
			Y_screen= 0;
		}
		if (Aspect->x<(static_cast<int>(X_screen)+Width))
		{
			Width= Aspect->x- X_screen;
		}
		if (Aspect->y<(static_cast<int>(Y_screen)+ Height))
		{
			Height= Aspect->y- Y_screen;
		}

		try
		{
			for ( int i= 0; i<Height; ++i)
			{
				int SrcLine= i* NukeAspect.x;
				int Line= (i+ Y_screen)* Aspect->x;
				for ( int j= 0; j<Width; ++j)
				{
					register int Color= Bits[SrcLine+ j];
					if (Color!=TransparentColor)
					{
						PixelBits[Line+ j+ X_screen]= Color;
					}
				}
			}
		}
		catch (...)
		{
			;
		}

	}
	else
	{
		CopyGafToBits ( PixelBits, Aspect, X_screen, Y_screen, Index2Frame_InSequence ( TAmainStruct_Ptr->nuclogo, TAmainStruct_Ptr->Players[Projectile_p->myLos_PlayerID].PlayerInfo->PlayerLogoColor));
	}
}

void  ProjectileMap::DrawWeapon (LPBYTE PixelBits, POINT*  Aspect, int X_screen,  int Y_screen)
{
	X_screen= X_screen- 1;
	Y_screen= Y_screen- 1;

	 int Width= 2;
	 int Height=2;

	if (X_screen<0)
	{
		Width= Width+ X_screen;
		X_screen= 0;
	}
	if (Y_screen<0)
	{
		Height= Height+ Y_screen;
		Y_screen= 0;
	}
	if (Aspect->x<(static_cast<int>(X_screen)+Width))
	{
		Width= Aspect->x- X_screen;
	}
	if (Aspect->y<(static_cast<int>(Y_screen)+ Height))
	{
		Height= Aspect->y- Y_screen;
	}

	register int Color= ProjectColor;
	for ( int i= 0; i<Height; ++i)
	{
		int Line= (i+ Y_screen)* Aspect->x;
		for ( int j= 0; j<Width; ++j)
		{
			PixelBits[Line+ j+ X_screen]= Color;
		}
	}
}


#endif