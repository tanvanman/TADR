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
	std::unique_ptr<InlineSingleHook> MultiplayerPlayerLostGuard;
	std::unique_ptr<InlineSingleHook> NewChatTextGuard;
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
	std::unique_ptr <SingleHook> WindSpeedSync;
	std::unique_ptr <SingleHook> NetworkRawReceiveLog;
	std::unique_ptr <SingleHook> NetworkDispatchLog;
	std::unique_ptr <SingleHook> OrderDispatchGuardMain;
	std::unique_ptr <SingleHook> OrderDispatchGuardBackground;
	std::vector<std::unique_ptr<SingleHook> > m_hooks;
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

// Crash-trace diagnostic instrumentation: hooks ExitProcess + TerminateProcess
// in kernel32, registers CRT _invalid_parameter_handler + SIGABRT handler, and
// extends the existing VEH to log first-chance access violations. All output
// goes to tdrawlog.txt. Called once from ddraw.cpp's DLL_PROCESS_ATTACH.
void InstallCrashTrace();

// General-purpose crash breadcrumb ring. Any hook can drop a cheap, always-on
// event here; the VEH's generic crash report dumps the last TRACE_RING_SIZE of
// them. Categories are FOURCC tags; payload meaning is per-category.
#define TRACE_CAT_RECV 0x52454356u  // 'RECV' : a=fromDpid b=size c=buf[0] d=buf[1]
#define TRACE_CAT_UNIT 0x554E4954u  // 'UNIT' : reserved (a=slot b=typeId c=owner d=event)
// ---- Order-state-machine breadcrumbs (see OrderDispatchGuard in TABugFix.cpp) ----
// These are all RARE by construction: nothing here fires on the per-unit-per-tick
// happy path, so they survive in the 128-entry ring for minutes of play.
#define TRACE_CAT_OBAD 0x4F424144u  // 'OBAD' : rejected order dispatch
                                    //          a=unit b=order c=idx|(count<<16) d=handler
#define TRACE_CAT_MBLD 0x4D424C44u  // 'MBLD' : Order_MobileBuild rotation envelope armed
                                    //          a=unit b=order c=depth d=savedReturn
#define TRACE_CAT_MBRE 0x4D425245u  // 'MBRE' : Order_MobileBuild envelope RE-ENTERED
                                    //          a=unit b=order c=depth d=outerSavedReturn
#define TRACE_CAT_KICK 0x4B49434Bu  // 'KICK' : ConstructionKickout mutated an order list
                                    //          a=unit b=oldOrders c=oldOrderType d=branch
void CrashTrace_RecordEvent(unsigned cat, unsigned a, unsigned b, unsigned c, unsigned d);
// RECV-style breadcrumb that also captures the first bytes of a packet buffer.
void CrashTrace_RecordPacket(unsigned cat, unsigned fromDpid, unsigned size,
                             const void* buf, unsigned buflen);

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
