//---------------------------------------------------------------------------
#include "oddraw.h"
#include "iddraw.h"

#include "iddrawsurface.h"

#include <vector>
using namespace std;

#include "weaponid.h"
#include <stdio.h>
#include "hook/etc.h"
#include "hook/hook.h"
#include "UnicodeSupport.h"
#include "MenuResolution.h"
#include "ChallengeResponse.h"

#include "tamem.h"
#include "tafunctions.h"
#include "ExternQuickKey.h"
#include "LimitCrack.h"
#include "taHPI.h"
#include "TAbugfix.h"
#include "fullscreenminimap.h"
#include "GUIExpand.h"
#include "StartPositions.h"
#include "AutoTeam.h"

#include "TAConfig.h"
//---------------------------------------------------------------------------
#include "ddraw.h"
//---------------------------------------------------------------------------


//#define DDRAW_INIT_STRUCT(ddstruct) { memset(&ddstruct,0,sizeof(ddstruct)); ddstruct.dwSize=sizeof(ddstruct); }

DataShare_* DataShare;
HANDLE hMemMap;
void *pMapView;

LocalShare_* LocalShare;
HANDLE LocalhMemMap;
void *LocalpMapView;

HANDLE TAHookhMemMap;
void *TAHookpMapView;



HINSTANCE SDDraw = NULL;

bool Windowed;
HINSTANCE HInstance;

InlineSingleHook * AddtionInitHook;
InlineSingleHook * AddtionInitAfterDDrawHook;// 

SingleHook * WndProc_SH;

int __stdcall AddtionInit (PInlineX86StackBuffer X86StrackBuffer)
{
	//EnableSound();

	IDDrawSurface::OutptTxt ("Init TAHPI");
	TAHPI= NULL;
	TAHPI= new _TAHPI ( FALSE);
	//break limit

	//IDDrawSurface::OutptTxt ("Init TAConfig");
//	MyConfig = new TADRConfig ();

// 	IDDrawSurface::OutptTxt ("Install Limit Crack");
// 	NowCrackLimit= new LimitCrack;
// 
// 	FixTABug= new TABugFixing;
// 
// 	GUIExpander= new GUIExpand;

	return 0;
}

void AddtionRelease (void)
{

// 	IDDrawSurface::OutptTxt ("Uninstall Limit Crack");
// 	delete NowCrackLimit;
// 
// 	delete FixTABug;

	delete TAHPI;

//	delete MyConfig;

	IDDrawSurface::OutptTxt ("Release AddtionRoutine_CircleSelect");
	
	//delete self :D
	delete AddtionInitHook;
}
int __stdcall AddtionInitAfterDDraw (PInlineX86StackBuffer X86StrackBuffer)
{
	char FontName[0x100];
	FontName[0]= 0;
	MyConfig->GetIniStr ( "UnicodeSupport", FontName, 0x100, NULL);
	if (0!=FontName[0])
	{
		NowSupportUnicode= new UnicodeSupport ( FontName, MyConfig->GetIniInt ( "UnicodeSupport_Color", 0xffffff), MyConfig->GetIniInt ( "UnicodeSupport_Background", 0x000000));
	}
	else
	{
		NowSupportUnicode= NULL;
	}

	myExternQuickKey= new ExternQuickKey ;
 
	if ((*TAProgramStruct_PtrPtr)
		&&(*TAProgramStruct_PtrPtr)->TAClass_Hwnd)
	{// when TA is running, run another instance will cause TA hwnd is not create in here.
		LocalShare->TAWndProc = (WNDPROC)SetWindowLong ( (*TAProgramStruct_PtrPtr)->TAClass_Hwnd, GWL_WNDPROC, (long)WinProc);
	}
 	

	return 0;
}
void AddtionReleaseAfterDDraw (void)
{
	SetWindowLong ( (*TAProgramStruct_PtrPtr)->TAClass_Hwnd, GWL_WNDPROC, (long)LocalShare->TAWndProc);
	if (NULL!=NowSupportUnicode)
	{
		delete NowSupportUnicode;
	}
	
	delete AddtionInitAfterDDrawHook;
}

//---------------------------------------------------------------------------
bool APIENTRY DllMain(HINSTANCE hinst, unsigned long reason, void*)
{
	HInstance = hinst;
	IDDrawSurface::OutptTxt("DLL EntryPoint");

	if(reason==DLL_PROCESS_ATTACH)
	{
		IDDrawSurface::OutptTxt("Process Attached");

		/* 
			dplayx.dll TA Patch loader - https://github.com/FunkyFr3sh/Total-Annihilation-Patch-Loader 
			Does alter the registry path of the game so it must be loaded early before tdraw loads its settings
		*/
		LoadLibrary("dplayx.dll");
		
		SDDraw = LoadLibrary("ddraw.dll");
		SetupFileMap();
		DataShare = reinterpret_cast<DataSharePTR>(pMapView);
		SetupLocalFileMap();
		LocalShare = reinterpret_cast<LocalSharePTR>(LocalpMapView);

		if(!LocalShare)
			IDDrawSurface::OutptTxt("Error creating shared lmap");
		else
			IDDrawSurface::OutptTxt("lmap success");

		Windowed = false;
		SetupTAHookFileMap();

		DataShare->IsRunning = 5;

		EnableSound();

		IDDrawSurface::OutptTxt ("Init TAConfig");
		MyConfig = new TADRConfig ();

		IDDrawSurface::OutptTxt ("Install Limit Crack");
		NowCrackLimit= new LimitCrack;

		FixTABug= new TABugFixing;

		StartPositions::GetInstance();
		AutoTeam::Install();

		GUIExpander= new GUIExpand;

		AddtionInitAfterDDrawHook= new InlineSingleHook ( AddtionInitAfterDDrawAddr, 5, 
			INLINE_5BYTESLAGGERJMP, AddtionInitAfterDDraw);

		ChallengeResponse::GetInstance();

	}
	if(reason==DLL_PROCESS_DETACH)
	{
		/* KillTimer(NULL, Timer);
		KillTimer(NULL, DetectTimer); */
		AddtionReleaseAfterDDraw ( );
		AddtionRelease ( );

		IDDrawSurface::OutptTxt ("Uninstall Limit Crack");
		delete NowCrackLimit;

		delete FixTABug;

		delete MyConfig;

		IDDrawSurface::OutptTxt ("Release AddtionRoutine_CircleSelect");



		FreeLibrary(SDDraw);
		ShutdownLocalFileMap();
		ShutDownTAHookFileMap();
	}

	return 1;
}
//---------------------------------------------------------------------------

HRESULT WINAPI DirectDrawCreate(GUID FAR *lpGUID, LPDIRECTDRAW FAR *lplpDD, IUnknown FAR *pUnkOuter)
{
	IDDrawSurface::OutptTxt("DirectDrawCreate");
	if(SDDraw == NULL)
	{
		SDDraw = LoadLibrary("ddraw.dll");
		SetupFileMap();
		DataShare = reinterpret_cast<DataSharePTR>(pMapView);
	}

	DirectDrawCreateProc Proc;
	Proc = (DirectDrawCreateProc) GetProcAddress(SDDraw, "DirectDrawCreate");
	HRESULT Result = Proc(lpGUID, lplpDD, pUnkOuter);

	IDDraw *DClass = new IDDraw(*lplpDD, Windowed);
	*lplpDD = (IDirectDraw*)DClass;

	IDDrawSurface::OutptTxt("returning from DirectDrawCreate");
	Windowed = false;

	return Result;
}

//---------------------------------------------------------------------------
bool SetupFileMap()
{
	bool bExists;

	//create the mapping to the file
	hMemMap = CreateFileMapping((HANDLE)0xFFFFFFFF,
		NULL,
		PAGE_READWRITE,
		0,
		sizeof(DataShare_),
		"TADemo-MKChat");

	//see weather this is the first time this file has been mapped to
	bExists = (GetLastError() == ERROR_ALREADY_EXISTS);

	//Map a view into the Mapped file
	pMapView = MapViewOfFile(hMemMap,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		sizeof(DataShare_));


	if (!bExists)
	{
		//if it is the first time this map has been made,
		//set all the members of our struct to NULL
		memset (reinterpret_cast<DataSharePTR>(pMapView), NULL, sizeof(DataShare_));
	}

	return true;
}

bool SetupLocalFileMap()
{
	bool bExists;
	LocalhMemMap = CreateFileMapping((HANDLE)0xFFFFFFFF,
		NULL,
		PAGE_READWRITE,
		0,
		sizeof(LocalShare_),
		"TAOverlayShare");

	bExists = (GetLastError() == ERROR_ALREADY_EXISTS);
	LocalpMapView = MapViewOfFile(LocalhMemMap,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		sizeof(LocalShare_));

	if(!LocalpMapView)
		IDDrawSurface::OutptTxt("Error creating shared lmap");


	if (!bExists)
	{
		memset (reinterpret_cast<LocalSharePTR>(LocalpMapView), NULL, sizeof(LocalShare_));
	}
	return true;
}

void ShutdownFileMap()
{
	//cleanup
	if(pMapView!=NULL)
		UnmapViewOfFile(pMapView);

	if(hMemMap != NULL)
		CloseHandle(hMemMap);
}

void ShutdownLocalFileMap()
{
	//cleanup
	if(LocalpMapView!=NULL)
		UnmapViewOfFile(LocalpMapView);

	if(LocalhMemMap != NULL)
		CloseHandle(LocalhMemMap);
}

void SetupTAHookFileMap()
{
	//initiate a tahook filmap so the recorder detects tahook running
	bool bExists;
	TAHookhMemMap = CreateFileMapping((HANDLE)0xFFFFFFFF,
		NULL,
		PAGE_READWRITE,
		0,
		6084,  //correct size so tahook doesnt krash if yank is running it
		"GlobalMap");

	bExists = (GetLastError() == ERROR_ALREADY_EXISTS);
	TAHookpMapView = MapViewOfFile(TAHookhMemMap,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		6084);
	if (!bExists)
	{
		//memset (TAHookpMapView, NULL, 6084);
	}
}

void ShutDownTAHookFileMap()
{
	//cleanup
	if(TAHookpMapView!=NULL)
		UnmapViewOfFile(TAHookpMapView);

	if(TAHookhMemMap != NULL)
		CloseHandle(TAHookhMemMap);
}

void EnableSound()
{
	//primary buffer
	int adress = 0x4011;
	WriteProcessMemory(GetCurrentProcess(), (void*)0x4CEFE2, &adress, 4, NULL);

	//secondary buffer
	//adress = 0x4092;
	//WriteProcessMemory(GetCurrentProcess(), (void*)0x4CF3CF, &adress, 4, NULL);
}

/*
int WINAPI VidMemLargestFree(LPVOID arg1)
{
VidMemLargestFreeProc Proc;
Proc = (VidMemLargestFreeProc)GetProcAddress(SDDraw, "VidMemLargestFree");
return Proc(arg1);
}

int WINAPI VidMemAmountFree(LPVOID arg1)
{
VidMemAmountFreeProc Proc;
Proc = (VidMemAmountFreeProc)GetProcAddress(SDDraw, "VidMemAmountFree");
return Proc(arg1);
}

int WINAPI VidMemFini(LPVOID arg1)
{
VidMemFiniProc Proc;
Proc = (VidMemFiniProc)GetProcAddress(SDDraw, "VidMemFini");
return Proc(arg1);
}

int WINAPI VidMemFree(LPVOID arg1, LPVOID arg2)
{
VidMemFreeProc Proc;
Proc = (VidMemFreeProc)GetProcAddress(SDDraw, "VidMemFree");
return Proc(arg1, arg2);
}

int WINAPI VidMemAlloc(LPVOID arg1, LPVOID arg2, LPVOID arg3)
{
VidMemAllocProc Proc;
Proc = (VidMemAllocProc)GetProcAddress(SDDraw, "VidMemAlloc");
return Proc(arg1, arg2, arg3);
}

int WINAPI VidMemInit(LPVOID arg1, LPVOID arg2, LPVOID arg3, LPVOID arg4, LPVOID arg5)
{
VidMemInitProc Proc;
Proc = (VidMemInitProc)GetProcAddress(SDDraw, "VidMemInit");
return Proc(arg1, arg2, arg3, arg4, arg5);
}        */

FARPROC WINAPI GetAddress(PCSTR pszProcName)
{
	FARPROC fpAddress;

	fpAddress = GetProcAddress ( SDDraw, pszProcName);

	return fpAddress;
}

ALCDECL AheadLib_AcquireDDThreadLock(void)
{
	GetAddress("AcquireDDThreadLock");
	__asm JMP EAX;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 导出函数
ALCDECL AheadLib_CompleteCreateSysmemSurface(void)
{
	GetAddress("CompleteCreateSysmemSurface");
	__asm JMP EAX;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 导出函数
ALCDECL AheadLib_D3DParseUnknownCommand(void)
{
	GetAddress("D3DParseUnknownCommand");
	__asm JMP EAX;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 导出函数
ALCDECL AheadLib_DDGetAttachedSurfaceLcl(void)
{
	GetAddress("DDGetAttachedSurfaceLcl");
	__asm JMP EAX;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 导出函数
ALCDECL AheadLib_DDInternalLock(void)
{
	GetAddress("DDInternalLock");
	__asm JMP EAX;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 导出函数
ALCDECL AheadLib_DDInternalUnlock(void)
{
	GetAddress("DDInternalUnlock");
	__asm JMP EAX;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 导出函数
ALCDECL AheadLib_DSoundHelp(void)
{
	GetAddress("DSoundHelp");
	__asm JMP EAX;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 导出函数
ALCDECL AheadLib_DirectDrawCreateClipper(void)
{
	GetAddress("DirectDrawCreateClipper");
	__asm JMP EAX;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 导出函数
ALCDECL AheadLib_DirectDrawCreateEx(void)
{
	GetAddress("DirectDrawCreateEx");
	__asm JMP EAX;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 导出函数
ALCDECL AheadLib_DirectDrawEnumerateA(void)
{
	GetAddress("DirectDrawEnumerateA");
	__asm JMP EAX;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 导出函数
ALCDECL AheadLib_DirectDrawEnumerateExA(void)
{
	GetAddress("DirectDrawEnumerateExA");
	__asm JMP EAX;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 导出函数
ALCDECL AheadLib_DirectDrawEnumerateExW(void)
{
	GetAddress("DirectDrawEnumerateExW");
	__asm JMP EAX;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 导出函数
ALCDECL AheadLib_DirectDrawEnumerateW(void)
{
	GetAddress("DirectDrawEnumerateW");
	__asm JMP EAX;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 导出函数
ALCDECL AheadLib_DllCanUnloadNow(void)
{
	GetAddress("DllCanUnloadNow");
	__asm JMP EAX;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 导出函数
ALCDECL AheadLib_DllGetClassObject(void)
{
	GetAddress("DllGetClassObject");
	__asm JMP EAX;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 导出函数
ALCDECL AheadLib_GetDDSurfaceLocal(void)
{
	GetAddress("GetDDSurfaceLocal");
	__asm JMP EAX;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 导出函数
ALCDECL AheadLib_GetOLEThunkData(void)
{
	GetAddress("GetOLEThunkData");
	__asm JMP EAX;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 导出函数
ALCDECL AheadLib_GetSurfaceFromDC(void)
{
	GetAddress("GetSurfaceFromDC");
	__asm JMP EAX;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 导出函数
ALCDECL AheadLib_RegisterSpecialCase(void)
{
	GetAddress("RegisterSpecialCase");
	__asm JMP EAX;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 导出函数
ALCDECL AheadLib_ReleaseDDThreadLock(void)
{
	GetAddress("ReleaseDDThreadLock");
	__asm JMP EAX;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 导出函数
ALCDECL AheadLib_SetAppCompatData(void)
{
	GetAddress("SetAppCompatData");
	__asm JMP EAX;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////