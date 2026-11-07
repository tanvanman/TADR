#include "TenPlayerReplay.h"
#include "ReplayFunnelInfo.h"
#include "HexDump.h"

#include "iddrawsurface.h"
#include "hook/etc.h"
#include "hook/hook.h"
#include "tafunctions.h"
#include "tamem.h"
#include "TPacket.h"
#include "TAbugfix.h"
#include "ChatHijackIds.h"
#include "GameTickHook.h"
#include "ReplayEndFences.h"
#include "ReplayTransportClose.h"

#include <sstream>

static bool HACK_ON = false;
static PlayerStruct* LOCAL_HUMAN_PLAYER = NULL;
static unsigned g_funnelTeamDpid = 0;
static int g_funnelTeamGameTime = -1;

static void SetFunnelStartingTeam(TAdynmemStruct* ta, PlayerStruct* player, unsigned char team)
{
	player->AllyTeam = team;
	const int slot = player->PlayerAryIndex;
	for (int n = 0; n < 10; ++n)
	{
		PlayerStruct& other = ta->Players[n];
		const bool allied = &other == player || (team < 5 && other.AllyTeam == team);
		player->AllyFlagAry[n] = allied;
		other.AllyFlagAry[slot] = allied;
	}
}

// Completion is an explicit handoff from the replay feeder, never a silence
// timeout. Its guaranteed, per-output-player fences follow every recorded event.
static ReplayEndFences g_endFences;
static ReplayEndSources g_endSources;
static int g_endLastGameTime = -1;
static bool g_replayFinished = false;
static bool g_replayEnding = false;
static bool g_insideReplaySimulation = false;
static bool g_forcedReplayEnd = false;

static void ResetReplayEnd(int gameTime)
{
	if (gameTime < g_endLastGameTime || !DataShare->PlayingDemo)
	{
		g_endFences = ReplayEndFences{};
		g_endSources = ReplayEndSources{};
		g_replayFinished = false;
		g_replayEnding = false;
		g_insideReplaySimulation = false;
		g_forcedReplayEnd = false;
	}
	g_endLastGameTime = gameTime;
}

static void RememberReplaySources(TAdynmemStruct* ta)
{
	// Explicit terminal role metadata also covers a tenth player with no data
	// packets. Resolve the real local controller independently of selected POV
	// before its final funnel marker reaches the existing raw receive wrapper.
	if (!LOCAL_HUMAN_PLAYER && g_endFences.viewerIsRecordedPlayer==1)
		for (unsigned i=0; i<10; ++i)
			if (ta->Players[i].PlayerActive && ta->Players[i].My_PlayerType==Player_LocalHuman) {
				LOCAL_HUMAN_PLAYER = &ta->Players[i];
				break;
			}
	for (unsigned i=0; i<10; ++i)
		if (ta->Players[i].PlayerActive && ta->Players[i].My_PlayerType==Player_RemoteHuman)
			g_endSources.observe((unsigned)ta->Players[i].DirectPlayID);
}

static int __stdcall ReplaySimulationBeginProc(PInlineX86StackBuffer)
{
	TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
	if (ta) {
		ResetReplayEnd(ta->GameTime);
		g_insideReplaySimulation = DataShare->PlayingDemo && DataShare->TAProgress==TAInGame &&
			*(unsigned*)((char*)ta+0x38a3b)!=0;
		if (g_insideReplaySimulation) RememberReplaySources(ta);
	}
	return 0;
}

static void ReceiveReplayEnd(unsigned fromDpid, const void* data)
{
	TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
	if (!ta || !DataShare->PlayingDemo || DataShare->TAProgress != TAInGame) return;
	ResetReplayEnd(ta->GameTime);
	const unsigned char* p = (const unsigned char*)data;
	if (!((p[3] == 16 && p[4] == 1) || (p[3] == 17 && p[4] == 2)) || memcmp(p + 5, "END", 3)) return;
	const int recordedViewer = p[4] == 2 ? p[16] : -1;
	unsigned tick;
	unsigned short players, bit;
	memcpy(&tick, p + 8, 4);
	memcpy(&players, p + 12, 2);
	memcpy(&bit, p + 14, 2);
	// The protocol token is a recorded colour ID. Native player-array indices
	// depend on the live session's allocation order and are not interchangeable.
	RememberReplaySources(ta);
	if (!g_endFences.accept(tick, players, bit, fromDpid, g_endSources.ids, (unsigned)ta->GameTime,
		recordedViewer, g_insideReplaySimulation)) return;
	IDDrawSurface::OutptFmtTxt("[ReplayEnd] fence from=%u token=%u received=%u required=%u final tick=%u game tick=%d",
		fromDpid, bit, g_endFences.received, g_endFences.required, tick, ta->GameTime);
}

static int __stdcall ReplayPacketDequeuedProc(PInlineX86StackBuffer stack)
{
	TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
	if (!ta || !DataShare->PlayingDemo) return 0;
	const unsigned guiState = *(unsigned*)((char*)ta+0x391f1);
	if (guiState!=5 && guiState!=6) return 0;
	ResetReplayEnd(ta->GameTime);
	RememberReplaySources(ta);
	const unsigned char* p = (const unsigned char*)ta->PacketBuffer_p;
	if (!p || p[0]!=0x05 || p[1]!=0 || p[2]!=ChatHijackId::ReplayEnd ||
		!((p[3]==16 && p[4]==1) || (p[3]==17 && p[4]==2))) return 0;
	if (p[4]==2 && !memcmp(p+5,"BEG",3)) {
		unsigned short players, bit;
		memcpy(&players,p+12,2);
		memcpy(&bit,p+14,2);
		g_endFences.announce(players,bit,ta->hapinet.fromDpid,g_endSources.ids,p[16]);
	}
	else if (!memcmp(p+5,"END",3) && DataShare->TAProgress==TAInGame)
		ReceiveReplayEnd(ta->hapinet.fromDpid,p);
	else return 0;
	// 0x453da8 is after the native per-sender queue returns a CHAT subpacket,
	// before it rejects an inactive/departed sender. Thus this observes the
	// actual dispatch boundary, not raw DirectPlay arrival or file EOF.
	// This is output-only protocol metadata. Consume it and dequeue the next
	// native packet without manufacturing a rejection for a departed sender.
	stack->rtnAddr_Pvoid = (void*)0x453d94;
	return X86STRACKBUFFERCHANGE;
}

static void CheckReplayEnd(int gameTime)
{
	const bool completedSimulation = g_insideReplaySimulation;
	g_insideReplaySimulation = false;
	ResetReplayEnd(gameTime);
	TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
	if (!ta || DataShare->TAProgress != TAInGame || g_replayFinished || !g_endFences.ready((unsigned)gameTime, completedSimulation)) return;
	// CHAT and unit updates share TA's per-sender dispatch queue (0x462f30).
	// All fences therefore follow the recorded events. This callback is after
	// Game_RunSimulationStep (0x495490). Its entry hook identifies fences read
	// during that pass; otherwise a later local tick must finish. Pending unit
	// deaths and other final updates run before this callback in either case.
	// Bit 4 requests battle->results (0x499600), not a victory/defeat verdict.
	g_replayFinished = true;
	ta->GameStateMask |= 4;
	IDDrawSurface::OutptFmtTxt("[ReplayEnd] all output streams drained; final tick=%u, game tick=%d", g_endFences.finalTick, gameTime);
}

static int __stdcall ReplayFinalStatsProc(PInlineX86StackBuffer stack)
{
	if (!DataShare->PlayingDemo || (!g_endFences.announced && !g_endFences.required)) return 0;
	g_replayEnding = true;
	// A recorded victory/leave cannot cut off later recorded final statistics,
	// the other output queues, or the final simulation pass. Explicit
	// user exits and actual session loss still follow TA's usual exit path.
	const bool draining = !g_replayFinished && !g_forcedReplayEnd;
	// At 0x499639 TA calls Network_AllFinalStatsReceived (0x4572a0).
	// Real peers exchange 0x28/0x29 here. Synthetic replay peers cannot answer;
	// their recorded snapshots have already been applied before the fences.
	// Skip just this network wait, retaining the native snapshot builder, all
	// cleanup, ENDMSN.GUI/stat bars, and its user-controlled Main Menu button.
	stack->Eax = draining ? 0 : 1;
	stack->rtnAddr_Pvoid = (void*)0x49963e;
	return X86STRACKBUFFERCHANGE;
}

static int __stdcall ReplayForcedEndProc(PInlineX86StackBuffer)
{
	if (DataShare->PlayingDemo) g_forcedReplayEnd = true;
	return 0;
}

static int __stdcall ReplayStatsPlayerProc(PInlineX86StackBuffer stack)
{
	if (!DataShare->PlayingDemo) return 0;
	PlayerStruct* player = (PlayerStruct*)stack->Edi;
	if (!g_endFences.excludeLocalViewer(player->My_PlayerType == Player_LocalHuman, player == LOCAL_HUMAN_PLAYER)) return 0;
	// Native snapshot builder (0x41dc20), after its normal row eligibility
	// checks. Skip only the extra viewer, before copying values or computing
	// bar scales. Do not modify any player, report slot, or recorded statistic.
	stack->rtnAddr_Pvoid = (void*)0x41debc;
	return X86STRACKBUFFERCHANGE;
}

// ===========================================================================
// HAPINET_receivepacket detour (0x4c9840)
// ---------------------------------------------------------------------------
// HAPINET_receivepacket (__stdcall, "RET 0xc") pulls one raw DirectPlay packet
// into the caller's buffer via CALL [vtable+0x64]. We wrap it with a manual
// 5-byte E9 detour + trampoline (rather than a framework InlineSingleHook at the
// entry) so our code runs AFTER the original has filled outBuf / hapi->fromDpid:
// it drops a RECV breadcrumb into the crash-trace ring and runs the 10-player-
// replay hack on/off detection + fromDpid rewrite.
//
// (Historical note: this wrapper used to SEH-guard the call to swallow the TAF
// demo recorder's 0x0EEDFADE Delphi over-read on funnel packets. That never
// worked -- the recorder catches its own exception first-chance -- and is no
// longer needed: the underlying crash is fixed in the replayer, which no longer
// feeds the recorder a 0x28 that would underflow its economy handler.)
typedef int (__stdcall* HapiReceiveFn)(HAPINETStruct* hapi, void* outBuf, int* bufSize);
static HapiReceiveFn RealHapiReceive = NULL;   // trampoline -> original body
static bool          g_hapiDetourInstalled = false;
unsigned int HapiReceiveDetourAddr = 0x4c9840;

int __stdcall HapiReceive_Detour(HAPINETStruct* hapi, void* outBuf, int* bufSize)
{
	int result = RealHapiReceive(hapi, outBuf, bufSize);

	// Breadcrumb: record the just-received packet at its true size into the general
	// crash-trace ring, so a later fault's report shows the last packets on this path.
	unsigned actualSize = bufSize ? (unsigned)*bufSize : 0u;
	CrashTrace_RecordPacket(TRACE_CAT_RECV,
		hapi ? (unsigned)hapi->fromDpid : 0u, actualSize, outBuf, actualSize);

	// 10-player-replay hack on/off detection + fromDpid rewrite -- the reason we wrap
	// the receive at all. Runs after the real Receive has filled outBuf / hapi->fromDpid.
	if (result == 0 && hapi && outBuf && actualSize >= 13 &&
		DataShare->PlayingDemo && TAInGame == DataShare->TAProgress)
	{
		std::uint8_t* buffer = (std::uint8_t*)outBuf;

		static const tapacket::bytestring hackon((const std::uint8_t*)"\x03\x04\x05\x06\x02\x08\x24\x0C\x0B\x0D", 10);
		static const tapacket::bytestring hackoff((const std::uint8_t*)"\x03\x04\x05\x06\x02\x08\x24\x0C\x0B\x0C", 10);

		const tapacket::bytestring hacktest(buffer + 3u, hackon.size());
		if (hacktest == hackon)
			HACK_ON = true;
		else// if (hacktest == hackoff)
			HACK_ON = false;

		if (HACK_ON)
		{
			TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
			if (!LOCAL_HUMAN_PLAYER && taPtr->Players[taPtr->LocalHumanPlayer_PlayerID].My_PlayerType == Player_LocalHuman)
			{
				LOCAL_HUMAN_PLAYER = &taPtr->Players[taPtr->LocalHumanPlayer_PlayerID];
				IDDrawSurface::OutptFmtTxt("[TenPlayerReplay] LOCAL_HUMAN_PLAYER.dpid=%d(%x)", LOCAL_HUMAN_PLAYER->DirectPlayID, LOCAL_HUMAN_PLAYER->DirectPlayID);
			}

			if (LOCAL_HUMAN_PLAYER)
			{
				taPtr->hapinet.fromDpid = LOCAL_HUMAN_PLAYER->DirectPlayID;
				LOCAL_HUMAN_PLAYER->My_PlayerType = Player_RemoteHuman;
				unsigned char startingTeam = 5;
				if (ReplayFunnelStartingTeam(buffer, actualSize, startingTeam))
				{
					// Initialise once; later recorded alliance changes must remain intact.
					if (g_funnelTeamDpid != (unsigned)LOCAL_HUMAN_PLAYER->DirectPlayID ||
						taPtr->GameTime < g_funnelTeamGameTime)
					{
						SetFunnelStartingTeam(taPtr, LOCAL_HUMAN_PLAYER, startingTeam);
						g_funnelTeamDpid = LOCAL_HUMAN_PLAYER->DirectPlayID;
					}
					g_funnelTeamGameTime = taPtr->GameTime;
					unsigned short mapX, mapY;
					if (ReplayFunnelMapPosition(buffer, actualSize, mapX, mapY))
					{
						const int slot = LOCAL_HUMAN_PLAYER->PlayerAryIndex;
						DataShare->OtherMapX[slot] = mapX;
						DataShare->OtherMapY[slot] = mapY;
					}
				}
				else if (g_funnelTeamDpid != (unsigned)LOCAL_HUMAN_PLAYER->DirectPlayID)
				{
					// Older replayers send no starting team. Preserve their existing
					// observer behaviour instead of guessing the recorded alliances.
					for (int n = 0; n < 10; ++n) LOCAL_HUMAN_PLAYER->AllyFlagAry[n] = 1;
				}
			}
		}
	}

	return result;
}

// Install a 5-byte E9 detour over the first instruction of HAPINET_receivepacket
// (PUSH 0x50b11c, exactly 5 bytes) and build a trampoline that runs that stolen
// instruction then jumps to target+5, so RealHapiReceive() invokes the untouched
// original. The original's own "RET 0xc" balances the 3 args we push, so stdcall
// stays correct.
static void InstallHapiReceiveDetour()
{
	if (g_hapiDetourInstalled) return;
	g_hapiDetourInstalled = true;

	const uintptr_t target = HapiReceiveDetourAddr;     // 0x4c9840
	const SIZE_T    stolen = 5;                          // PUSH imm32

	BYTE* tramp = (BYTE*)VirtualAlloc(NULL, 16, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!tramp) { IDDrawSurface::OutptTxt("[TenPlayerReplay] receive detour: VirtualAlloc failed"); return; }
	memcpy(tramp, (const void*)target, stolen);
	tramp[stolen] = 0xE9;
	*(LONG*)(tramp + stolen + 1) = (LONG)((target + stolen) - ((uintptr_t)tramp + stolen + 5));
	RealHapiReceive = (HapiReceiveFn)tramp;

	BYTE patch[5];
	patch[0] = 0xE9;
	*(LONG*)(patch + 1) = (LONG)((uintptr_t)&HapiReceive_Detour - (target + 5));

	DWORD oldProt = 0;
	VirtualProtect((void*)target, stolen, PAGE_EXECUTE_READWRITE, &oldProt);
	memcpy((void*)target, patch, stolen);
	VirtualProtect((void*)target, stolen, oldProt, &oldProt);
	FlushInstructionCache(GetCurrentProcess(), (void*)target, stolen);
	IDDrawSurface::OutptTxt("[TenPlayerReplay] HAPINET_receivepacket detour installed (10-player-replay hack detection)");
}

unsigned int GetLocalPlayerDpidHackAddr = 0x44fdb1;
int __stdcall GetLocalPlayerDpidHackProc(PInlineX86StackBuffer X86StrackBuffer)
{
	if (LOCAL_HUMAN_PLAYER)
	{
		X86StrackBuffer->Eax = LOCAL_HUMAN_PLAYER->DirectPlayID;
		//IDDrawSurface::OutptFmtTxt("[GetLocalPlayerDpidHackProc] dpid=%d(%x)", X86StrackBuffer->Eax, X86StrackBuffer->Eax);
		X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x44fdf3;	// return eax
		return X86STRACKBUFFERCHANGE;
	}

	return 0;
}

#define LOCAL_PLAYER_CHECK_HACK(hookAddr, n, playerRegister, returnAddr) \
unsigned int LocalPlayerCheckHack##n##Addr = (hookAddr); \
int __stdcall LocalPlayerCheckHack##n##Proc(PInlineX86StackBuffer X86StrackBuffer) \
{ \
    PlayerStruct* fromPlayer = (PlayerStruct*)(X86StrackBuffer->##playerRegister); \
    if (fromPlayer == LOCAL_HUMAN_PLAYER) \
    { \
		/*IDDrawSurface::OutptFmtTxt("[LocalPlayerCheckHack%d] fromPlayer=%d(%x)", n, fromPlayer->DirectPlayID, fromPlayer->DirectPlayID);*/ \
		X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)(returnAddr); \
        return X86STRACKBUFFERCHANGE; \
    } \
    return 0; \
}

LOCAL_PLAYER_CHECK_HACK(0x45650d, 1, Ebp, 0x456518)
LOCAL_PLAYER_CHECK_HACK(0x4564c7, 2, Ebp, 0x4564d6)
LOCAL_PLAYER_CHECK_HACK(0x45635c, 3, Ebp, 0x45636b)
LOCAL_PLAYER_CHECK_HACK(0x454aa8, 4, Ebx, 0x454ab3)
LOCAL_PLAYER_CHECK_HACK(0x45324a, 5, Eax, 0x453263)
LOCAL_PLAYER_CHECK_HACK(0x4531c2, 6, Edx, 0x4531db)
LOCAL_PLAYER_CHECK_HACK(0x453126, 7, Ecx-0x73, 0x453140)
LOCAL_PLAYER_CHECK_HACK(0x452bda, 8, Ecx, 0x452beb)
LOCAL_PLAYER_CHECK_HACK(0x451e8f, 9, Eax, 0x451ea0) // HAPI_BroadcastMessage
LOCAL_PLAYER_CHECK_HACK(0x451cd0, 10, Esi, 0x451cda) // HAPI_SendBuf
LOCAL_PLAYER_CHECK_HACK(0x451ca5, 11, Esi, 0x451cb4) // HAPI_SendBuf
LOCAL_PLAYER_CHECK_HACK(0x451ac5, 12, Ebp, 0x451ad0)
LOCAL_PLAYER_CHECK_HACK(0x451a7f, 13, Ebp, 0x451a8e)
LOCAL_PLAYER_CHECK_HACK(0x451010, 14, Ebp, 0x45101b)
LOCAL_PLAYER_CHECK_HACK(0x450fca, 15, Ebp, 0x450fd9)
LOCAL_PLAYER_CHECK_HACK(0x450cc6, 16, Ebp, 0x450cd1)
LOCAL_PLAYER_CHECK_HACK(0x450c80, 17, Ebp, 0x450c8f)
LOCAL_PLAYER_CHECK_HACK(0x455237, 18, Ebx, 0x455241) // Packet_Dispatcher CHAT_05 handler
LOCAL_PLAYER_CHECK_HACK(0x45339d, 19, Edx-0x73, 0x4534c2) // BroadcastText


// ===========================================================================
// Free-roam watcher "see everything" sensor buff
// ---------------------------------------------------------------------------
// Hook site 0x46754A sits in pass 2 of DrawOtherPlayer_MAPPEDMEM (0x467440),
// the loop that walks the VIEW player's own radar/sonar emitters and calls
// CallbackForUnitsInDistance -> RadarSonarDetect_MarkUnitDetected to flag which
// units they detect.  The scan context being assembled on the stack is
//     [ESP+0x20] vtbl, [ESP+0x24] radarRangeSq, [ESP+0x28] sonarRangeSq,
//     [ESP+0x2C..0x34] scan centre XYZ.
// We give the view player's first unit a whole-map radius so a watcher with no
// units of their own can still see submarines (which need a *sonar* contact --
// plain LOS never reveals them, see CheckUnitInPlayerLOS).
//
// The centre is forced to (0,0,0) because a huge range measured from the unit's
// own position overflows the fixed-point distance test unless the unit happens
// to sit near the map origin.
//
// IMPORTANT -- this must not run while the watcher is following a single
// player's POV.  It writes BOTH range slots (the resume address 0x46758D
// executes "MOV [ESP+0x28],EDX" for us, so EDX lands in sonarRangeSq while we
// store radarRangeSq ourselves), so it is a map-wide RADAR as well as a
// map-wide sonar: every unit in the game ends up with UnitStateMask 0x100/0x200
// and therefore a blip on the native minimap AND, via RadarUnits/
// NumHotRadarUnits, on the megamap.  ViewPlayerLos_Replay() sets BOTH
// LOS_Sight_PlayerID and LocalHumanPlayer_PlayerID to the selected slot, so the
// old LOS_Sight_PlayerID == LocalHumanPlayer_PlayerID test did not distinguish
// the two modes and POV mode showed every player's dots.  IsWatchingOtherPlayerPov()
// is the gate; in free-roam mode LosType is 0 and everything is visible anyway,
// so the buff only ever has to serve the free-roam case.
// ===========================================================================
//unsigned int PermSonarLosAddr = 0x42c3e6;  // patch commander unit definition
unsigned int PermSonarLosAddr = 0x46754a;	 // patch sonar calculation
int __stdcall PermSonarLosProc(PInlineX86StackBuffer X86StrackBuffer)
{
	static const int RADIUS = 30000;

	// patch commander unit definition
	//UnitDefStruct* u = (UnitDefStruct*)X86StrackBuffer->Ebp;
	//if (DataShare->PlayingDemo && !strcmp(u->Name, "Commander"))
	//{
	//	u->nSonarDistance = RADIUS;
	//}
	//return 0;

	// patch sonar calculation
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	if (DataShare->PlayingDemo && TAInGame == DataShare->TAProgress &&
		taPtr->LOS_Sight_PlayerID == taPtr->LocalHumanPlayer_PlayerID &&
		!IsWatchingOtherPlayerPov())
	{
		UnitStruct* unit = (UnitStruct*)X86StrackBuffer->Esi;
		PlayerStruct* viewingPlayer = &taPtr->Players[taPtr->LOS_Sight_PlayerID];
		if (unit->Owner_PlayerPtr0 == viewingPlayer)
		{
			for (int i = 0; i < taPtr->PlayerUnitsNumber_Skim; ++i)
			{
				if (viewingPlayer->Units[i].IsUnit)
				{
					if (viewingPlayer->Units[i].UnitInGameIndex == unit->UnitInGameIndex)
					{
						// viewing player's first unit gets a sonar buff
						int posy = *(short*)(X86StrackBuffer->Esi + 0x70);
						X86StrackBuffer->Ecx = RADIUS;// +2 * posy;
						X86StrackBuffer->Edx = RADIUS;
						X86StrackBuffer->Eax = RADIUS;
						X86StrackBuffer->Ecx *= X86StrackBuffer->Ecx;
						X86StrackBuffer->Edx *= X86StrackBuffer->Edx;

						//X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x467586;
						//return X86STRACKBUFFERCHANGE;

						// large nSonarDistance doesn't seem to work unless unit is near top left of map ...
						static DWORD fakePosition[3] = { 0, 0, 0 };
						*(DWORD*)(X86StrackBuffer->Esp + 0x24) = X86StrackBuffer->Ecx;
						X86StrackBuffer->Ecx = (DWORD)&fakePosition[0];
						X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x46758d;
						return X86STRACKBUFFERCHANGE;
					}
					return 0;
				}
			}
		}
	}
	return 0;
}

// ===========================================================================
// Funnel-player income / expense display (10-player replay)
// ---------------------------------------------------------------------------
// One demo player is funneled to the watcher's slot (its 0x28 packets get fromDpid
// rewritten to LOCAL_HUMAN_PLAYER), so its cumulative produced/consumed totals land
// in LOCAL_HUMAN_PLAYER->PlayerRes. But the income/expense arrows read the per-tick
// fMetalProduction/fEnergyProducton/f*Expense fields, which only the local economy
// sim writes -- and that sim is skipped for the funnel slot (it's type 3,
// Player_RemoteHuman), so the arrows stay frozen on the spectator commander.
// Fix: sample the totals at each 0x28, derive per-second rates, write the arrow
// fields. Legacy-safe (no valid sample -> fields untouched, never worse). Assumes a
// single funnel player (true today); multiple would interleave the totals series.
struct FunnelEconomy
{
	bool   haveSample = false;
	int    dpid = 0;
	double prevMetalProduced = 0.0, prevEnergyProduced = 0.0;
	double prevMetalConsumed = 0.0, prevEnergyConsumed = 0.0;
	int    prevGameTime = 0;
	// Last good per-second rates; only updated when a sample passes every guard.
	bool   haveRates = false;
	float  metalProduction = 0.f, energyProduction = 0.f;
	float  metalExpense = 0.f, energyExpense = 0.f;
};
static FunnelEconomy g_funnelEcon;

bool TenPlayerReplay_HasFunnelEconomySample(const PlayerStruct* player)
{
	if (!DataShare->PlayingDemo || DataShare->TAProgress != TAInGame) return false;
	if (g_funnelEcon.haveSample && (*TAmainStruct_PtrPtr)->GameTime < g_funnelEcon.prevGameTime)
		g_funnelEcon = FunnelEconomy();
	return player && player == LOCAL_HUMAN_PLAYER && g_funnelEcon.haveSample &&
		player->DirectPlayID == g_funnelEcon.dpid;
}

// 0x28 packet float offsets (see Receive_PacketPlayerRes @0x457540):
//   +0x22 fTotalEnergyProduced, +0x26 fTotalEnergyConsumed,
//   +0x2e fTotalMetalProduced,  +0x32 fTotalMetalConsumed.
static void SampleFunnelEconomy(const std::uint8_t* payload, PlayerStruct* player)
{
	if (!HACK_ON || player == NULL || player != LOCAL_HUMAN_PLAYER || payload == NULL)
		return;

	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	int gameTime = taPtr->GameTime;

	double mProd = *(const float*)(payload + 0x2e);
	double eProd = *(const float*)(payload + 0x22);
	double mCons = *(const float*)(payload + 0x32);
	double eCons = *(const float*)(payload + 0x26);

	if (g_funnelEcon.haveSample)
	{
		int    dt  = gameTime - g_funnelEcon.prevGameTime;
		double dmp = mProd - g_funnelEcon.prevMetalProduced;
		double dep = eProd - g_funnelEcon.prevEnergyProduced;
		double dmc = mCons - g_funnelEcon.prevMetalConsumed;
		double dec = eCons - g_funnelEcon.prevEnergyConsumed;
		// Need forward time and monotonic deltas; a reset (new game/rollover) gives
		// dt<=0 or a negative delta -> skip and just reseed the baseline below.
		if (dt > 0 && dmp >= 0.0 && dep >= 0.0 && dmc >= 0.0 && dec >= 0.0)
		{
			// fMetalProduction is the per-resource-tick increment of the cumulative total, and
			// the resource tick runs ~once per game-second. Convert our per-game-tick delta to
			// that scale. (Sole calibration knob: flip to 1 if magnitudes read ~30x off.)
			const float TICKS_PER_RESOURCE_UPDATE = 30.0f;
			float mp = (float)(dmp / dt) * TICKS_PER_RESOURCE_UPDATE;
			float ep = (float)(dep / dt) * TICKS_PER_RESOURCE_UPDATE;
			float me = (float)(dmc / dt) * TICKS_PER_RESOURCE_UPDATE;
			float ee = (float)(dec / dt) * TICKS_PER_RESOURCE_UPDATE;
			const float LIM = 1.0e7f;   // reject absurd rates (and, implicitly, non-finite)
			if (mp < LIM && ep < LIM && me < LIM && ee < LIM)
			{
				g_funnelEcon.metalProduction  = mp;
				g_funnelEcon.energyProduction = ep;
				g_funnelEcon.metalExpense     = me;
				g_funnelEcon.energyExpense    = ee;
				if (!g_funnelEcon.haveRates)
					IDDrawSurface::OutptFmtTxt("[TenPlayerReplay] funnel econ first sample: prod M+%.1f E+%.1f / use M-%.1f E-%.1f (dt=%d)", mp, ep, me, ee, dt);
				g_funnelEcon.haveRates = true;

				// Write the arrow fields directly (player == LOCAL_HUMAN_PLAYER here): the
				// funnel slot is type 3, so UNITS_GenerateResources never runs for it and won't
				// overwrite these -- they persist until the next 0x28. (0x401bae override is a
				// backup for any type-1/2 path where the sim does run.)
				player->PlayerRes.fMetalProduction  = mp;
				player->PlayerRes.fEnergyProducton  = ep;
				player->PlayerRes.fMetalExpense     = me;
				player->PlayerRes.fEnergyExpense    = ee;
			}
		}
	}

	g_funnelEcon.prevMetalProduced = mProd;
	g_funnelEcon.prevEnergyProduced = eProd;
	g_funnelEcon.prevMetalConsumed = mCons;
	g_funnelEcon.prevEnergyConsumed = eCons;
	g_funnelEcon.prevGameTime = gameTime;
	g_funnelEcon.haveSample = true;
	g_funnelEcon.dpid = player->DirectPlayID;
}

// Entry hook on Receive_PacketPlayerRes (0x457540, __stdcall(packet, player)).
// At the captured entry Esp: [Esp+4]=packet payload, [Esp+8]=PlayerStruct*. Read-only.
unsigned int FunnelEconSampleAddr = 0x457540;
int __stdcall FunnelEconSampleProc(PInlineX86StackBuffer X86StrackBuffer)
{
	const std::uint8_t* payload = *(const std::uint8_t**)(X86StrackBuffer->Esp + 4);
	PlayerStruct* player = *(PlayerStruct**)(X86StrackBuffer->Esp + 8);
	SampleFunnelEconomy(payload, player);
	return 0;
}

void TenPlayerReplay_OverrideFunnelIncome(PlayerStruct* player)
{
	// Backup path (economy-sim tail @0x401bae) for a type-1/2 slot where the sim runs;
	// the funnel slot is type 3, so this normally never fires. Don't gate on HACK_ON
	// (a per-packet flag, already reset by the time the sim runs) -- gate on the
	// persistent replay state + haveRates; PlayingDemo keeps us off a live game.
	if (player == NULL || player != LOCAL_HUMAN_PLAYER || !g_funnelEcon.haveRates)
		return;
	if (!DataShare->PlayingDemo || TAInGame != DataShare->TAProgress)
		return;
	static bool loggedOverride = false;
	if (!loggedOverride) { loggedOverride = true; IDDrawSurface::OutptTxt("[TenPlayerReplay] funnel income override applied to LOCAL_HUMAN_PLAYER"); }
	player->PlayerRes.fMetalProduction  = g_funnelEcon.metalProduction;
	player->PlayerRes.fEnergyProducton  = g_funnelEcon.energyProduction;
	player->PlayerRes.fMetalExpense     = g_funnelEcon.metalExpense;
	player->PlayerRes.fEnergyExpense    = g_funnelEcon.energyExpense;
}

bool TenPlayerReplay_IsFunnelPlayer(const PlayerStruct* player)
{
	return DataShare->PlayingDemo && player && player == LOCAL_HUMAN_PLAYER;
}

bool TenPlayerReplay_HasPovMetadata()
{
	return DataShare->PlayingDemo && LOCAL_HUMAN_PLAYER &&
		g_funnelTeamDpid == (unsigned)LOCAL_HUMAN_PLAYER->DirectPlayID;
}

int ReplayCameraFollowSlot()
{
	if (DataShare->LockOn == 10 && TenPlayerReplay_HasPovMetadata()) return 0;
	return DataShare->LockOn > 0 && DataShare->LockOn < 10 ? DataShare->LockOn : -1;
}

void SetReplayCameraFollowSlot(int slot)
{
	DataShare->LockOn = slot == 0 && TenPlayerReplay_HasPovMetadata() ? 10 :
		(slot > 0 && slot < 10 ? slot : 0);
}

// In a ten-player replay the native HUD otherwise reports the shared player's
// resources even in the observer's original camera. Only change the draw cache
// and skip the player-value paint pass; never touch simulated resources.
static bool HideOriginalCameraResourceHud()
{
	return TAInGame == DataShare->TAProgress && TenPlayerReplay_HasPovMetadata() &&
		((*TAmainStruct_PtrPtr)->LosType & (NOMAPPING | Permanent)) == 0;
}

static int __stdcall OriginalCameraHudCacheProc(PInlineX86StackBuffer stack)
{
	if (HideOriginalCameraResourceHud())
		*(unsigned char*)(stack->Esp + 0x78) = 0xff;
	return 0;
}

static int __stdcall OriginalCameraHudPaintProc(PInlineX86StackBuffer stack)
{
	if (!HideOriginalCameraResourceHud()) return 0;
	stack->rtnAddr_Pvoid = (LPVOID)0x469610;
	return X86STRACKBUFFERCHANGE;
}

// Only the system provider's blocking Close runs off the GUI thread. The
// native snapshot builder, recorder, unit cleanup and results GUI stay native.
static std::unique_ptr<replayclose::Closer> g_transportCloser;
static unsigned g_pendingMenu = 0;
static int g_pendingMenuControl = 0;
static bool g_closingReplayToResults = false;

static bool ReplayMayCloseTransportAsync()
{
	TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
	return g_closingReplayToResults && g_replayEnding && ta &&
		*(unsigned*)((char*)ta+0x391f1)==6 && (ta->GameStateMask & 4);
}

static int __stdcall ReplayQueueMainMenuProc(PInlineX86StackBuffer stack)
{
	if (!g_transportCloser || !g_transportCloser->pending()) return 0;
	// Retain a genuine Main Menu click until transport cleanup finishes.
	// The results continue rendering and handling input during that time.
	g_pendingMenu = stack->Esi;
	g_pendingMenuControl = *(int*)(g_pendingMenu+0x60);
	stack->rtnAddr_Pvoid = (void*)0x41ee6e;
	return X86STRACKBUFFERCHANGE;
}

static int __stdcall ReplayContinueMainMenuProc(PInlineX86StackBuffer stack)
{
	if (!g_pendingMenu || !g_transportCloser || g_transportCloser->pending()) return 0;
	unsigned menu = g_pendingMenu;
	g_pendingMenu = 0;
	*(int*)(menu+0x60) = g_pendingMenuControl;
	// Resume the saved native event in its ordinary GUI dispatch slot.
	((void(__stdcall*)(unsigned))0x41ec50)(menu);
	stack->rtnAddr_Pvoid = stack->myInlineHookClass_Pish->m_AddrToHook==(void*)0x41ffc3
		? (void*)0x41ffd7 : (void*)0x420595;
	return X86STRACKBUFFERCHANGE;
}

static int __stdcall ReplayBeginResultsCleanupProc(PInlineX86StackBuffer)
{
	// Other quit/load paths cannot release a job at a results frame.
	g_closingReplayToResults = g_replayEnding;
	return 0;
}

static int __stdcall ReplayResultsFrameProc(PInlineX86StackBuffer)
{
	if (g_transportCloser) {
		// Native HAPINET_close and all GUI-side Release calls have returned.
		g_transportCloser->startQueued();
		g_transportCloser->collect([](HRESULT result, DWORD) {
			if (FAILED(result)) IDDrawSurface::OutptFmtTxt("[ReplayEnd] transport Close failed: %08x", (unsigned)result);
		});
	}
	return 0;
}

std::unique_ptr<TenPlayerReplay> TenPlayerReplay::m_instance = NULL;

TenPlayerReplay::TenPlayerReplay()
{
	IDDrawSurface::OutptTxt("[TenPlayerReplay] initialising ...");
	g_transportCloser.reset(new replayclose::Closer(ReplayMayCloseTransportAsync));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(0x4996aa, 5, INLINE_5BYTESLAGGERJMP, ReplayBeginResultsCleanupProc));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(0x41f7f0, 5, INLINE_5BYTESLAGGERJMP, ReplayResultsFrameProc));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(0x41ee3c, 7, INLINE_5BYTESLAGGERJMP, ReplayQueueMainMenuProc));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(0x41ffc3, 5, INLINE_5BYTESLAGGERJMP, ReplayContinueMainMenuProc));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(0x420581, 5, INLINE_5BYTESLAGGERJMP, ReplayContinueMainMenuProc));
	GameTickHook::GetInstance()->addCallback(CheckReplayEnd);
	m_hooks.push_back(std::make_unique<InlineSingleHook>(0x495490, 5, INLINE_5BYTESLAGGERJMP, ReplaySimulationBeginProc));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(0x453da8, 6, INLINE_5BYTESLAGGERJMP, ReplayPacketDequeuedProc));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(0x499639, 5, INLINE_5BYTESLAGGERJMP, ReplayFinalStatsProc));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(0x41dd61, 5, INLINE_5BYTESLAGGERJMP, ReplayStatsPlayerProc));
	// Native confirmed quit/resign branches and transport-loss callback.
	m_hooks.push_back(std::make_unique<InlineSingleHook>(0x46061a, 5, INLINE_5BYTESLAGGERJMP, ReplayForcedEndProc));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(0x464e13, 5, INLINE_5BYTESLAGGERJMP, ReplayForcedEndProc));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(0x464e50, 5, INLINE_5BYTESLAGGERJMP, ReplayForcedEndProc));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(0x4578f0, 6, INLINE_5BYTESLAGGERJMP, ReplayForcedEndProc));
	// HAPINET_receivepacket is wrapped with a manual detour so the 10-player-replay
	// hack on/off detection + fromDpid rewrite can run after each packet is received
	// (and drops a RECV breadcrumb into the crash-trace ring).
	InstallHapiReceiveDetour();
	// Validate both instruction boundaries and the resume point before installing.
	static const unsigned char cacheBytes[] = {0x8d,0x83,0x3f,0x7e,0x03,0x00};
	static const unsigned char paintBytes[] = {0x8d,0x46,0x42,0x6a,0x00};
	static const unsigned char resumeBytes[] = {0x8d,0x54,0x24,0x34};
	if (!memcmp((void*)0x468fc6, cacheBytes, sizeof(cacheBytes)) &&
		!memcmp((void*)0x46909f, paintBytes, sizeof(paintBytes)) &&
		!memcmp((void*)0x469610, resumeBytes, sizeof(resumeBytes)))
	{
		m_hooks.push_back(std::make_unique<InlineSingleHook>(0x468fc6, 6, INLINE_5BYTESLAGGERJMP, OriginalCameraHudCacheProc));
		m_hooks.push_back(std::make_unique<InlineSingleHook>(0x46909f, 5, INLINE_5BYTESLAGGERJMP, OriginalCameraHudPaintProc));
	}
	m_hooks.push_back(std::make_unique<InlineSingleHook>(GetLocalPlayerDpidHackAddr, 5, INLINE_5BYTESLAGGERJMP, GetLocalPlayerDpidHackProc));
	//m_hooks.push_back(std::make_unique<InlineSingleHook>(LocalPlayerCheckHack1Addr, 5, INLINE_5BYTESLAGGERJMP, LocalPlayerCheckHack1Proc));
	//m_hooks.push_back(std::make_unique<InlineSingleHook>(LocalPlayerCheckHack2Addr, 5, INLINE_5BYTESLAGGERJMP, LocalPlayerCheckHack2Proc));
	//m_hooks.push_back(std::make_unique<InlineSingleHook>(LocalPlayerCheckHack3Addr, 5, INLINE_5BYTESLAGGERJMP, LocalPlayerCheckHack3Proc));
	//m_hooks.push_back(std::make_unique<InlineSingleHook>(LocalPlayerCheckHack4Addr, 5, INLINE_5BYTESLAGGERJMP, LocalPlayerCheckHack4Proc));
	//m_hooks.push_back(std::make_unique<InlineSingleHook>(LocalPlayerCheckHack5Addr, 5, INLINE_5BYTESLAGGERJMP, LocalPlayerCheckHack5Proc));
	//m_hooks.push_back(std::make_unique<InlineSingleHook>(LocalPlayerCheckHack6Addr, 5, INLINE_5BYTESLAGGERJMP, LocalPlayerCheckHack6Proc));
	//m_hooks.push_back(std::make_unique<InlineSingleHook>(LocalPlayerCheckHack7Addr, 5, INLINE_5BYTESLAGGERJMP, LocalPlayerCheckHack7Proc));
	//m_hooks.push_back(std::make_unique<InlineSingleHook>(LocalPlayerCheckHack8Addr, 5, INLINE_5BYTESLAGGERJMP, LocalPlayerCheckHack8Proc));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(LocalPlayerCheckHack9Addr, 5, INLINE_5BYTESLAGGERJMP, LocalPlayerCheckHack9Proc));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(LocalPlayerCheckHack10Addr, 5, INLINE_5BYTESLAGGERJMP, LocalPlayerCheckHack10Proc));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(LocalPlayerCheckHack11Addr, 5, INLINE_5BYTESLAGGERJMP, LocalPlayerCheckHack11Proc));
	//m_hooks.push_back(std::make_unique<InlineSingleHook>(LocalPlayerCheckHack12Addr, 5, INLINE_5BYTESLAGGERJMP, LocalPlayerCheckHack12Proc));
	//m_hooks.push_back(std::make_unique<InlineSingleHook>(LocalPlayerCheckHack13Addr, 5, INLINE_5BYTESLAGGERJMP, LocalPlayerCheckHack13Proc));
	//m_hooks.push_back(std::make_unique<InlineSingleHook>(LocalPlayerCheckHack14Addr, 5, INLINE_5BYTESLAGGERJMP, LocalPlayerCheckHack14Proc));
	//m_hooks.push_back(std::make_unique<InlineSingleHook>(LocalPlayerCheckHack15Addr, 5, INLINE_5BYTESLAGGERJMP, LocalPlayerCheckHack15Proc));
	//m_hooks.push_back(std::make_unique<InlineSingleHook>(LocalPlayerCheckHack16Addr, 5, INLINE_5BYTESLAGGERJMP, LocalPlayerCheckHack16Proc));
	//m_hooks.push_back(std::make_unique<InlineSingleHook>(LocalPlayerCheckHack17Addr, 5, INLINE_5BYTESLAGGERJMP, LocalPlayerCheckHack17Proc));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(LocalPlayerCheckHack18Addr, 5, INLINE_5BYTESLAGGERJMP, LocalPlayerCheckHack18Proc));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(LocalPlayerCheckHack19Addr, 5, INLINE_5BYTESLAGGERJMP, LocalPlayerCheckHack19Proc));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(PermSonarLosAddr, 5, INLINE_5BYTESLAGGERJMP, PermSonarLosProc));
	// Sample the funnel player's 0x28 resource totals so we can show its real
	// income/expense arrows (overridden at the economy-sim tail). See the
	// FunnelEconomy block above for the full rationale.
	m_hooks.push_back(std::make_unique<InlineSingleHook>(FunnelEconSampleAddr, 5, INLINE_5BYTESLAGGERJMP, FunnelEconSampleProc));
}


TenPlayerReplay* TenPlayerReplay::GetInstance()
{
	if (m_instance == NULL) {
		m_instance.reset(new TenPlayerReplay());
	}
	return m_instance.get();
}
