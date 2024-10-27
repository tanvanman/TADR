#include "GameTickHook.h"
#include "tamem.h"
#include "iddrawsurface.h"
#include "hook/hook.h"

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

std::unique_ptr<GameTickHook> GameTickHook::m_instance;
GameTickHook* GameTickHook::GetInstance()
{
	if (!m_instance)
	{
		m_instance.reset(new GameTickHook());
	}
	return m_instance.get();
}



static unsigned int GameTickHookAddr = 0x4969cb;
static unsigned int GameTickHookProc(PInlineX86StackBuffer X86StrackBuffer)
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	int gameTime = taPtr->GameTime;
	for (auto& f : GameTickHook::GetInstance()->getCallbacks())
	{
		f(gameTime);
	}
	return 0;
}

GameTickHook::GameTickHook()
{
	m_hooks.push_back(std::make_shared<InlineSingleHook>(GameTickHookAddr, 5, INLINE_5BYTESLAGGERJMP, GameTickHookProc));
}

GameTickHook::~GameTickHook()
{
}

void GameTickHook::addCallback(void (*f)(int))
{
	m_callbacks.push_back(f);
}

const std::vector<void(*)(int)> & GameTickHook::getCallbacks()
{
	return m_callbacks;
}
