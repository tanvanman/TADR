#pragma once


#include "headers.h"
#include "patcher.h"
#include "builtin.h"
#include "defines.h"



BYTE* WeightBits;
BYTE* LimitBits;


LPVOID ____TotalA_Category_MaskUnit;// = (LPVOID)0x00488D30;
LPVOID ____AIWeightCallback_Hook_1_Return;// = (LPVOID)0x00406DEE;
LPVOID ____AILimitCallback_Hook_1_Return;// = (LPVOID)0x00406E82;
LPVOID ____Malloc_Layered;// = (LPVOID)0x004B4F10;
LPVOID ____Category_FindStorageLocationInArray_Hook_1_Return;// = (LPVOID)0x00488CC9;



void InitBitsArrays();
void ApplyUnitIdPatches();
void UnitIds();





__declspec(naked) void ____AIWeightCallback_Hook_1()
{
	__asm
	{
		pushad
		pushfd
	}

	InitBitsArrays();

	__asm
	{
		popfd
		popad

	}

	__asm
	{
		mov ecx, [WeightBits]
		call ____TotalA_Category_MaskUnit
		jmp ____AIWeightCallback_Hook_1_Return
	}
}

__declspec(naked) void ____AILimitCallback_Hook_1()
{
	__asm
	{
		pushad
		pushfd
	}

	InitBitsArrays();

	__asm
	{
		popfd
		popad

	}

	__asm
	{
		mov ecx, [LimitBits]
		call ____TotalA_Category_MaskUnit
		jmp ____AILimitCallback_Hook_1_Return
	}
}

__declspec(naked) void ____Category_FindStorageLocationInArray_Hook_1()
{
	__asm
	{
		push MAX_NUMBER_OF_UNITS / 8
		call ____Malloc_Layered
		jmp ____Category_FindStorageLocationInArray_Hook_1_Return
	}
}






DWORD ____Fix_UnitSelectionStack_Return = 0x0048BE11;


__declspec(naked) void ____Fix_UnitSelectionStack()
{
	__asm
	{
		sub esp, MAX_NUMBER_OF_UNITS / 8
		mov cl, [edx + 0x2A42]

		jmp ____Fix_UnitSelectionStack_Return
	}

}


__declspec(naked) void ____FixUnitSelectionStack_ReturnHook()
{
	__asm
	{
		add esp, MAX_NUMBER_OF_UNITS / 8
		ret
	}
}






void InitBitsArrays()
{
	if (WeightBits != nullptr)
		____free(WeightBits);

	if (LimitBits != nullptr)
		____free(LimitBits);

	WeightBits = (BYTE*)____malloc(MAX_NUMBER_OF_UNITS / 8);
	LimitBits = (BYTE*)____malloc(MAX_NUMBER_OF_UNITS / 8);

	____memset(WeightBits, 0, MAX_NUMBER_OF_UNITS / 8);
	____memset(LimitBits, 0, MAX_NUMBER_OF_UNITS / 8);
}










void ApplyUnitIdPatches()
{
	WriteJumpHook((LPVOID)0x00406DE5, (LPVOID)____AIWeightCallback_Hook_1);
	WriteJumpHook((LPVOID)0x00406E79, (LPVOID)____AILimitCallback_Hook_1);

	WriteJumpHook((LPVOID)0x00488CC2, (LPVOID)____Category_FindStorageLocationInArray_Hook_1);
	*((DWORD*)(0x00488CD2 + 1)) = MAX_NUMBER_OF_UNITS / 8 / 4; // allocation



	// fix Ctrl_ stack functions
	WriteJumpHook((LPVOID)0x0048BE08, (LPVOID)____Fix_UnitSelectionStack);
	
	*((DWORD*)(0x0048BE21 + 1)) = MAX_NUMBER_OF_UNITS / 8 / 4;
	
	
	WriteJumpHook((LPVOID)0x0048BF1E, (LPVOID)____FixUnitSelectionStack_ReturnHook);



}




void StaticInitializers_UnitIds()
{
	____TotalA_Category_MaskUnit = (LPVOID)0x00488D30;
	____AIWeightCallback_Hook_1_Return = (LPVOID)0x00406DEE;
	____AILimitCallback_Hook_1_Return = (LPVOID)0x00406E82;
	____Malloc_Layered = (LPVOID)0x004B4F10;
	____Category_FindStorageLocationInArray_Hook_1_Return = (LPVOID)0x00488CC9;
}


void UnitIds()
{
	StaticInitializers_UnitIds();

	ApplyUnitIdPatches();
}

