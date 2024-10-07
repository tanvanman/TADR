#pragma once

#include "tamem.h"

class SingleHook;

#include <memory>
#include <vector>

#undef min
#undef max

class TenPlayerReplay
{
public:
	TenPlayerReplay();
	static TenPlayerReplay* GetInstance();

private:
	static std::unique_ptr<TenPlayerReplay> m_instance;
	std::vector< std::unique_ptr<SingleHook> > m_hooks;
};
