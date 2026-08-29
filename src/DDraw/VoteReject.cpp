#include "VoteReject.h"
#include "VoteDialog.h"
#include "ChatHijackIds.h"
#include "PacketChatRouter.h"
#include "HudNotifications.h"
#include "iddrawsurface.h"
#include "tafunctions.h"
#include "tamem.h"
#ifdef TADR_DEBUG_PIPE
#include "DebugPipeServer.h"
#endif

#include <algorithm>
#include <cstring>
#include <windows.h>

VoteReject* VoteReject::m_instance = nullptr;

// -----------------------------------------------------------------------
// Send_PacketPlayerState_1B @ 0x00453010
//   Called with (DirectPlayID, mask) to reject a player.
//   mask=1 for battleroom reject, mask=6 for timeout reject.
// -----------------------------------------------------------------------
typedef void(__stdcall* _Send_PacketPlayerState_1B)(unsigned dpid, int mask);
static _Send_PacketPlayerState_1B Send_PacketPlayerState_1B =
	(_Send_PacketPlayerState_1B)0x00453010;

// RejectPlayer @ 0x00446080
//   Opens the YESORNO.GUI reject dialog for a given player slot, installs
//   RejectYesNo_OnCommand as the callback.  Used to show the yes/no prompt on
//   non-proposing clients when a manual reject vote is proposed.
typedef void(__stdcall* _RejectPlayer)(int playerIndex);
static _RejectPlayer RejectPlayer_fn = (_RejectPlayer)0x00446080;

// GameRunSec @ 0x004b6340 -- game clock in 1/30 s units, the same reference
// CheckForDroppedPlayers uses for its dropout gap test.
typedef int(__cdecl* _GameRunSec)(void);
static _GameRunSec GameRunSec_fn = (_GameRunSec)0x004b6340;

static bool AreAllies(TAdynmemStruct* taPtr, int slotA, int slotB)
{
	if (slotA < 0 || slotB < 0 || slotA >= 10 || slotB >= 10 || slotA == slotB)
		return false;
	return taPtr->Players[slotA].AllyFlagAry[slotB] != 0
		&& taPtr->Players[slotB].AllyFlagAry[slotA] != 0;
}

// CanCastVote: can the player in this slot actually deliver a vote to us?
//
//   Only humans run tdraw, so an AI slot never sends a VoteRejectMessage; and a
//   remote human who has stopped sending packets cannot send one either.  A
//   dropped player stays in TA's Players[] table (PlayerActive, DirectPlayID != 0)
//   until the reject packet lands, so both kinds of non-voter used to be counted
//   as eligible voters.  That inflated votesNeeded AND made the teammate-consent
//   gate unsatisfiable: when an entire team disconnected at once, the surviving
//   team could never reach the threshold and could never obtain consent from an
//   ally of the target, because every ally of the target had dropped too.  Manual
//   votes have no auto-execute path, so those deadlocked outright.
//
//   Gap test matches CheckForDroppedPlayers / MultiDropoutRouter:
//     gameNow - max(LastMsgTimeStamp, GameTimeSec) > NetworkDropoutTimeoutSec * 30
static bool CanCastVote(TAdynmemStruct* taPtr, int slot, int gameNow)
{
	const PlayerStruct& p = taPtr->Players[slot];
	if (!p.PlayerActive || p.DirectPlayID == 0)
		return false;
	if (p.My_PlayerType == Player_LocalHuman)
		return true;                    // ourselves: responsive by definition
	if (p.My_PlayerType != Player_RemoteHuman)
		return false;                   // AI: never sends a VoteReject packet

	int ts = p.LastMsgTimeStamp;
	int gameTimeSec = *(int*)0x00512c7c;
	if (ts < gameTimeSec) ts = gameTimeSec;
	return (gameNow - ts) <= (int)taPtr->NetworkDropoutTimeoutSec * 30;
}

static int FindSlotByDpid(TAdynmemStruct* taPtr, unsigned dpid)
{
	for (int i = 0; i < 10; ++i)
		if (taPtr->Players[i].PlayerActive && taPtr->Players[i].DirectPlayID == dpid)
			return i;
	return -1;
}

static bool IsVoteRejectMessage(const VoteRejectMessage& msg)
{
	return msg.chatByte == 0x05
		&& msg.nullText == 0x00
		&& msg.msgId    == ChatHijackId::VoteReject
		&& msg.size     == sizeof(VoteRejectMessage);
}

void VoteReject::HandleVoteRejectPacket(unsigned fromDpid, const void* buf)
{
	const VoteRejectMessage* msg = (const VoteRejectMessage*)buf;
	if (!msg || !IsVoteRejectMessage(*msg))
		return;
	VoteReject::GetInstance()->OnReceive(fromDpid, *msg);
}

// -----------------------------------------------------------------------

void VoteReject::Install()
{
	if (!m_instance)
		m_instance = new VoteReject();
}

VoteReject* VoteReject::GetInstance()
{
	if (!m_instance)
		m_instance = new VoteReject();
	return m_instance;
}

VoteReject::VoteReject()
{
	// Hook 1: RejectYesNo_OnCommand @ 0x00446044, 6 bytes
	//   mov cl, [Global_PlayerIndexTemp]  -- 6 bytes, position-independent
	//   Fires when host confirms YESORNO reject, both in battleroom and in-game
	//   (Tab → Control → player name → reject prompt).
	//   Target player slot at *((BYTE*)0x00505510)
	//   Epilogue (skip reject): pop edi; pop esi; ret 4 @ 0x00446076
	m_yesNoRejectHook.reset(new InlineSingleHook(
		0x00446044, 6, INLINE_5BYTESLAGGERJMP, YesNoRejectRouter));

	// Hook 2 (NO): @ 0x00446070, 6 bytes
	//   push esi; call SetUIChangeFlag  -- confirmed NO after both CHOICE1 and CHOICE2 checks
	//   Fires when any player clicks NO in a YESORNO reject dialog (in-game only).
	//   Target player slot at *((BYTE*)0x00505510)
	//   Returns 0: SetUIChangeFlag still runs to dismiss the dialog normally.
	m_yesNoNoHook.reset(new InlineSingleHook(
		0x00446070, 6, INLINE_5BYTESLAGGERJMP, YesNoNoRouter));

	// Hook 3: MultiDropoutRouter @ 0x00453CC3, 6 bytes
	//   MOV EDI, [GameTimeSec] — first instruction of second loop in CheckForDroppedPlayers.
	//   Fires only when bMultiDropout=true (multiple players timed out simultaneously).
	//   TA's second loop normally skips ALL ShowRejectWindow calls in this case.
	//   Our router calls ProposeReject for each timed-out player and jumps to the epilogue.
	//   bMultiDropout is at [ESP+0x10] in CheckForDroppedPlayers' frame; EAX = GameRunSec().
	m_multiDropoutHook.reset(new InlineSingleHook(
		0x00453CC3, 6, INLINE_5BYTESLAGGERJMP, MultiDropoutRouter));

	// Hook 4: ShowRejectWindow @ 0x00453B0A, 5 bytes
	//   push 800h -- 5 bytes, just before LoadGUIFile call that creates TIMEOUT.GUI.
	//   EDI holds the timed-out player's dpid at this point.
	//   Calls ProposeReject(dpid, 6) and redirects to ShowRejectWindow epilogue
	//   (pop edi/esi/ebp/ebx; ret 4 @ 0x00453C10) so the modal dialog is never opened.
	//   Replay: skips to epilogue without proposing.
	m_showRejectWindowHook.reset(new InlineSingleHook(
		0x00453B0A, 5, INLINE_5BYTESLAGGERJMP, ShowRejectWindowRouter));


	PacketChatRouter::GetInstance()->RegisterHandler(ChatHijackId::VoteReject, HandleVoteRejectPacket);
}

// -----------------------------------------------------------------------
// Hook 1: RejectYesNo_OnCommand — player clicks YES in the YESORNO reject dialog.
//   Fires in the battleroom AND in-game (Tab → Control → player → reject prompt,
//   or the prompt opened on other clients when a manual vote is proposed).
//   *((BYTE*)0x00505510) = player slot index being rejected.
//
//   Battleroom/loading (TAProgress != TAInGame): let reject through immediately (return 0).
//   In-game (TAProgress == TAInGame):
//     - No vote in progress for target → propose a new vote.
//     - Vote already in progress     → cast our vote.
//   Epilogue (skip reject): pop edi; pop esi; ret 4 @ 0x00446076
// -----------------------------------------------------------------------
int __stdcall VoteReject::YesNoRejectRouter(PInlineX86StackBuffer pBuf)
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;

	// Battleroom/loading or replay: allow TA to handle reject directly.
	if (DataShare->TAProgress != TAInGame || DataShare->PlayingDemo)
		return 0;

	BYTE playerIndex = *((BYTE*)0x00505510);
	if (playerIndex < 10
		&& taPtr->Players[playerIndex].PlayerActive
		&& taPtr->Players[playerIndex].DirectPlayID != 0)
	{
		unsigned targetDpid = taPtr->Players[playerIndex].DirectPlayID;
		auto* vr = VoteReject::GetInstance();
		auto it = vr->m_votes.find(targetDpid);

		if (it != vr->m_votes.end())
		{
			// Vote already in progress — this is a cast vote (we were shown the dialog
			// by OnReceive when a peer proposed the reject).
			unsigned myDpid = taPtr->Players[taPtr->LocalHumanPlayer_PlayerID].DirectPlayID;
			if (std::find(it->second.voters.begin(), it->second.voters.end(), myDpid)
				== it->second.voters.end())
			{
				// YES cancels any prior NO from the same player
				auto& noVoters = it->second.noVoters;
				noVoters.erase(std::remove(noVoters.begin(), noVoters.end(), myDpid), noVoters.end());
				it->second.voters.push_back(myDpid);
				vr->BroadcastMsg(VoteRejectCommand::CastVote, targetDpid, it->second.rejectMask);
				vr->CheckAndExecuteReject(targetDpid);
				vr->RefreshVoteLine(targetDpid);
			}
		}
		else
		{
			vr->ProposeReject(targetDpid, 1);
		}

		pBuf->rtnAddr_Pvoid = (LPVOID)0x00446076;
		return X86STRACKBUFFERCHANGE;
	}
	return 0;
}

// -----------------------------------------------------------------------
// Hook 2 (NO): Player clicks NO in the YESORNO reject dialog.
//   Fires at 0x00446070, after both CHOICE1 (YES) and CHOICE2 (NO) comparisons
//   have confirmed it is a genuine NO click.
//   *((BYTE*)0x00505510) = player slot being rejected.
//   Returns 0: SetUIChangeFlag still runs to dismiss the dialog normally.
// -----------------------------------------------------------------------
int __stdcall VoteReject::YesNoNoRouter(PInlineX86StackBuffer pBuf)
{
	if (DataShare->TAProgress != TAInGame || DataShare->PlayingDemo)
		return 0;

	BYTE playerIndex = *((BYTE*)0x00505510);
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	if (playerIndex >= 10 || !taPtr->Players[playerIndex].PlayerActive)
		return 0;

	unsigned targetDpid = taPtr->Players[playerIndex].DirectPlayID;
	if (targetDpid == 0)
		return 0;

	auto* vr = VoteReject::GetInstance();
	auto it = vr->m_votes.find(targetDpid);
	if (it == vr->m_votes.end())
		return 0;

	unsigned myDpid = taPtr->Players[taPtr->LocalHumanPlayer_PlayerID].DirectPlayID;
	auto& noVoters = it->second.noVoters;
	if (std::find(noVoters.begin(), noVoters.end(), myDpid) != noVoters.end())
		return 0;  // already recorded

	// NO cancels any prior YES from the same player
	auto& yesVoters = it->second.voters;
	yesVoters.erase(std::remove(yesVoters.begin(), yesVoters.end(), myDpid), yesVoters.end());
	noVoters.push_back(myDpid);
	vr->BroadcastMsg(VoteRejectCommand::CastNoVote, targetDpid, it->second.rejectMask);
	vr->CheckAndExecuteReject(targetDpid);
	vr->RefreshVoteLine(targetDpid);
	return 0;
}

// -----------------------------------------------------------------------
// Hook 3: ShowRejectWindow @ 0x00453B0A — suppress the native modal timeout
//   dialog and replace it with a VoteReject proposal + VoteDialog overlay.
//
//   Hook site: 5 bytes ("push 800h"), inside ShowRejectWindow, immediately
//   before the LoadGUIFile call that would create TIMEOUT.GUI.
//   At this point EDI holds arg_0 = the timed-out player's DirectPlayID.
//
//   We redirect to the ShowRejectWindow epilogue (0x00453C10:
//   pop edi; pop esi; pop ebp; pop ebx; ret 4) so the function returns
//   without ever opening the dialog.  The 4 register saves pushed by the
//   prologue are still on the stack, so the epilogue unwinds correctly.
//
//   Replay: skip to epilogue without proposing (replay observers don't vote).
// -----------------------------------------------------------------------
int __stdcall VoteReject::ShowRejectWindowRouter(PInlineX86StackBuffer pBuf)
{
	if (DataShare->TAProgress != TAInGame)
	{
		return 0;
	}

	// Replay: let TA open its native TIMEOUT.GUI unmodified (return 0). Its ticker,
	// TimeoutDialog_Ticker @ 0x00453640, auto-calls Send_PacketPlayerState_1B(dpid,6)
	// once the countdown elapses — with NO user click — and that is the ONLY thing
	// that removes a dropped player during playback (a replay has no live voters).
	// Redirecting to the epilogue here suppressed ShowRejectWindow, so the ticker was
	// never installed, the dropped player was never rejected, his units froze and the
	// replay eventually lagged out (game 182125). Matches pre-VoteReject dll behaviour.
	if (DataShare->PlayingDemo)
		return 0;

	unsigned targetDpid = (unsigned)pBuf->Edi;
	if (targetDpid != 0 && targetDpid != 0xFFFFFFFF)
		VoteReject::GetInstance()->ProposeReject(targetDpid, 6);

	// In-game: skip to ShowRejectWindow epilogue — we never want the modal dialog.
	pBuf->rtnAddr_Pvoid = (LPVOID)0x00453C0C;
	return X86STRACKBUFFERCHANGE;
}


// -----------------------------------------------------------------------
// Hook 3: MultiDropoutRouter — fires at 0x00453CC3 (start of CheckForDroppedPlayers'
//   second loop) when bMultiDropout=true (multiple players timed out simultaneously).
//
//   TA's second loop skips ShowRejectWindow for all players when bMultiDropout=true,
//   so our ShowRejectWindowHook never fires for this case.  We replicate the same
//   gap check (gameNow - max(LastMsgTimeStamp, GameTimeSec) > NetworkDropoutTimeoutSec * 30) and
//   call ProposeReject for each qualifying player, then redirect to the function
//   epilogue (0x00453D16) to skip the second loop entirely.
//
//   EAX at the hook site = GameRunSec() computed at the non-paused entry (0x00453C52).
//   bMultiDropout = *(int*)(pBuf->Esp + 0x10)  (reuses the saved-ECX stack slot).
// -----------------------------------------------------------------------
int __stdcall VoteReject::MultiDropoutRouter(PInlineX86StackBuffer pBuf)
{
	// If bMultiDropout == 0, this is a single dropout; let ShowRejectWindowHook handle it.
	if (*(int*)(pBuf->Esp + 0x10) == 0)
		return 0;

	// Replay: don't intercept — run TA's native second loop (return 0). For simultaneous
	// dropouts TA natively auto-rejects nobody (the second loop only calls ShowRejectWindow
	// for a single dropout; multi-dropout falls through to ShowRejectWindow(0xFFFFFFFF), a
	// no-op), so this matches pre-VoteReject dll behaviour. Redirecting to the epilogue was
	// equivalent for the multi case, but returning 0 keeps the replay contract uniform with
	// ShowRejectWindowRouter: in a replay the hooks defer entirely to TA.
	if (DataShare->PlayingDemo)
		return 0;

	if (DataShare->TAProgress == TAInGame)
	{
		int gameNow     = (int)pBuf->Eax;          // GameRunSec() from 0x00453C52
		int gameTimeSec = *(int*)0x00512c7c;        // GameTimeSec (min reference)
		TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
		if (taPtr)
		{
			int threshold = (int)taPtr->NetworkDropoutTimeoutSec * 30;
			for (int i = 0; i < 10; ++i)
			{
				PlayerStruct& p = taPtr->Players[i];
				if (!p.PlayerActive || p.DirectPlayID == 0)
					continue;
				if (p.My_PlayerType != Player_RemoteHuman)
					continue;
				int ts = p.LastMsgTimeStamp;
				if (ts < gameTimeSec) ts = gameTimeSec;  // max(LastMsgTimeStamp, GameTimeSec)
				if (gameNow - ts > threshold)
					GetInstance()->ProposeReject(p.DirectPlayID, 6);
			}
		}
	}

	// Skip the second loop; jump to CheckForDroppedPlayers epilogue.
	pBuf->rtnAddr_Pvoid = (LPVOID)0x00453D16;
	return X86STRACKBUFFERCHANGE;
}

// -----------------------------------------------------------------------
// OnReceive: dispatched from PacketChatRouter for incoming VoteReject packets.
// -----------------------------------------------------------------------
void VoteReject::OnReceive(unsigned fromDpid, const VoteRejectMessage& msg)
{
	if (msg.command == VoteRejectCommand::ProposeVote)
	{
		if (m_votes.find(msg.targetDpid) != m_votes.end())
			return;  // duplicate proposal, ignore

		DWORD now = GetTickCount();
		auto cdIt = m_cooldownExpiry.find(msg.targetDpid);
		if (cdIt != m_cooldownExpiry.end() && now < cdIt->second)
			return;  // cooldown active — ignore remote proposal too

		// Never open a *timeout* reject vote that targets the LOCAL player.
		// The auto-cancel in Tick() watches Players[targetSlot].LastMsgTimeStamp,
		// which TA only advances for packets received FROM that slot over the
		// network (Packet_Dispatcher @ 0x00453d40). A node never receives its own
		// packets, so its own slot's timestamp never advances — a self-targeted
		// timeout vote can therefore NEVER be cancelled and always self-executes
		// on expiry, ejecting this player from the game even after their network
		// recovered. If that player is the host, their departure collapses the
		// whole DirectPlay session. You know you are alive, so ignore the proposal
		// (peers still run their own copy and will drop you only if you stay gone).
		// Manual (non-timeout) votes are threshold-based and legitimately may
		// target you, so this guard is limited to rejectMask == 6.
		// Incident: game 175237 — Magpie (lagger) and Chao_Storm (host) both
		// self-ejected, collapsing a 3v3.
		{
			TAdynmemStruct* taSelf = *(TAdynmemStruct**)0x00511de8;
			unsigned myDpidSelf = taSelf->Players[taSelf->LocalHumanPlayer_PlayerID].DirectPlayID;
			if (msg.rejectMask == 6 && msg.targetDpid == myDpidSelf)
			{
				IDDrawSurface::OutptFmtTxt(
					"[VoteReject] others are voting to reject you (you timed out to them); "
					"ignoring self-targeted timeout vote dpid=%u", msg.targetDpid);
				return;
			}
		}

		VoteState state;
		state.rejectMask   = msg.rejectMask;
		state.proposerName = GetPlayerName(fromDpid);
		state.targetName   = GetPlayerName(msg.targetDpid);
		state.targetSlot   = -1;
		state.expiryTime   = now + (msg.rejectMask == 6 ? VOTE_TIMEOUT_MS : MANUAL_VOTE_TIMEOUT_MS);
		state.lastMsgTimeStampAtProposal = 0;
		state.hudLineId    = INVALID_HUD_LINE_ID;
		state.votingClosed = false;

		TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
		for (int i = 0; i < 10; ++i) {
			if (taPtr->Players[i].DirectPlayID == msg.targetDpid) {
				state.targetSlot = i;
				state.lastMsgTimeStampAtProposal = taPtr->Players[i].LastMsgTimeStamp;
				break;
			}
		}

		// Manual vote: proposer's YES is implicit in the proposal.
		// Timeout vote: no implicit votes — all participants must vote explicitly.
		if (msg.rejectMask != 6)
			state.voters.push_back(fromDpid);
		m_votes[msg.targetDpid] = std::move(state);
		m_votes[msg.targetDpid].hudLineId = HudNotifications::GetInstance()->AddLine(
			"vote", FormatVoteLine(msg.targetDpid, m_votes[msg.targetDpid]));

		IDDrawSurface::OutptFmtTxt("[VoteReject] ProposeVote from dpid=%u to reject dpid=%u",
			fromDpid, msg.targetDpid);

		// Inform the rejectee via console; all other clients see VoteDialog.
		TAdynmemStruct* taPtr2 = *(TAdynmemStruct**)0x00511de8;
		unsigned myDpid2 = taPtr2->Players[taPtr2->LocalHumanPlayer_PlayerID].DirectPlayID;
		if (msg.targetDpid == myDpid2)
		{
			const char* reason = (msg.rejectMask == 6)
				? "[VoteReject] you have timed out — others are voting to reject you"
				: "[VoteReject] a vote is in progress to reject you";
			IDDrawSurface::OutptFmtTxt("%s", reason);
		}
		if (g_VoteDialog) g_VoteDialog->Refresh();

		// Queue, never execute here: OnReceive runs inside TA's chat dispatch.
		m_pendingThresholdChecks.push_back(msg.targetDpid);
	}
	else if (msg.command == VoteRejectCommand::CastVote)
	{
		auto it = m_votes.find(msg.targetDpid);
		if (it == m_votes.end())
			return;

		auto& voters = it->second.voters;
		if (std::find(voters.begin(), voters.end(), fromDpid) != voters.end())
			return;  // duplicate vote

		// YES cancels any prior NO from the same player
		auto& noVoters = it->second.noVoters;
		noVoters.erase(std::remove(noVoters.begin(), noVoters.end(), fromDpid), noVoters.end());
		voters.push_back(fromDpid);
		IDDrawSurface::OutptFmtTxt("[VoteReject] CastVote from dpid=%u for reject of dpid=%u (%d yes)",
			fromDpid, msg.targetDpid, (int)voters.size());

		m_pendingThresholdChecks.push_back(msg.targetDpid);
		RefreshVoteLine(msg.targetDpid);
	}
	else if (msg.command == VoteRejectCommand::CastNoVote)
	{
		auto it = m_votes.find(msg.targetDpid);
		if (it == m_votes.end())
			return;

		auto& noVoters = it->second.noVoters;
		if (std::find(noVoters.begin(), noVoters.end(), fromDpid) != noVoters.end())
			return;  // duplicate no vote

		// NO cancels any prior YES from the same player
		auto& voters2 = it->second.voters;
		voters2.erase(std::remove(voters2.begin(), voters2.end(), fromDpid), voters2.end());
		noVoters.push_back(fromDpid);
		IDDrawSurface::OutptFmtTxt("[VoteReject] CastNoVote from dpid=%u for reject of dpid=%u (%d no)",
			fromDpid, msg.targetDpid, (int)noVoters.size());

		m_pendingThresholdChecks.push_back(msg.targetDpid);
		RefreshVoteLine(msg.targetDpid);
	}
}

// -----------------------------------------------------------------------
// ProposeReject: called by the hook routers on the local machine.
//   Registers own vote, broadcasts ProposeVote, checks threshold.
// -----------------------------------------------------------------------
void VoteReject::ProposeReject(unsigned targetDpid, char rejectMask)
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	unsigned myDpid = taPtr->Players[taPtr->LocalHumanPlayer_PlayerID].DirectPlayID;

	if (m_votes.find(targetDpid) != m_votes.end())
		return;  // vote already in progress for this player

	DWORD now = GetTickCount();
	auto cdIt = m_cooldownExpiry.find(targetDpid);
	if (cdIt != m_cooldownExpiry.end() && now < cdIt->second) {
		IDDrawSurface::OutptFmtTxt("[VoteReject] reject of dpid=%u on cooldown (%us remaining)",
			targetDpid, (cdIt->second - now) / 1000);
		return;
	}

	VoteState state;
	state.rejectMask   = rejectMask;
	state.proposerName = GetPlayerName(myDpid);
	state.targetName   = GetPlayerName(targetDpid);
	state.targetSlot   = -1;
	state.expiryTime   = now + (rejectMask == 6 ? VOTE_TIMEOUT_MS : MANUAL_VOTE_TIMEOUT_MS);
	state.lastMsgTimeStampAtProposal = 0;
	state.hudLineId    = INVALID_HUD_LINE_ID;
	state.votingClosed = false;
	for (int i = 0; i < 10; ++i) {
		if (taPtr->Players[i].DirectPlayID == targetDpid) {
			state.targetSlot = i;
			state.lastMsgTimeStampAtProposal = taPtr->Players[i].LastMsgTimeStamp;
			break;
		}
	}
	if (rejectMask != 6)
		state.voters.push_back(myDpid);  // manual vote: proposer auto-votes YES
	m_votes[targetDpid] = std::move(state);
	m_votes[targetDpid].hudLineId = HudNotifications::GetInstance()->AddLine(
		"vote", FormatVoteLine(targetDpid, m_votes[targetDpid]));

	IDDrawSurface::OutptFmtTxt("[VoteReject] ProposeReject: dpid=%u mask=%d", targetDpid, (int)rejectMask);
	BroadcastMsg(VoteRejectCommand::ProposeVote, targetDpid, rejectMask);

	if (g_VoteDialog) g_VoteDialog->Refresh();

	// May already pass threshold (e.g. 2-player game: 1 vote needed)
	CheckAndExecuteReject(targetDpid);
}

// -----------------------------------------------------------------------
// BroadcastMsg: send a VoteRejectMessage to all players.
// -----------------------------------------------------------------------
void VoteReject::BroadcastMsg(VoteRejectCommand command, unsigned targetDpid, char rejectMask)
{
	if (m_suppressBroadcast)
		return;

	VoteRejectMessage msg;
	std::memset(&msg, 0, sizeof(msg));
	msg.chatByte   = 0x05;
	msg.nullText   = 0x00;
	msg.msgId      = ChatHijackId::VoteReject;
	msg.size       = sizeof(VoteRejectMessage);
	msg.command    = command;
	msg.targetDpid = targetDpid;
	msg.rejectMask = rejectMask;

	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	unsigned fromDpid = taPtr->Players[taPtr->LocalHumanPlayer_PlayerID].DirectPlayID;
	HAPI_BroadcastMessage(fromDpid, (const char*)&msg, sizeof(msg));
}

// -----------------------------------------------------------------------
// ComputeTally: single source of truth for the vote arithmetic, shared by the
//   threshold check, the HUD line and the VoteDialog rows so they can never
//   disagree with each other.
//
//   Eligible voters = active players who CanCastVote(), excluding the target.
//   Players who have dropped (or AI slots) are excluded: they cannot vote, so
//   counting them only makes the ballot harder or impossible to carry.
//
//   Timeout reject: proposer + 1 seconder (1 in a 2-player game).
//   Manual reject : ceiling(2/3 * eligibleVoters), minimum 1.
//   Both additionally require teammate consent when the target still has an
//   ally who is able to vote.
// -----------------------------------------------------------------------
VoteReject::VoteTally VoteReject::ComputeTally(unsigned targetDpid, const VoteState& state) const
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	int gameNow = GameRunSec_fn();

	VoteTally t;
	t.yesVotes = (int)state.voters.size();
	t.noVotes  = (int)state.noVoters.size();

	t.eligibleVoters = 0;
	for (int i = 0; i < 10; ++i) {
		if (taPtr->Players[i].DirectPlayID == targetDpid) continue;
		if (!CanCastVote(taPtr, i, gameNow)) continue;
		++t.eligibleVoters;
	}

	if (state.rejectMask == 6)
		t.votesNeeded = (t.eligibleVoters <= 1) ? 1 : 2;
	else
		t.votesNeeded = (t.eligibleVoters <= 1) ? 1 : ((t.eligibleVoters * 2 + 2) / 3);

	// Teammate consent: at least one ally of the target that is *able to vote*
	// must have voted YES.  Allies who have dropped out themselves are skipped —
	// they can never consent, and requiring them deadlocks the ballot.
	t.teammateConsent   = true;
	t.needsTeammateVote = false;
	if (state.targetSlot >= 0) {
		for (int i = 0; i < 10; ++i) {
			if (!CanCastVote(taPtr, i, gameNow)) continue;
			if (!AreAllies(taPtr, state.targetSlot, i)) continue;
			unsigned allyDpid = taPtr->Players[i].DirectPlayID;
			if (std::find(state.voters.begin(), state.voters.end(), allyDpid) != state.voters.end()) {
				t.teammateConsent   = true;
				t.needsTeammateVote = false;
				break;
			}
			t.teammateConsent   = false;
			t.needsTeammateVote = true;
		}
	}
	return t;
}

// -----------------------------------------------------------------------
// CheckAndExecuteReject: called after each new vote is recorded.
//   When threshold met, each client independently calls Send_PacketPlayerState_1B.
// -----------------------------------------------------------------------
void VoteReject::CheckAndExecuteReject(unsigned targetDpid)
{
	auto it = m_votes.find(targetDpid);
	if (it == m_votes.end())
		return;

	bool isTimeoutReject = (it->second.rejectMask == 6);
	VoteTally tally = ComputeTally(targetDpid, it->second);

	int voteCount   = tally.yesVotes;
	int noVoteCount = tally.noVotes;
	int votesNeeded = tally.votesNeeded;

	IDDrawSurface::OutptFmtTxt("[VoteReject] threshold check: %d yes, %d no / %d eligible (need %d), teammate consent: %d",
		voteCount, noVoteCount, tally.eligibleVoters, votesNeeded, (int)tally.teammateConsent);

	if (voteCount >= votesNeeded && tally.teammateConsent)
	{
		std::string targetName = it->second.targetName;
		HudLineId voteHudId = it->second.hudLineId;
		ExecuteReject(targetDpid, it->second.rejectMask, targetName);
		HudNotifications::GetInstance()->RemoveLine(voteHudId);
		m_votes.erase(it);
		if (g_VoteDialog) g_VoteDialog->Refresh();
	}
	else if (noVoteCount > tally.eligibleVoters - votesNeeded)
	{
		// Enough NO votes that the YES threshold can never be reached.
		IDDrawSurface::OutptFmtTxt("[VoteReject] vote for dpid=%u failed by NO majority (%d no, need %d yes from %d)",
			targetDpid, noVoteCount, votesNeeded, tally.eligibleVoters);
		if (!isTimeoutReject)
		{
			// Manual vote: cancel immediately, show transient failure notice.
			std::string targetName = it->second.targetName;
			HudNotifications::GetInstance()->RemoveLine(it->second.hudLineId);
			m_cooldownExpiry[targetDpid] = GetTickCount() + VOTE_COOLDOWN_MS;
			m_votes.erase(it);
			AddTransientNotice("Vote to reject " + targetName + " failed", NOTICE_DURATION_MS);
			if (g_VoteDialog) g_VoteDialog->Refresh();
		}
		else
		{
			// Timeout vote: voting is over but the player is still gone — auto-reject
			// will fire when the timer expires.  Keep the vote entry alive and update
			// the HUD line, but remove the dialog row (votingClosed entries are excluded
			// from GetActiveVotes so VoteDialog closes or shows remaining open votes).
			it->second.votingClosed = true;
			HudNotifications::GetInstance()->UpdateLine(
				it->second.hudLineId, FormatVoteLine(targetDpid, it->second));
			if (g_VoteDialog) g_VoteDialog->Refresh();
		}
	}
}

// -----------------------------------------------------------------------
// ExecuteReject: send the actual reject packet.
//   For timeout rejects (mask=6), record completion so allies can .take.
// -----------------------------------------------------------------------
void VoteReject::ExecuteReject(unsigned targetDpid, char rejectMask, const std::string& targetName)
{
	IDDrawSurface::OutptFmtTxt("[VoteReject] vote passed -- rejecting dpid=%u mask=%d",
		targetDpid, (int)rejectMask);
	if (!m_suppressBroadcast)
		Send_PacketPlayerState_1B(targetDpid, (int)(unsigned char)rejectMask);

	if (rejectMask == 6)
	{
		// Use the stored targetName (player may already be gone from the game tables).
		CompletedTimeoutReject ctr;
		ctr.targetName = targetName;
		ctr.expiryTime = GetTickCount() + VOTE_COOLDOWN_MS;
		ctr.hudLineId  = HudNotifications::GetInstance()->AddLine(
			"vote", FormatTakeLine(ctr));
		m_completedTimeoutRejects[targetDpid] = std::move(ctr);
		IDDrawSurface::OutptFmtTxt(
			"[VoteReject] timeout reject complete for dpid=%u",
			targetDpid);
	}
}

// -----------------------------------------------------------------------
// GetPlayerName: look up player display name from DirectPlayID.
// -----------------------------------------------------------------------
std::string VoteReject::GetPlayerName(unsigned dpid)
{
	PlayerStruct* p = FindPlayerByDPID(dpid);
	if (p && p->Name[0])
		return p->Name;
	char buf[32];
	wsprintfA(buf, "dpid:%u", dpid);
	return buf;
}

// -----------------------------------------------------------------------
// HasActiveTimeoutVote: returns true if any timeout vote (mask=6) is open.
// -----------------------------------------------------------------------
bool VoteReject::HasActiveTimeoutVote() const
{
	for (auto& kv : m_votes) {
		if (kv.second.rejectMask == 6)
			return true;
	}
	return false;
}

// -----------------------------------------------------------------------
// Tick: expire timed-out votes, purge cooldowns, expire .take windows,
//   and refresh countdown text on the HUD every second.
//   Called every frame from the render loop.
// -----------------------------------------------------------------------
void VoteReject::Tick()
{
#ifdef TADR_DEBUG_PIPE
	DebugPipeServer::DrainQueue();
#endif

	DWORD now = GetTickCount();

	// Expire transient failure notices
	for (auto it = m_transientNotices.begin(); it != m_transientNotices.end(); ) {
		if (now >= it->expiryTime) {
			HudNotifications::GetInstance()->RemoveLine(it->hudLineId);
			it = m_transientNotices.erase(it);
		} else {
			++it;
		}
	}

	// Purge stale cooldown entries
	for (auto it = m_cooldownExpiry.begin(); it != m_cooldownExpiry.end(); ) {
		if (now >= it->second)
			it = m_cooldownExpiry.erase(it);
		else
			++it;
	}

	// Expire .take windows
	for (auto it = m_completedTimeoutRejects.begin(); it != m_completedTimeoutRejects.end(); ) {
		if (now >= it->second.expiryTime) {
			HudNotifications::GetInstance()->RemoveLine(it->second.hudLineId);
			it = m_completedTimeoutRejects.erase(it);
		} else {
			++it;
		}
	}

	// Threshold checks deferred out of the packet handler (see the header).
	if (!m_pendingThresholdChecks.empty())
	{
		std::vector<unsigned> due;
		due.swap(m_pendingThresholdChecks);
		for (size_t i = 0; i < due.size(); ++i)
			CheckAndExecuteReject(due[i]);
	}

	// Expire timed-out votes
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	for (auto it = m_votes.begin(); it != m_votes.end(); ) {
		// For timeout votes, check whether the player's network traffic has resumed.
		// Packet_Dispatcher writes LastMsgTimeStamp on every received packet.
		if (it->second.rejectMask == 6 && it->second.targetSlot >= 0) {
			// Defense in depth (see OnReceive): a timeout vote whose target is the
			// local player can never be cancelled by the LastMsgTimeStamp heuristic
			// (own slot never advances) and would always self-execute. We are alive
			// by definition — cancel it unconditionally rather than reject ourselves.
			if (it->second.targetSlot == taPtr->LocalHumanPlayer_PlayerID) {
				IDDrawSurface::OutptFmtTxt(
					"[VoteReject] ignoring self-targeted timeout vote for dpid=%u (cannot reject self)",
					it->first);
				HudNotifications::GetInstance()->RemoveLine(it->second.hudLineId);
				it = m_votes.erase(it);
				if (g_VoteDialog) g_VoteDialog->Refresh();
				continue;
			}
			// A take in flight injects gives carrying the TARGET's dpid, so the
			// target looks alive again. That traffic is ours, not theirs.
			int currentTs = taPtr->Players[it->second.targetSlot].LastMsgTimeStamp;
			if (!it->second.takeInProgress && currentTs != it->second.lastMsgTimeStampAtProposal) {
				IDDrawSurface::OutptFmtTxt("[VoteReject] network resumed for dpid=%u, cancelling timeout vote",
					it->first);
				HudNotifications::GetInstance()->RemoveLine(it->second.hudLineId);
				it = m_votes.erase(it);
				if (g_VoteDialog) g_VoteDialog->Refresh();
				continue;
			}
		}

		if (now >= it->second.expiryTime) {
			HudLineId voteHudId = it->second.hudLineId;
			std::string targetName = it->second.targetName;
			if (it->second.rejectMask == 6) {
				// Network timeout: player has been gone the full 90s — reject regardless of votes.
				IDDrawSurface::OutptFmtTxt("[VoteReject] timeout vote for dpid=%u expired, executing reject",
					it->first);
				ExecuteReject(it->first, it->second.rejectMask, targetName);
			} else {
				// Manual reject vote: just expire with cooldown, don't force the reject.
				IDDrawSurface::OutptFmtTxt("[VoteReject] vote for dpid=%u timed out, cooldown %ds",
					it->first, VOTE_COOLDOWN_MS / 1000);
				std::string targetName = it->second.targetName;
				m_cooldownExpiry[it->first] = now + VOTE_COOLDOWN_MS;
				AddTransientNotice("Vote to reject " + targetName + " timed out", NOTICE_DURATION_MS);
			}
			HudNotifications::GetInstance()->RemoveLine(voteHudId);
			it = m_votes.erase(it);
			if (g_VoteDialog) g_VoteDialog->Refresh();
		} else {
			++it;
		}
	}

	DWORD nowSec = now / 1000;
	if (nowSec != m_lastHudUpdateSec)
	{
		m_lastHudUpdateSec = nowSec;

		// Re-evaluate every open vote against the CURRENT player table.
		//
		// ComputeTally always reads live state -- nothing is snapshotted when the
		// vote is proposed -- but CheckAndExecuteReject only runs when a vote
		// packet arrives.  Voters can drop out after casting, and in particular
		// the target's last remaining live ally can drop after everyone else has
		// already voted: that lowers votesNeeded and makes teammate consent
		// vacuous, turning an already-cast ballot into a passing one with no
		// further packet to trigger the check.  Without this pass such a vote
		// would sit open until it expired -- and a manual vote expires as a
		// FAILURE, which is exactly the whole-team-disconnect deadlock.
		//
		// Collect the keys first: CheckAndExecuteReject may erase from m_votes.
		std::vector<unsigned> openVotes;
		openVotes.reserve(m_votes.size());
		for (const auto& kv : m_votes)
			openVotes.push_back(kv.first);
		for (unsigned targetDpid : openVotes)
			CheckAndExecuteReject(targetDpid);

		// Refresh countdown text every second so the HUD timer stays live.
		for (auto& kv : m_votes)
			HudNotifications::GetInstance()->UpdateLine(
				kv.second.hudLineId, FormatVoteLine(kv.first, kv.second));
		for (auto& kv : m_completedTimeoutRejects)
			HudNotifications::GetInstance()->UpdateLine(
				kv.second.hudLineId, FormatTakeLine(kv.second));
	}

}

// -----------------------------------------------------------------------
// CancelTimeoutVote: cancel an active timeout vote without executing the reject.
//   Network resumption is detected automatically via LastMsgTimeStamp in Tick().
// -----------------------------------------------------------------------
// True while the completed-timeout-reject window for targetDpid is still open,
// i.e. TA is about to destroy that player's units and an ally may claim them.
bool VoteReject::IsTakeWindowOpen(unsigned targetDpid) const
{
	return m_completedTimeoutRejects.find(targetDpid) != m_completedTimeoutRejects.end();
}

void VoteReject::NoteTakeInProgress(unsigned targetDpid, const std::string& takerName)
{
	auto it = m_votes.find(targetDpid);
	if (it == m_votes.end())
		return;
	if (it->second.takeInProgress)
		return;
	it->second.takeInProgress = true;
	it->second.takerName      = takerName;
	IDDrawSurface::OutptFmtTxt("[VoteReject] take in progress for dpid=%u by %s; vote held open",
		targetDpid, takerName.c_str());
	RefreshVoteLine(targetDpid);
	if (g_VoteDialog) g_VoteDialog->Refresh();
}

void VoteReject::ExecuteRejectAfterTake(unsigned targetDpid)
{
	// The vote may already be gone; the reject still has to happen.
	std::string name;
	auto it = m_votes.find(targetDpid);
	if (it != m_votes.end())
	{
		name = it->second.targetName;
		HudNotifications::GetInstance()->RemoveLine(it->second.hudLineId);
		m_votes.erase(it);
	}
	if (name.empty())
		name = GetPlayerName(targetDpid);

	IDDrawSurface::OutptFmtTxt("[VoteReject] take complete for dpid=%u; executing reject", targetDpid);
	ExecuteReject(targetDpid, 6, name);
	if (g_VoteDialog) g_VoteDialog->Refresh();
}

void VoteReject::CancelTimeoutVote(unsigned targetDpid)
{
	auto it = m_votes.find(targetDpid);
	if (it == m_votes.end() || it->second.rejectMask != 6)
		return;

	IDDrawSurface::OutptFmtTxt("[VoteReject] timeout vote for dpid=%u cancelled (network resumed)",
		targetDpid);
	HudNotifications::GetInstance()->RemoveLine(it->second.hudLineId);
	m_votes.erase(it);
	if (g_VoteDialog) g_VoteDialog->Refresh();
}

// -----------------------------------------------------------------------
// FormatVoteLine: build the HUD text for a single active vote.
// -----------------------------------------------------------------------
std::string VoteReject::FormatVoteLine(unsigned targetDpid, const VoteState& state) const
{
	// Nothing left to vote on; just report until the taker's Complete arrives.
	if (state.takeInProgress)
	{
		char line[256];
		wsprintfA(line, "%s is taking %s's base...",
			state.takerName.empty() ? "An ally" : state.takerName.c_str(),
			state.targetName.c_str());
		return line;
	}

	// Timeout vote where the NO majority closed voting — show countdown to auto-reject.
	if (state.votingClosed)
	{
		DWORD now = GetTickCount();
		int secsLeft = (now < state.expiryTime) ? (int)((state.expiryTime - now) / 1000) : 0;
		char line[256];
		wsprintfA(line, "Vote rejected -- %s auto-rejects in %ds", state.targetName.c_str(), secsLeft);
		return line;
	}

	bool isTimeoutReject = (state.rejectMask == 6);
	VoteTally tally = ComputeTally(targetDpid, state);

	int voteCount   = tally.yesVotes;
	int noVoteCount = isTimeoutReject ? 0 : tally.noVotes;
	int votesNeeded = tally.votesNeeded;
	bool needsTeammateVote = tally.needsTeammateVote;

	DWORD now = GetTickCount();
	int secsLeft = (now < state.expiryTime) ? (int)((state.expiryTime - now) / 1000) : 0;

	char line[256];
	if (isTimeoutReject && needsTeammateVote) {
		wsprintfA(line, "Timeout: reject %s (%d yes/%d, need ally vote, %ds)",
			state.targetName.c_str(), voteCount, votesNeeded, secsLeft);
	}
	else if (isTimeoutReject) {
		wsprintfA(line, "Timeout: reject %s (%d yes/%d, %ds)",
			state.targetName.c_str(), voteCount, votesNeeded, secsLeft);
	}
	else if (needsTeammateVote) {
		wsprintfA(line, "%s: reject %s (%d yes/%d no/%d, need teammate vote, %ds)",
			state.proposerName.c_str(), state.targetName.c_str(),
			voteCount, noVoteCount, votesNeeded, secsLeft);
	}
	else {
		wsprintfA(line, "%s: reject %s (%d yes/%d no/%d, %ds)",
			state.proposerName.c_str(), state.targetName.c_str(),
			voteCount, noVoteCount, votesNeeded, secsLeft);
	}
	return line;
}

// -----------------------------------------------------------------------
// FormatTakeLine: build the HUD text for a completed timeout-reject take window.
// -----------------------------------------------------------------------
std::string VoteReject::FormatTakeLine(const CompletedTimeoutReject& ctr) const
{
	DWORD now = GetTickCount();
	char line[256];
	wsprintfA(line, "%s rejected",
		ctr.targetName.c_str());
	return line;
}

// -----------------------------------------------------------------------
// AddTransientNotice: add a self-expiring HUD line in the "notice" group.
// -----------------------------------------------------------------------
void VoteReject::AddTransientNotice(const std::string& text, DWORD durationMs)
{
	TransientNotice n;
	n.hudLineId  = HudNotifications::GetInstance()->AddLine("notice", text);
	n.expiryTime = GetTickCount() + durationMs;
	m_transientNotices.push_back(std::move(n));
}

// -----------------------------------------------------------------------
// RefreshVoteLine: if a vote for targetDpid is still active, update its HUD line.
//   No-op if the vote was already resolved (e.g. passed or failed in
//   CheckAndExecuteReject called just before this).
// -----------------------------------------------------------------------
void VoteReject::RefreshVoteLine(unsigned targetDpid)
{
	auto it = m_votes.find(targetDpid);
	if (it != m_votes.end())
		HudNotifications::GetInstance()->UpdateLine(
			it->second.hudLineId, FormatVoteLine(targetDpid, it->second));
}

// -----------------------------------------------------------------------
// CastLocalYesVote: record a YES vote from the local player and broadcast it.
// -----------------------------------------------------------------------
void VoteReject::CastLocalYesVote(unsigned targetDpid)
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	unsigned myDpid = taPtr->Players[taPtr->LocalHumanPlayer_PlayerID].DirectPlayID;

	auto it = m_votes.find(targetDpid);
	if (it == m_votes.end()) return;

	auto& voters = it->second.voters;
	if (std::find(voters.begin(), voters.end(), myDpid) != voters.end()) return;

	// YES cancels any prior NO from the same player
	auto& noVoters2 = it->second.noVoters;
	noVoters2.erase(std::remove(noVoters2.begin(), noVoters2.end(), myDpid), noVoters2.end());
	voters.push_back(myDpid);
	IDDrawSurface::OutptFmtTxt("[VoteReject] CastLocalYesVote dpid=%u for target dpid=%u", myDpid, targetDpid);
	BroadcastMsg(VoteRejectCommand::CastVote, targetDpid, it->second.rejectMask);
	CheckAndExecuteReject(targetDpid);
	RefreshVoteLine(targetDpid);
}

// -----------------------------------------------------------------------
// CastLocalNoVote: record a NO vote from the local player and broadcast it.
// -----------------------------------------------------------------------
void VoteReject::CastLocalNoVote(unsigned targetDpid)
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	unsigned myDpid = taPtr->Players[taPtr->LocalHumanPlayer_PlayerID].DirectPlayID;

	auto it = m_votes.find(targetDpid);
	if (it == m_votes.end()) return;

	auto& noVoters = it->second.noVoters;
	if (std::find(noVoters.begin(), noVoters.end(), myDpid) != noVoters.end()) return;

	// NO cancels any prior YES from the same player
	auto& yesVoters2 = it->second.voters;
	yesVoters2.erase(std::remove(yesVoters2.begin(), yesVoters2.end(), myDpid), yesVoters2.end());
	noVoters.push_back(myDpid);
	IDDrawSurface::OutptFmtTxt("[VoteReject] CastLocalNoVote dpid=%u for target dpid=%u", myDpid, targetDpid);
	BroadcastMsg(VoteRejectCommand::CastNoVote, targetDpid, it->second.rejectMask);
	CheckAndExecuteReject(targetDpid);
	RefreshVoteLine(targetDpid);
}

// -----------------------------------------------------------------------
// GetActiveVotes: populate a snapshot vector for VoteDialog display.
//   Excludes votes where the local player is the target (they see a
//   console message instead).
// -----------------------------------------------------------------------
void VoteReject::GetActiveVotes(std::vector<VoteDisplayInfo>& out) const
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	unsigned myDpid = taPtr->Players[taPtr->LocalHumanPlayer_PlayerID].DirectPlayID;

	out.clear();
	for (const auto& kv : m_votes)
	{
		if (kv.first == myDpid)
			continue;  // rejectee sees console message, not dialog
		if (kv.second.votingClosed)
			continue;  // voting over; HUD shows countdown, dialog row removed

		const VoteState& s = kv.second;
		VoteDisplayInfo info;
		info.targetDpid   = kv.first;
		info.rejectMask   = s.rejectMask;
		info.proposerName = s.proposerName;
		info.targetName   = s.targetName;
		info.yesVotes     = (int)s.voters.size();
		info.noVotes      = (int)s.noVoters.size();
		info.expiryTime   = s.expiryTime;
		info.votingClosed = s.votingClosed;

		info.votesNeeded = ComputeTally(kv.first, s).votesNeeded;

		info.isAllyOfLocal = false;
		if (s.rejectMask == 6 && s.targetSlot >= 0) {
			int localSlot = (int)(unsigned char)taPtr->LocalHumanPlayer_PlayerID;
			if (localSlot >= 0 && localSlot < 10)
				info.isAllyOfLocal = AreAllies(taPtr, localSlot, s.targetSlot);
		}

		out.push_back(std::move(info));
	}
}

// -----------------------------------------------------------------------
// Debug pipe API — all called from the render thread via DrainQueue.
// -----------------------------------------------------------------------

void VoteReject::InjectReceive(unsigned fromDpid, const VoteRejectMessage& msg)
{
	OnReceive(fromDpid, msg);
}

void VoteReject::ResetAllVotes()
{
	for (auto& kv : m_votes)
		HudNotifications::GetInstance()->RemoveLine(kv.second.hudLineId);
	m_votes.clear();

	for (auto& n : m_transientNotices)
		HudNotifications::GetInstance()->RemoveLine(n.hudLineId);
	m_transientNotices.clear();

	for (auto& kv : m_completedTimeoutRejects)
		HudNotifications::GetInstance()->RemoveLine(kv.second.hudLineId);
	m_completedTimeoutRejects.clear();

	m_cooldownExpiry.clear();

	if (g_VoteDialog) g_VoteDialog->Refresh();
}

void VoteReject::ExpireVote(unsigned targetDpid)
{
	auto it = m_votes.find(targetDpid);
	if (it != m_votes.end())
		it->second.expiryTime = GetTickCount() - 1;
}

std::string VoteReject::DumpVotes() const
{
	std::string json = "{\"votes\":{";
	bool first = true;
	for (const auto& kv : m_votes)
	{
		if (!first) json += ",";
		first = false;
		char buf[512];
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"\"%u\":{\"mask\":%d,\"yes\":%d,\"no\":%d,\"closed\":%s,"
			"\"proposer\":\"%s\",\"target\":\"%s\"}",
			kv.first,
			(int)(unsigned char)kv.second.rejectMask,
			(int)kv.second.voters.size(),
			(int)kv.second.noVoters.size(),
			kv.second.votingClosed ? "true" : "false",
			kv.second.proposerName.c_str(),
			kv.second.targetName.c_str());
		json += buf;
	}
	char tail[128];
	_snprintf_s(tail, sizeof(tail), _TRUNCATE,
		"},\"notices\":%d,\"completedRejects\":%d}",
		(int)m_transientNotices.size(),
		(int)m_completedTimeoutRejects.size());
	json += tail;
	return json;
}
