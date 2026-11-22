#pragma once

#include <memory>
#include <vector>

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
	std::unique_ptr <SingleHook> OrderDispatchGuardTeardown;
	std::unique_ptr <SingleHook> SoundInstanceLimit;
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

// The unit whose order handler is currently running, captured by OrderDispatchGuard and stamped
// with the GameTime it was captured on. TeamColorNanolathe uses it to identify the nanolathe
// emitter directly instead of scanning the unit array; reject it when the tick does not match.
extern DWORD g_currentOrderUnit;
extern int   g_currentOrderUnitTick;

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
#define TRACE_CAT_CFNR 0x43464E52u  // 'CFNR' : UNITS_CreateFromNetwork envelope RE-ENTERED --
                                    //          unitrotate.cpp saves [Esp] in ONE global, so any
                                    //          occurrence means the outer call returns to the
                                    //          INNER caller and applies the WRONG spawn record
                                    //          a=depth b=outer return c=outer record d=inner record
// ---- Unit-identity breadcrumbs (see UnitIdentity.cpp) --------------------------
// A wrong local UnitID means a wrong UnitWeapons[].p_Weapon (a pure function of it), which is what
// kills ReceiveWeaponFired at 0049CE6A. These make that divergence visible without the crash.
#define TRACE_CAT_MORF 0x4D4F5246u  // 'MORF' : UNITS_CreateFromNetwork onto an OCCUPIED slot
                                    //          a=unitIdx|(ownerSlot<<16) b=oldType|(newType<<16)
                                    //          c=call site: 004553E4 0x09 pkt, 0048BA00 2C dirty,
                                    //            0048B497 2C round-robin    d=GameTime
#define TRACE_CAT_GHST 0x47485354u  // 'GHST' : PENDING_DEATH set at 0048B42C -- owner says the slot
                                    //          is empty and we still have a live unit
                                    //          a=unitIdx b=localType c=ownerSlot d=GameTime
#define TRACE_CAT_WPNX 0x57504E58u  // 'WPNX' : ReceiveWeaponFired packet weapon != slot weapon
                                    //          a=unitIdx|(weaponSlot<<16) b=packet WeaponTypedef*
                                    //          c=slot WeaponTypedef*      d=GameTime
#define TRACE_CAT_2CBD 0x32434244u  // '2CBD' : 0x2C dirty entry with impossible fields
                                    //          a=iteration|(reason<<16) b=unit ptr c=typeID
                                    //          d=bitCursorBytes|(declaredLen<<16); cursor past the
                                    //          declared length means misframed, not merely stale
#define TRACE_CAT_SYNC 0x53594E43u  // 'SYNC' : identity audit finding.
                                    //          a=slot            -> nNumUnits drift:
                                    //             b=walked live count c=nNumUnits d=GameTime
                                    //          a=slot|0x8000     -> persistent peer disagreement:
                                    //             b=(myLive<<16)|ownerLive c=digest xor d=GameTime
// ---- BuildWeaponSlotGuard breadcrumbs (see BuildWeaponSlotGuard.cpp) ----------
#define TRACE_CAT_BWSG 0x42575347u  // 'BWSG' : BuildWeapon weapon slot not usable (throttled)
                                    //          a=unit ptr b=slot idx (0xFFFFFFFF: order unreadable)
                                    //          c=resolved WeaponStruct* (0: not reached)
                                    //          d=site(0 sim,1 hud) | verdict<<4 | GameTime<<8
                                    //          verdict 1=zeroDivisor 2=badIndex 3=unreadableOrder
                                    //          4=unreadableUnit 5=unreadableWeapon
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
