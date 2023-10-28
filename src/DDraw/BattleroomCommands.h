#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

class SingleHook;

class BattleroomCommands
{
public:
	~BattleroomCommands();
	static BattleroomCommands* GetInstance();

	void RegisterCommand(const char* name, void(*function)(const std::vector<std::string> &));
	void RunCommand(const char* cmd);

private:
	BattleroomCommands();

	static std::unique_ptr<BattleroomCommands> m_instance;
	std::unique_ptr<SingleHook> m_battleroomCommandHook;
	std::map<std::string, void(*)(const std::vector<std::string> &)> m_commands;
};
