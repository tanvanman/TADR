#include "AutoTeam.h"
#include "BattleroomCommands.h"
#include "iddrawsurface.h"
#include "tafunctions.h"
#include "StartPositions.h"
#include "hook/hook.h"

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

static void SetOfferAlliance(PlayerStruct& p1, PlayerStruct& p2, bool isAllied)
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

	if (!p1IsLocal)
	{
		msg.unknown = -1;
		HAPI_SendBuf(ta->Players[ta->LocalHumanPlayer_PlayerID].DirectPlayID, p1.DirectPlayID, (const char*)&msg, sizeof(msg));
	}
	else if (!p2IsLocal)
	{
		msg.unknown = 0;
		p1.AllyFlagAry[p2.PlayerAryIndex] = isAllied;
		HAPI_SendBuf(p1.DirectPlayID, p2.DirectPlayID, (const char*)&msg, sizeof(msg));
	}
	else 
	{
		p1.AllyFlagAry[p2.PlayerAryIndex] = isAllied;
	}
}

// hack the ALLY23 message dispatch to look for remoteRequest(unknown=-1)
// and if found, respond with our own regular ALLY23 message
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

	SetOfferAlliance(*p1, *p2, (*allianceMessage)->isAllied);

	X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x455f50;
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
		SendText("+autoteam not available because you're not the host", 0);
		return;
	}

	int countActivePlayers = std::accumulate(isActivePlayer, isActivePlayer + 10, 0);
	if (countActivePlayers < 2)
	{
		SendText("+autoteam not available. too few players", 0);
		return;
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
			SetOfferAlliance(ta->Players[n], ta->Players[m], isAllied);
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

// callback for battleroom autoteam: assign teams as prescribed by external process via shared memory
static void BattleroomAutoteamCommandHandler(const std::vector<std::string> &args)
{
	const int teamCount = args.size() > 1
		? std::max(2, std::min(5, std::atoi(args[1].c_str())))
		: 2;

	const StartPositionsData* sm = StartPositions::GetInstance()->GetSharedMemory();
	if (!sm || sm->positionCount == 0)
	{
		SendText("Battleroom +autoteam not available", 0);
		SendText("Create teams manually or use +autoteam after launch", 0);
		return;
	}

	int* PTR = (int*)0x00511de8;
	TAdynmemStruct* ta = (TAdynmemStruct*)(*PTR);

	for (int n = 0; n < sm->positionCount; ++n)
	{
		if (sm->orderedPlayerNames[n][0] == '\0')
		{
			continue;
		}

		PlayerStruct* player = FindPlayerByName(sm->orderedPlayerNames[n]);
		if (!player)
		{
			continue;
		}

		int teamNumber = n % teamCount;
		if (player->My_PlayerType == Player_LocalHuman || player->My_PlayerType == Player_LocalAI)
		{
			PlaySound_Effect("Ally", 0);
			OnTeamChange_BeforeChange_SendMessage_Conditonal_Team23(player);
			player->AllyTeam = teamNumber;
			SendMessage_Team24(player);
			UpdateAlliancesFromTeamSelections();
			UpdateTeamSelectionButtonIcons();
			SendMessage_Team24(player);

			char txt[0x100];
			std::sprintf(txt, "Local player %s: SendMessage_Team24(teamNumber=%d)", player->Name, teamNumber);
			ShowText(&ta->Players[ta->LocalHumanPlayer_PlayerID], txt, 4, 0);
		}
		else if (player->My_PlayerType == Player_RemoteHuman || player->My_PlayerType == Player_RemoteAI)
		{
			char buffer[6];
			buffer[0] = 0x24;
			*(std::uint32_t*)&buffer[1] = player->DirectPlayID;
			buffer[5] = teamNumber;
			std::uint32_t fromDpid = ta->Players[ta->LocalHumanPlayer_PlayerID].DirectPlayID;
			HAPI_BroadcastMessage(fromDpid, buffer, sizeof(buffer));

			char txt[0x100];
			std::sprintf(txt, "HAPI_BroadcastMessage(sender=%s, subject=%s, teamNumber=%d)", 
				ta->Players[ta->LocalHumanPlayer_PlayerID].Name,
				player->Name,
				teamNumber);
			ShowText(&ta->Players[ta->LocalHumanPlayer_PlayerID], txt, 4, 0);
		}
	}
}

// Right at the point of receiving a TEAM(0x24) message from a remote.
// We need to see if remote left our team so we can respond with an ALLY(0x23) message to indicate that we withdraw our alliance
static unsigned int TeamBugfixHookAddr = 0x454de4;;
static unsigned int TeamBugfixHookProc(PInlineX86StackBuffer X86StrackBuffer)
{
	if (DataShare->TAProgress == TAInGame) {
		return 0;
	}

	unsigned remoteDPID = *(unsigned*)(X86StrackBuffer->Esp + 0x14 + 8);
	int newRemoteTeamNumber = (X86StrackBuffer->Edx & 0x0f);
	PlayerStruct* remotePlayer = FindPlayerByDPID(remoteDPID);
	if (remotePlayer == NULL)
	{
		return 0;
	}

	int* PTR = (int*)0x00511de8;
	TAdynmemStruct* ta = (TAdynmemStruct*)(*PTR);

	for (int n = 0; n < 10; ++n)
	{
		// for every local active player
		if (!ta->Players[n].PlayerActive || ta->Players[n].DirectPlayID == remoteDPID ||
			ta->Players[n].My_PlayerType != Player_LocalHuman && ta->Players[n].My_PlayerType != Player_LocalAI)
		{
			continue;
		}
		PlayerStruct* localPlayer = &ta->Players[n];

		bool wasSameTeam = remotePlayer->AllyTeam < 5 && remotePlayer->AllyTeam == localPlayer->AllyTeam;
		bool nowSameTeam = newRemoteTeamNumber < 5 && newRemoteTeamNumber == localPlayer->AllyTeam;

		if (wasSameTeam != nowSameTeam)
		{
			// send an ALLY23 message
			localPlayer->AllyFlagAry[remotePlayer->PlayerAryIndex] = nowSameTeam;
			char buffer[14];
			buffer[0] = 0x23;	// ally
			*(unsigned int*)&buffer[1] = localPlayer->DirectPlayID;
			*(unsigned int*)&buffer[5] = remotePlayer->DirectPlayID;
			buffer[9] = nowSameTeam;
			*(unsigned int*)&buffer[10] = 0;

			// to the remote player
			//HAPI_SendBuf(localPlayer->DirectPlayID, remoteDPID, buffer, sizeof(buffer));
			for (int m = 0; m < 10; ++m)
			{
				if (ta->Players[m].PlayerActive) //ta->Players[m].PlayerNum == 1 && ta->Players[m].My_PlayerType == Player_RemoteHuman)
				{
					// and to the host
					HAPI_SendBuf(localPlayer->DirectPlayID, ta->Players[m].DirectPlayID, buffer, sizeof(buffer));
				}
			}
		}
	}

	return 0;
}

// In Send_PacketPlayerInfo, right after broadcasting PLAYER_STATUS(0x20).
// Send ALLY(0x23) messages for every player pair
// Host especially needs to know who's with who because host assigns start positions based on alliances
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

		// send an ALLY23 message
		char buffer[14];
		buffer[0] = 0x23;	// ally
		*(unsigned int*)&buffer[1] = localPlayer.DirectPlayID;
		*(unsigned int*)&buffer[5] = remotePlayer.DirectPlayID;
		buffer[9] = isAllied;
		*(unsigned int*)&buffer[10] = 0;

		// to the remote player
		//HAPI_SendBuf(localPlayer.DirectPlayID, remotePlayer.DirectPlayID, buffer, sizeof(buffer));
		for (int m = 0; m < 10; ++m)
		{
			if (ta->Players[m].PlayerActive) // && ta->Players[m].PlayerNum == 1 && ta->Players[m].My_PlayerType == Player_RemoteHuman)
			{
				// and to the host
				HAPI_SendBuf(localPlayer.DirectPlayID, ta->Players[m].DirectPlayID, buffer, sizeof(buffer));
			}
		}
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
	m_hooks.push_back(std::make_unique<InlineSingleHook>(TeamBugfixHookAddr, 5, INLINE_5BYTESLAGGERJMP, TeamBugfixHookProc));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(RemoteAllianceRequestHookAddr, 5, INLINE_5BYTESLAGGERJMP, RemoteAllianceRequestHookProc));
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
