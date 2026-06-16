#pragma once

#include "tamem.h"

class SingleHook;

#include <memory>
#include <vector>

#undef min
#undef max

struct PlayerStruct;

// Backup override for the funnel player's income/expense arrows, called from the
// economy-sim tail (RemoveSharedResourcesFromTotalProc @0x401bae). No-op unless this
// is the funnel slot in a 10-player replay with a sampled rate. See TenPlayerReplay.cpp.
void TenPlayerReplay_OverrideFunnelIncome(PlayerStruct* player);

class TenPlayerReplay
{
public:
	TenPlayerReplay();
	static TenPlayerReplay* GetInstance();

private:
	static std::unique_ptr<TenPlayerReplay> m_instance;
	std::vector< std::unique_ptr<SingleHook> > m_hooks;
};
