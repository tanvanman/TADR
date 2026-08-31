#pragma once

#include "config.h"

#include <vector>

#if ALLIED_BUILD_QUEUE_ENABLE

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

#endif // ALLIED_BUILD_QUEUE_ENABLE
