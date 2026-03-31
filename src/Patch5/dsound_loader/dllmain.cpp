

#include <Windows.h>

#pragma warning (disable : 4996)


LPVOID dsoundcreate;// = 0;
HMODULE(WINAPI* ____LoadLibraryA)(LPCSTR Path);
FARPROC(WINAPI* ____GetProcAddress)(HMODULE Module, LPCSTR Name);
DWORD(WINAPI* ____GetCurrentDirectoryA)(DWORD nBufferLength, LPSTR lpBuffer);
void(__cdecl* ____memset)(LPVOID Dst, int Val, size_t Len);// = (void(__cdecl*)(LPVOID Dst, int Val, size_t Len))0x004D82C0;
DWORD(WINAPI* ____GetSystemDirectoryA)(LPSTR, DWORD);

__declspec(naked) HRESULT WINAPI DirectSoundCreate(LPVOID lpGuid, LPVOID* ppDS, LPVOID pUnkOuter)
{
	__asm
	{
		jmp dsoundcreate
	}
}




BOOL WINAPI DllMain(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		dsoundcreate = 0;
		____LoadLibraryA = (HMODULE(WINAPI *)(LPCSTR))0x0049F77C;
		____GetProcAddress = (FARPROC(WINAPI *)(HMODULE Module, LPCSTR Name))0x0049F776;
		____GetCurrentDirectoryA = (DWORD(WINAPI *)(DWORD nBufferLength, LPSTR lpBuffer))0x004FB18E;
		____memset = (void(__cdecl*)(LPVOID Dst, int Val, size_t Len))0x004D82C0;

		HMODULE kernel32module = ____LoadLibraryA("kernel32.dll");
		____GetSystemDirectoryA = (DWORD(WINAPI*)(LPSTR, DWORD))____GetProcAddress(kernel32module, "GetSystemDirectoryA");

		char lpBuffer[MAX_PATH];
		char destBuffer[MAX_PATH];
		DWORD index;
		HMODULE dsound;

		index = ____GetSystemDirectoryA(lpBuffer, MAX_PATH);
		strcpy(destBuffer, lpBuffer);
		strcpy(destBuffer + index, "\\dsound.dll"); // is char ptr so we're safe with addition
		dsound = ____LoadLibraryA(destBuffer);
		if(dsound)
			dsoundcreate = (LPVOID)____GetProcAddress(dsound, MAKEINTRESOURCEA(1));

		____memset(lpBuffer, 0, MAX_PATH);
		____memset(destBuffer, 0, MAX_PATH);

		index = ____GetCurrentDirectoryA(MAX_PATH, lpBuffer);
		strcpy(destBuffer, lpBuffer);
		strcpy(destBuffer + index, "\\Patch5.dll"); // is char ptr so we're safe with addition
		____LoadLibraryA(destBuffer);

		____memset(lpBuffer, 0, MAX_PATH);
		____memset(destBuffer, 0, MAX_PATH);

		index = ____GetCurrentDirectoryA(MAX_PATH, lpBuffer);
		strcpy(destBuffer, lpBuffer);
		strcpy(destBuffer + index, "\\Fixes.dll"); // is char ptr so we're safe with addition
		____LoadLibraryA(destBuffer);
	}

	return TRUE;
}


