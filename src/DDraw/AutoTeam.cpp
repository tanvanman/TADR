#include "AutoTeam.h"
#include "BattleroomCommands.h"
#include "tafunctions.h"
#include "StartPositions.h"
#include "hook/hook.h"

#include <numeric>

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
	std::uint32_t dpidFrom;			//1
	std::uint32_t dpidTo;			//5
	std::uint8_t alliedFromWithTo;	//9
	std::uint32_t alliedToWithFrom;	//10
									//14
};

static void EnsurePlayersAllianceState(PlayerStruct& p1, PlayerStruct& p2, bool isAllied)
{
	p1.AllyFlagAry[p2.PlayerAryIndex] = isAllied;
	p2.AllyFlagAry[p1.PlayerAryIndex] = isAllied;

	AllianceMessage msg;
	msg.id23 = 0x23;
	msg.dpidFrom = p1.DirectPlayID;
	msg.dpidTo = p2.DirectPlayID;
	msg.alliedFromWithTo = isAllied;
	msg.alliedToWithFrom = isAllied;
	HAPI_BroadcastMessage(p1.DirectPlayID, (const char*)&msg, sizeof(msg));
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
			EnsurePlayersAllianceState(ta->Players[n], ta->Players[m], isAllied);
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
		}
		else if (player->My_PlayerType == Player_RemoteHuman || player->My_PlayerType == Player_RemoteAI)
		{
			char buffer[6];
			buffer[0] = 0x24;
			*(std::uint32_t*)&buffer[1] = player->DirectPlayID;
			buffer[5] = teamNumber;
			std::uint32_t fromDpid = ta->Players[ta->LocalHumanPlayer_PlayerID].DirectPlayID;
			HAPI_BroadcastMessage(fromDpid, buffer, sizeof(buffer));
		}
	}
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
