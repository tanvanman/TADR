


#include "headers.h"
#include "patcher.h"

#include "weaponids.h"
#include "unitids.h"

#include "totalafunctions.h"





HANDLE(WINAPI* ____FindFirstFileA)(LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData);
BOOL(WINAPI* ____FindClose)(HANDLE hFindFile);
int (WINAPI* ____MessageBoxA)(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType);
HANDLE(WINAPI* ____GetCurrentProcess)();
BOOL(WINAPI* ____TerminateProcess)(HANDLE hProcess, UINT uExitCode);

void ____exit(UINT Code)
{
	HANDLE pHandle = ____GetCurrentProcess();
	____TerminateProcess(pHandle, Code);

}







BOOL WINAPI DllMain(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpReserved)
{

	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		StaticInitializers_DllMain();
		StaticInitializers_TotalAFunctions();



		____FindFirstFileA = (HANDLE(WINAPI*)(LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData))0x004FB19A;
		____FindClose = (BOOL(WINAPI*)(HANDLE hFindFile))0x004FB1A6;

		____MessageBoxA = (int (WINAPI*)(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType))0x0049F7F4;
		____GetCurrentProcess = (HANDLE(WINAPI*)())0x004FB0C8;
		____TerminateProcess = (BOOL(WINAPI*)(HANDLE hProcess, UINT uExitCode))0x004FB134;




		SetExeMemoryProtection((LPVOID)0x401000, (LPVOID)(0x401000 + 0xFAA00), PAGE_EXECUTE_READWRITE);



		WeaponIds();
		UnitIds();
	
		SetExeMemoryProtection((LPVOID)0x401000, (LPVOID)(0x401000 + 0xFAA00), PAGE_EXECUTE_READ);
	}
	else if (fdwReason == DLL_PROCESS_DETACH)
	{

	}

	return TRUE;
}



