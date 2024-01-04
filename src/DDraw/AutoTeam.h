#pragma once

#include <memory>
#include <vector>

class SingleHook;

class AutoTeam
{
public:
	~AutoTeam();

	static void Install();

private:
	AutoTeam();
	static std::unique_ptr<AutoTeam> m_instance;
	std::vector<std::unique_ptr<SingleHook> > m_hooks;
};
