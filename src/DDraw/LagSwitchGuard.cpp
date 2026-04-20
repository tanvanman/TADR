#include "LagSwitchGuard.h"
#include "iddrawsurface.h"
#include "tamem.h"

#include <cstring>

LagSwitchGuard* LagSwitchGuard::m_instance = nullptr;

LagSwitchGuard* LagSwitchGuard::GetInstance()
{
	if (!m_instance)
		m_instance = new LagSwitchGuard();
	return m_instance;
}

LagSwitchGuard::LagSwitchGuard()
	: m_frozen(false)
	, m_frozenSinceMs(0)
	, m_lastTickLeakMs(0)
	, m_hudLineId(0)
	, m_resumeLineId(0)
	, m_resumeLineExpiry(0)
{
	memset(m_playerTrack, 0, sizeof(m_playerTrack));

	// Hook at 0x4967f8 in GameTickFunction, immediately after CALL ApplyDeltaTime.
	// Instruction: MOV EAX,[0x00511de8]  (5 bytes, A1 E8 1D 51 00)
	// INLINE_5BYTESLAGGERJMP: router fires, then overwritten MOV re-executes.
	// While frozen, the router controls DeltaTime:
	//   - Normally: DeltaTime=0 (suppress simulation)
	//   - Every TICK_LEAK_INTERVAL_MS: DeltaTime=1 (let Packet_Dispatcher run)
	m_postApplyDeltaTimeHook = std::make_unique<InlineSingleHook>(
		(unsigned int)0x4967f8, 5, INLINE_5BYTESLAGGERJMP,
		(InlineX86HookRouter)&LagSwitchGuard::PostApplyDeltaTimeRouter);

	// Hook the pause key handler at 0x496099 (case 0xF8 in Keyboard_Proc_P25).
	// Instruction: MOV ECX,[0x00511de8]  (6 bytes: 8B 0D E8 1D 51 00)
	// When frozen, redirect to the switch epilogue (0x4965CE) to skip the
	// entire pause block.  When not frozen, return 0 to let pause work normally.
	m_pauseKeyHook = std::make_unique<InlineSingleHook>(
		(unsigned int)0x496099, 6, INLINE_5BYTESLAGGERJMP,
		(InlineX86HookRouter)&LagSwitchGuard::PauseKeyRouter);
}

// -----------------------------------------------------------------------
// PostApplyDeltaTimeRouter: fires right after ApplyDeltaTime() returns,
// before GameTickFunction reads DeltaTime.
//
// While frozen: overrides DeltaTime.  Normally 0 to suppress simulation.
// Every TICK_LEAK_INTERVAL_MS, sets DeltaTime=1 so one tick runs
// (including Packet_Dispatcher) to keep LastMsgTimeStamp alive.
//
// While not frozen: no-op, normal execution.
// -----------------------------------------------------------------------
int __stdcall LagSwitchGuard::PostApplyDeltaTimeRouter(PInlineX86StackBuffer pBuf)
{
	LagSwitchGuard* guard = GetInstance();
	if (guard->m_frozen)
	{
		TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
		if (taPtr)
		{
			DWORD now = GetTickCount();
			if (now - guard->m_lastTickLeakMs >= TICK_LEAK_INTERVAL_MS)
			{
				// Allow one tick through for packet dispatch
				guard->m_lastTickLeakMs = now;
				taPtr->DeltaTime = 1;
			}
			else
			{
				// Suppress simulation
				taPtr->DeltaTime = 0;
			}
		}
	}
	(void)pBuf;
	return 0;
}

// -----------------------------------------------------------------------
// PauseKeyRouter: fires at the entry of the pause key case (0xF8) in
// Keyboard_Proc_P25 (0x496099).
//
// When frozen: redirect to the switch epilogue (0x4965CE), skipping the
// entire pause toggle + broadcast block.  The pause key simply does nothing.
//
// When not frozen: return 0 to let the overwritten instructions re-execute
// and the pause key work normally.
// -----------------------------------------------------------------------
int __stdcall LagSwitchGuard::PauseKeyRouter(PInlineX86StackBuffer pBuf)
{
	if (GetInstance()->m_frozen)
	{
		pBuf->rtnAddr_Pvoid = (LPVOID)0x4965CE;  // switch epilogue
		return X86STRACKBUFFERCHANGE;
	}
	return 0;
}

// -----------------------------------------------------------------------
// Tick: called each render frame from IDDrawSurface::Unlock on the GUI thread.
// -----------------------------------------------------------------------
void LagSwitchGuard::Tick()
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	if (!taPtr)
		return;

	DWORD now = GetTickCount();

	// Expire transient resume notification
	if (m_resumeLineId && now >= m_resumeLineExpiry)
	{
		HudNotifications::GetInstance()->RemoveLine(m_resumeLineId);
		m_resumeLineId = 0;
		m_resumeLineExpiry = 0;
	}

	// Not active outside in-game multiplayer or during demo playback.
	if (DataShare->TAProgress != TAInGame || DataShare->PlayingDemo)
	{
		if (m_frozen)
		{
			m_frozen = false;
			if (m_hudLineId)
			{
				HudNotifications::GetInstance()->RemoveLine(m_hudLineId);
				m_hudLineId = 0;
			}
		}
		memset(m_playerTrack, 0, sizeof(m_playerTrack));
		return;
	}

	// When paused and NOT frozen, skip detection — timestamps don't advance
	// during pause so we'd false-trigger.
	if (taPtr->IsGamePaused && !m_frozen)
	{
		memset(m_playerTrack, 0, sizeof(m_playerTrack));
		return;
	}

	int localSlot = taPtr->LocalHumanPlayer_PlayerID;
	unsigned localDpid = 0;
	if (localSlot >= 0 && localSlot < 10)
		localDpid = taPtr->Players[localSlot].DirectPlayID;

	DWORD maxLastReceiveMs = 0;
	int remoteHumanCount = 0;

	for (int i = 0; i < 10; ++i)
	{
		PlayerStruct& p = taPtr->Players[i];

		// Only track active remote human players
		if (!p.PlayerActive || p.DirectPlayID == 0 || p.DirectPlayID == localDpid)
			continue;
		if (p.My_PlayerType != Player_RemoteHuman)
			continue;

		++remoteHumanCount;
		PlayerTrack& track = m_playerTrack[i];

		if (track.lastReceiveTickMs == 0)
		{
			// First time seeing this player — initialise
			track.lastSeenTimestamp = p.LastMsgTimeStamp;
			track.lastReceiveTickMs = now;
		}
		else if (p.LastMsgTimeStamp != track.lastSeenTimestamp)
		{
			// Timestamp advanced — network traffic arrived
			track.lastSeenTimestamp = p.LastMsgTimeStamp;
			track.lastReceiveTickMs = now;
		}

		if (track.lastReceiveTickMs > maxLastReceiveMs)
			maxLastReceiveMs = track.lastReceiveTickMs;
	}

	// No remote humans — don't freeze (single player, loading, or all AI)
	if (remoteHumanCount == 0)
	{
		if (m_frozen)
		{
			m_frozen = false;
			if (m_hudLineId)
			{
				HudNotifications::GetInstance()->RemoveLine(m_hudLineId);
				m_hudLineId = 0;
			}
		}
		return;
	}

	DWORD silenceMs = now - maxLastReceiveMs;

	if (!m_frozen && silenceMs >= FREEZE_THRESHOLD_MS)
	{
		// All peers went silent — freeze simulation
		m_frozen = true;
		m_frozenSinceMs = now;
		m_lastTickLeakMs = 0;  // allow first tick leak immediately
		m_hudLineId = HudNotifications::GetInstance()->AddLine(
			"lagguard", "Network gap detected - simulation paused");
	}
	else if (m_frozen && silenceMs < FREEZE_THRESHOLD_MS)
	{
		// Peers are back — unfreeze
		DWORD frozenDuration = now - m_frozenSinceMs;
		m_frozen = false;
		m_frozenSinceMs = 0;
		if (m_hudLineId)
		{
			HudNotifications::GetInstance()->RemoveLine(m_hudLineId);
			m_hudLineId = 0;
		}
		if (m_resumeLineId)
			HudNotifications::GetInstance()->RemoveLine(m_resumeLineId);
		char buf[80];
		_snprintf(buf, sizeof(buf), "Network resumed after %lu ms freeze", frozenDuration);
		m_resumeLineId = HudNotifications::GetInstance()->AddLine("lagguard", buf);
		m_resumeLineExpiry = now + RESUME_NOTICE_MS;
	}
}
