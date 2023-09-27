#pragma once

class SingleHook;
#define GUIERRORCOUNT (4)

class TABugFixing
{

private:
	SingleHook * NullUnitDeathVictim;
	SingleHook * CircleRadius;
	SingleHook * CrackCd;
	SingleHook * CrackCd2;
	SingleHook * CrackCd3;
    SingleHook* SinglePlayerStartButton;
    SingleHook* LosTypeShouldBeACheatCode;
	InlineSingleHook * BadModelHunter_ISH;

	SingleHook * GUIErrorLengthHookAry[GUIERRORCOUNT];

	SingleHook * CDMusic_TAB;

	InlineSingleHook * CDMusic_Menu_Pause;
	InlineSingleHook * CDMusic_Victory_Pause;

	SingleHook* CDMusic_StopButton;

	InlineSingleHook * UnitVolumeYequZero;

	InlineSingleHook * UnitIDOutRange;

	InlineSingleHook *  UnitDeath_BeforeUpdateUI;


	InlineSingleHook *  EnterDrawPlayer_MAPPEDMEM;
	InlineSingleHook *  LeaveDrawPlayer_MAPPEDMEM;

	InlineSingleHook *  EnterUnitLoop;
	InlineSingleHook *  LeaveUnitLoop;
	InlineSingleHook *  LeaveUnitLoop2;
	
	SingleHook * DisplayModeMinHeight768Enum;
	InlineSingleHook * DisplayModeMinHeight768Reg;
	SingleHook * DisplayModeMinHeight768Def;

	SingleHook * DisplayModeMinWidth1024Def;
	InlineSingleHook * DisplayModeMinWidth1024Reg;

	CRITICAL_SECTION DrawPlayer_MAPPEDMEM_cris;
	CRITICAL_SECTION UnitLoop_cris;

	unsigned int MaxUnitID;
public:
	TABugFixing ();
	~TABugFixing ();
	BOOL AntiCheat (void);
};

extern TABugFixing * FixTABug;;

int __stdcall BadModelHunter (PInlineX86StackBuffer X86StrackBuffer);

int __stdcall CDMusic_VictoryProc (PInlineX86StackBuffer X86StrackBuffer);
int __stdcall CDMusic_MenuProc (PInlineX86StackBuffer X86StrackBuffer);

int __stdcall UnitVolumeYequZero_Proc (PInlineX86StackBuffer X86StrackBuffer);

int __stdcall UnitIDOutRange_Proc (PInlineX86StackBuffer X86StrackBuffer);

int __stdcall UnitDeath_BeforeUpdateUI_Proc  (PInlineX86StackBuffer X86StrackBuffer);




int __stdcall LeaveProc  (PInlineX86StackBuffer X86StrackBuffer);
int __stdcall EnterProc  (PInlineX86StackBuffer X86StrackBuffer);

int __stdcall CheckDisplayModeHeightReg(PInlineX86StackBuffer X86StrackBuffer);
int __stdcall CheckDisplayModeWidthReg(PInlineX86StackBuffer X86StrackBuffer);
