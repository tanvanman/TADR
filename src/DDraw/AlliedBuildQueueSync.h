#pragma once

#include <vector>

struct AlliedBuildQueueRecord
{
	unsigned short buildUnitId;
	unsigned short x;
	unsigned short y;
	unsigned short z;
	unsigned char rotation;
};

namespace AlliedBuildQueueSync
{
	void Install();
	void Shutdown();
	const std::vector<AlliedBuildQueueRecord>& GetPlayerQueue(int playerSlot);
}
