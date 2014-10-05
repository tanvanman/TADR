#include "oddraw.h"
#include <vector>
using namespace std;
#include "TAConfig.h"
#include "MenuResolution.h"
#include "LimitCrack.h"
#include "tamem.h"
#include "tafunctions.h"
#include "hook\etc.h"
#include "hook\hook.h"
#include "GUIExpand.h"



//-----------
MenuResolution::MenuResolution (BOOL EqualIt_b)
{
	Hook_0049E91C= NULL;
	Hook_0049E93B= NULL;
	Hook_00491A75= NULL;
	Hook_00491B01= NULL;
	Hook_0049802B= NULL;
	Hook_004980AD= NULL;

	Width= 0;
	Height= 0;

	if (EqualIt_b)
	{
		InstallHook ( );
	}
}

MenuResolution::MenuResolution (int Width_arg, int Height_arg)
{
	Hook_0049E91C= NULL;
	Hook_0049E93B= NULL;
	Hook_00491A75= NULL;
	Hook_00491B01= NULL;
	Hook_0049802B= NULL;
	Hook_004980AD= NULL;

	Width= 0;
	Height= 0;

	if ((0!=Width_arg)
		&&(0!=Height_arg))
	{
		Width= Width_arg;
		Height= Height_arg;

		InstallHook ( );
	}
}

MenuResolution::~MenuResolution ()
{
	UninstallHook ();
}


void MenuResolution::InstallHook (void)
{
	Hook_0049E91C= new InlineSingleHook ( Addr_0049E91C, 10, INLINE_5BYTESLAGGERJMP, sub_0049E91C);
	Hook_0049E93B= new InlineSingleHook ( Addr_0049E93B, 10, INLINE_5BYTESLAGGERJMP, sub_0049E93B);

	Hook_00491A75= new InlineSingleHook ( Addr_00491A75, 25, INLINE_5BYTESLAGGERJMP, sub_00491A75);
	Hook_00491B01=  new InlineSingleHook ( Addr_00491B01, 15, INLINE_5BYTESLAGGERJMP, sub_00491B01);
	Hook_0049802B= new InlineSingleHook ( Addr_0049802B, 25, INLINE_5BYTESLAGGERJMP, sub_00491A75);
	Hook_004980AD=  new InlineSingleHook ( Addr_004980AD, 15, INLINE_5BYTESLAGGERJMP, sub_00491B01);
}

void MenuResolution::UninstallHook (void)
{
	if (Hook_0049E91C!= NULL)
	{
		delete Hook_0049E91C;
		Hook_0049E91C= NULL;
	}
	if (Hook_0049E93B!= NULL)
	{
		delete Hook_0049E93B;
		Hook_0049E93B= NULL;
	}
	if (Hook_00491A75!= NULL)
	{
		delete Hook_00491A75;
		Hook_00491A75= NULL;
	}
	if (Hook_00491B01!= NULL)
	{
		delete Hook_00491B01;
		Hook_00491B01= NULL;
	}
	if (Hook_0049802B!= NULL)
	{
		delete Hook_0049802B;
		Hook_0049802B= NULL;
	}
	if (Hook_004980AD!= NULL)
	{
		delete Hook_004980AD;
		Hook_004980AD= NULL;
	}
}
/*
0049E91C 0AC C7 05 1A F5 51 00 80 02 00 00                                   mov     TAProgram.MainMenuWidth, 280h

	0049E93B 0AC C7 05 1E F5 51 00 E0 01 00 00                                   mov     TAProgram.MainMenuHeight, 1E0h
	*/
int  __stdcall  sub_0049E91C (PInlineX86StackBuffer X86StrackBuffer)
{
	DWORD Width;
	
	if (0!=GUIExpander->SyncMenuResolution->Width)
	{
		Width= GUIExpander->SyncMenuResolution->Width;
	}
	else
	{
		Width= MyConfig->FindRegDword ( "DisplayModeWidth",0);
	}
	
	if (0==Width)
	{
		MyConfig->ReadTAReg_Dword ( "DisplayModeWidth", &Width);
	}
	if (0==Width)
	{
		Width= 640;
	}
	__asm MOV EAX, Width;
	__asm MOV DWORD PTR DS:[0x51F51A],EAX;

	return X86STRACKBUFFERCHANGE;

}

int  __stdcall  sub_0049E93B (PInlineX86StackBuffer X86StrackBuffer)
{
	DWORD Height;

	if (0!=GUIExpander->SyncMenuResolution->Height)
	{
		Height= GUIExpander->SyncMenuResolution->Height;
	}
	else
	{
		Height= MyConfig->FindRegDword ( "DisplayModeHeight",0);
	}

	if (0==Height)
	{
		MyConfig->ReadTAReg_Dword ( "DisplayModeHeight", &Height);
	}
	if (0==Height)
	{
		Height= 480;
	}
	__asm MOV EAX, Height;
	__asm MOV DWORD PTR DS:[0x51F51E],EAX;

	return X86STRACKBUFFERCHANGE;
}

/*
	.text:00491A75 000 C7 80 1F 7E 03 00 80 02 00 00                                   mov     [eax+TAMainStruct.ScreenWidth], 280h
	.text:00491A7F 000 8B 0D E8 1D 51 00                                               mov     ecx, TAMainStructPtr
	.text:00491A85 000 C7 81 23 7E 03 00 E0 01 00 00                                   mov     [ecx+TAMainStruct.ScreenHeight], 1E0h

	*/

int  __stdcall  sub_00491A75 (PInlineX86StackBuffer X86StrackBuffer)
{
	DWORD Height= 0;
	DWORD Width= 0;

	if ((0!=GUIExpander->SyncMenuResolution->Width)
		&&(0!=GUIExpander->SyncMenuResolution->Height))
	{
		Width= GUIExpander->SyncMenuResolution->Width;
		Height= GUIExpander->SyncMenuResolution->Height;
	}
	else
	{
		MyConfig->ReadTAReg_Dword ( "DisplayModeWidth", &Width);
		MyConfig->ReadTAReg_Dword ( "DisplayModeHeight", &Height);
	}

	
	if (0==Width)
	{
		Width= 640;
	}
	if (0==Height)
	{
		Height= 480;
	}
	
	*reinterpret_cast<DWORD *>(reinterpret_cast<DWORD>(*TAmainStruct_PtrPtr)+ 0x37E1F)= Width;
	*reinterpret_cast<DWORD *>(reinterpret_cast<DWORD>(*TAmainStruct_PtrPtr)+ 0x37E23)= Height;

	return X86STRACKBUFFERCHANGE;
}

/*
	.text:00491B01 000 68 E0 01 00 00                                                  push    1E0h            ; Height
	.text:00491B06 004 68 80 02 00 00                                                  push    280h            ; Width

	*/

int __stdcall sub_00491B01 (PInlineX86StackBuffer X86StrackBuffer)
{
	DWORD Height= 0;
	DWORD Width= 0;

	if ((0!=GUIExpander->SyncMenuResolution->Width)
		&&(0!=GUIExpander->SyncMenuResolution->Height))
	{
		Width= GUIExpander->SyncMenuResolution->Width;
		Height= GUIExpander->SyncMenuResolution->Height;
	}
	else
	{
		MyConfig->ReadTAReg_Dword ( "DisplayModeWidth", &Width);
		MyConfig->ReadTAReg_Dword ( "DisplayModeHeight", &Height);
	}

	if (0==Width)
	{
		Width= 640;
	}
	if (0==Height)
	{
		Height= 480;
	}
	
/*
	X86StrackBuffer->Esp-= 4; 
	*reinterpret_cast<DWORD *>(X86StrackBuffer->Esp)= Height;

	X86StrackBuffer->Esp-= 4; 
	*reinterpret_cast<DWORD *>(X86StrackBuffer->Esp)= Width;
*/
	DWORD NewTAScreen= 0x4B5940;
	__asm
	{
		PUSH Height;
		PUSH Width;
		CALL NewTAScreen;
	}

	return X86STRACKBUFFERCHANGE;
}

/*	.text:0049802B 244 C7 82 1F 7E 03 00 80 02 00 00                                   mov     [edx+TAMainStruct.ScreenWidth], 280h
	.text:00498035 244 A1 E8 1D 51 00                                                  mov     eax, TAMainStructPtr
	.text:0049803A 244 C7 80 23 7E 03 00 E0 01 00 00                                   mov     [eax+TAMainStruct.ScreenHeight], 1E0h
	*/
//sub_00491A75
/*	004980AD     68 84030000    PUSH 384
	004980B2     68 40060000    PUSH 640
	*/

//sub_00491B01