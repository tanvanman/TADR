#pragma once

#include <memory>
#include <vector>

class SingleHook;

class GameTickHook
{
public:
	// Create and Get the instance
	static GameTickHook* GetInstance();
	~GameTickHook();

	void addCallback(void (*)(int));
	const std::vector<void (*)(int)> & getCallbacks();

private:
	GameTickHook();

	static std::unique_ptr<GameTickHook> m_instance;
	std::vector<std::shared_ptr<SingleHook> > m_hooks;
	std::vector<void (*)(int)> m_callbacks;
};
