#pragma once

#include <memory>
#include <functional>
#include <vector>
#include "hook/hook.h"

// Single hook into LoadWeaponTdf that dispatches to all registered handlers.
// Handlers are registered at install time (DLL_PROCESS_ATTACH) before the game
// loads any weapon TDFs. The hook is installed lazily on the first Register() call.
//
// Hook site: LoadWeaponTdf @ 0x42E4AB
//   Opcode: 8D 4D 20  51  8B CB  -- 6 bytes, all position-independent
//           lea ecx,[ebp+20h]  push ecx  mov ecx,ebx
//   ebx = TdfFile* (loaded @ 0x42E447, unchanged to this point)
//   ebp = WeaponTypedef* (set @ 0x42E489, unchanged to this point)
//   TdfFile::GetInt is __thiscall: this->ecx, args right-to-left on stack, callee cleans.
class WeaponTdfHook
{
public:
    struct Context
    {
        DWORD    tdfThis;    // TdfFile*
        LPVOID   pWeaponDef; // WeaponTypedef*

        // Read an integer field from the weapon TDF (0 if absent).
        int getInt(const char* key) const;
    };

    using Handler = std::function<void(const Context&)>;

    // Register a handler to be called for every weapon TDF that is loaded.
    // Installs the hook on the first call.
    static void Register(Handler handler);

private:
    WeaponTdfHook();
    static WeaponTdfHook& instance();
    static WeaponTdfHook* m_instance;

    std::unique_ptr<InlineSingleHook> m_hook;
    std::vector<Handler>              m_handlers;

    static int __stdcall ParseRouter(PInlineX86StackBuffer pBuf);
};
