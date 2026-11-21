#pragma once
#include "config.h"
#include <Windows.h>
#include <mutex>

struct UnitStruct;
typedef struct _Position_Dword Position_Dword;

namespace PrependOrders
{
    // Ctrl+Shift is sampled when a world command is issued. Keyboard shortcuts
    // and selection remain native; custom bindings can use an extra key.
    bool ModifierDown();
    bool ModifierForMouse(unsigned keys);
    bool Active();
    bool ModifierKey(UINT key);
    void Install();
    bool KeyMessage(UINT message, WPARAM key);
    bool CommandMessage(UINT message);

    class Batch
    {
    public:
        explicit Batch(bool enabled, bool reuseActive = false);
        ~Batch();
        Batch(const Batch&) = delete;
        Batch& operator=(const Batch&) = delete;
    private:
        std::unique_lock<std::recursive_mutex> m_lock;
        void* m_previous;
        bool m_previousRequested;
        void* m_context;
    };

    void __stdcall Dispatch(unsigned action, int shift, UnitStruct* unit,
        UnitStruct* target, const Position_Dword* position, int param1, int param2);
}
