 
#include "oddraw.h"
#include "iddrawsurface.h"

#include "weaponid.h"
#include <stdio.h>

/*

char WeaponArray[0x1150000+ 0x115];//4096¸ö
//char UnitArray[0x249*5000+ 0x249];//[];5000¸ö

//int UPTR1;//=(int)UnitArray;
int UPTR2;//=(int)(&UPTR1);;// = (int)&UPTR;

int PTR1; //= (int)WeaponArray;
int PTR2;// = (int)(&PTR1);;//= (int)&PTR;

void  Func1();
void  Func2();

void IncreaseWeaponID()
{
	//weaponarray, 0x115 bytes for each element 0x115*16000 = 4432000 bytes
// 	WeaponArray= new char[0x4432000+0x20000];
// 	UnitArray= new char[0x2846A*10];
	memset(WeaponArray, 0x0, sizeof(WeaponArray));
	memset(UnitArray, 0x0, sizeof(UnitArray));

	char nop[10];
	memset(nop, 0x90, 10);

	//UPTR1=(int)UnitArray;
	UPTR2=(int)(UnitArray);// = (int)&UPTR;

	PTR1 = (int)WeaponArray;
	PTR2 = (int)(&PTR1);//= (int)&PTR;
	//   int Adress = 0x511de8;
	//   int BasePTR;
	//   ReadProcessMemory(GetCurrentProcess(), (void*)Adress, (void*)&BasePTR, sizeof(int), NULL);
	//   //ReadProcessMemory(TAProc, (void*)(BasePTR+0x1b8e+0x3c), (void*)&UnitPTR, sizeof(int), NULL);
	//   WriteProcessMemory(GetCurrentProcess(), (void*)(BasePTR+0x1439B), &UPTR, 4, NULL);
	//uvervrites the OwnUnitBegin array
	// 42AA65
//	int onk= 0xe8;
//	WriteProcessMemory(GetCurrentProcess(), (void*)0x0042AA11, nop, 6, NULL);
//	WriteProcessMemory(GetCurrentProcess(), (void*)0x0042AA98, nop, 6, NULL);

//	WriteProcessMemory(GetCurrentProcess(), (void*)0x0042AA11, (void *)&onk, 1, NULL);
//	WriteProcessMemory(GetCurrentProcess(), (void*)0x0042AA98, (void *)&onk, 1, NULL);

//	onk= (int)Func1- 0x0042AA11- 0x5;
//	WriteProcessMemory(GetCurrentProcess(), (void*)0x0042AA12, (void *)&onk, 4, NULL);

//	onk= (int)Func1- 0x0042AA98- 0x5;
//	WriteProcessMemory(GetCurrentProcess(), (void*)0x0042AA99, (void *)&onk, 4, NULL);

	//wep
	WriteProcessMemory(GetCurrentProcess(), (void*)0x0049E5CC, &PTR2, 4, NULL); ///
	WriteProcessMemory(GetCurrentProcess(), (void*)0x0042CDCE, &PTR2, 4, NULL); ///
	WriteProcessMemory(GetCurrentProcess(), (void*)0x0042E31E, &PTR2, 4, NULL); ///
	WriteProcessMemory(GetCurrentProcess(), (void*)0x0042E46A, &PTR2, 4, NULL); ///

	WriteProcessMemory(GetCurrentProcess(), (void*)0x0042F3AC, &PTR2, 4, NULL); //
	WriteProcessMemory(GetCurrentProcess(), (void*)0x00437CF9, &PTR2, 4, NULL); //
	WriteProcessMemory(GetCurrentProcess(), (void*)0x00437D15, &PTR2, 4, NULL); //
	WriteProcessMemory(GetCurrentProcess(), (void*)0x00455476, &PTR2, 4, NULL); //

	WriteProcessMemory(GetCurrentProcess(), (void*)0x0042ED48, &PTR2, 4, NULL);
	WriteProcessMemory(GetCurrentProcess(), (void*)0x0042ED70, &PTR2, 4, NULL);
	WriteProcessMemory(GetCurrentProcess(), (void*)0x0042F360, &PTR2, 4, NULL);
	WriteProcessMemory(GetCurrentProcess(), (void*)0x0042F378, &PTR2, 4, NULL);

	onk = 0x747589;
	WriteProcessMemory(GetCurrentProcess(), (void*)0x0042ED67, &onk, 4, NULL);
	onk = 0x90909090;
	WriteProcessMemory(GetCurrentProcess(), (void*)0x0042ED6A, &onk, 4, NULL);


	//in recieve 0xd packet function
	//jump to unused space (short jump)
	onk = 0xceeb;
	WriteProcessMemory(GetCurrentProcess(), (void*)0x0049D291, &onk, 2, NULL);
	//move edi,PTR2
	onk = 0xbf;
	WriteProcessMemory(GetCurrentProcess(), (void*)0x0049D261, &onk, 1, NULL);
	WriteProcessMemory(GetCurrentProcess(), (void*)0x0049D262, &PTR2, 4, NULL);
	//return (jmp 0x49D293)
	onk = 0x2beb;
	WriteProcessMemory(GetCurrentProcess(), (void*)0x0049D266, &onk, 2, NULL);

	onk = 0xBB;
	WriteProcessMemory(GetCurrentProcess(), (void*)0x0049D3D8, &onk, 1, NULL);
	onk = (int)Func2;
	WriteProcessMemory(GetCurrentProcess(), (void*)0x0049D3D9, &onk, 4, NULL);
	onk = 0xe3ff;
	WriteProcessMemory(GetCurrentProcess(), (void*)0x0049D3DD, &onk, 2, NULL);
	onk = 0x90909090;
	WriteProcessMemory(GetCurrentProcess(), (void*)0x0049D3DF, &onk, 2, NULL);


	int Count = 0x1150000;
	WriteProcessMemory(GetCurrentProcess(), (void*)0x0042E33F, &Count, 4, NULL);
	WriteProcessMemory(GetCurrentProcess(), (void*)0x0042F433, &Count, 4, NULL);
	WriteProcessMemory(GetCurrentProcess(), (void*)0x0049E5ED, &Count, 4, NULL);

	//more units

	WriteProcessMemory(GetCurrentProcess(), (void*)0x0042F3F4, nop, 5, NULL);  //nops a memory cleanup call
	//   unsigned char alocunitarray[14] = {0x89, 0x9A, 0x8F, 0x43, 0x01, 0x00, 0x90, 0x90, 0x90, 0xBD, 0x24, 0x2C, 0x19, 0x00};
	//   WriteProcessMemory(GetCurrentProcess(), (void*)0x0042AA72, alocunitarray, 14, NULL);  //change instruction to create larger OwnUnitBegin array

}

void __declspec(naked) Func1()
{
	__asm{
		//push ebx;
		mov ecx, DWORD PTR [0x0511DE8]
		mov eax, UPTR2
		mov ecx, [ecx]
		mov [ecx+ 0x1439B], eax;
		retn;
	}
}


void __declspec(naked) Func2()
{
	__asm{
		//push ebx;
		mov ebx, PTR2;
		mov dl, [ebx+0x2A42];
		pop ebx;
		cmp     [ecx+0x3A], dl;
		mov ebx, 0x49D3E1;
		jmp ebx;
	}
}*/