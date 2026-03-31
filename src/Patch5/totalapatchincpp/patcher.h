#pragma once

#include "headers.h"
#include "builtin.h"



BOOL SetExeMemoryProtection(LPVOID startAddr, LPVOID endAddr, DWORD newProtection)
{
	DWORD Junk;

	return ____TotalA_VirtualProtect(startAddr, (DWORD)endAddr - (DWORD)startAddr, newProtection, &Junk);
}




void WriteJumpHook(LPVOID target, LPVOID hookTarget)
{
	*((BYTE*)(target)) = 0xE9;
	*((DWORD*)((DWORD)target + 1)) = (DWORD)hookTarget - (DWORD)target - 5;
}




void WriteBytes(LPVOID target, BYTE bytes[], SIZE_T byteCount)
{
	____memcpy(target, bytes, byteCount);
}


