#include "AutoTeam.h"
#include "BattleroomCommands.h"
#include "iddrawsurface.h"
#include "tafunctions.h"
#include "StartPositions.h"
#include "hook/hook.h"

#include <chrono>
#include <numeric>
#include <sstream>

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#pragma pack(1)
struct AllianceMessage
{
	std::uint8_t id23;				//0
	std::uint32_t subjectDPID;		//1
	std::uint32_t objectDPID;		//5
	std::uint8_t isAllied;			//9
	std::uint32_t unknown;			//10
									//14
};

struct TeamMessage
{
	std::uint8_t id24;				//0
	std::uint32_t subjectDPID;		//1
	std::uint8_t teamNumber;		//5  bits 0-2 = team (0-5); bit 7 = TEAM24_NO_CASCADE flag
									//6
};

// Bit 7 of TeamMessage::teamNumber.  Set by SetBattleroomTeamsAndAlliances to suppress
// the per-TEAM24 alliance cascade in TeamMessageDispatchHookProc on all clients.
// The alliance state is fully managed by the explicit ALLY23 pass that follows, so the
// intermediate cascade (which races with stale AllyTeam values) is unnecessary and harmful.
// Bit 7 is safe to use: teamNumber valid values are 0-5 (3 bits), so bit 7 is always 0
// in normal TA packets and the packet size is unchanged.
static const std::uint8_t TEAM24_NO_CASCADE = 0x80;

// do what ever is necessary to make p1 offer alliance to p2
// regardless of whether or not p1 or p2 is local
static void OfferAlliance(PlayerStruct& p1, PlayerStruct& p2, bool isAllied)
{
	bool p1IsLocal = p1.My_PlayerType == Player_LocalAI || p1.My_PlayerType == Player_LocalHuman;
	bool p2IsLocal = p2.My_PlayerType == Player_LocalAI || p2.My_PlayerType == Player_LocalHuman;

	int* PTR = (int*)0x00511de8;
	TAdynmemStruct* ta = (TAdynmemStruct*)(*PTR);

	AllianceMessage msg;
	msg.id23 = 0x23;
	msg.subjectDPID = p1.DirectPlayID;
	msg.objectDPID = p2.DirectPlayID;
	msg.isAllied = isAllied;
	msg.unknown = 0;

	if (!p1IsLocal)
	{
		// send a remote request to p1 to offer alliance to p2
		msg.unknown = -1;
		HAPI_SendBuf(ta->Players[ta->LocalHumanPlayer_PlayerID].DirectPlayID, p1.DirectPlayID, (const char*)&msg, sizeof(msg));
		return;
	}

	// in-game +autoteam only works properly with broadcast=true
	// (otherwise some players' gpgnet4ta doesn't know about the alliance and therefore may report incorrect game outcome)
	bool broadcast = true;
	if (broadcast)
	{
		p1.AllyFlagAry[p2.PlayerAryIndex] = isAllied;
		DataShare->allies[p2.PlayerAryIndex] = isAllied;	// Needed for battleroom +autoteam otherwise host's ally is missing resource bars ...
		HAPI_BroadcastMessage(p1.DirectPlayID, (const char*)&msg, sizeof(msg));
		return;
	}

	if (!p2IsLocal)
	{
		// as p1, offer alliance to p2
		p1.AllyFlagAry[p2.PlayerAryIndex] = isAllied;
		HAPI_SendBuf(p1.DirectPlayID, p2.DirectPlayID, (const char*)&msg, sizeof(msg));
	}
	else 
	{
		// both p1 and p2 are local
		p1.AllyFlagAry[p2.PlayerAryIndex] = isAllied;
	}

	if (p1.PlayerNum != 1 && p2.PlayerNum != 1)
	{
		// host needs to know about alliance so they can assign start positions
		PlayerStruct* host = FindPlayerByPlayerNum(1);
		if (host)
		{
			HAPI_SendBuf(p1.DirectPlayID, host->DirectPlayID, (const char*)&msg, sizeof(msg));
		}
	}
}

// intercept the ALLY23 message dispatch to look for remoteRequest(unknown=-1)
// and treat it is a remote request for us to offer alliance
static unsigned int RemoteAllianceRequestHookAddr = 0x454ca1;
static unsigned int RemoteAllianceRequestHookProc(PInlineX86StackBuffer X86StrackBuffer)
{
	int* PTR = (int*)0x00511de8;
	TAdynmemStruct* ta = (TAdynmemStruct*)(*PTR);

	AllianceMessage** allianceMessage = (AllianceMessage**)(X86StrackBuffer->Esp + 0x10);
	if (!allianceMessage || !*allianceMessage || (*allianceMessage)->unknown != -1)
	{
		return 0;
	}

	PlayerStruct* p1 = FindPlayerByDPID((*allianceMessage)->subjectDPID);
	if (p1 == NULL || p1->My_PlayerType != Player_LocalAI && p1->My_PlayerType != Player_LocalHuman)
	{
		return 0;
	}

	PlayerStruct* p2 = FindPlayerByDPID((*allianceMessage)->objectDPID);
	if (p2 == NULL)
	{
		return 0;
	}

	OfferAlliance(*p1, *p2, (*allianceMessage)->isAllied);

	X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x455f50;
	return X86STRACKBUFFERCHANGE;
}


// replace TEAM24 message dispatch to ensure alliances match team selections
static unsigned int TeamMessageDispatchHookAddr = 0x454d3a;
static unsigned int TeamMessageDispatchHookProc(PInlineX86StackBuffer X86StrackBuffer)
{
	int* PTR = (int*)0x00511de8;
	TAdynmemStruct* ta = (TAdynmemStruct*)(*PTR);

	const TeamMessage* msg = (TeamMessage*)(X86StrackBuffer->Eax);
	if (msg)
	{
		const bool noCascade      = (msg->teamNumber & TEAM24_NO_CASCADE) != 0;
		const std::uint8_t team   = msg->teamNumber & ~TEAM24_NO_CASCADE;

		if (team < 6u)
		{
			PlayerStruct* subjectPlayer = FindPlayerByDPID(msg->subjectDPID);
			if (subjectPlayer)
			{
				// When noCascade is clear (manual team click): update alliances for every
				// local player whose state disagrees with the new team assignment.
				// Only the local-player→subject direction is sent here; the subject player's
				// own client handles the reciprocal direction when it processes the same
				// TEAM24 packet.  Sending a remote-request (unknown=-1) for the subject
				// player creates a second cascade wave that races against these packets with
				// stale AllyTeam values, so it is intentionally omitted.
				//
				// When noCascade is set (autoteam): skip the cascade entirely.
				// SetBattleroomTeamsAndAlliances sends an explicit ALLY23 for every pair
				// immediately after all TEAM24s, so the cascade is redundant and harmful
				// (it runs before all TEAM24s have been processed, producing wrong intermediate
				// states that race against the authoritative ALLY23 packets from the host).
				if (!noCascade)
				{
					// Manual team click: each client updates its own local player's alliance
					// toward the subject, and sends a remote request (unknown=-1) asking the
					// subject's client to reciprocate.  The remote request is the only mechanism
					// that causes the subject player's own AllyFlagAry to be updated on their
					// machine — their client excludes them from their own loop iteration.
					// This second wave is harmless for single team-click events because only one
					// TEAM24 is in-flight at a time (no intermediate stale-AllyTeam problem).
					// It is suppressed by TEAM24_NO_CASCADE for autoteam, where the host's
					// explicit ALLY23 pass handles everything instead.
					for (int n = 0; n < 10; ++n)
					{
						PlayerStruct* localPlayer = &ta->Players[n];
						if (localPlayer->PlayerActive &&
							subjectPlayer != localPlayer &&
							!(localPlayer->PlayerInfo->PropertyMask & WATCH) &&
							(localPlayer->AllyTeam < 5 || team < 5) &&
							(localPlayer->My_PlayerType == Player_LocalAI || localPlayer->My_PlayerType == Player_LocalHuman))
						{
							bool isAllied = localPlayer->AllyTeam == team;
							if (isAllied != bool(localPlayer->AllyFlagAry[subjectPlayer->PlayerAryIndex]))
							{
								OfferAlliance(*localPlayer, *subjectPlayer, isAllied);
							}
							if (isAllied != bool(subjectPlayer->AllyFlagAry[localPlayer->PlayerAryIndex]))
							{
								OfferAlliance(*subjectPlayer, *localPlayer, isAllied);
							}
						}
					}
				}

				subjectPlayer->AllyTeam = team;
			}
		}
	}

	X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x454dea;
	return X86STRACKBUFFERCHANGE;
}


// callback for in-game autoteam: assign alliances based on actual start positions
static void __stdcall InternalCommand_autoteam(char *argv[])
{
	const int argc = *(int*)(argv + 52);
	const int teamCount = argc > 1
		? std::max(2, std::min(5, std::atoi(argv[1])))
		: 2;

	int* PTR = (int*)0x00511de8;
	TAdynmemStruct* ta = (TAdynmemStruct*)(*PTR);

	int isActivePlayer[10];
	int startPositions[10];
	if (!StartPositions::GetInstance()->GetInitedStartPositions(isActivePlayer, startPositions))
	{
		SendText("+autoteam is only available to the host of a multiplayer game", 0);
		return;
	}

	int countActivePlayers = std::accumulate(isActivePlayer, isActivePlayer + 10, 0);
	if (countActivePlayers < 2)
	{
		SendText("+autoteam not available b/c too few players", 0);
		return;
	}

	for (int n = 0; n < 10; ++n)
	{
		if (ta->Players[n].PlayerActive && ta->Players[n].AllyTeam < 5 && !(ta->Players[n].PlayerInfo->PropertyMask & WATCH))
		{
			SendText("+autoteam not available b/c players have team selections", 0);
			return;
		}
	}

	for (int n = 0; n < 10; ++n)
	{
		if (!isActivePlayer[n])
		{
			continue;
		}
		for (int m = 0; m < 10; ++m)
		{
			if (m == n || !isActivePlayer[m])
			{
				continue;
			}
			bool isAllied = startPositions[n] % teamCount == startPositions[m] % teamCount;
			OfferAlliance(ta->Players[n], ta->Players[m], isAllied);
		}
	}

	char txt[0x100];
	std::sprintf(txt, "Alliances created with %d teams", teamCount);
	ShowText(&ta->Players[ta->LocalHumanPlayer_PlayerID], txt, 4, 0);
}

static InternalCommandTableEntryStruct AUTO_TEAMS_COMMAND_TABLE[] = {
	{"autoteam", InternalCommand_autoteam, InternalCommandRunLevel::CMD_LEVEL_NORMAL},
	{NULL, NULL, InternalCommandRunLevel::CMD_LEVEL_NULL}
};

static std::default_random_engine RNG(std::chrono::system_clock::now().time_since_epoch().count());

static void SetBattleroomTeamsAndAlliances(const StartPositionsData& spd, int teamCount)
{
	int* PTR = (int*)0x00511de8;
	TAdynmemStruct* ta = (TAdynmemStruct*)(*PTR);

	// Pass 0: break every existing alliance before assigning new ones.
	// TA's native TEAM24 handler does not clear AllyFlagAry, so stale alliances from
	// previous manual selections or a prior +autoteam can linger.  Explicitly clearing
	// them first ensures the subsequent ALLY23 pass writes a clean authoritative state.
	for (int n = 0; n < 10; ++n)
	{
		PlayerStruct* pn = &ta->Players[n];
		if (!pn->PlayerActive || (pn->PlayerInfo->PropertyMask & WATCH))
			continue;
		for (int m = n + 1; m < 10; ++m)
		{
			PlayerStruct* pm = &ta->Players[m];
			if (!pm->PlayerActive || (pm->PlayerInfo->PropertyMask & WATCH))
				continue;
			if (pn->AllyFlagAry[pm->PlayerAryIndex] || pm->AllyFlagAry[pn->PlayerAryIndex])
			{
				OfferAlliance(*pn, *pm, false);
				OfferAlliance(*pm, *pn, false);
			}
		}
	}

	// Pass 1: assign team numbers.
	// TEAM24_NO_CASCADE is set in teamNumber so that TeamMessageDispatchHookProc on every
	// client skips the per-TEAM24 alliance cascade.  The cascade derives alliances from
	// AllyTeam values that may not yet reflect the other TEAM24s in-flight, producing wrong
	// intermediate state that races against the authoritative ALLY23 pass below.
	for (int n = 0; n < spd.positionCount; ++n)
	{
		PlayerStruct* player = FindPlayerByName(spd.orderedPlayerNames[n]);
		if (player && player->PlayerActive && !(player->PlayerInfo->PropertyMask & WATCH))
		{
			ta->Players[player->PlayerAryIndex].AllyTeam = n % teamCount;
			TeamMessage teamMessage;
			teamMessage.id24 = 0x24;
			teamMessage.subjectDPID = player->DirectPlayID;
			teamMessage.teamNumber = (n % teamCount) | TEAM24_NO_CASCADE;
			HAPI_BroadcastMessage(ta->Players[ta->LocalHumanPlayer_PlayerID].DirectPlayID, (const char*)&teamMessage, sizeof(teamMessage));
		}
	}

	// Pass 2: send explicit alliance state for every ordered pair.
	// This is the authoritative pass.  Because TEAM24_NO_CASCADE suppresses the cascade on
	// all clients, these packets are the only ones setting AllyFlagAry.  Sending them after
	// all TEAM24s means all clients have correct AllyTeam values before any alliance packet
	// arrives, so AlliancesBroadcastHookProc will faithfully re-broadcast the right state.
	for (int n = 0; n < spd.positionCount; ++n)
	{
		PlayerStruct* player = FindPlayerByName(spd.orderedPlayerNames[n]);
		if (player && player->PlayerActive && !(player->PlayerInfo->PropertyMask & WATCH))
		{
			for (int m = 0; m < spd.positionCount; ++m)
			{
				PlayerStruct* playerOther = FindPlayerByName(spd.orderedPlayerNames[m]);
				if (playerOther && playerOther != player && playerOther->PlayerActive && !(playerOther->PlayerInfo->PropertyMask & WATCH))
				{
					OfferAlliance(*player, *playerOther, n % teamCount == m % teamCount);
				}
			}
		}
	}
}

// callback for battleroom autoteam: assign teams as prescribed by external process via shared memory (or randomly if shared mem unavailable)
static void BattleroomAutoteamCommandHandler(const std::vector<std::string> &args)
{
	int* PTR = (int*)0x00511de8;
	TAdynmemStruct* ta = (TAdynmemStruct*)(*PTR);

	if (ta->Players[ta->LocalHumanPlayer_PlayerID].PlayerNum != 1)
	{
		SendText("+autoteam can only be used by host", 0);
		return;
	}

	const int teamCount = args.size() > 1
		? std::max(2, std::min(5, std::atoi(args[1].c_str())))
		: 2;

	const StartPositionsData* sm = StartPositions::GetInstance()->GetSharedMemory();
	StartPositionsData _sm;

	if (!sm || sm->positionCount == 0)
	{
		std::memset(&_sm, 0, sizeof(_sm));
		sm = &_sm;

		SendText("Autobalance not available. Setting random teams", 0);
		int shuffle[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
		std::shuffle(shuffle, shuffle + 10, RNG);

		for (int n = 0; n < 10; ++n)
		{
			int idx = shuffle[n];
			if (ta->Players[idx].PlayerActive && !(ta->Players[idx].PlayerInfo->PropertyMask & WATCH))
			{
				std::strncpy(_sm.orderedPlayerNames[_sm.positionCount], ta->Players[idx].Name, 32);
				++_sm.positionCount;
			}
		}
	}
	else
	{
		SendText("Setting autobalanced teams", 0);
	}

	SetBattleroomTeamsAndAlliances(*sm, teamCount);
}

// callback for battleroom autoteam: assign teams as prescribed by external process via shared memory (or randomly if shared mem unavailable)
static void BattleroomRandomteamCommandHandler(const std::vector<std::string>& args)
{
	int* PTR = (int*)0x00511de8;
	TAdynmemStruct* ta = (TAdynmemStruct*)(*PTR);

	if (ta->Players[ta->LocalHumanPlayer_PlayerID].PlayerNum != 1)
	{
		SendText("+randomteam can only be used by host", 0);
		return;
	}

	const int teamCount = args.size() > 1
		? std::max(2, std::min(5, std::atoi(args[1].c_str())))
		: 2;

	SendText("Setting random teams", 0);

	StartPositionsData sm;
	std::memset(&sm, 0, sizeof(sm));

	int shuffle[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	std::shuffle(shuffle, shuffle + 10, RNG);

	for (int n = 0; n < 10; ++n)
	{
		int idx = shuffle[n];
		if (ta->Players[idx].PlayerActive && !(ta->Players[idx].PlayerInfo->PropertyMask & WATCH))
		{
			std::strncpy(sm.orderedPlayerNames[sm.positionCount], ta->Players[idx].Name, 32);
			++sm.positionCount;
		}
	}

	SetBattleroomTeamsAndAlliances(sm, teamCount);
}

// In Send_PacketPlayerInfo, right after broadcasting PLAYER_STATUS(0x20).
// Send ALLY(0x23) messages for every player pair.
// Host needs to know who is allied with whom in order to assign start positions.
// If players manually ally (without using teams icons) then host won't know about all the alliances yet.
// This hook rectifies that situation.
static unsigned int AlliancesBroadcastHookAddr = 0x45100a;
static unsigned int AlliancesBroadcastHookProc(PInlineX86StackBuffer X86StrackBuffer)
{
	if (DataShare->TAProgress == TAInGame) {
		return 0;
	}

	int* PTR = (int*)0x00511de8;
	TAdynmemStruct* ta = (TAdynmemStruct*)(*PTR);

	PlayerStruct& localPlayer = *(PlayerStruct*)X86StrackBuffer->Ebp;
	if (!localPlayer.PlayerActive)
	{
		return 0;
	}

	for (int n = 0; n < 10; ++n)
	{
		// for every other player
		PlayerStruct& remotePlayer = ta->Players[n];
		if (remotePlayer.DirectPlayID == localPlayer.DirectPlayID || !remotePlayer.PlayerActive)
		{
			continue;
		}

		// Use AllyFlagAry directly as the single source of truth.
		// Previously this derived alliance from AllyTeam when AllyTeam<5, but that
		// silently overwrites manual cross-team alliances and creates a second source of
		// truth that can diverge from AllyFlagAry (which is what TA actually enforces
		// at runtime for resource sharing and combat AI).
		bool isAlliedAB = bool(localPlayer.AllyFlagAry[n]);

		OfferAlliance(localPlayer, remotePlayer, isAlliedAB);
	}

	return 0;
}

static unsigned int initAutoTeamCommandHookAddr = 0x4195dd;
static unsigned int initAutoTeamCommandHookProc(PInlineX86StackBuffer X86StrackBuffer)
{
	InitInternalCommand(AUTO_TEAMS_COMMAND_TABLE);
	return 0;
}

std::unique_ptr<AutoTeam> AutoTeam::m_instance;

AutoTeam::AutoTeam()
{
	BattleroomCommands::GetInstance()->RegisterCommand("+autoteam", BattleroomAutoteamCommandHandler);
	BattleroomCommands::GetInstance()->RegisterCommand("+randomteam", BattleroomRandomteamCommandHandler);
	m_hooks.push_back(std::make_unique<InlineSingleHook>(initAutoTeamCommandHookAddr, 5, INLINE_5BYTESLAGGERJMP, initAutoTeamCommandHookProc));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(AlliancesBroadcastHookAddr, 5, INLINE_5BYTESLAGGERJMP, AlliancesBroadcastHookProc));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(RemoteAllianceRequestHookAddr, 5, INLINE_5BYTESLAGGERJMP, RemoteAllianceRequestHookProc));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(TeamMessageDispatchHookAddr, 5, INLINE_5BYTESLAGGERJMP, TeamMessageDispatchHookProc));
}

AutoTeam::~AutoTeam()
{
}

void AutoTeam::Install()
{
	if (!m_instance)
	{
		m_instance.reset(new AutoTeam());
	}
}
