#pragma once

#include "config.h"

#include <vector>

#if ALLIED_BUILD_QUEUE_ENABLE

#include "AlliedBuildQueueProtocol.h"

struct UnitOrdersStruct;

namespace AlliedBuildQueueSync
{
	void Install();
	bool Enabled();
	bool CanSeePlayer(int playerSlot);
	bool IsBuildOrder(const UnitOrdersStruct* order);
	void Shutdown();
	const std::vector<AlliedBuildQueueRecord>& GetPlayerQueue(int playerSlot);
}

#endif // ALLIED_BUILD_QUEUE_ENABLE
