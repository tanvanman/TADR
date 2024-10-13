#include "MultiplayerSchemaUnits.h"
#include "tamem.h"
#include "iddrawsurface.h"
#include "hook/hook.h"
#include "tafunctions.h"
#include "StartPositions.h"
#include "BattleroomCommands.h"

#include <set>
#include <string>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

std::unique_ptr<MultiplayerSchemaUnits> MultiplayerSchemaUnits::m_instance;
MultiplayerSchemaUnits* MultiplayerSchemaUnits::GetInstance()
{
	if (!m_instance)
	{
		m_instance.reset(new MultiplayerSchemaUnits());
	}
	return m_instance.get();
}

static bool SpawnUnits(PlayerStruct* targetPlayer, int targetPlayerPosition)
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;

	int countActivePlayers = 0;
	for (int i = 0; i < 10; ++i) {
		if (taPtr->Players[i].PlayerActive && taPtr->Players[i].My_PlayerType != Player_none &&
			!(taPtr->Players[i].PlayerInfo->PropertyMask & WATCH)) {
			++countActivePlayers;
		}
	}

	bool anyNeutralUnits = MultiplayerSchemaUnits::GetInstance()->mapHasNeutralSpawnUnits();

	PlayerStruct* neutralPlayer = NULL;
	if (anyNeutralUnits &&
		targetPlayer->PlayerActive &&
		1 + targetPlayerPosition == countActivePlayers &&
		GetInferredPlayerType(targetPlayer) == Player_LocalAI &&
		!(targetPlayer->PlayerInfo->PropertyMask & WATCH))
	{
		neutralPlayer = targetPlayer;
	}

	std::set<std::string> allCommanderUnitNames;
	for (int i = 0; i < 5; ++i) {
		allCommanderUnitNames.insert(taPtr->RaceSideDataAry[i].commanderUnitName);
	}

	bool anySpawnedUnits = false;
	for (int iMissionUnit = 0; iMissionUnit < taPtr->GameingState_Ptr->uniqueIdentifierCount; ++iMissionUnit)
	{
		MissionUnitsStruct* missionUnit = &taPtr->GameingState_Ptr->uniqueIdentifiers[iMissionUnit];

		std::string unitName = missionUnit->Unitname;
		bool isCommanderUnit = allCommanderUnitNames.count(unitName);
		if (isCommanderUnit) {
			//unitName = taPtr->RaceSideDataAry[targetPlayer->PlayerInfo->RaceSide].commanderUnitName;
		}
		int unitInfoId = -1;
		for (int i = 0; i < taPtr->UNITINFOCount; ++i) {
			if (std::string(taPtr->UnitDef[i].UnitName) == unitName) {
				unitInfoId = i;
			}
		}

		unsigned x = missionUnit->XPos;
		unsigned y = missionUnit->YPos;
		unsigned z = missionUnit->ZPos;
		unsigned idx = (x >> 20) + (z >> 20) * taPtr->FeatureMapSizeX;
		if (idx < taPtr->FeatureMapSizeX * taPtr->FeatureMapSizeY) {
			FeatureStruct* f = &taPtr->FeatureMap[idx];
			y = unsigned(f->height) << 16;
		}

		// Ghost-Commander bugfix only works for commanders.  90 + iMissionUnit is a workaround for other types of units
		int unitNumber = isCommanderUnit ? 0 : 90 + iMissionUnit + targetPlayer->UnitsIndex_Begin;
		if (targetPlayer == neutralPlayer && missionUnit->Player == 11)
		{
			UNITS_CreateUnit(targetPlayer->PlayerAryIndex, unitInfoId, x, y, z, true, 1, unitNumber);
			anySpawnedUnits = true;
		}
		else if (targetPlayer != neutralPlayer &&
			1 + targetPlayerPosition == missionUnit->Player &&
			InferredPlayerTypeIsLocal(targetPlayer) &&
			!(targetPlayer->PlayerInfo->PropertyMask & WATCH))
		{
			UNITS_CreateUnit(targetPlayer->PlayerAryIndex, unitInfoId, x, y, z, true, 1, unitNumber);
			anySpawnedUnits = true;
		}
	}

	return anySpawnedUnits;
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

	int positionNumber = targetPlayer->mapStartPos;
	for (int i = 0; i < 10; ++i)
	{
		if (taPtr->GameingState_Ptr->mapStartPosAry_[i].validStartMapPos &&
			taPtr->GameingState_Ptr->mapStartPosAry_[i].playerId == *startPosMapPlayerId)
		{
			positionNumber = i;
		}
	}

	if (SpawnUnits(targetPlayer, positionNumber))
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

	if (SpawnUnits(targetPlayer, targetPlayer->mapStartPos))
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

MultiplayerSchemaUnits::MultiplayerSchemaUnits():
	m_spawnEnabled(true)
{
	m_hooks.push_back(std::make_shared<InlineSingleHook>(SkirmishSpawnPlayerCommanderHookAddr, 5, INLINE_5BYTESLAGGERJMP, SkirmishSpawnPlayerCommanderHookProc));
	m_hooks.push_back(std::make_shared<InlineSingleHook>(MultiplayerSpawnPlayerCommanderHookAddr, 5, INLINE_5BYTESLAGGERJMP, MultiplayerSpawnPlayerCommanderHookProc));
	m_hooks.push_back(std::make_shared<InlineSingleHook>(BattleroomStartButtonHookAddr, 5, INLINE_5BYTESLAGGERJMP, BattleroomStartButtonHookProc));

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
