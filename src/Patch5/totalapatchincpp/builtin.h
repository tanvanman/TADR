#pragma once

#include "headers.h"




void(__cdecl* ____memcpy)(LPVOID Dst, const LPVOID Src, size_t size);// = (void(__cdecl*)(LPVOID Dst, const LPVOID Src, size_t size))0x004E84E0;
void(__cdecl* ____memset)(LPVOID Dst, int Val, size_t Len);// = (void(__cdecl*)(LPVOID Dst, int Val, size_t Len))0x004D82C0;
int(__cdecl* ____strcmpi)(const char* Str1, const char* Str2);// = (int(__cdecl*)(const char* Str1, const char* Str2))0x004F8A70;
//LPVOID(__cdecl* ____malloc)(size_t Size);// = (LPVOID(__cdecl*)(size_t Size))0x004E8890;
//void(__cdecl* ____free)(LPVOID Memory);// = (void(__cdecl*)(LPVOID Memory))0x004E8820;
BOOL(WINAPI* ____TotalA_VirtualProtect)(LPVOID, SIZE_T, DWORD, LPDWORD);// = (BOOL(WINAPI*)(LPVOID, SIZE_T, DWORD, LPDWORD))0x004FB086;

LPVOID(__cdecl* ____nh_malloc)(HANDLE Heap, size_t Size);// = (LPVOID(__cdecl*)(HANDLE Heap, size_t Size))0x004E88B0;
HANDLE(WINAPI* ____HeapCreate)(DWORD flOptions, size_t dwInitialSize, size_t dwMaximumSize);// = (HANDLE(WINAPI*)(DWORD flOptions, size_t dwInitialSize, size_t dwMaximumSize))0x004FB25A;
LPVOID(WINAPI* ____HeapAlloc)(HANDLE hHeap, DWORD dwFlags, size_t dwBytes);// = (HANDLE(WINAPI*)(HANDLE hHeap, DWORD dwFlags, size_t dwBytes))0x004FB1C4;
BOOL(WINAPI* ____HeapFree)(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem);


HANDLE PatchHeap;


LPVOID __cdecl ____malloc(size_t Size)
{
	//return ____nh_malloc(PatchHeap, Size);
	return ____HeapAlloc(PatchHeap, 0, Size);
}


void __cdecl ____free(LPVOID Memory)
{
	____HeapFree(PatchHeap, 0, Memory);
}



size_t ____strlen(const char* Str)
{
	size_t len = 0;

	while (Str[len] != '\0')
	{
		len++;
	}

	return len;
}


void StaticInitializers_DllMain()
{
	____HeapCreate = (HANDLE(WINAPI*)(DWORD flOptions, size_t dwInitialSize, size_t dwMaximumSize))0x004FB25A;
	____memcpy = (void(__cdecl*)(LPVOID Dst, const LPVOID Src, size_t size))0x004E84E0;
	____memset = (void(__cdecl*)(LPVOID Dst, int Val, size_t Len))0x004D82C0;
	____strcmpi = (int(__cdecl*)(const char* Str1, const char* Str2))0x004F8A70;
	//____malloc = (LPVOID(__cdecl*)(size_t Size))0x004E8890;
	//____free = (void(__cdecl*)(LPVOID Memory))0x004E8820;
	//____free = (void(__cdecl*)(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem))0x004FB1BE;
	____TotalA_VirtualProtect = (BOOL(WINAPI*)(LPVOID, SIZE_T, DWORD, LPDWORD))0x004FB086;
	____nh_malloc = (LPVOID(__cdecl*)(HANDLE Heap, size_t Size))0x004E88B0;
	____HeapAlloc = (LPVOID(WINAPI*)(HANDLE hHeap, DWORD dwFlags, size_t dwBytes))0x004FB1C4;
	____HeapFree = (BOOL(WINAPI*)(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem))0x004FB1BE;


	PatchHeap = ____HeapCreate(0, 0x1000, 0);



}
