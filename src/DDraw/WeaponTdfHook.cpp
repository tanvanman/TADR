#include "WeaponTdfHook.h"

static const DWORD kParseHookAddr = 0x42E4ABu;
static const DWORD kParseHookLen  = 6u;

// TdfFile::GetInt(char* key, int default) — __thiscall, this in ECX
typedef int (__thiscall *GetIntFn)(void*, const char*, int);
static const GetIntFn kGetInt = (GetIntFn)0x4C46C0u;

int WeaponTdfHook::Context::getInt(const char* key) const
{
    return kGetInt((void*)tdfThis, key, 0);
}

// -----------------------------------------------------------------------

WeaponTdfHook* WeaponTdfHook::m_instance = nullptr;

WeaponTdfHook& WeaponTdfHook::instance()
{
    if (!m_instance)
        m_instance = new WeaponTdfHook();
    return *m_instance;
}

void WeaponTdfHook::Register(Handler handler)
{
    instance().m_handlers.push_back(std::move(handler));
}

WeaponTdfHook::WeaponTdfHook()
{
    m_hook.reset(new InlineSingleHook(
        kParseHookAddr, kParseHookLen,
        INLINE_5BYTESLAGGERJMP,
        (InlineX86HookRouter)ParseRouter));
}

int __stdcall WeaponTdfHook::ParseRouter(PInlineX86StackBuffer pBuf)
{
    Context ctx { pBuf->Ebx, (LPVOID)pBuf->Ebp };
    for (auto& h : m_instance->m_handlers)
        h(ctx);
    return 0;
}
