#pragma once

struct ProjectileStruct;
struct TAdynmemStruct;
class UnitsMinimap;
class FullScreenMinimap;
class ProjectileMap
{
public:
	ProjectileMap::ProjectileMap (FullScreenMinimap * par_arg, UnitsMinimap * UnitMap_p);
	~ProjectileMap ();

	void NowDrawProjectile  (LPBYTE PixelBits, POINT * AspectSrc);
private:
	BOOL IsPosInPlayerLos (unsigned int XPos_Tile, unsigned int YPos_Tile, ProjectileStruct * Projectile_p);
	void DrawProjectile (LPBYTE PixelBits, POINT *Aspect, ProjectileStruct * Projectile_p);
	void DrawNuke (LPBYTE PixelBits, POINT * Aspect, int X_screen,  int Y_screen, ProjectileStruct * Projectile_p);
	void DrawWeapon (LPBYTE PixelBits, POINT * Aspect, int X_screen,  int Y_screen);

	TAdynmemStruct * TAmainStruct_Ptr;
	int ProjectColor;

	UnitsMinimap * myUnitMap;
	FullScreenMinimap * parent;
};