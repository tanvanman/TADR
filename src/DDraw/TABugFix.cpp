
#include "iddraw.h"
#include "iddrawsurface.h"


#include "hook/etc.h"
#include "hook/hook.h"

#include "tamem.h"
#include "tafunctions.h"
#include "TAbugfix.h"
#include "TAConfig.h"


#include "ddraw.h"


TABugFixing * FixTABug;
///////---------------------
/*
.text:004866E8 078 75 04                                                           jnz     short loc_4866EE
	.text:004866EA 078 33 F6                                                           xor     esi, esi
	.text:004866EC 078 EB 18                                                           jmp     short loc_486706
	-> if it's null, straight jmp to across the routine that used esi as unit ptr
	*/

unsigned int NullUnitDeathVictimAddr= 0x04866E8;
BYTE NullUnitDeathVictimBits[]={0x0F, 0x84, 0x6B, 0x07, 0x00, 0x00};

//.text:00438EDE 03C 0F 8C 69 01 00 00                                               jl      loc_43904D
//->    jle      loc_43904D  Radius most bigger than 0
unsigned int CircleRadiusAddr= 0x00438EDE;
BYTE CircleRadiusBits[]= {0x0F, 0x8E, 0x69, 0x01, 0x00, 0x00};

unsigned int CrackCdAddr= 0x0041D4CD;

BYTE CrackCDBits[]= {0x90, 0x90 , 0x90 , 0x90 , 0x90 , 0x90};
unsigned int CrackCd2Addr= 0x0050289C;
BYTE CrackCD2Bits[]= {0};

unsigned int CrackCd3Addr= 0x41D6B0;
BYTE CrackCD3Bits[]= {0x90, 0x90, 0xB0, 0x2E};

unsigned int LosTypeShouldBeACheatCodeAddr = 0x501df4;  // command level for +lostype handler
BYTE LosTypeShouldBeACheatCodeBits[] = { 2 };           // 1=normal; 2=cheat; 7=debug

unsigned int GUIErrorLengthAry[GUIERRORCOUNT]=
{
	0x04AEBBE,
	0x04AEBCA,
	0x04AEC2C,
	0x04AEC87
};
BYTE GUIErrorLengthBits[]= {0x80};

unsigned int CDMusic_TABAddr= 0x00460E0D;
BYTE CDMusic_TABBits[]= {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90};

unsigned int CDMusic_Menu_PauseAddr= 0x00490B30;
unsigned int CDMusic_Victory_PauseAddr= 0x4996DF;


unsigned int CDMusic_StopAddr= 0x4CED4b;
BYTE CDMusic_StopBits[]= {0x44, 0xb6, 0x50};


unsigned int UnitVolumeYequZero_Addr= 0x049CE65;

unsigned int UnitIDOutRangeAddr= 0x48A26B;
unsigned int UnitIDOutRangeRtn= 0x048A270;
unsigned int UnitDeath_BeforeUpdateUIAddr= 0x04995EF;


unsigned int EnterDrawPlayer_MAPPEDMEMAddr= 0x0467440;
unsigned int LeaveDrawPlayer_MAPPEDMEMAddr= 0x0465572;

unsigned int EnterUnitLoopAddr= 0x0464F80;
unsigned int LeaveUnitLoopAddr= 0x046563B;
unsigned int LeaveUnitLoop2Addr= 0x04655F4;

unsigned int DisplayModeMinHeight768EnumAddr = 0x45E589;
BYTE DisplayModeMinHeight768EnumBits[] = { 0x00, 0x03 };
unsigned int DisplayModeMinHeight768DefAddr = 0x42FA97;
BYTE DisplayModeMinHeight768DefBits[] = { 0x00, 0x03 };
unsigned int DisplayModeMinHeight768RegAddr = 0x42FA83;

unsigned int DisplayModeMinWidth1024DefAddr = 0x42FA42;
BYTE DisplayModeMinWidth1024DefBits[] = { 0x00, 0x04 };
unsigned int DisplayModeMinWidth1024RegAddr = 0x42FA2E;

unsigned int SinglePlayerStartButtonAddr = 0x456780;
BYTE SinglePlayerStartButtonBits[] = { 0x02, 0x7d };

LONG CALLBACK VectoredHandler(
	_In_  PEXCEPTION_POINTERS ExceptionInfo
	)
{
	//return EXCEPTION_CONTINUE_EXECUTION;
	return EXCEPTION_CONTINUE_SEARCH;
}


TABugFixing::TABugFixing ()
{

	MaxUnitID= 0;

	NullUnitDeathVictim= NULL;
	CircleRadius= NULL;
	CrackCd= NULL;
	CrackCd2= NULL;
	CrackCd3= NULL;
	BadModelHunter_ISH= NULL;
	for (int i= 0; i<GUIERRORCOUNT; i++)
	{
		GUIErrorLengthHookAry[i]= NULL;
	}


	CDMusic_TAB= NULL;

	CDMusic_Menu_Pause= NULL;
	CDMusic_Victory_Pause= NULL;

	CDMusic_StopButton= NULL;

	UnitVolumeYequZero= NULL;

	UnitIDOutRange= NULL;

	UnitDeath_BeforeUpdateUI= NULL;

	if (MyConfig->GetIniBool("DisplayModeMinHeight768", FALSE))
	{
		DisplayModeMinHeight768Enum = new SingleHook(DisplayModeMinHeight768EnumAddr, sizeof(DisplayModeMinHeight768EnumBits), INLINE_UNPROTECTEVINMENT, DisplayModeMinHeight768EnumBits);
		DisplayModeMinHeight768Def = new SingleHook(DisplayModeMinHeight768DefAddr, sizeof(DisplayModeMinHeight768DefBits), INLINE_UNPROTECTEVINMENT, DisplayModeMinHeight768DefBits);
		DisplayModeMinHeight768Reg = new InlineSingleHook(DisplayModeMinHeight768RegAddr, 5, INLINE_5BYTESLAGGERJMP, CheckDisplayModeHeightReg);
	
		DisplayModeMinWidth1024Def = new SingleHook(DisplayModeMinWidth1024DefAddr, sizeof(DisplayModeMinWidth1024DefBits), INLINE_UNPROTECTEVINMENT, DisplayModeMinWidth1024DefBits);
		DisplayModeMinWidth1024Reg = new InlineSingleHook(DisplayModeMinWidth1024RegAddr, 5, INLINE_5BYTESLAGGERJMP, CheckDisplayModeWidthReg);
	}

  SinglePlayerStartButton = new SingleHook(SinglePlayerStartButtonAddr, sizeof(SinglePlayerStartButtonBits), INLINE_UNPROTECTEVINMENT, SinglePlayerStartButtonBits);

	NullUnitDeathVictim= new SingleHook ( NullUnitDeathVictimAddr, sizeof(NullUnitDeathVictimBits), INLINE_UNPROTECTEVINMENT, NullUnitDeathVictimBits);

	CircleRadius=  new SingleHook ( CircleRadiusAddr, sizeof(CircleRadiusBits), INLINE_UNPROTECTEVINMENT, CircleRadiusBits);

	BadModelHunter_ISH= new InlineSingleHook ( BadModelHunterAddr, 5, INLINE_5BYTESLAGGERJMP, BadModelHunter);

	CrackCd= new SingleHook ( CrackCdAddr, sizeof(CrackCDBits), INLINE_UNPROTECTEVINMENT, CrackCDBits);

	CrackCd2= new SingleHook ( CrackCd2Addr, sizeof(CrackCD2Bits), INLINE_UNPROTECTEVINMENT, CrackCD2Bits);
	
	CrackCd3= new SingleHook ( CrackCd3Addr, sizeof(CrackCD3Bits), INLINE_UNPROTECTEVINMENT, CrackCD3Bits);
	for (int i= 0; i<GUIERRORCOUNT; i++)
	{
		GUIErrorLengthHookAry[i]= new SingleHook ( GUIErrorLengthAry[i], sizeof(GUIErrorLengthBits), INLINE_UNPROTECTEVINMENT, GUIErrorLengthBits);
	}

    LosTypeShouldBeACheatCode = new SingleHook(LosTypeShouldBeACheatCodeAddr, sizeof(LosTypeShouldBeACheatCodeBits), INLINE_UNPROTECTEVINMENT, LosTypeShouldBeACheatCodeBits);

	HMODULE Audiere_hm= GetModuleHandleA ( "audiere.dll");
	CDMusic_TAB= NULL;
	CDMusic_Menu_Pause= NULL;
	CDMusic_Victory_Pause= NULL;
	CDMusic_StopButton= NULL;
	if (NULL!=Audiere_hm)
	{// install music cd hook

		CDMusic_TAB= new SingleHook ( CDMusic_TABAddr, sizeof(CDMusic_TABBits), INLINE_UNPROTECTEVINMENT, CDMusic_TABBits);
		CDMusic_Menu_Pause= new InlineSingleHook ( CDMusic_Menu_PauseAddr, 5, INLINE_5BYTESLAGGERJMP, CDMusic_MenuProc);
		CDMusic_Victory_Pause= new InlineSingleHook ( CDMusic_Victory_PauseAddr, 5, INLINE_5BYTESLAGGERJMP, CDMusic_VictoryProc);
		//CDMusic_StopButton= new  SingleHook ( CDMusic_StopAddr  , sizeof(CDMusic_StopBits), INLINE_UNPROTECTEVINMENT, CDMusic_StopBits); 
		
	}


	
	//UnitVolumeYequZero= new InlineSingleHook ( UnitVolumeYequZero_Addr, 5, INLINE_5BYTESLAGGERJMP, UnitVolumeYequZero_Proc);

	//MaxUnitID= (*TAmainStruct_PtrPtr)->ActualUnitLimit* 10;

	//UnitIDOutRange= new InlineSingleHook  ( UnitIDOutRangeAddr, 5, INLINE_5BYTESLAGGERJMP, UnitIDOutRange_Proc);

	//UnitIDOutRange->SetParamOfHook (  (LPVOID) MaxUnitID);

	UnitDeath_BeforeUpdateUI= new InlineSingleHook ( UnitDeath_BeforeUpdateUIAddr, 5, INLINE_5BYTESLAGGERJMP, UnitDeath_BeforeUpdateUI_Proc);

// 	InitializeCriticalSection ( &DrawPlayer_MAPPEDMEM_cris);
// 	LeaveDrawPlayer_MAPPEDMEM= new InlineSingleHook ( LeaveDrawPlayer_MAPPEDMEMAddr, 5, INLINE_5BYTESLAGGERJMP, LeaveProc);
// 	LeaveDrawPlayer_MAPPEDMEM->SetParamOfHook (  (LPVOID) &DrawPlayer_MAPPEDMEM_cris);
// 
// 	EnterDrawPlayer_MAPPEDMEM= new InlineSingleHook ( EnterDrawPlayer_MAPPEDMEMAddr, 5, INLINE_5BYTESLAGGERJMP, EnterProc);
// 	EnterDrawPlayer_MAPPEDMEM->SetParamOfHook (  (LPVOID) &DrawPlayer_MAPPEDMEM_cris);
// 
// 
// 	InitializeCriticalSection ( &UnitLoop_cris);
// 	LeaveUnitLoop= new InlineSingleHook ( LeaveUnitLoopAddr, 5, INLINE_5BYTESLAGGERJMP, LeaveProc);
// 	LeaveUnitLoop->SetParamOfHook (  (LPVOID) &UnitLoop_cris);
// 	LeaveUnitLoop2= new InlineSingleHook ( LeaveUnitLoop2Addr, 5, INLINE_5BYTESLAGGERJMP, LeaveProc);
// 	LeaveUnitLoop2->SetParamOfHook (  (LPVOID) &UnitLoop_cris);
// 	
// 
// 	EnterUnitLoop= new InlineSingleHook ( EnterUnitLoopAddr, 5, INLINE_5BYTESLAGGERJMP, EnterProc);
// 	EnterUnitLoop->SetParamOfHook (  (LPVOID) &UnitLoop_cris);
// 
// 
// 
// 	
	AddVectoredExceptionHandler ( TRUE, VectoredHandler );
}

TABugFixing::~TABugFixing ()
{
	RemoveVectoredExceptionHandler  ( VectoredHandler);
	if (NULL != DisplayModeMinHeight768Enum)
	{
		delete DisplayModeMinHeight768Enum;
	}
	if (NULL != DisplayModeMinHeight768Def)
	{
		delete DisplayModeMinHeight768Def;
	}
	if (NULL != DisplayModeMinWidth1024Def)
	{
		delete DisplayModeMinWidth1024Def;
	}
    if (NULL != SinglePlayerStartButton)
    {
        delete SinglePlayerStartButton;
    }
	if (NULL!=NullUnitDeathVictim)
	{
		delete NullUnitDeathVictim;
	}
	if (NULL!=BadModelHunter_ISH)
	{
		delete BadModelHunter_ISH;
	}
	
	if (NULL!=CrackCd)
	{
		delete CrackCd;
	}
	if (NULL!=CrackCd2)
	{
		delete CrackCd2;

	}
	if (NULL!=CrackCd3)
	{
		delete CrackCd3;

	}
    if (NULL != LosTypeShouldBeACheatCode)
    {
        delete LosTypeShouldBeACheatCode;
    }


	if (NULL!=CircleRadius)
	{
		delete CircleRadius;
	}
	for (int i= 0; i<GUIERRORCOUNT; i++)
	{
		if (NULL!=GUIErrorLengthHookAry[i])
		{
			delete GUIErrorLengthHookAry[i];
		}
		
	}

	if (CDMusic_TAB)
	{
		delete CDMusic_TAB;
	}
	if (CDMusic_Menu_Pause)
	{
		delete CDMusic_Menu_Pause;
	}
	if (CDMusic_Victory_Pause)
	{
		delete CDMusic_Victory_Pause;
	}
	if (CDMusic_StopButton)
	{
		delete CDMusic_StopButton;
	}
	
	if (UnitVolumeYequZero)
	{
		delete UnitVolumeYequZero;
	}

	if (UnitDeath_BeforeUpdateUI)
	{
		delete UnitDeath_BeforeUpdateUI;
	}
// 
// 	EnterCriticalSection (&UnitLoop_cris);
// 	EnterCriticalSection (&DrawPlayer_MAPPEDMEM_cris);
// 	
// 	if (EnterDrawPlayer_MAPPEDMEM)
// 	{
// 		delete EnterDrawPlayer_MAPPEDMEM;
// 	}
// 	if (LeaveDrawPlayer_MAPPEDMEM)
// 	{
// 		delete LeaveDrawPlayer_MAPPEDMEM;
// 	}
// 
// 	if (EnterUnitLoop)
// 	{
// 		delete EnterUnitLoop;
// 	}
// 	if (LeaveUnitLoop)
// 	{
// 		delete LeaveUnitLoop;
// 	}
// 	if (LeaveUnitLoop2)
// 	{
// 		delete LeaveUnitLoop2;
// 	}
// 	
// 	LeaveCriticalSection (&DrawPlayer_MAPPEDMEM_cris);
// 	LeaveCriticalSection (&UnitLoop_cris);
// 	
// 	DeleteCriticalSection ( &DrawPlayer_MAPPEDMEM_cris);
// 	DeleteCriticalSection ( &UnitLoop_cris);
}


BOOL TABugFixing::AntiCheat (void)
{
	// sync "+now Film Chris Include Reload Assert"  with cheating

	if (TRUE==*IsCheating)
	{
		(*TAmainStruct_PtrPtr)->SoftwareDebugMode|= 2;
	}
	else
	{
		(*TAmainStruct_PtrPtr)->SoftwareDebugMode= ((*TAmainStruct_PtrPtr)->SoftwareDebugMode)& (~ 2);
	}

	return TRUE;
}



void LogToErrorlog (LPSTR Str)
{
	HANDLE file = CreateFileA("ErrorLog.txt", GENERIC_WRITE, 0, NULL, OPEN_ALWAYS	, 0, NULL);
	DWORD tempWritten;
	SetFilePointer ( file, 0, 0, FILE_END);
	WriteFile ( file, Str, strlen(Str), &tempWritten, NULL);
	WriteFile ( file, "\r\n", 2, &tempWritten, NULL);

	CloseHandle ( file);
}
/*
.text:00458C5A 078 C1 E9 02                                                        shr     ecx, 2          ; let's add a check in here, if  ecx is bigger than 600*600
	.text:00458C5D 078 F3 AB                                                           rep stosd               ; init the CompositeBuffer as background*/

int __stdcall BadModelHunter (PInlineX86StackBuffer X86StrackBuffer)
{
	OFFSCREEN * Offscreen_p= (OFFSCREEN *)(X86StrackBuffer->Esp+ 0x48);
	if ((600<(Offscreen_p->Width))
		||(600<(Offscreen_p->Height)))
	{// record thsi shit.
		Object3doStruct * Obj_ptr=  *(Object3doStruct * *)(X86StrackBuffer->Esp+ 0x68+ 0x10+ 0x8);

		LogToErrorlog ( "\r\n===============================\r\n");
		LogToErrorlog ( "Erroneous unit model :");
		LogToErrorlog ( "Bad Unit ID:");
		LogToErrorlog ( Obj_ptr->ThisUnit->UnitType->Name);
		LogToErrorlog ( "Bad Unit Name:");
		LogToErrorlog ( Obj_ptr->ThisUnit->UnitType->UnitName);
		LogToErrorlog ( "Bad Unit Description:");
		LogToErrorlog ( Obj_ptr->ThisUnit->UnitType->UnitDescription);
		LogToErrorlog ( "Bad Unit ObjectName:");
		LogToErrorlog ( Obj_ptr->ThisUnit->UnitType->ObjectName);

		LogToErrorlog ( "\r\n===============================\r\n");

		SendText ( "Plz Send Errorlog.txt(In Your TA Path) And The Replay Tad To XPoy(In TAUniverse Or In TAClub).\r\nMan, You Meet A Bad Crash Cause By Unit. \r\nOr If You Are A Modder, You Can Know Which Unit Had The Problem In Above", 1);


		X86StrackBuffer->Edi= *(DWORD *)(X86StrackBuffer->Esp);

		X86StrackBuffer->Esi= *(DWORD *)(X86StrackBuffer->Esp+ 4);

		X86StrackBuffer->Ebp= *(DWORD *)(X86StrackBuffer->Esp+ 8);

		X86StrackBuffer->Ebx= *(DWORD *)(X86StrackBuffer->Esp+ 0xc);

		X86StrackBuffer->Esp= X86StrackBuffer->Esp+ 4+ 4+ 4+ 4+ 0x68+ 4+ 8;
		
		X86StrackBuffer->rtnAddr_Pvoid= (LPVOID)SafeModelAddr;

		return X86STRACKBUFFERCHANGE;
		//rtn to 	
	}
	return 0;
}

int __stdcall CDMusic_MenuProc (PInlineX86StackBuffer X86StrackBuffer)
{
	if (1==*(DWORD * )(X86StrackBuffer->Esp+ 4))
	{
		PauseCDMusic ( );
	}

	return 0;
}

int __stdcall CDMusic_VictoryProc (PInlineX86StackBuffer X86StrackBuffer)
{
	PauseCDMusic ( );
	return 0;
}


int __stdcall UnitVolumeYequZero_Proc (PInlineX86StackBuffer X86StrackBuffer)
{
	if (0==((UnitStruct *)X86StrackBuffer->Edx)->Turn.Y)
	{
		//CalcUnitTurn ( (UnitStruct *)X86StrackBuffer->Edx);
		((UnitStruct *)X86StrackBuffer->Edx)->Turn.Y= 1;
	}

	return 0;
}



int __stdcall UnitIDOutRange_Proc (PInlineX86StackBuffer X86StrackBuffer)
{
	if (((DWORD)X86StrackBuffer->myInlineHookClass_Pish->ParamOfHook)<(0xffff& X86StrackBuffer->Eax))
	{
		X86StrackBuffer->rtnAddr_Pvoid= (LPVOID)UnitIDOutRangeRtn;

		return X86STRACKBUFFERCHANGE;
	}
	return 0;
}


int __stdcall UnitDeath_BeforeUpdateUI_Proc  (PInlineX86StackBuffer X86StrackBuffer)
{
	if (ordertype::STOP!=(*TAmainStruct_PtrPtr)->PrepareOrder_Type)
	{//
		(*TAmainStruct_PtrPtr)->PrepareOrder_Type= ordertype::STOP;
	}

	return 0;
}


int __stdcall EnterProc  (PInlineX86StackBuffer X86StrackBuffer)
{
	EnterCriticalSection ((LPCRITICAL_SECTION)X86StrackBuffer->myInlineHookClass_Pish->ParamOfHook);
	return 0;
}
int __stdcall LeaveProc  (PInlineX86StackBuffer X86StrackBuffer)
{
	LeaveCriticalSection ((LPCRITICAL_SECTION)X86StrackBuffer->myInlineHookClass_Pish->ParamOfHook);
	return 0;
}

int __stdcall CheckDisplayModeHeightReg(PInlineX86StackBuffer X86StrackBuffer)
{
	if (X86StrackBuffer->Eax < 768)
	{
		X86StrackBuffer->Eax = 768;
	}

	return 0;
}

int __stdcall CheckDisplayModeWidthReg(PInlineX86StackBuffer X86StrackBuffer)
{
	if (X86StrackBuffer->Eax < 1024)
	{
		X86StrackBuffer->Eax = 1024;
	}

	return 0;
}

