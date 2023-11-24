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
	std::uint8_t teamNumber;		//5
									//6
};

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
	if (msg && msg->teamNumber < 6u)
	{
		PlayerStruct* subjectPlayer = FindPlayerByDPID(msg->subjectDPID);
		if (subjectPlayer)
		{
			for (int n = 0; n < 10; ++n)
			{
				PlayerStruct* localPlayer = &ta->Players[n];
				if (localPlayer->PlayerActive &&
					subjectPlayer != localPlayer &&
					!(localPlayer->PlayerInfo->PropertyMask & WATCH) &&
					(localPlayer->AllyTeam < 5 || msg->teamNumber < 5) &&
					(localPlayer->My_PlayerType == Player_LocalAI || localPlayer->My_PlayerType == Player_LocalHuman))
				{
					bool isAllied = localPlayer->AllyTeam == msg->teamNumber;
					if (isAllied != localPlayer->AllyFlagAry[subjectPlayer->PlayerAryIndex])
					{
						OfferAlliance(*localPlayer, *subjectPlayer, isAllied);
					}
					if (isAllied != subjectPlayer->AllyFlagAry[subjectPlayer->PlayerAryIndex])
					{
						OfferAlliance(*subjectPlayer, *localPlayer, isAllied);
					}
				}
			}
			subjectPlayer->AllyTeam = msg->teamNumber;
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

	for (int n = 0; n < sm->positionCount; ++n)
	{
		PlayerStruct* player = FindPlayerByName(sm->orderedPlayerNames[n]);
		if (player && player->PlayerActive && !(player->PlayerInfo->PropertyMask & WATCH))
		{
			for (int m = 0; m < sm->positionCount; ++m)
			{
				PlayerStruct* playerOther = FindPlayerByName(sm->orderedPlayerNames[m]);
				if (playerOther && playerOther != player && playerOther->PlayerActive && !(playerOther->PlayerInfo->PropertyMask & WATCH))
				{
					OfferAlliance(*player, *playerOther, n % teamCount == m % teamCount);
				}
			}

			ta->Players[player->PlayerAryIndex].AllyTeam = n % teamCount;
			TeamMessage teamMessage;
			teamMessage.id24 = 0x24;
			teamMessage.subjectDPID = player->DirectPlayID;
			teamMessage.teamNumber = n % teamCount;
			HAPI_BroadcastMessage(ta->Players[ta->LocalHumanPlayer_PlayerID].DirectPlayID, (const char*)&teamMessage, sizeof(teamMessage));
		}
	}
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

		bool isAllied = 
			localPlayer.AllyTeam == 5 && localPlayer.AllyFlagAry[n] ||
			localPlayer.AllyTeam < 5 && localPlayer.AllyTeam == remotePlayer.AllyTeam;

		OfferAlliance(localPlayer, remotePlayer, isAllied);
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
