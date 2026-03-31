#pragma once


#include "headers.h"




int (WINAPI* ____PlaySoundEffect)(const char*, int);
extern LPVOID ____TotalA_strcmpi;




void StaticInitializers_TotalAFunctions()
{
	____TotalA_strcmpi = (LPVOID)0x004F8A70;
	____PlaySoundEffect = (int (WINAPI*)(const char*, int))0x0047F1A0;
}