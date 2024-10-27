#pragma once

#include <cinttypes>
#include <memory>
#include <random>
#include <vector>
#include <windows.h>

class SingleHook;
struct MissionUnitsStruct;
struct PlayerStruct;

// spawns units for the first AI player as defined in the OTA file's schema
class MultiplayerSchemaUnits
{
public:
	// Create and Get the instance
	static MultiplayerSchemaUnits* GetInstance();
	~MultiplayerSchemaUnits();

	bool mapHasSpawnUnits();
	bool mapHasNeutralSpawnUnits();

	bool isUserSpawnEnabled();
	void setUserSpawnEnabled(bool enabled);

	void initMissionUnitSpawnQueue(void);
	int peekNextMissionUnit(int gameTime); // index into taPtr->GameingState_Ptr->uniqueIdentifiers, or -1
	void popMissionUnit();

	bool spawnInitialUnits(PlayerStruct* targetPlayer, int targetPlayerPosition);
	void spawnLaterUnits(int gameTime);

private:
	MultiplayerSchemaUnits();

	static std::unique_ptr<MultiplayerSchemaUnits> m_instance;

	std::vector<std::shared_ptr<SingleHook> > m_hooks;
	bool m_spawnEnabled;
	std::vector<int> m_spawnQueue;	// indices into mission units sorted by CreationCountdown
	std::vector<int>::iterator m_spawnQueueIterator;
	int m_startPositionsByPlayer[10];
	int m_playersByStartPosition[10];
	PlayerStruct* m_neutralPlayer;
};
