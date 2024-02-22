#include "BattleroomCommands.h"
#include "hook/hook.h"
#include "tamem.h"
#include <sstream>
#include <vector>

std::unique_ptr<BattleroomCommands> BattleroomCommands::m_instance;

// static unsigned int battleroomCommandHookAddr = 0x448421;	// before the command is echoed in chat
static unsigned int battleroomCommandHookAddr = 0x448479;		// after the command is echoed in chat
static unsigned int battleroomCommandHookProc(PInlineX86StackBuffer X86StrackBuffer)
{
	int* PTR = (int*)0x00511de8;
	TAdynmemStruct* ta = (TAdynmemStruct*)(*PTR);
	const char* command = (const char*)X86StrackBuffer->Ebx;
	BattleroomCommands::GetInstance()->RunCommand(command);
	return 0;
}

BattleroomCommands::BattleroomCommands()
{
	m_battleroomCommandHook.reset(new InlineSingleHook(battleroomCommandHookAddr, 5, INLINE_5BYTESLAGGERJMP, battleroomCommandHookProc));
}

BattleroomCommands::~BattleroomCommands()
{
}

BattleroomCommands* BattleroomCommands::GetInstance()
{
	if (!m_instance)
	{
		m_instance.reset(new BattleroomCommands());
	}
	return m_instance.get();
}

void BattleroomCommands::RegisterCommand(const char* name, void(*function)(const std::vector<std::string> &))
{
	m_commands[name] = function;
}

void BattleroomCommands::RunCommand(const char* cmd)
{
	if (cmd[0] != '+' && cmd[0] != '.')
	{
		return;
	}

	std::istringstream buffer(cmd);
	std::vector<std::string> tokens{ 
		std::istream_iterator<std::string>(buffer),
		std::istream_iterator<std::string>()
	};

	auto it = m_commands.find(tokens[0]);
	if (it != m_commands.end())
	{
		it->second(tokens);
	}
}
