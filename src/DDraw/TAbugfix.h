#pragma once

#include <memory>

class SingleHook;
#define GUIERRORCOUNT (4)

class TABugFixing
{

private:
	std::unique_ptr <SingleHook> NullUnitDeathVictim;
	std::unique_ptr <SingleHook> CircleRadius;
	std::unique_ptr <SingleHook> CrackCd;
	std::unique_ptr <SingleHook> CrackCd2;
	std::unique_ptr <SingleHook> CrackCd3;
	std::unique_ptr <SingleHook> SinglePlayerStartButton;
	std::unique_ptr <SingleHook> LosTypeShouldBeACheatCode;
	std::unique_ptr<InlineSingleHook> BadModelHunter_ISH;
	std::unique_ptr <SingleHook> GUIErrorLengthHookAry[GUIERRORCOUNT];
	std::unique_ptr <SingleHook> CDMusic_TAB;
	std::unique_ptr <SingleHook> CDMusic_Menu_Pause;
	std::unique_ptr <SingleHook> CDMusic_Victory_Pause;
	std::unique_ptr <SingleHook> CDMusic_StopButton;
	std::unique_ptr <SingleHook> UnitVolumeYequZero;
	std::unique_ptr <SingleHook> UnitIDOutRange;
	std::unique_ptr <SingleHook> UnitDeath_BeforeUpdateUI;
	std::unique_ptr <SingleHook> EnterDrawPlayer_MAPPEDMEM;
	std::unique_ptr <SingleHook> LeaveDrawPlayer_MAPPEDMEM;
	std::unique_ptr <SingleHook> EnterUnitLoop;
	std::unique_ptr <SingleHook> LeaveUnitLoop;
	std::unique_ptr <SingleHook> LeaveUnitLoop2;
	std::unique_ptr <SingleHook> SavePlayerColor;
	std::unique_ptr <SingleHook> RestorePlayerColor;
	std::unique_ptr <SingleHook> DisplayModeMinHeight768Enum;
	std::unique_ptr <SingleHook> DisplayModeMinHeight768Reg;
	std::unique_ptr <SingleHook> DisplayModeMinHeight768Def;
	std::unique_ptr <SingleHook> DisplayModeMinWidth1024Def;
	std::unique_ptr <SingleHook> DisplayModeMinWidth1024Reg;
	std::unique_ptr <SingleHook> ResourceStripHeightFix;
	std::unique_ptr <SingleHook> PatrolDisableBuildRepair;
	std::unique_ptr <SingleHook> VTOLPatrolDisableBuildRepair;
	std::unique_ptr <SingleHook> KeepOnReclaimPreparedOrder;
	std::unique_ptr <SingleHook> PatrolDisableReclaim;
	std::unique_ptr <SingleHook> VTOLPatrolDisableReclaim;
	std::unique_ptr <InlineSingleHook> DrawPlayer11DT;
	std::unique_ptr <SingleHook> DrawPlayer11DTEnable[3];
	std::unique_ptr <SingleHook> JammingOwnRadar;
	std::unique_ptr <SingleHook> GhostComFix;
	std::unique_ptr <SingleHook> GhostComFixAssist;
	std::unique_ptr <SingleHook> FixFactoryExplosionsInit;
	std::unique_ptr <SingleHook> FixFactoryExplosionsAssignUnitId;
	std::unique_ptr <SingleHook> FixFactoryExplosionsRecycleUnitId;
	std::unique_ptr <SingleHook> JunkYardmapFix;
	std::unique_ptr <SingleHook> CanBuildArrayBufferOverrunFix;
	std::unique_ptr <SingleHook> HostDoesntLeave;
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

int __stdcall SavePlayerColorProc(PInlineX86StackBuffer X86StrackBuffer);
int __stdcall RestorePlayerColorProc(PInlineX86StackBuffer X86StrackBuffer);

int __stdcall CheckDisplayModeHeightReg(PInlineX86StackBuffer X86StrackBuffer);
int __stdcall CheckDisplayModeWidthReg(PInlineX86StackBuffer X86StrackBuffer);
