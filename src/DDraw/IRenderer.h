// IArmyAI.h: interface for the IArmyAI class.
// Dont modify this file
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IARMYAI_H__7A933264_A3D8_4969_9003_3122E2512161__INCLUDED_)
#define AFX_IARMYAI_H__7A933264_A3D8_4969_9003_3122E2512161__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>

struct int3 {
	int x,y,z;
};
struct SubPart{
	bool visible;
	int3 offset;
	int3 turn;
};

struct BeginUnitsArray_p  {
	bool active;
	bool changed;
	char name[32];
	int beingBuilt;
	int3 pos;
	int3 turn;
	SubPart parts[32];
	char health;
	short kills;
	unsigned char RecentDamage;
};

struct Player{
	char name[32];
	int color;
	int maxUsedUnit;
};

struct WeaponDef{
	char name[32];
};

struct Projectile{
	int type;
	int3 pos;
	int3 pos2;
	int3 turn;
};

struct Explosion{
	bool isDebris;
	char name[32];
	int frame;
	int3 pos;
	int3 vertices[4];
	int3 turn;
};

struct SmokeSub {
	int3 pos;
	int frame;
};

struct SmokeParticle{
	char name[32];
	int numSub;
	SmokeSub subs[10];
};

struct TextInterface{
	char text[10][256];
	bool hasText[10];
};

struct MarkerArray{
	int x;
	int y;
	bool IsNew;
};

struct SharedMem{
	TextInterface to3D;
	TextInterface toDDraw;
	char mapname[MAX_PATH];
	int maxUnits;
	int numPlayers;
	int numProjectiles;
	int numExplosions;
	int numSmoke;
	bool updated;
	Player players[10];
	BeginUnitsArray_p  units[20000];
	Projectile projectiles[300];
	WeaponDef weapons[256];
	Explosion explosions[300];
	SmokeParticle smoke[1000];
	int NumFeatureDef;
	int FeatureMapXSize;
	int FeatureMapYSize;
	int WreckageArraySize;
	float camX;
	float camY;
	int RadarpicX;
	int RadarpicY;
	MarkerArray Markers[500];
};

#endif // !defined(AFX_IARMYAI_H__7A933264_A3D8_4969_9003_3122E2512161__INCLUDED_)
