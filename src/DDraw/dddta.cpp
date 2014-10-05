#include "oddraw.h"
#include "MinimapHandler.h"
#include "dddta.h"
#include "iddrawsurface.h"
#include <stdio.h>
#include <windows.h>
#include <dsound.h>
#include "whiteboard.h"
#include "tamem.h"
#include "tafunctions.h"

#define OutptTxt IDDrawSurface::OutptTxt
char* __cdecl ChatStrnCpy(char *strDest, const char *strSource, size_t count );
void __cdecl DeleteMem(void *Mem);

bool display = false;
int offset;
extern HINSTANCE HInstance;

TAdynmemStruct *gTAdynmem;
ParticleBase *gSmoke;
SharedMem *gDDDSharedMem;

CDDDTA::CDDDTA()
{
	LocalShare->DDDTA = this;

	int *PTR = (int*)0x00511de8;
	TAdynmem = (TAdynmemStruct*)(*PTR);

	Smoke = (ParticleBase*)0x51e610;

	TAhWnd = GetForegroundWindow();

	TA3dEnabled = false;
	SoundEnabled = false;
	lpDs3dListener = NULL;

	MapView= NULL;
	MemMap= NULL;
	if(DataShare->TAProgress == TAInGame && DataShare->ta3d)
		InitDDDTA();

}

CDDDTA::~CDDDTA()
{
	if(MapView!=NULL)
		UnmapViewOfFile(MapView);

	MapView= NULL;
	if(MemMap != NULL)
		CloseHandle(MemMap);
	MemMap= NULL;
}

void CDDDTA::DeInitDDDTA()
{
	RestoreAllSharedTAMem();

	int adress = 0x80A36;
	WriteProcessMemory(GetCurrentProcess(), (void*)0x463D26, &adress, 4, NULL);

	adress = 0x000B63B8;
	WriteProcessMemory(GetCurrentProcess(), (void*)0x4221E4, &adress, 4, NULL);

	//MinimapHandler.DeinitMinimap();

	TA3dEnabled = false;

	if(MapView!=NULL)
		UnmapViewOfFile(MapView);
	MapView = NULL;

	if(MemMap != NULL)
	{
		CloseHandle ( MemMap);
	}
	MemMap = NULL;
}

void CDDDTA::Print(HDC hdc, PrimitiveStruct *Object)
{
	char Outstr[50];

	wsprintf(Outstr, "%s Turn, x:%i y:%i z:%i", Object->PrimitiveInfo->Name, 
		(int)Object->XTurn,
		(int)Object->YTurn,
		(int)Object->ZTurn);
	TextOut(hdc, 200 , 35 + offset, Outstr, strlen(Outstr));
	wsprintf(Outstr, "%s Pos, x:%i y:%i z:%i", Object->PrimitiveInfo->Name, 
		(int)Object->XPos,
		(int)Object->YPos,
		(int)Object->ZPos);
	TextOut(hdc, 200 , 50 + offset , Outstr, strlen(Outstr));

	wsprintf(Outstr, "%s visible, %i", Object->PrimitiveInfo->Name, 
		(int)Object->Visible.Visible);
	TextOut(hdc, 200 , 65 + offset , Outstr, strlen(Outstr));

	offset += 45;

	if(Object->SiblingObject)
		Print(hdc, Object->SiblingObject);
	if(Object->ChildObject)
		Print(hdc, Object->ChildObject);
}

void CDDDTA::Blit(LPDIRECTDRAWSURFACE DestSurf)
{

	if(DataShare->ta3d && !SoundEnabled)
	{
		//secondary buffer, enable sound
		int adress = 0x4092;
		WriteProcessMemory(GetCurrentProcess(), (void*)0x4CF3CF, &adress, 4, NULL);
		SoundEnabled = true;
	}

#ifndef _DEBUG
	return;
#endif

	if(!display)
		return;

	HDC hdc;
	DestSurf->GetDC(&hdc);
	offset = 0;

	char Outstr[50];
	Outstr[0] = '\0';
	//Print(hdc, TAdynmem->Players[0].Units[0].Object3do->BaseObject);

	/*unsigned char *ptrn = (unsigned char*)&TAdynmem->Players[0].Units[1].Nanoframe;
	wsprintf(Outstr, "%i %i %i %i", (int)ptrn[0], (int)ptrn[1], (int)ptrn[2], (int)ptrn[3]);	
	TextOut(hdc, 200, 665, Outstr, strlen(Outstr));*/

	/*unsigned short (__stdcall *FindMouseUnit)(void);
	FindMouseUnit = (unsigned short (__stdcall *)(void))0x48CD80;

	int BeginUnitsArray_p  = FindMouseUnit();

	wsprintf(Outstr, "%i", BeginUnitsArray_p );	
	TextOut(hdc, 200, 690, Outstr, strlen(Outstr));*/

	for(int i=0; i<0x118; i++)
	{
		unsigned char *ptr = (unsigned char*)TAdynmem->Players[0].Units;

		char Outstr[20];

		if(1)
		{
			wsprintf(Outstr, "%1i : %1i", i, (int)(ptr[i]));	

			TextOut(hdc, 200 + (i/40)*80, 35 + ((i%40)*15), Outstr, strlen(Outstr));
		}
	}


	/*wsprintf(Outstr, "%i", (int)TAdynmem->Players[0].Units[0].Kills);	
	TextOut(hdc, 200, 200, Outstr, strlen(Outstr));
	wsprintf(Outstr, "%i", (int)TAdynmem->UnitDef[TAdynmem->BuildNum].FootY);	
	TextOut(hdc, 200, 220, Outstr, strlen(Outstr));
	wsprintf(Outstr, "%i", (int)TAdynmem->UnitDef[40].FootX);	
	TextOut(hdc, 200, 240, Outstr, strlen(Outstr));*/
	//wsprintf(Outstr, "%i", (int)TAdynmem->Players[0].Units[0].Kills);	
	//TextOut(hdc, 200, 260, Outstr, strlen(Outstr));

	DestSurf->ReleaseDC(hdc);
}

void CDDDTA::InitDDDTA()
{
	bool bExists;

	//create the mapping to the file
	MemMap = CreateFileMapping((HANDLE)0xFFFFFFFF,
		NULL,
		PAGE_READWRITE,
		0,
		sizeof(SharedMem),
		"TA3D");

	//see weather this is the first time this file has been mapped to
	bExists = (GetLastError() == ERROR_ALREADY_EXISTS);

	//Map a view into the Mapped file
	MapView = MapViewOfFile(MemMap,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		sizeof(SharedMem));

	if (!bExists)
		memset((SharedMem*)MapView, NULL, sizeof(SharedMem));

	DDDSharedMem = (SharedMem*)MapView;



	lstrcpyA(DDDSharedMem->mapname, TAdynmem->GameingState_Ptr->TNTFile);

	if(!DataShare->MaxUnits)
		DataShare->MaxUnits = 500;

	DDDSharedMem->maxUnits = DataShare->MaxUnits;


	DDDSharedMem->numPlayers = 0;
	for(int i=0; i<10; i++)
	{
		DDDSharedMem->players[i].color = TAdynmem->Players[i].PlayerInfo->PlayerLogoColor;
		lstrcpyA(DDDSharedMem->players[i].name, TAdynmem->Players[i].Name);

		if(TAdynmem->Players[i].PlayerActive)
			DDDSharedMem->numPlayers++;
	}

	for(int i=0; i<256; i++)
	{
		lstrcpyA(DDDSharedMem->weapons[i].name, TAdynmem->Weapons[i].WeaponName);
	}

	try
	{
		DDDSharedMem->NumFeatureDef = TAdynmem->NumFeatureDefs;
		CreateSharedTAMem((int)&TAdynmem->FeatureDef, (TAdynmem->NumFeatureDefs+ 1)* 0x100, "FeatureDef");

		DDDSharedMem->FeatureMapXSize = TAdynmem->FeatureMapSizeX;
		DDDSharedMem->FeatureMapYSize = TAdynmem->FeatureMapSizeY;
		CreateSharedTAMem((int)&TAdynmem->Features, TAdynmem->FeatureMapSizeX*TAdynmem->FeatureMapSizeY*sizeof(FeatureStruct), "Features");

		DDDSharedMem->WreckageArraySize = 0x18000;
		CreateSharedTAMem((int)&TAdynmem->WreckageInfo, 0x18000, "WreckageInfo");
	}
	catch(...)
	{
		//wreckage array mindre än 5000 enheter
	}

	//replace the TAtext strincopy
	int adress = ((int)ChatStrnCpy)-0x463D2A;
	WriteProcessMemory(GetCurrentProcess(), (void*)0x463D26, &adress, 4, NULL);

	//replace TA deallocation of wreackagearray
	adress = ((int)DeleteMem)-0x4221E8;
	WriteProcessMemory(GetCurrentProcess(), (void*)0x4221E4, &adress, 4, NULL);

	ShowText = (void (__stdcall *)(PlayerStruct *Player, char *Text, int Unk1, int Unk2))0x463E50;
	InterpretCommand = (void (__stdcall *)(char *Command, int Unk1))0x417B50;

	TADeleteMem = (void (__cdecl *)(void *Mem))0x4D85A0;

	//MinimapHandler.InitMinimap(DDDSharedMem);

	TA3dEnabled = true;
}

void CDDDTA::Moo()
{
	if(!display)
		return;

	struct posstruct{
		int x;
		int y;
	};

	void (__stdcall *TADrawCircle)(char *context, void *CirclePointer, posstruct *pos, int radius, int color, char *text, int unk);
	TADrawCircle = (void (__stdcall *)(char *context , void *CirclePointer, posstruct *pos, int radius, int color, char *text, int unk))0x438EA0;				

	int (__stdcall *GetContext)(char *ptr);
	GetContext = (int (__stdcall *)(char *ptr))0x4C5E70;				

	char buf[500];

	posstruct mo = {300,300};
	int onk = 0;

	if(GetContext(buf))
		TADrawCircle(buf, &TAdynmem->CameraToUnit, (posstruct*)&TAdynmem->Players[0].Units[0].XPos , 150, 0xc2, TADRCONFIGREGNAME, 1);

	//TADrawCircle(TAdynmem->RadarMapped, 10, 20, 15, 25, 0xC2);
	//TADrawCircle(TAdynmem->RadarPicture, 10, 20, 15, 25, 0xC2);
	//TestBuildSpot();

	/*tagRECT rect = {(TAdynmem->BuildPosRealX - TAdynmem->MapX) + 128,
	(TAdynmem->BuildPosRealY - TAdynmem->MapY) + 32 - (TAdynmem->Height/2),
	((TAdynmem->BuildPosRealX+200)  - TAdynmem->MapX) + 128,
	((TAdynmem->BuildPosRealY+200)  - TAdynmem->MapY) + 32 - (TAdynmem->Height/2)};

	tt(NULL, &rect, TAdynmem->BuildSpotState);*/


	//TAdynmem->MouseMapPosX = TAdynmem->MouseMapPosX-100;
	//TAdynmem->MouseMapPosY = TAdynmem->MouseMapPosY-100;

	//TestBuildSpot();
	//tt(&x, &y, 0xdd);

	//TAdynmem->MouseMapPosX = TAdynmem->MouseMapPosX+200;
	//TAdynmem->MouseMapPosY = TAdynmem->MouseMapPosY+200;

	//TestBuildSpot();
	//tt(&x, &y, 0xdd);

	//4197D0 (void)  nop - buildrektangeln stannar p?samma plats
	//4BF8C0 drawrectangel? (struct(resx, resy), struct(posx, posy), int color?)
	//maybe 4CC7AB
}

bool CDDDTA::Message(HWND WinProchWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
#ifndef _DEBUG
	return false;
#endif

	__try
	{

		switch(Msg)
		{
		case WM_KEYDOWN:
			if(wParam == 0x43)
			{

				/*if(display)
				display = false;
				else
				display = true;*/

				TAdynmem->Players[0].Units[1].Turn.X += 0x0500;
			}



			/*gTAdynmem = TAdynmem;
			if(lpDs3dListener==NULL)
			lpDsbPrimary->QueryInterface(IID_IDirectSound3DListener, 
			&lpDs3dListener); */

			//TAdynmem->DSound->DirectsoundBuffer2->Release();

			/*DSBUFFERDESC bufdesc;
			bufdesc.dwSize = sizeof(DSBUFFERDESC);
			bufdesc.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_GLOBALFOCUS;
			bufdesc.dwBufferBytes = NULL;
			bufdesc.dwReserved = NULL;
			bufdesc.lpwfxFormat = NULL;

			TAdynmem->DSound->Directsound->CreateSoundBuffer(&bufdesc, &TAdynmem->DSound->DirectsoundBuffer, NULL);
			*/
			//TAdynmem->DSound->Directsound->SetCooperativeLevel(WinProchWnd, DSSCL_NORMAL);

			//InitDDDTA();

			//DeInitDDDTA();


			//ShowText(&TAdynmem->Players[0], "onk", 4, 0);
			//InterpretCommand("los", 3);

			/*	char buf[100];
			for(int i=0; i<10; i++)
			{
			if(TAdynmem->Players[i].PlayerActive)
			{
			wsprintf(buf, "Player %i %s active, UnitDef: %i", i, (int)TAdynmem->UnitDef);
			OutptTxt(buf);
			}
			}*/





			//int *x = (int*)0x12fcfc;
			//int *y = (int*)0x12FD00;


			//baseptr+2CAA + 2 = xled markör
			//baseptr+2CAA + 2 + 8 = yled markör
			//baseptr+2CC4 = buildselected num

			//unitdef 14A, short footx, short footy

			//(*x) = 500;
			//(*y) = 500;


			/*for(int x=0; x<TAdynmem->FeatureMapSizeX; x++)
			for(int y=0; y<TAdynmem->FeatureMapSizeY; y++)
			if(TAdynmem->Features[x+y*TAdynmem->FeatureMapSizeY].FeatureDefIndex >= 0)
			{
			wsprintf(buf, "Feature on  %i %i, %s %s", x, y, 
			TAdynmem->FeatureDef[TAdynmem->Features[x+y*TAdynmem->FeatureMapSizeY].FeatureDefIndex].Name,
			TAdynmem->FeatureDef[TAdynmem->Features[x+y*TAdynmem->FeatureMapSizeY].FeatureDefIndex].Description);
			OutptTxt(buf);
			}*/


			/*if(display)
			display = false;
			else
			display = true;*/

			//int adress = ((int)HookFunc)-0x465572;
			//WriteProcessMemory(GetCurrentProcess(), (void*)0x46556E, &adress, 4, NULL);

			break;
		case WM_KEYUP:
			if(wParam == 67) //c
			{

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

void CDDDTA::FrameUpdate()
{
	if(!TA3dEnabled)
		return;

	gTAdynmem = TAdynmem;
	gSmoke = Smoke;
	gDDDSharedMem = DDDSharedMem;

	WriteUnits();
	WriteProjectiles();
	WriteExplosion();
	WriteSmoke();
	//MinimapHandler.FrameUpdate();
	((AlliesWhiteboard*)LocalShare->Whiteboard)->GetMarkers(DDDSharedMem->Markers);
	CheckMessages();

	TAdynmem->MapXScrollingTo = static_cast<int>(DDDSharedMem->camX);
	if(TAdynmem->MapXScrollingTo<0)
		TAdynmem->MapXScrollingTo = 0;
	if(TAdynmem->MapXScrollingTo>GetMaxScrollX())
		TAdynmem->MapXScrollingTo = GetMaxScrollX();
	TAdynmem->MapYScrollingTo = static_cast<int>(DDDSharedMem->camY);
	if(TAdynmem->MapYScrollingTo<0)
		TAdynmem->MapYScrollingTo = 0;
	if(TAdynmem->MapYScrollingTo>GetMaxScrollY())
		TAdynmem->MapYScrollingTo = GetMaxScrollY();




	/*	try
	{
	WriteUnits();
	}
	catch(...)
	{
	OutptTxt("WriteUnits failed");
	}
	try
	{
	WriteProjectiles();
	}
	catch(...)
	{
	OutptTxt("WriteProjectiles failed");
	}
	try
	{
	WriteExplosion();
	}
	catch(...)
	{
	OutptTxt("WriteExplosion failed");
	}
	try
	{
	WriteSmoke();
	}
	catch(...)
	{
	OutptTxt("WriteSmoke failed");
	}
	try
	{
	CheckMessages();
	}
	catch(...)
	{
	OutptTxt("CheckMessages failed");
	}*/

	DDDSharedMem->updated = true;

}

void CDDDTA::WriteUnits()
{
	for(int player=0; player<DDDSharedMem->numPlayers; player++)
	{
		int units=0;

		for(int i=0; i<DDDSharedMem->maxUnits; i++)
		{
			if(TAdynmem->Players[player].Units[i].UnitType)  //BeginUnitsArray_p  active
			{
				SetActiveUnit(&DDDSharedMem->units[units + player*DataShare->MaxUnits], &TAdynmem->Players[player].Units[i]);
				units++;
			}

		}

		DDDSharedMem->players[player].maxUsedUnit = units;
	}
}

void CDDDTA::WriteProjectiles()
{
	DDDSharedMem->numProjectiles = TAdynmem->NumProjectiles;

	for(int i=0; i<TAdynmem->NumProjectiles; i++)
	{
		DDDSharedMem->projectiles[i].pos.x = TAdynmem->Projectiles[i].XPos;
		DDDSharedMem->projectiles[i].pos.y = TAdynmem->Projectiles[i].YPos;
		DDDSharedMem->projectiles[i].pos.z = TAdynmem->Projectiles[i].ZPos;
		DDDSharedMem->projectiles[i].pos2.x = TAdynmem->Projectiles[i].XPosStart;
		DDDSharedMem->projectiles[i].pos2.y = TAdynmem->Projectiles[i].YPosStart;
		DDDSharedMem->projectiles[i].pos2.z = TAdynmem->Projectiles[i].ZPosStart;

		DDDSharedMem->projectiles[i].turn.x = TAdynmem->Projectiles[i].XTurn;
		DDDSharedMem->projectiles[i].turn.y = TAdynmem->Projectiles[i].YTurn;
		DDDSharedMem->projectiles[i].turn.z = TAdynmem->Projectiles[i].ZTurn;

		DDDSharedMem->projectiles[i].type = TAdynmem->Projectiles[i].Weapon->ID;
	}

}

void CDDDTA::WriteExplosion()
{
	DDDSharedMem->numExplosions = TAdynmem->NumExplosions;

	for(int i=0; i<TAdynmem->NumExplosions; i++)
	{
		if(TAdynmem->Explosions[i].Debris)
		{
			DDDSharedMem->explosions[i].isDebris = true;

			DDDSharedMem->explosions[i].pos.x = TAdynmem->Explosions[i].XPos;
			DDDSharedMem->explosions[i].pos.y = TAdynmem->Explosions[i].YPos;
			DDDSharedMem->explosions[i].pos.z = TAdynmem->Explosions[i].ZPos;

			DDDSharedMem->explosions[i].turn.x = TAdynmem->Explosions[i].XTurn;
			DDDSharedMem->explosions[i].turn.y = TAdynmem->Explosions[i].YTurn;
			DDDSharedMem->explosions[i].turn.z = TAdynmem->Explosions[i].ZTurn;

			for(int vert=0; vert<4; vert++)
			{
				DDDSharedMem->explosions[i].vertices[vert].x = TAdynmem->Explosions[i].Debris->Vertices[vert].x;
				DDDSharedMem->explosions[i].vertices[vert].y = TAdynmem->Explosions[i].Debris->Vertices[vert].y;
				DDDSharedMem->explosions[i].vertices[vert].z = TAdynmem->Explosions[i].Debris->Vertices[vert].z;
			}

			lstrcpyA(DDDSharedMem->explosions[i].name, "glow2");
		}
		else
		{
			DDDSharedMem->explosions[i].isDebris = false;

			if(TAdynmem->Explosions[i].FXGaf)
				lstrcpyA(DDDSharedMem->explosions[i].name, TAdynmem->Explosions[i].FXGaf->Name);
			else
				DDDSharedMem->explosions[i].name[0] = 0;

			DDDSharedMem->explosions[i].pos.x = TAdynmem->Explosions[i].XPos;
			DDDSharedMem->explosions[i].pos.y = TAdynmem->Explosions[i].YPos;
			DDDSharedMem->explosions[i].pos.z = TAdynmem->Explosions[i].ZPos;

			DDDSharedMem->explosions[i].frame = TAdynmem->Explosions[i].Frame;
		}
	}
}

void CDDDTA::WriteSmoke()
{
	int particle = 0;

	TAdynmemStruct *lTAdynmem = TAdynmem;
	ParticleBase *lSmoke = Smoke;
	SharedMem *lDDDSharedMem = DDDSharedMem;

	for(int i=0; i<Smoke->curParticles; i++)
	{
		if((*Smoke->ParticlePTRArray[i]).firstDraw)
		{
			switch((*Smoke->ParticlePTRArray[i]).Type)
			{
			case 1:
				int n = ((int)(*Smoke->ParticlePTRArray[i]).lastDraw-(int)(*Smoke->ParticlePTRArray[i]).firstDraw)/0x20;
				DDDSharedMem->smoke[particle].numSub = 0;

				for(int j=0; j<n && (*Smoke->ParticlePTRArray[i]).firstDraw[j].MoreSubs; j++)
				{
					DDDSharedMem->smoke[particle].subs[j].frame = (*Smoke->ParticlePTRArray[i]).firstDraw[j].frame;

					DDDSharedMem->smoke[particle].subs[j].pos.x = (*Smoke->ParticlePTRArray[i]).firstDraw[j].XPos;
					DDDSharedMem->smoke[particle].subs[j].pos.y = (*Smoke->ParticlePTRArray[i]).firstDraw[j].YPos;
					DDDSharedMem->smoke[particle].subs[j].pos.z = (*Smoke->ParticlePTRArray[i]).firstDraw[j].ZPos;
					lstrcpyA(DDDSharedMem->smoke[particle].name, (*Smoke->ParticlePTRArray[i]).firstDraw[j].gaf->Name);

					DDDSharedMem->smoke[particle].numSub++;

				}
				particle++;
				break;

			}
		}
	}

	DDDSharedMem->numSmoke = particle;
}

void inline CDDDTA::SetActiveUnit(BeginUnitsArray_p  *DestUnit, UnitStruct *TAUnit)
{
	DestUnit->active = 1;
	lstrcpyA(DestUnit->name, TAUnit->UnitType->ObjectName);
	DestUnit->pos.x = TAUnit->XPos;
	DestUnit->pos.y = TAUnit->YPos;
	DestUnit->pos.z = TAUnit->ZPos;

	DestUnit->turn.x = TAUnit->Turn.X;
	DestUnit->turn.y = TAUnit->Turn.Y;
	DestUnit->turn.z = TAUnit->Turn.Y;

	DestUnit->beingBuilt = (int)TAUnit->Nanoframe;

	DestUnit->health = TAUnit->HealthPerA;
	DestUnit->kills = TAUnit->Kills;
	DestUnit->RecentDamage = TAUnit->RecentDamage;

	PartOffset = 0;
	if(TAUnit->Object3do)
		WriteSubPart(DestUnit->parts, TAUnit->Object3do->BaseObject);

}

void CDDDTA::WriteSubPart(SubPart *Part, PrimitiveStruct *Primitive)
{

	Part[PartOffset].turn.x = Primitive->XTurn;
	Part[PartOffset].turn.y = Primitive->YTurn;
	Part[PartOffset].turn.z = Primitive->ZTurn;
	Part[PartOffset].offset.x = Primitive->XPos;
	Part[PartOffset].offset.y = Primitive->YPos;
	Part[PartOffset].offset.z = Primitive->ZPos;
	Part[PartOffset].visible = Primitive->Visible.Visible;

	PartOffset++;

	if(Primitive->ChildObject)
		WriteSubPart(Part, Primitive->ChildObject);
	if(Primitive->SiblingObject)
		WriteSubPart(Part, Primitive->SiblingObject);

}

void CDDDTA::CheckMessages()
{
	for(int i=0; i<10; i++)
	{
		if(DDDSharedMem->toDDraw.hasText[i])
		{
			if(lstrcmpiA(DDDSharedMem->toDDraw.text[i], "pause")==0)
			{
				LocalShare->TAWndProc(TAhWnd, WM_KEYDOWN, VK_PAUSE, 0);
			}
			else if(lstrcmpiA(DDDSharedMem->toDDraw.text[i], "speedup")==0)
			{
				LocalShare->TAWndProc(TAhWnd, WM_CHAR, '+', 0);
			}
			else if(lstrcmpiA(DDDSharedMem->toDDraw.text[i], "speeddown")==0)
			{
				LocalShare->TAWndProc(TAhWnd, WM_CHAR, '-', 0);
			}
			else
			{
				ShowText(&TAdynmem->Players[0], DDDSharedMem->toDDraw.text[i], 4, 0);
			}


			DDDSharedMem->toDDraw.hasText[i] = false;

		}
	}
}

void CDDDTA::CreateSharedTAMem(int TAPTR, int Size, char *Name)
{
	char buf[50];

	wsprintf(buf, "TAPTR: %i", TAPTR);
	OutptTxt(buf);

	wsprintf(buf, "*((int*)TAPTR): %i", *((int*)TAPTR));
	OutptTxt(buf);

	TAShareMemStruct ShareMem;
	ShareMem.Size = Size;
	ShareMem.TAAcessPTR = TAPTR;
	ShareMem.OldMem = *((int*)TAPTR);

	bool bExists;

	ShareMem.MemMap = CreateFileMapping((HANDLE)0xFFFFFFFF, NULL, PAGE_READWRITE, 0, Size, Name);
	bExists = (GetLastError() == ERROR_ALREADY_EXISTS);
	ShareMem.MapView = MapViewOfFile(ShareMem.MemMap, FILE_MAP_ALL_ACCESS, 0, 0, Size);

	if ((!bExists)&&(NULL!=MapView))
	{
		memset(ShareMem.MapView, NULL, Size);
	}
	SharedTAMemList.push_front(ShareMem);

	//change TA pointer
	(*((int*)TAPTR)) = (int)ShareMem.MapView;

	//copy over data to new mem
	if ((NULL!=MapView)&&(NULL!=ShareMem.OldMem))
	{
		memcpy(ShareMem.MapView, (void*)ShareMem.OldMem, Size);
	}


	//zero old mem
	//memset((void*)ShareMem.OldMem, 0, Size);
}

void CDDDTA::RestoreAllSharedTAMem()
{
	for(size_t i=0; i<SharedTAMemList.size(); i++)
	{
		//copy mem from sharemapping to tamem
		//memcpy((void*)SharedTAMemList[i].OldMem, SharedTAMemList[i].MapView, SharedTAMemList[i].Size);

		//restore TA pointer
		(*((int*)SharedTAMemList[i].TAAcessPTR)) = SharedTAMemList[i].OldMem;

		//delete mapping object
		if(SharedTAMemList[i].MapView!=NULL)
			UnmapViewOfFile(SharedTAMemList[i].MapView);
		if(SharedTAMemList[i].MemMap != NULL)
			CloseHandle(SharedTAMemList[i].MemMap);
	}

	SharedTAMemList.clear();
}

void CDDDTA::SendTextToSpring(const char *Text)
{
	bool written = false;

	for(int i=0; i<10; i++)
	{
		if(!DDDSharedMem->to3D.hasText[i])
		{
			lstrcpyA(DDDSharedMem->to3D.text[i], Text);
			written = true;
			DDDSharedMem->to3D.hasText[i] = true;
			break;
		}
	}
	if(!written) //owerwrite first text
	{
		lstrcpyA(DDDSharedMem->to3D.text[0], Text);
		DDDSharedMem->to3D.hasText[0] = true;
	}
}

char* __cdecl ChatStrnCpy(char *strDest, const char *strSource, size_t count )
{

	((CDDDTA*)LocalShare->DDDTA)->SendTextToSpring(strSource);

	char* (__cdecl *tastrncpy)(char*, const char*, size_t);
	tastrncpy = (char* (__cdecl *)(char*, const char*, size_t))0x4E4760;
	return tastrncpy(strDest, strSource, count);
}

void __cdecl DeleteMem(void *Mem)
{
	((CDDDTA*)LocalShare->DDDTA)->DeInitDDDTA();

	((CDDDTA*)LocalShare->DDDTA)->TADeleteMem(Mem);
}