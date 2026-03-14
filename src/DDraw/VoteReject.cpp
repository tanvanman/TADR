#include "VoteReject.h"
#include "VoteDialog.h"
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

static bool AreAllies(TAdynmemStruct* taPtr, int slotA, int slotB)
{
	if (slotA < 0 || slotB < 0 || slotA >= 10 || slotB >= 10 || slotA == slotB)
		return false;
	return taPtr->Players[slotA].AllyFlagAry[slotB] != 0
		&& taPtr->Players[slotB].AllyFlagAry[slotA] != 0;
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
		&& msg.msgId    == 0x2c
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


	PacketChatRouter::GetInstance()->RegisterHandler(0x2c, HandleVoteRejectPacket);
	PacketChatRouter::GetInstance()->RegisterChatHandler(
		[](unsigned fromDpid, const char* text) {
			VoteReject::GetInstance()->OnIncomingChat(fromDpid, text);
		});

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

	if (!DataShare->PlayingDemo)
	{
		unsigned targetDpid = (unsigned)pBuf->Edi;
		if (targetDpid != 0 && targetDpid != 0xFFFFFFFF)
			VoteReject::GetInstance()->ProposeReject(targetDpid, 6);
	}

	// Always skip to ShowRejectWindow epilogue — we never want the modal dialog.
	pBuf->rtnAddr_Pvoid = (LPVOID)0x00453C0C;
	return X86STRACKBUFFERCHANGE;
}


// -----------------------------------------------------------------------
// Hook 3: MultiDropoutRouter — fires at 0x00453CC3 (start of CheckForDroppedPlayers'
//   second loop) when bMultiDropout=true (multiple players timed out simultaneously).
//
//   TA's second loop skips ShowRejectWindow for all players when bMultiDropout=true,
//   so our ShowRejectWindowHook never fires for this case.  We replicate the same
//   gap check (gameNow - max(LastMsgTimeStamp, GameTimeSec) > field_37F31 * 30) and
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

	if (DataShare->TAProgress == TAInGame && !DataShare->PlayingDemo)
	{
		int gameNow     = (int)pBuf->Eax;          // GameRunSec() from 0x00453C52
		int gameTimeSec = *(int*)0x00512c7c;        // GameTimeSec (min reference)
		TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
		if (taPtr)
		{
			int threshold = (int)taPtr->field_37F31 * 30;
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
// OnIncomingChat: called by PacketChatRouter for every incoming chat message.
//   Detects the recorder's acceptance signal: "<taker> taking <target>s units".
//   When a confirmed .take for a timeout-vote target is seen:
//     - If a vote is still active, cancel it (recorder is taking over).
//     - If a completed-reject window is open for the target, close it.
// -----------------------------------------------------------------------
void VoteReject::OnIncomingChat(unsigned fromDpid, const char* text)
{
	if (!text)
		return;

	// Match "<taker> taking <target>s units"
	// The recorder sends exactly: Players[1].Name + ' taking ' + Players[i].Name + 's units'
	const char* takingPtr = strstr(text, " taking ");
	if (!takingPtr)
		return;
	const char* suffix = "s units";
	size_t textLen = strlen(text);
	size_t sufLen  = strlen(suffix);
	if (textLen < sufLen || strcmp(text + textLen - sufLen, suffix) != 0)
		return;

	// Extract target name: between " taking " and "s units" at end
	const char* targetStart = takingPtr + 8;  // skip " taking "
	size_t targetLen = (text + textLen - sufLen) - targetStart;
	if (targetLen == 0 || targetLen > 64)
		return;
	std::string targetName(targetStart, targetLen);

	// Cancel any active timeout vote whose target name matches
	for (auto it = m_votes.begin(); it != m_votes.end(); ++it)
	{
		if (it->second.rejectMask != 6) continue;
		if (it->second.targetName == targetName)
		{
			IDDrawSurface::OutptFmtTxt(
				"[VoteReject] recorder accepted .take for %s — cancelling timeout vote",
				targetName.c_str());
			CancelTimeoutVote(it->first);
			return;
		}
	}

	// No active vote — if a completed-reject window is open for this target, close it
	for (auto it = m_completedTimeoutRejects.begin(); it != m_completedTimeoutRejects.end(); ++it)
	{
		if (it->second.targetName == targetName)
		{
			IDDrawSurface::OutptFmtTxt(
				"[VoteReject] recorder accepted .take for %s — closing completed-reject window",
				targetName.c_str());
			HudNotifications::GetInstance()->RemoveLine(it->second.hudLineId);
			m_completedTimeoutRejects.erase(it);
			return;
		}
	}
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

		// Check immediately in case threshold is already met (2-player game)
		CheckAndExecuteReject(msg.targetDpid);
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

		CheckAndExecuteReject(msg.targetDpid);
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

		CheckAndExecuteReject(msg.targetDpid);
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
	msg.msgId      = 0x2c;
	msg.size       = sizeof(VoteRejectMessage);
	msg.command    = command;
	msg.targetDpid = targetDpid;
	msg.rejectMask = rejectMask;

	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	unsigned fromDpid = taPtr->Players[taPtr->LocalHumanPlayer_PlayerID].DirectPlayID;
	HAPI_BroadcastMessage(fromDpid, (const char*)&msg, sizeof(msg));
}

// -----------------------------------------------------------------------
// CheckAndExecuteReject: called after each new vote is recorded.
//   Votes needed = ceiling(2/3 * nonTargetActivePlayers), minimum 1.
//   When threshold met, each client independently calls Send_PacketPlayerState_1B.
// -----------------------------------------------------------------------
void VoteReject::CheckAndExecuteReject(unsigned targetDpid)
{
	auto it = m_votes.find(targetDpid);
	if (it == m_votes.end())
		return;

	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	int totalActive = 0;
	for (int i = 0; i < 10; ++i) {
		if (taPtr->Players[i].PlayerActive && taPtr->Players[i].DirectPlayID != 0)
			++totalActive;
	}

	bool isTimeoutReject = (it->second.rejectMask == 6);
	int voteCount = (int)it->second.voters.size();

	// Timeout reject: just need proposer + 1 seconder; no teammate consent required.
	// Manual reject: 2/3 of non-target active players; plus teammate consent if applicable.
	int votesNeeded;
	bool teammateConsent;
	if (isTimeoutReject)
	{
		// Need at least 2 votes (or 1 in a 2-player game); plus teammate consent
		// (at least one ally of the target must have voted YES).
		int nonTarget = totalActive - 1;
		votesNeeded     = (nonTarget <= 1) ? 1 : 2;

		int targetSlot = it->second.targetSlot;
		teammateConsent = true;
		if (targetSlot >= 0) {
			for (int i = 0; i < 10; ++i) {
				if (i == targetSlot) continue;
				if (!taPtr->Players[i].PlayerActive || taPtr->Players[i].DirectPlayID == 0) continue;
				if (!taPtr->Players[targetSlot].AllyFlagAry[i] || !taPtr->Players[i].AllyFlagAry[targetSlot]) continue;
				// Found an active ally of the target — they must vote YES.
				teammateConsent = false;
				if (std::find(it->second.voters.begin(), it->second.voters.end(),
					taPtr->Players[i].DirectPlayID) != it->second.voters.end())
				{
					teammateConsent = true;
					break;
				}
			}
		}
	}
	else
	{
		int nonTarget = totalActive - 1;
		votesNeeded   = (nonTarget <= 1) ? 1 : ((nonTarget * 2 + 2) / 3);

		int targetSlot = it->second.targetSlot;
		teammateConsent = true;
		if (targetSlot >= 0) {
			for (int i = 0; i < 10; ++i) {
				if (i == targetSlot) continue;
				if (!taPtr->Players[i].PlayerActive || taPtr->Players[i].DirectPlayID == 0) continue;
				if (!taPtr->Players[targetSlot].AllyFlagAry[i] || !taPtr->Players[i].AllyFlagAry[targetSlot]) continue;
				// Target has an active teammate — they must have voted
				teammateConsent = false;
				if (std::find(it->second.voters.begin(), it->second.voters.end(),
					taPtr->Players[i].DirectPlayID) != it->second.voters.end())
				{
					teammateConsent = true;
					break;
				}
			}
		}
	}

	int noVoteCount = (int)it->second.noVoters.size();
	int nonTarget = totalActive - 1;

	IDDrawSurface::OutptFmtTxt("[VoteReject] threshold check: %d yes, %d no / %d (need %d), teammate consent: %d",
		voteCount, noVoteCount, totalActive, votesNeeded, (int)teammateConsent);

	if (voteCount >= votesNeeded && teammateConsent)
	{
		std::string targetName = it->second.targetName;
		HudLineId voteHudId = it->second.hudLineId;
		ExecuteReject(targetDpid, it->second.rejectMask, targetName);
		HudNotifications::GetInstance()->RemoveLine(voteHudId);
		m_votes.erase(it);
		if (g_VoteDialog) g_VoteDialog->Refresh();
	}
	else if (noVoteCount > nonTarget - votesNeeded)
	{
		// Enough NO votes that the YES threshold can never be reached.
		IDDrawSurface::OutptFmtTxt("[VoteReject] vote for dpid=%u failed by NO majority (%d no, need %d yes from %d)",
			targetDpid, noVoteCount, votesNeeded, nonTarget);
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
		// Open the .take window so allies of the rejected player can take over.
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
			IDDrawSurface::OutptFmtTxt(
				"[VoteReject] .take window expired for %s", it->second.targetName.c_str());
			HudNotifications::GetInstance()->RemoveLine(it->second.hudLineId);
			it = m_completedTimeoutRejects.erase(it);
		} else {
			++it;
		}
	}

	// Expire timed-out votes
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	for (auto it = m_votes.begin(); it != m_votes.end(); ) {
		// For timeout votes, check whether the player's network traffic has resumed.
		// Packet_Dispatcher writes LastMsgTimeStamp on every received packet.
		if (it->second.rejectMask == 6 && it->second.targetSlot >= 0) {
			int currentTs = taPtr->Players[it->second.targetSlot].LastMsgTimeStamp;
			if (currentTs != it->second.lastMsgTimeStampAtProposal) {
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

	// Refresh countdown text every second so the HUD timer stays live.
	DWORD nowSec = now / 1000;
	if (nowSec != m_lastHudUpdateSec)
	{
		m_lastHudUpdateSec = nowSec;
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
	// Timeout vote where the NO majority closed voting — show countdown to auto-reject.
	if (state.votingClosed)
	{
		DWORD now = GetTickCount();
		int secsLeft = (now < state.expiryTime) ? (int)((state.expiryTime - now) / 1000) : 0;
		char line[256];
		wsprintfA(line, "Vote rejected -- %s auto-rejects in %ds", state.targetName.c_str(), secsLeft);
		return line;
	}

	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	int totalActive = 0;
	for (int i = 0; i < 10; ++i)
		if (taPtr->Players[i].PlayerActive && taPtr->Players[i].DirectPlayID != 0)
			++totalActive;

	bool isTimeoutReject = (state.rejectMask == 6);
	int voteCount  = (int)state.voters.size();
	int noVoteCount = isTimeoutReject ? 0 : (int)state.noVoters.size();

	int votesNeeded;
	bool needsTeammateVote = false;
	if (isTimeoutReject) {
		int nonTarget = totalActive - 1;
		votesNeeded = (nonTarget <= 1) ? 1 : 2;
	} else {
		int nonTarget = totalActive - 1;
		votesNeeded = (nonTarget <= 1) ? 1 : ((nonTarget * 2 + 2) / 3);
		if (state.targetSlot >= 0) {
			for (int i = 0; i < 10; ++i) {
				if (i == state.targetSlot) continue;
				if (!taPtr->Players[i].PlayerActive || taPtr->Players[i].DirectPlayID == 0) continue;
				if (!taPtr->Players[state.targetSlot].AllyFlagAry[i]
					|| !taPtr->Players[i].AllyFlagAry[state.targetSlot]) continue;
				unsigned tdpid = taPtr->Players[i].DirectPlayID;
				if (std::find(state.voters.begin(), state.voters.end(), tdpid) == state.voters.end())
					needsTeammateVote = true;
				else
					needsTeammateVote = false;
				break;
			}
		}
	}

	DWORD now = GetTickCount();
	int secsLeft = (now < state.expiryTime) ? (int)((state.expiryTime - now) / 1000) : 0;

	char line[256];
	if (isTimeoutReject) {
		wsprintfA(line, "Timeout: reject %s (%d yes/%d, %ds)",
			state.targetName.c_str(), voteCount, votesNeeded, secsLeft);
	}
	else if (state.targetSlot >= 0 && needsTeammateVote) {
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

	int totalActive = 0;
	for (int i = 0; i < 10; ++i)
		if (taPtr->Players[i].PlayerActive && taPtr->Players[i].DirectPlayID != 0)
			++totalActive;

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

		int nonTarget = totalActive - 1;
		if (s.rejectMask == 6)
			info.votesNeeded = (nonTarget <= 1) ? 1 : 2;
		else
			info.votesNeeded = (nonTarget <= 1) ? 1 : ((nonTarget * 2 + 2) / 3);

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
