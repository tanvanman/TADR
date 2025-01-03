#include "MultiplayerSchemaUnits.h"
#include "tamem.h"
#include "iddrawsurface.h"
#include "hook/hook.h"
#include "tafunctions.h"
#include "StartPositions.h"
#include "BattleroomCommands.h"
#include "GameTickHook.h"

#include <set>
#include <string>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

static UnitStruct* DoSpawnUnit(PlayerStruct *targetPlayer, MissionUnitsStruct* missionUnit, int playerUnitNumberOverride = 0)
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;

	std::string unitName = missionUnit->Unitname;
	int unitInfoId = -1;
	for (int i = 0; i < taPtr->UNITINFOCount; ++i) {
		if (std::string(taPtr->UnitDef[i].UnitName) == unitName) {
			unitInfoId = i;
			break;
		}
	}

	{
		int missionUnitIndex = std::distance(taPtr->GameingState_Ptr->uniqueIdentifiers, missionUnit);
		const char* identity = missionUnit->Ident ? missionUnit->Ident : "<null>";
		const char* initialMission = missionUnit->InitialMission ? missionUnit->InitialMission : "<null>";
		IDDrawSurface::OutptTxt("[DoSpawnUnit] unitNumber=%d unitInfoId=%d, player=%d unitName=%s identity=%s mission=%s",
			missionUnitIndex, unitInfoId, int(missionUnit->Player), missionUnit->Unitname, identity, initialMission);
	}

	if (unitInfoId < 0)
	{
		return NULL;
	}

	unsigned x = missionUnit->XPos;
	unsigned y = missionUnit->YPos;
	unsigned z = missionUnit->ZPos;
	unsigned idx = (x >> 20) + (z >> 20) * taPtr->FeatureMapSizeX;
	if (idx < taPtr->FeatureMapSizeX * taPtr->FeatureMapSizeY) {
		FeatureStruct* f = &taPtr->FeatureMap[idx];
		y = unsigned(f->height) << 16;
	}

	int unitNumber = playerUnitNumberOverride ? playerUnitNumberOverride + targetPlayer->UnitsIndex_Begin : 0;
	UnitStruct *newUnit = UNITS_CreateUnit(targetPlayer->PlayerAryIndex, unitInfoId, x, y, z, true, 1, unitNumber);

	return newUnit;
}

static void DoParseInitialMissionCommands(int iMissionUnit, MissionUnitsStruct* missionUnit, UnitStruct *spawnedUnit, UnitStruct *allSpawnedUnits[])
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;

	if (missionUnit->InitialMission && missionUnit->InitialMission[0] != '\0')
	{
		struct
		{
			int iMissionUnit;
			UnitStruct** spawnedUnitsAry;
		}
		spawnedUnitsAry;
		spawnedUnitsAry.iMissionUnit = iMissionUnit;
		spawnedUnitsAry.spawnedUnitsAry = allSpawnedUnits;

		Campaign_ParseUnitInitialMissionCommands(spawnedUnit, missionUnit->InitialMission, (void*)&spawnedUnitsAry);
	}
}

static bool BattleroomAddAi(const std::string controlPrefix, int numClicks)
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	int availableSlot = -1;
	for (int i = 0; i < 10; ++i)
	{
		if (taPtr->Players[i].My_PlayerType == Player_none)
		{
			availableSlot = i;
			break;
		}
	}

	if (availableSlot >= 0)
	{
		// Theres a bit that needs doing and the required functionality is baked into the battleroom callback, not an encapsulated function.
		// So we'll invoke it by faking a GUI button press ...

		std::string targetControlName = controlPrefix + std::to_string(availableSlot);
		_GUI0IDControl* playerGuiControl = taPtr->desktopGUI.TheActive_GUIMEM->ControlsAry;
		int idxPlayerGuiControl = -1;
		if (playerGuiControl)
		{
			int totalGadgets = playerGuiControl->totalgadgets;
			for (int i = 1; i <= totalGadgets; ++i)
			{
				if (targetControlName == playerGuiControl[i].name)
				{
					idxPlayerGuiControl = i;
					break;
				}
			}
		}

		for (int i = 0; i < numClicks; ++i)
		{
			// cycle from "open" to "blocked" to "AI" (numClicks=2)
			taPtr->desktopGUI.UIChange_f = idxPlayerGuiControl;
			taPtr->desktopGUI.GUIUpdated_b = idxPlayerGuiControl;
			battleroom_OnCommand(&taPtr->desktopGUI);
		}
	}

	return availableSlot >= 0;
}

static unsigned int InitMissionUnitSpawnQueueAddr = 0x49759f;
static unsigned int InitMissionUnitSpawnQueueProc(PInlineX86StackBuffer X86StrackBuffer)
{
	MultiplayerSchemaUnits::GetInstance()->initMissionUnitSpawnQueue();
	return 0;
}

static unsigned int BattleroomStartButtonHookAddr = 0x44872a;
static unsigned int BattleroomStartButtonHookProc(PInlineX86StackBuffer X86StrackBuffer)
{
	static bool userNotified = false;

	if (!MultiplayerSchemaUnits::GetInstance()->isUserSpawnEnabled()) {
		return 0;
	}
	if (!MultiplayerSchemaUnits::GetInstance()->mapHasSpawnUnits())
	{
		return 0;
	}
	if (userNotified)
	{
		return 0;
	}

	bool aiAdded = false;
	if (MultiplayerSchemaUnits::GetInstance()->mapHasNeutralSpawnUnits())
	{
		TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
		bool alreadyHasAi = false;
		for (int i = 0; i < 10; ++i)
		{
			if (taPtr->Players[i].PlayerInfo->PlayerType == Player_LocalAI)
			{
				alreadyHasAi = true;
				break;
			}
		}
		if (!alreadyHasAi)
		{
			aiAdded = BattleroomAddAi("PLAYER", 2);
		}
	}

	if (aiAdded)
	{
		SendText("An AI has been added to accept neutral units for this map", 0);
		SendText("Remove the AI now if you don't want it", 0);
		SendText("Use +spawnoff to disable extra unit spawn in general", 0);
		userNotified = true;
		X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x448a62;		// discard the START command
		return X86STRACKBUFFERCHANGE;
	}

	return 0;
}

static unsigned int SkirmishSpawnPlayerCommanderHookAddr = 0x496fe1;
static unsigned int SkirmishSpawnPlayerCommanderHookProc(PInlineX86StackBuffer X86StrackBuffer)
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	if (taPtr->GameingState_Ptr->uniqueIdentifierCount == 0u) {
		return 0;
	}
	if (DataShare->PlayingDemo) {
		return 0;
	}

	int* targetPlayerIndex = (int*)(X86StrackBuffer->Esp + 0x9c + 0x04);
	int* startPosMapPlayerId = (int*)(X86StrackBuffer->Esp + 0x9c + 0x08);
	PlayerStruct* targetPlayer = &taPtr->Players[*targetPlayerIndex];

	if (MultiplayerSchemaUnits::GetInstance()->mapHasNeutralSpawnUnits() &&
		taPtr->skirmishInfo->location[0] == 0u) // random position
	{
		int* startPosArray = (int*)(X86StrackBuffer->Esp + 0x9c + 0x40);	// beware, parent scope only valid when called from "random positions" context

		int mapPositionLastHuman = -1;
		int mapPositionLastAi = -1;
		int idxLastHuman = -1;
		int idxLastAi = -1;
		for (int i = 0; i < 10; ++i)
		{
			int mapPosition = startPosArray[i];
			if (taPtr->Players[i].My_PlayerType == Player_LocalHuman && mapPosition > mapPositionLastHuman)
			{
				mapPositionLastHuman = mapPosition;
				idxLastHuman = i;
			}
			else if (taPtr->Players[i].My_PlayerType == Player_LocalAI && mapPosition > mapPositionLastAi)
			{
				mapPositionLastAi = mapPosition;
				idxLastAi = i;
			}
		}

		if (mapPositionLastAi < mapPositionLastHuman)
		{
			*startPosMapPlayerId = *startPosMapPlayerId == mapPositionLastHuman
				? mapPositionLastAi
				: mapPositionLastHuman;
			std::swap(startPosArray[idxLastHuman], startPosArray[idxLastAi]);
		}
	}

	if (MultiplayerSchemaUnits::GetInstance()->spawnInitialUnits(targetPlayer, *startPosMapPlayerId))
	{
		X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x497026;
		return X86STRACKBUFFERCHANGE;
	}
	else
	{
		return 0;
	}
}

static unsigned int MultiplayerSpawnPlayerCommanderHookAddr = 0x497794;
static unsigned int MultiplayerSpawnPlayerCommanderHookProc(PInlineX86StackBuffer X86StrackBuffer)
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	if (taPtr->GameingState_Ptr->uniqueIdentifierCount == 0u) {
		return 0;
	}
	if (DataShare->PlayingDemo) {
		return 0;
	}
	if (!MultiplayerSchemaUnits::GetInstance()->isUserSpawnEnabled()) {
		return 0;
	}

	PlayerStruct* targetPlayer = (PlayerStruct*)(X86StrackBuffer->Esi);

	if (MultiplayerSchemaUnits::GetInstance()->spawnInitialUnits(targetPlayer, targetPlayer->mapStartPos))
	{
		X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x4977c0;
		return X86STRACKBUFFERCHANGE;
	}
	else
	{
		return 0;
	}
}

static void BattleroomCommand_SpawnOff(const std::vector<std::string>&)
{
	MultiplayerSchemaUnits::GetInstance()->setUserSpawnEnabled(false);
	SendText("Unit spawn is disabled ...", 0);
}

static void BattleroomCommand_SpawnOn(const std::vector<std::string>&)
{
	MultiplayerSchemaUnits::GetInstance()->setUserSpawnEnabled(true);
	SendText("Unit spawn is enabled ...", 0);
}

static void SpawnLaterUnits(int gameTime)
{
	MultiplayerSchemaUnits::GetInstance()->spawnLaterUnits(gameTime);
}

std::unique_ptr<MultiplayerSchemaUnits> MultiplayerSchemaUnits::m_instance;
MultiplayerSchemaUnits* MultiplayerSchemaUnits::GetInstance()
{
	if (!m_instance)
	{
		m_instance.reset(new MultiplayerSchemaUnits());
	}
	return m_instance.get();
}

MultiplayerSchemaUnits::MultiplayerSchemaUnits():
	m_spawnEnabled(true),
	m_spawnQueueIterator(m_spawnQueue.end()),
	m_neutralPlayer(NULL)
{
	std::fill(m_startPositionsByPlayer, m_startPositionsByPlayer + 10, -1);
	std::fill(m_playersByStartPosition, m_playersByStartPosition + 10, -1);
	m_hooks.push_back(std::make_shared<InlineSingleHook>(SkirmishSpawnPlayerCommanderHookAddr, 5, INLINE_5BYTESLAGGERJMP, SkirmishSpawnPlayerCommanderHookProc));
	m_hooks.push_back(std::make_shared<InlineSingleHook>(MultiplayerSpawnPlayerCommanderHookAddr, 5, INLINE_5BYTESLAGGERJMP, MultiplayerSpawnPlayerCommanderHookProc));
	m_hooks.push_back(std::make_shared<InlineSingleHook>(BattleroomStartButtonHookAddr, 5, INLINE_5BYTESLAGGERJMP, BattleroomStartButtonHookProc));
	m_hooks.push_back(std::make_shared<InlineSingleHook>(InitMissionUnitSpawnQueueAddr, 5, INLINE_5BYTESLAGGERJMP, InitMissionUnitSpawnQueueProc));

	GameTickHook::GetInstance()->addCallback(&SpawnLaterUnits);

	BattleroomCommands::GetInstance()->RegisterCommand("+spawnoff", &BattleroomCommand_SpawnOff);
	BattleroomCommands::GetInstance()->RegisterCommand("+spawnon", &BattleroomCommand_SpawnOn);
}

MultiplayerSchemaUnits::~MultiplayerSchemaUnits()
{
}

bool MultiplayerSchemaUnits::isUserSpawnEnabled()
{
	return m_spawnEnabled;
}

void MultiplayerSchemaUnits::setUserSpawnEnabled(bool enabled)
{
	m_spawnEnabled = enabled;
}

bool MultiplayerSchemaUnits::mapHasSpawnUnits()
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	return taPtr->GameingState_Ptr->uniqueIdentifierCount > 0;
}

bool MultiplayerSchemaUnits::mapHasNeutralSpawnUnits()
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	for (int iMissionUnit = 0; iMissionUnit < taPtr->GameingState_Ptr->uniqueIdentifierCount; ++iMissionUnit)
	{
		MissionUnitsStruct* missionUnit = &taPtr->GameingState_Ptr->uniqueIdentifiers[iMissionUnit];
		if (missionUnit->Player == 11) {
			return true;
		}
	}
	return false;
}

void MultiplayerSchemaUnits::initMissionUnitSpawnQueue(void)
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;

	m_spawnQueue.clear();
	for (int iMissionUnit = 0; iMissionUnit < taPtr->GameingState_Ptr->uniqueIdentifierCount; ++iMissionUnit)
	{
		MissionUnitsStruct* missionUnit = &taPtr->GameingState_Ptr->uniqueIdentifiers[iMissionUnit];
		if (missionUnit->Unitname[0] != '\0' && missionUnit->creationCountdown > 0)
		{
			m_spawnQueue.push_back(iMissionUnit);
		}
	}

	std::sort(m_spawnQueue.begin(), m_spawnQueue.end(), [](int iLhs, auto& iRhs) {
		TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
		MissionUnitsStruct* missionUnitLhs = &taPtr->GameingState_Ptr->uniqueIdentifiers[iLhs];
		MissionUnitsStruct* missionUnitRhs = &taPtr->GameingState_Ptr->uniqueIdentifiers[iRhs];
		return missionUnitLhs->creationCountdown < missionUnitRhs->creationCountdown;
	});

	m_spawnQueueIterator = m_spawnQueue.begin();
}

int MultiplayerSchemaUnits::peekNextMissionUnit(int gameTime)
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;

	int result = -1;
	if (m_spawnQueueIterator != m_spawnQueue.end() &&
		unsigned(*m_spawnQueueIterator) < taPtr->GameingState_Ptr->uniqueIdentifierCount &&
		unsigned(taPtr->GameingState_Ptr->uniqueIdentifiers[*m_spawnQueueIterator].creationCountdown) <= unsigned(gameTime))
	{
		result = *m_spawnQueueIterator;
	}
	return result;
}

void MultiplayerSchemaUnits::popMissionUnit()
{
	if (m_spawnQueueIterator != m_spawnQueue.end())
	{
		++m_spawnQueueIterator;
	}
}

bool MultiplayerSchemaUnits::spawnInitialUnits(PlayerStruct* targetPlayer, int targetPlayerPosition)
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;

	bool anyNeutralUnits = mapHasNeutralSpawnUnits();
	m_startPositionsByPlayer[targetPlayer->PlayerAryIndex] = targetPlayerPosition;
	m_playersByStartPosition[targetPlayerPosition] = targetPlayer->PlayerAryIndex;

	int countActivePlayers = 0;
	for (int i = 0; i < 10; ++i) {
		if (taPtr->Players[i].PlayerActive && taPtr->Players[i].My_PlayerType != Player_none &&
			!(taPtr->Players[i].PlayerInfo->PropertyMask & WATCH)) {
			++countActivePlayers;
		}
	}

	PlayerStruct* neutralPlayer = NULL;
	if (anyNeutralUnits &&
		targetPlayer->PlayerActive &&
		1 + targetPlayerPosition == countActivePlayers &&
		GetInferredPlayerType(targetPlayer) == Player_LocalAI &&
		!(targetPlayer->PlayerInfo->PropertyMask & WATCH))
	{
		neutralPlayer = targetPlayer;
		m_neutralPlayer = targetPlayer;
	}

	std::set<std::string> allCommanderUnitNames;
	for (int i = 0; i < 5; ++i) {
		allCommanderUnitNames.insert(taPtr->RaceSideDataAry[i].commanderUnitName);
	}

	bool anySpawnedUnits = false;
	std::vector<UnitStruct*> newUnits(taPtr->GameingState_Ptr->uniqueIdentifierCount);
	for (int iMissionUnit = 0; iMissionUnit < taPtr->GameingState_Ptr->uniqueIdentifierCount; ++iMissionUnit)
	{
		UnitStruct* newUnit = NULL;
		MissionUnitsStruct* missionUnit = &taPtr->GameingState_Ptr->uniqueIdentifiers[iMissionUnit];
		if (missionUnit->creationCountdown <= 0)
		{
			std::string unitName = missionUnit->Unitname;
			bool isCommanderUnit = allCommanderUnitNames.count(unitName);

			// Ghost-Commander bugfix only works for commanders.  90 + iMissionUnit is a workaround for other types of units
			int unitNumber = isCommanderUnit ? 0 : 90 + iMissionUnit;
			if (targetPlayer == neutralPlayer && missionUnit->Player == 11)
			{
				newUnit = DoSpawnUnit(targetPlayer, missionUnit, unitNumber);
				anySpawnedUnits = true;
			}
			else if (targetPlayer != neutralPlayer &&
				1 + targetPlayerPosition == missionUnit->Player &&
				InferredPlayerTypeIsLocal(targetPlayer) &&
				!(targetPlayer->PlayerInfo->PropertyMask & WATCH))
			{
				newUnit = DoSpawnUnit(targetPlayer, missionUnit, unitNumber);
				anySpawnedUnits = true;
			}
		}
		newUnits[iMissionUnit] = newUnit;
	}

	for (int iMissionUnit = 0; iMissionUnit < taPtr->GameingState_Ptr->uniqueIdentifierCount; ++iMissionUnit)
	{
		if (newUnits[iMissionUnit])
		{
			MissionUnitsStruct* missionUnit = &taPtr->GameingState_Ptr->uniqueIdentifiers[iMissionUnit];
			DoParseInitialMissionCommands(iMissionUnit, missionUnit, newUnits[iMissionUnit], newUnits.data());
		}
	}

	return anySpawnedUnits;
}

void MultiplayerSchemaUnits::spawnLaterUnits(int gameTime)
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;

	int gameTimeSecs = gameTime / 30;
	int iMissionUnit = peekNextMissionUnit(gameTimeSecs);
	while (unsigned(iMissionUnit) < taPtr->GameingState_Ptr->uniqueIdentifierCount)
	{
		popMissionUnit();
		MissionUnitsStruct* missionUnit = &taPtr->GameingState_Ptr->uniqueIdentifiers[iMissionUnit];
		if (missionUnit->Unitname[0] != '\0')
		{
			int idxPosition = missionUnit->Player - 1;
			int idxPlayer = unsigned(idxPosition) < 10 ? m_playersByStartPosition[idxPosition] : -1;

			PlayerStruct* targetPlayer = NULL;
			if (idxPosition == 10) {
				targetPlayer = m_neutralPlayer;
			}
			else if (unsigned(idxPlayer) < 10) {
				targetPlayer = &taPtr->Players[idxPlayer];
			}

			if (missionUnit->Player < 11 && targetPlayer == m_neutralPlayer) {
				targetPlayer = NULL;
			}

			if (targetPlayer && InferredPlayerTypeIsLocal(targetPlayer) && !(targetPlayer->PlayerInfo->PropertyMask & WATCH))
			{
				DoSpawnUnit(targetPlayer, missionUnit, 0);
			}
		}

		iMissionUnit = peekNextMissionUnit(gameTimeSecs);
	}
}
