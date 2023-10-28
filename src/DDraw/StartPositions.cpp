#include "StartPositions.h"
#include "tamem.h"
#include "iddrawsurface.h"
#include "hook/hook.h"
#include "tafunctions.h"

#ifdef max
#undef max
#endif
#include <algorithm>
#include <chrono>

static unsigned int TeamBugfixHookAddr = 0x452ac7;
static unsigned int TeamBugfixHookProc(PInlineX86StackBuffer X86StrackBuffer)
{
	int* PTR = (int*)0x00511de8;
	TAdynmemStruct* ta = (TAdynmemStruct*)(*PTR);

	unsigned remoteDPID = *(unsigned*)(X86StrackBuffer->Esp + 0x14 + 4);
	unsigned localDPID = *(unsigned*)(X86StrackBuffer->Esp + 0x14 + 8);

	char buffer[14];
	buffer[0] = 0x23;	// ally
	*(unsigned int*)&buffer[1] = localDPID;
	*(unsigned int*)&buffer[5] = remoteDPID;
	buffer[9] = (unsigned char)X86StrackBuffer->Edx;
	*(unsigned int*)&buffer[10] = *(unsigned*)(X86StrackBuffer->Esp + 0x14 + 16);
	HAPI_SendBuf(localDPID, remoteDPID, buffer, sizeof(buffer));
	return 0;
}

std::unique_ptr<StartPositions> StartPositions::m_instance;
std::default_random_engine StartPositions::m_rng(std::chrono::system_clock::now().time_since_epoch().count());

// initialised by InitStartPositionsHookProc, accessed by FixedStartPositionsHookProc or RandomStartPositionsHookProc
static int IS_ACTIVE_PLAYER[10];
static int START_POSITIONS[10];
static bool START_POSITIONS_VALID = false;

static unsigned int InitStartPositionsHookAddr = 0x45698f;
static unsigned int InitStartPositionsHookProc(PInlineX86StackBuffer X86StrackBuffer)
{
	StartPositions* sp = StartPositions::GetInstance();
	if (!sp)
	{
		return 0;
	}

	int* PTR = (int*)0x00511de8;
	TAdynmemStruct* ta = (TAdynmemStruct*)(*PTR);

	int idx = ta->LocalHumanPlayer_PlayerID;
	bool randomised = !(ta->Players[idx].PlayerInfo->PropertyMask & PlayerPropertyMask::FIXEDSTARTPOS);

	int* isActivePlayer = IS_ACTIVE_PLAYER;
	int* startPositions = START_POSITIONS;

	StartPositions::GetInstance()->InitStartPositions(isActivePlayer, startPositions, randomised);
	START_POSITIONS_VALID = true;
	return 0;
}

// Hook at the point where multiplayer fixed positions are assigned.
static unsigned int FixedStartPositionsHookAddr = 0x4569da;
static unsigned int FixedStartPositionsHookProc(PInlineX86StackBuffer X86StrackBuffer)
{
	if (!StartPositions::GetInstance())
	{
		return 0;
	}
	X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x4569e5;

	PlayerInfoStruct* playerInfo = (PlayerInfoStruct*)X86StrackBuffer->Ecx;
	PlayerStruct* player = (PlayerStruct*)(X86StrackBuffer->Edx + X86StrackBuffer->Eax + 0x1b63);

	*(char*)X86StrackBuffer->Esi = START_POSITIONS[player->PlayerAryIndex];
	return X86STRACKBUFFERCHANGE;
}

// Hook at the point where multiplayer random positions are assigned.
static unsigned int RandomStartPositionsHookAddr = 0x456a45;
static unsigned int RandomStartPositionsHookProc(PInlineX86StackBuffer X86StrackBuffer)
{
	if (!StartPositions::GetInstance())
	{
		return 0;
	}
	X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x456a4b;

	PlayerStruct* player = (PlayerStruct*)(X86StrackBuffer->Eax - 0x73);
	*(char*)X86StrackBuffer->Edi = START_POSITIONS[player->PlayerAryIndex];
	X86StrackBuffer->Edi += 4;
	return X86STRACKBUFFERCHANGE;
}

StartPositions* StartPositions::GetInstance()
{
	if (!m_instance)
	{
		m_instance.reset(new StartPositions());
	}
	return m_instance.get();
}

StartPositions::StartPositions():
	m_startPositionsShare(NULL),
	m_hMemMap(NULL)
{
	CreateSharedMemory();
	if (m_hMemMap && m_startPositionsShare)
	{
		m_hooks.push_back(std::make_shared<InlineSingleHook>(InitStartPositionsHookAddr, 5, INLINE_5BYTESLAGGERJMP, InitStartPositionsHookProc));
		m_hooks.push_back(std::make_shared<InlineSingleHook>(FixedStartPositionsHookAddr, 5, INLINE_5BYTESLAGGERJMP, FixedStartPositionsHookProc));
		m_hooks.push_back(std::make_shared<InlineSingleHook>(RandomStartPositionsHookAddr, 5, INLINE_5BYTESLAGGERJMP, RandomStartPositionsHookProc));
		m_hooks.push_back(std::make_shared<InlineSingleHook>(TeamBugfixHookAddr, 5, INLINE_5BYTESLAGGERJMP, TeamBugfixHookProc));
	}
}

StartPositions::~StartPositions()
{
	if (m_hMemMap != NULL)
	{
		UnmapViewOfFile(m_hMemMap);
		CloseHandle(m_hMemMap);
	}
	m_hMemMap = m_startPositionsShare = NULL;
}

void StartPositions::CreateSharedMemory()
{
	m_hMemMap = CreateFileMapping((HANDLE)0xFFFFFFFF,
		NULL,
		PAGE_READWRITE,
		0,
		sizeof(StartPositionsData),
		"TADemo-StartPositions");

	bool bExists = (GetLastError() == ERROR_ALREADY_EXISTS);

	void* mem = MapViewOfFile(m_hMemMap,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		sizeof(StartPositionsData));

	if (!bExists)
	{
		memset(mem, 0, sizeof(StartPositionsData));
	}

	m_startPositionsShare = static_cast<StartPositionsData*>(mem);
}

StartPositionsData* StartPositions::GetSharedMemory()
{
	return m_startPositionsShare;
}

const void StartPositions::InitStartPositions(int isActivePlayer[10], int startPositions[10], bool randomised)
{
	int playerTeamNumbers[10] = { 0 };
	GetTeamsFromAlliances(playerTeamNumbers, randomised);

	if (CountLargestTeamSize(playerTeamNumbers) > 1)
	{
		GetStartPositionsFromTeamNumbers(playerTeamNumbers, isActivePlayer, startPositions, randomised);
	}
	else if (!randomised && GetSharedMemory() && GetSharedMemory()->positionCount > 0)
	{
		GetStartPositionsFromSharedMemory(GetSharedMemory(), isActivePlayer, startPositions);
	}
	else
	{
		GetStartPositionsSequentialy(isActivePlayer, startPositions, randomised);
	}
}

const bool StartPositions::GetInitedStartPositions(int isActivePlayer[10], int startPositions[10])
{
	std::memcpy(isActivePlayer, IS_ACTIVE_PLAYER, 10*sizeof(int));
	std::memcpy(startPositions, START_POSITIONS, 10*sizeof(int));
	return START_POSITIONS_VALID;
}

static void assignTeam(TAdynmemStruct* ta, bool visited[10], int playerTeamNumbers[10], int player, int teamNumber) {
	visited[player] = true;
	playerTeamNumbers[player] = teamNumber;

	for (int i = 0; i < 10; i++)
	{
		if (ta->Players[i].PlayerActive && !(ta->Players[i].PlayerInfo->PropertyMask & PlayerPropertyMask::WATCH) &&
			ta->Players[player].AllyFlagAry[i] &&
			// uncomment to enforce mutual alliance
			// ta->Players[i].AllyFlagAry[player] &&
			!visited[i])
		{
			assignTeam(ta, visited, playerTeamNumbers, i, teamNumber);
		}
	}
}

void StartPositions::GetTeamsFromAlliances(int playerTeamNumbers[10], bool randomise)
{
	int* PTR = (int*)0x00511de8;
	TAdynmemStruct* ta = (TAdynmemStruct*)(*PTR);

	int shuffle[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	if (randomise)
	{
		std::shuffle(shuffle, shuffle + 10, m_rng);
	}

	bool visited[10] = { false };
	int teamNumber = 1;
	for (int _i = 0; _i < 10; _i++)
	{
		int i = shuffle[_i];
		if (ta->Players[i].PlayerActive &&
			!(ta->Players[i].PlayerInfo->PropertyMask & PlayerPropertyMask::WATCH) &&
			!visited[i])
		{
			assignTeam(ta, visited, playerTeamNumbers, i, teamNumber);
			teamNumber++;
		}
	}
}

int StartPositions::CountLargestTeamSize(const int playerTeamNumbers[10])
{
	int playersPerTeam[10] = { 0 };
	int largestTeamSize = 0;
	for (int i = 0; i < 10; ++i)
	{
		int teamNumber = playerTeamNumbers[i] - 1;
		if (teamNumber >= 0)
		{
			playersPerTeam[teamNumber]++;
			largestTeamSize = std::max(largestTeamSize, playersPerTeam[teamNumber]);
		}
	}
	return largestTeamSize;
}

void StartPositions::GetStartPositionsFromTeamNumbers(const int teamNumbers[10], int isActivePlayer[10], int positions[10], bool randomise)
{
	// track which positions have been assigned
	bool isUsedPosition[10] = { false };

	int numTeams = 0;
	int teamSize[10] = { 0 };
	// initialise
	for (int i = 0; i < 10; i++)
	{
		int teamNumber = teamNumbers[i] - 1;
		positions[i] = -1;
		isUsedPosition[i] = false;
		isActivePlayer[i] = teamNumber >= 0;
		if (isActivePlayer[i])
		{
			if (teamSize[teamNumber] == 0)
			{
				numTeams++;
			}
			teamSize[teamNumber]++;
		}
	}

	// track next positions to assign to each team
	int nextPositionByTeam[10] = { -1 };
	for (int i = 0; i < numTeams; ++i)
	{
		nextPositionByTeam[i] = i;
	}

	// randomise order of assignment if required
	int shuffle[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	if (randomise)
	{
		std::shuffle(shuffle, shuffle + 10, m_rng);
	}

	// make 1st pass naive assignments.  they might out of the range >= 10
	for (int _i = 0; _i < 10; ++_i)
	{
		int i = shuffle[_i];
		if (teamNumbers[i] > 0)
		{
			int teamNumber = teamNumbers[i] - 1;
			positions[i] = nextPositionByTeam[teamNumber];
			nextPositionByTeam[teamNumber] += numTeams;
			if (positions[i] < 10)
			{
				isUsedPosition[positions[i]] = true;
			}
		}
	}

	// repair out of range position assignments >= 10
	for (int _i = 0; _i < 10; ++_i)
	{
		int i = shuffle[_i];
		if (teamNumbers[i] > 0 && positions[i] >= 10)
		{
			for (int j = 0; j < 10; ++j)
			{
				if (!isUsedPosition[j])
				{
					positions[i] = j;
					isUsedPosition[j] = true;
					break;
				}
			}
			positions[i] %= 10;
		}
	}
}

void StartPositions::GetStartPositionsFromSharedMemory(const StartPositionsData* sm, int isActivePlayer[10], int startPositions[10])
{
	int* PTR = (int*)0x00511de8;
	TAdynmemStruct* ta = (TAdynmemStruct*)(*PTR);

	for (int n = 0; n < 10; ++n)
	{
		isActivePlayer[n] = ta->Players[n].PlayerActive && !(ta->Players[n].PlayerInfo->PropertyMask & PlayerPropertyMask::WATCH);
		startPositions[n] = -1;
	}

	bool assignedPositions[10] = { false };
	for (int nPlayer = 0; nPlayer < 10; ++nPlayer)
	{
		if (!isActivePlayer[nPlayer])
		{
			continue;
		}
		for (int nStartPosition = 0; nStartPosition < 10; ++nStartPosition)
		{
			if (!strncmp(ta->Players[nPlayer].Name, sm->orderedPlayerNames[nStartPosition], sizeof(sm->orderedPlayerNames[nStartPosition])))
			{
				startPositions[nPlayer] = nStartPosition;
				assignedPositions[nStartPosition] = true;
				break;
			}
		}
	}
	
	for (int nPlayer = 0; nPlayer < 10; ++nPlayer)
	{
		if (!isActivePlayer[nPlayer] || startPositions[nPlayer] != -1)
		{
			continue;
		}
		for (int nStartPosition = 0; nStartPosition < 10; ++nStartPosition)
		{
			if (!assignedPositions[nStartPosition])
			{
				startPositions[nPlayer] = nStartPosition;
				assignedPositions[nStartPosition] = true;
				break;
			}
		}
		if (startPositions[nPlayer] == -1)
		{
			startPositions[nPlayer] = 0;
		}
	}
}

int StartPositions::GetStartPositionsSequentialy(int isActivePlayer[10], int startPositions[10], bool randomise)
{
	int* PTR = (int*)0x00511de8;
	TAdynmemStruct* ta = (TAdynmemStruct*)(*PTR);

	for (int i = 0; i < 10; ++i)
	{
		isActivePlayer[i] = ta->Players[i].PlayerActive && !(ta->Players[i].PlayerInfo->PropertyMask & PlayerPropertyMask::WATCH);
		startPositions[i] = -1;
	}

	int shuffle[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	if (randomise)
	{
		std::shuffle(shuffle, shuffle + 10, m_rng);
	}

	int nextStartPosition = 0;
	for (int _i = 0; _i < 10; ++_i)
	{
		int i = shuffle[_i];
		if (isActivePlayer[i])
		{
			startPositions[i] = nextStartPosition++;
		}
	}
	return nextStartPosition;
}
