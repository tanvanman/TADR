#pragma once

#define EXTERNC extern "C"
#define NAKED __declspec(naked)
#define EXPORT __declspec(dllexport)

#define ALCPP EXPORT NAKED
#define ALSTD EXTERNC EXPORT NAKED void __stdcall
#define ALCFAST EXTERNC EXPORT NAKED void __fastcall
#define ALCDECL EXTERNC NAKED void __cdecl


typedef HRESULT (*DirectDrawCreateProc)(GUID FAR *, LPDIRECTDRAW FAR *, IUnknown FAR *);
HRESULT WINAPI DirectDrawCreate(GUID FAR *lpGUID, LPDIRECTDRAW FAR *lplpDD, IUnknown FAR *pUnkOuter);

/*typedef int WINAPI (*VidMemLargestFreeProc)(LPVOID);
extern "C" __declspec(dllexport) int WINAPI VidMemLargestFree(LPVOID arg1);
typedef int WINAPI (*VidMemAmountFreeProc)(LPVOID);
extern "C" __declspec(dllexport) int WINAPI VidMemAmountFree(LPVOID arg1);
typedef int WINAPI (*VidMemFiniProc)(LPVOID);
extern "C" __declspec(dllexport) int WINAPI VidMemFini(LPVOID arg1);
typedef int WINAPI (*VidMemFreeProc)(LPVOID, LPVOID);
extern "C" __declspec(dllexport) int WINAPI VidMemFree(LPVOID arg1, LPVOID arg2);
typedef int WINAPI (*VidMemAllocProc)(LPVOID, LPVOID, LPVOID);
extern "C" __declspec(dllexport) int WINAPI VidMemAlloc(LPVOID arg1, LPVOID arg2, LPVOID arg3);
typedef int WINAPI (*VidMemInitProc)(LPVOID, LPVOID, LPVOID, LPVOID, LPVOID);
extern "C" __declspec(dllexport) int WINAPI VidMemInit(LPVOID arg1, LPVOID arg2, LPVOID arg3, LPVOID arg4, LPVOID arg5);
*/


bool SetupFileMap();
void ShutdownFileMap();
bool SetupLocalFileMap();
void ShutdownLocalFileMap();
void GetSysDir();

void SetupTAHookFileMap();
void ShutDownTAHookFileMap();
void EnableSound();
//void ReplaceTAProc();



// ALCDECL AheadLib_AcquireDDThreadLock(void);
// 
// ALCDECL AheadLib_CompleteCreateSysmemSurface(void);
// 
//  
// ALCDECL AheadLib_D3DParseUnknownCommand(void);
// ALCDECL AheadLib_DDGetAttachedSurfaceLcl(void);
// ALCDECL AheadLib_DDInternalLock(void);
// ALCDECL AheadLib_DDInternalUnlock(void);
// ALCDECL AheadLib_DSoundHelp(void);
// ALCDECL AheadLib_DirectDrawCreate(void);
// ALCDECL AheadLib_DirectDrawCreateClipper(void);
// ALCDECL AheadLib_DirectDrawCreateEx(void);
// ALCDECL AheadLib_DirectDrawEnumerateA(void);
// ALCDECL AheadLib_DirectDrawEnumerateExA(void);
// ALCDECL AheadLib_DirectDrawEnumerateExW(void);
// ALCDECL AheadLib_DirectDrawEnumerateW(void);
// ALCDECL AheadLib_DllCanUnloadNow(void);
// ALCDECL AheadLib_DllGetClassObject(void);
// ALCDECL AheadLib_GetDDSurfaceLocal(void);
// ALCDECL AheadLib_GetOLEThunkData(void);
// ALCDECL AheadLib_GetSurfaceFromDC(void);
// ALCDECL AheadLib_RegisterSpecialCase(void);
// ALCDECL AheadLib_ReleaseDDThreadLock(void);
// ALCDECL AheadLib_SetAppCompatData(void);
//  
	
	

 