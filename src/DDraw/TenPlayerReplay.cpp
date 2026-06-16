#include "TenPlayerReplay.h"
#include "HexDump.h"

#include "iddrawsurface.h"
#include "hook/etc.h"
#include "hook/hook.h"
#include "tafunctions.h"
#include "tamem.h"
#include "TPacket.h"
#include "TAbugfix.h"

#include <sstream>

static bool HACK_ON = false;
static PlayerStruct* LOCAL_HUMAN_PLAYER = NULL;

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
	if (hapi && DataShare->PlayingDemo && TAInGame == DataShare->TAProgress)
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
				for (int i = 0; i < 10; ++i)
					LOCAL_HUMAN_PLAYER->AllyFlagAry[i] = 1;
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
		taPtr->LOS_Sight_PlayerID == taPtr->LocalHumanPlayer_PlayerID)
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
	double prevMetalProduced = 0.0, prevEnergyProduced = 0.0;
	double prevMetalConsumed = 0.0, prevEnergyConsumed = 0.0;
	int    prevGameTime = 0;
	// Last good per-second rates; only updated when a sample passes every guard.
	bool   haveRates = false;
	float  metalProduction = 0.f, energyProduction = 0.f;
	float  metalExpense = 0.f, energyExpense = 0.f;
};
static FunnelEconomy g_funnelEcon;

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

std::unique_ptr<TenPlayerReplay> TenPlayerReplay::m_instance = NULL;

TenPlayerReplay::TenPlayerReplay()
{
	IDDrawSurface::OutptTxt("[TenPlayerReplay] initialising ...");
	// HAPINET_receivepacket is wrapped with a manual detour so the 10-player-replay
	// hack on/off detection + fromDpid rewrite can run after each packet is received
	// (and drops a RECV breadcrumb into the crash-trace ring).
	InstallHapiReceiveDetour();
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
