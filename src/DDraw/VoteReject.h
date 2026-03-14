#pragma once

#include "HudNotifications.h"

#include <memory>
#include <map>
#include <string>
#include <vector>
#include "hook/hook.h"
#include "windows.h"

enum class VoteRejectCommand : char {
	ProposeVote = 1,
	CastVote    = 2,
	CastNoVote  = 3,
};

#pragma pack(1)
struct VoteRejectMessage {
	char chatByte;           // 0x05
	char nullText;           // 0x00
	char msgId;              // 0x2c
	short size;              // sizeof(VoteRejectMessage) = 65
	VoteRejectCommand command;
	unsigned targetDpid;     // dpid of player being voted to reject
	char rejectMask;         // 1=battleroom reject, 6=timeout reject
	char pad[54];
};  // total = 1+1+1+2+1+4+1+54 = 65 bytes
#pragma pack()

// Intercepts TA's reject actions and replaces them with a majority vote
// once the game is in progress (TAProgress == TAInGame).
//
// Vote initiation — manual reject:
//   YesNoRejectHook  @ 0x00446044 (6 bytes): RejectYesNo_OnCommand.
//                      Fires when any player clicks YES in a YESORNO reject dialog.
//                      Battleroom/loading or replay: reject proceeds immediately.
//                      In-game:
//                        - No vote in progress -> propose a new vote and broadcast it.
//                        - Vote already in progress -> cast this player's YES vote.
//                      Non-proposing, non-rejectee clients are shown VoteDialog.
//                      Rejectee sees the HUD notification instead.
//   YesNoNoHook      @ 0x00446070 (6 bytes): PUSH ESI before CALL SetUIChangeFlag.
//                      Fires when any player clicks NO in a YESORNO reject dialog.
//                      Records a CastNoVote; if enough NO votes accumulate to make
//                      the YES threshold unreachable, the vote is cancelled immediately.
//                      Returns 0 so SetUIChangeFlag still runs to dismiss the dialog.
//
// Vote initiation — timeout reject:
//   ShowRejectWindowHook @ 0x00453B0A (5 bytes): push 800h, inside ShowRejectWindow,
//                      just before the LoadGUIFile call that creates TIMEOUT.GUI.
//                      Fires when TA's CheckForDroppedPlayers detects a single timed-out
//                      player (bVar2=false path).  Intercepts the native modal timeout
//                      dialog, calls ProposeReject(dpid, 6) instead, and redirects to
//                      the ShowRejectWindow epilogue so the dialog is never shown.
//                      Replay: skips entirely (no dialog, no vote).
//
//   Tick() dropout scan: CheckForDroppedPlayers deliberately suppresses ShowRejectWindow
//                      when bVar2=true (multiple players timed out simultaneously),
//                      so the hook above never fires.  Tick() independently replicates
//                      TA's dropout condition (gap in GameRunSec > 900 units = 30 s)
//                      and calls ProposeReject for each timed-out player every frame.
//                      ProposeReject is idempotent, so duplicate calls are no-ops.
//
// Ally .take handling:
//   When the recorder accepts a .take command it broadcasts a chat message:
//     "<taker> taking <target>s units"
//   PacketChatRouter's chat handler routes this to OnIncomingChat, which:
//     - Cancels any active timeout vote for the target (ally is taking over).
//     - Closes the completed-reject window for the target if one is open.
//
// After a timeout vote passes:
//   - targetDpid is added to m_completedTimeoutRejects
//   - The persistent HUD notification invites allies to issue .take
//   - Once the recorder confirms .take (or the window expires), the state is cleared
class VoteReject
{
public:
	static void Install();
	static VoteReject* GetInstance();
	void Tick();

	// Cancel an active timeout vote for targetDpid without executing the reject.
	// Called when the recorder confirms .take acceptance; also auto-triggered via
	// LastMsgTimeStamp in Tick() when the player's network traffic resumes.
	void CancelTimeoutVote(unsigned targetDpid);

	// VoteDialog integration: cast a local yes/no vote.
	void CastLocalYesVote(unsigned targetDpid);
	void CastLocalNoVote(unsigned targetDpid);

	// Per-vote display snapshot used by VoteDialog to populate its rows.
	struct VoteDisplayInfo {
		unsigned    targetDpid;
		char        rejectMask;      // 1=manual, 6=timeout
		std::string proposerName;
		std::string targetName;
		int         yesVotes;
		int         noVotes;
		int         votesNeeded;
		DWORD       expiryTime;
		bool        votingClosed;    // NO majority reached; auto-reject still pending
	};
	void GetActiveVotes(std::vector<VoteDisplayInfo>& out) const;

	// Debug pipe API (DebugPipeServer calls these from the render thread).
	void        ProposeTimeoutReject(unsigned targetDpid) { ProposeReject(targetDpid, 6); }
	void        InjectReceive(unsigned fromDpid, const VoteRejectMessage& msg);
	void        ResetAllVotes();
	void        ExpireVote(unsigned targetDpid);
	std::string DumpVotes() const;
	void        SetSuppressBroadcast(bool suppress) { m_suppressBroadcast = suppress; }

	static int __stdcall YesNoRejectRouter(PInlineX86StackBuffer pBuf);
	static int __stdcall YesNoNoRouter(PInlineX86StackBuffer pBuf);
	static int __stdcall ShowRejectWindowRouter(PInlineX86StackBuffer pBuf);
	static int __stdcall MultiDropoutRouter(PInlineX86StackBuffer pBuf);

private:
	VoteReject();

	static void HandleVoteRejectPacket(unsigned fromDpid, const void* buf);
	void OnReceive(unsigned fromDpid, const VoteRejectMessage& msg);
	void OnIncomingChat(unsigned fromDpid, const char* text);

	void ProposeReject(unsigned targetDpid, char rejectMask);
	void BroadcastMsg(VoteRejectCommand command, unsigned targetDpid, char rejectMask);
	void CheckAndExecuteReject(unsigned targetDpid);
	void ExecuteReject(unsigned targetDpid, char rejectMask, const std::string& targetName);

	std::string GetPlayerName(unsigned dpid);
	bool HasActiveTimeoutVote() const;

	static VoteReject* m_instance;

	static const DWORD VOTE_TIMEOUT_MS        = 90000;
	static const DWORD MANUAL_VOTE_TIMEOUT_MS = 60000;
	static const DWORD VOTE_COOLDOWN_MS       = 90000;

	struct VoteState {
		char rejectMask;
		std::string proposerName;
		std::string targetName;
		int targetSlot;                    // 0-9 player slot, -1 if unknown
		std::vector<unsigned> voters;      // dpids who voted YES (proposer counted at creation)
		std::vector<unsigned> noVoters;    // dpids who voted NO
		DWORD expiryTime;                  // GetTickCount() value at which vote expires
		int lastMsgTimeStampAtProposal;    // PlayerStruct.LastMsgTimeStamp when vote was created
		                                   // (timeout votes only: advance = network resumed)
		HudLineId hudLineId;               // token for the live HUD status line
		bool votingClosed;                 // timeout vote: NO-majority reached, auto-reject still pending
	};

	// Timeout reject completed -- allies may now issue .take.
	struct CompletedTimeoutReject {
		std::string targetName;
		DWORD       expiryTime;            // GetTickCount() value when this window closes
		HudLineId   hudLineId;             // token for the live HUD take-window line
	};

	// Transient failure notice: shown briefly on the HUD then auto-removed.
	struct TransientNotice {
		HudLineId hudLineId;
		DWORD     expiryTime;
	};

	void AddTransientNotice(const std::string& text, DWORD durationMs);

	// Format a single HUD line for an active vote or a completed take-window.
	// Declared after the struct definitions so VoteState/CompletedTimeoutReject
	// are fully in scope (avoids spurious global-scope forward declarations).
	std::string FormatVoteLine(unsigned targetDpid, const VoteState& state) const;
	std::string FormatTakeLine(const CompletedTimeoutReject& ctr) const;

	// If a vote for targetDpid is still active, refresh its HUD line text.
	void RefreshVoteLine(unsigned targetDpid);

	std::map<unsigned, VoteState>              m_votes;                   // keyed by targetDpid
	std::map<unsigned, DWORD>                  m_cooldownExpiry;          // targetDpid -> cooldown end
	std::map<unsigned, CompletedTimeoutReject> m_completedTimeoutRejects; // targetDpid -> take window
	std::vector<TransientNotice>               m_transientNotices;

	DWORD m_lastHudUpdateSec   = 0;    // tracks last per-second countdown refresh
	bool  m_suppressBroadcast  = false; // set by DebugPipeServer to skip DirectPlay sends

	static const DWORD NOTICE_DURATION_MS  = 10000;

	std::unique_ptr<InlineSingleHook> m_yesNoRejectHook;
	std::unique_ptr<InlineSingleHook> m_yesNoNoHook;
	std::unique_ptr<InlineSingleHook> m_showRejectWindowHook;
	std::unique_ptr<InlineSingleHook> m_multiDropoutHook;
};
