#pragma once

#include <cinttypes>
#include <memory>
#include <random>
#include <vector>
#include <windows.h>


class SingleHook;


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

private:
	MultiplayerSchemaUnits();

	static std::unique_ptr<MultiplayerSchemaUnits> m_instance;
	std::vector<std::shared_ptr<SingleHook> > m_hooks;

	bool m_spawnEnabled;
};
