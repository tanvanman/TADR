#include "config.h"
#include "PrependOrders.h"
#if PREPEND_ORDERS_ENABLE
#include "oddraw.h"
#include "tamem.h"
#include "tafunctions.h"
#include "TAConfig.h"
#include "iddrawsurface.h"
#include "unitrotate.h"
#include "hook/hook.h"
#include "PrependQueue.h"
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

namespace
{
    static_assert(sizeof(UnitOrdersStruct) == 0x56, "Native order size");
    static_assert(offsetof(UnitOrdersStruct, BuildUnitID) == 0x36, "Order parameter 1");
    static_assert(offsetof(UnitOrdersStruct, NextOrder) == 0x4a, "Native order link");
    using SendFn = void (__stdcall*)(unsigned, int, UnitStruct*, UnitStruct*, const Position_Dword*, int, int);
    const auto NativeSend = reinterpret_cast<SendFn>(0x0043AFC0u);
    const auto NativeAllocate = reinterpret_cast<void* (__cdecl*)(std::size_t)>(0x004B4F10u);
    const auto NativeFree = reinterpret_cast<void (__cdecl*)(void*)>(0x004B4F20u);
    const auto NativeConstruct = reinterpret_cast<UnitOrdersStruct* (__thiscall*)(UnitOrdersStruct*, unsigned,
        UnitStruct*, const Position_Dword*, int, int, int)>(0x0043A0C0u);
    const auto NativeDestroy = reinterpret_cast<void (__thiscall*)(UnitOrdersStruct*)>(0x0043A1F0u);
    const auto NativeRemove = reinterpret_cast<void (__stdcall*)(UnitStruct*, UnitOrdersStruct*)>(0x00439F80u);
    const auto NativeInsert = reinterpret_cast<void (__stdcall*)(UnitStruct*, UnitOrdersStruct*, UnitOrdersStruct*)>(0x0043AC60u);
    const auto NativeAction = reinterpret_cast<unsigned char* (__stdcall*)(unsigned*, unsigned,
        UnitStruct*, UnitStruct*, const Position_Dword*)>(0x0043F0E0u);

    std::recursive_mutex inputMutex;
    bool installed = false, enabled = true, mutating = false;
    unsigned modifier = 0, modifierMask = 3, successLogs = 0, failureLogs = 0;

    TAdynmemStruct* World() { return *TAmainStruct_PtrPtr; }

    bool Playable(TAdynmemStruct* ta)
    {
        if (!ta || !DataShare || DataShare->TAProgress != TAInGame
            || DataShare->PlayingDemo || DataShare->ehaOff) return false;
        const int p = ta->LocalHumanPlayer_PlayerID;
        return p >= 0 && p < 10 && ta->Players[p].PlayerInfo
            && !(ta->Players[p].PlayerInfo->PropertyMask & WATCH)
            && ta->Players[p].My_PlayerType == Player_LocalHuman;
    }

    bool InBank(UnitStruct* unit, UnitStruct* first, UnitStruct* last)
    {
        const std::uintptr_t u = reinterpret_cast<std::uintptr_t>(unit);
        const std::uintptr_t a = reinterpret_cast<std::uintptr_t>(first);
        const std::uintptr_t b = reinterpret_cast<std::uintptr_t>(last);
        return first && last && a <= b && u >= a && u <= b && (u-a) % sizeof(UnitStruct) == 0;
    }

    bool LiveOwned(TAdynmemStruct* ta, UnitStruct* unit)
    {
        if (!Playable(ta) || !unit) return false;
        PlayerStruct* owner = &ta->Players[ta->LocalHumanPlayer_PlayerID];
        return InBank(unit, owner->Units, owner->UnitsAry_End)
            && InBank(unit, ta->BeginUnitsArray_p, ta->EndOfUnitsArray_p)
            && unit->UnitID && unit->UnitType
            && unit->Owner_PlayerPtr0 == owner && (unit->UnitSelected & 0x10000000u)
            && !(unit->UnitSelected & 0x4000u) && unit->Nanoframe == 0.0f;
    }

    bool LiveTarget(TAdynmemStruct* ta, UnitStruct* unit)
    {
        if (!unit) return true;
        return ta && ta->BeginUnitsArray_p && ta->EndOfUnitsArray_p
            && InBank(unit, ta->BeginUnitsArray_p, ta->EndOfUnitsArray_p)
            && unit->UnitID && unit->UnitType && (unit->UnitSelected & 0x10000000u)
            && !(unit->UnitSelected & 0x4000u);
    }

    bool TextInputOpen(TAdynmemStruct* ta)
    {
        // The same active TALK.GUI chain used by ChatLayout's scrollback.
        // Do not intercept spaces in the native chat compose prompt.
        __try
        {
            GUIMEMSTRUCT* gui = ta->desktopGUI.TheActive_GUIMEM;
            for (unsigned hops = 0; gui && hops < 32; ++hops, gui = gui->per_active)
                if (std::memcmp(gui->GUIName, "TALK", 4) == 0) return true;
            return gui != nullptr; // Corrupt/cyclic chains must fail closed.
        }
        __except (EXCEPTION_EXECUTE_HANDLER) { return true; }
    }

    unsigned ActionFor(unsigned type, UnitStruct& unit, UnitStruct* target, const Position_Dword& pos)
    {
        unsigned value = 0;
        Position_Dword probe = pos; // Classification must not edit a queued destination.
        const unsigned char* action = NativeAction(&value, type, &unit, target, &probe);
        return action ? static_cast<unsigned>(*action) : 0;
    }

    unsigned KindFor(unsigned action, UnitStruct& unit, UnitStruct* target, const Position_Dword& pos)
    {
        if (!action || !COBSciptHandler_Begin || !COBSciptHandler_End) return 0;
        const COBHandle* begin = *COBSciptHandler_Begin;
        const COBHandle* end = *COBSciptHandler_End;
        if (!begin || !end || end <= begin || action >= static_cast<unsigned>(end-begin)) return 0;
        // Only normal user-queue actions are admitted. Stateful transport and
        // self-destruct handlers use separate continuation rules.
        const unsigned types[] = {ordertype::MOVE, ordertype::ATTACK, ordertype::DEFEND,
            ordertype::REPAIR, ordertype::PATROL, ordertype::RECLAIM,
            ordertype::CAPTURE, ordertype::BUILD, ordertype::STOP};
        for (unsigned type : types)
            if (ActionFor(type, unit, target, pos) == action) return type;
        return 0;
    }

    bool Idle(UnitStruct& unit)
    {
        const UnitOrdersStruct* head = unit.UnitOrders;
        if (!head) return true;
        if (head->Unit_ptr != &unit || head->NextOrder || head->AttackTargat
            || (head->Order_State & (0x40000u | 0x8000u))
            || !COBSciptHandler_Begin || !COBSciptHandler_End) return false;
        const COBHandle* begin = *COBSciptHandler_Begin;
        const COBHandle* end = *COBSciptHandler_End;
        if (!begin || !end || end <= begin
            || head->COBHandler_index >= static_cast<unsigned>(end-begin)) return false;
        const char* name = begin[head->COBHandler_index].technicalName;
        // Standby is a real native order with no explicit-user bit. The
        // contextual STOP command selector is not an inverse handler lookup.
        return name && (_stricmp(name, "Standby") == 0 || _stricmp(name, "VTOL_Standby") == 0
            || _stricmp(name, "Stop") == 0);
    }

    int ReadOrderParameter(const UnitOrdersStruct* order, unsigned offset)
    {
        int value;
        std::memcpy(&value, reinterpret_cast<const unsigned char*>(order)+offset, sizeof(value));
        return value;
    }

    struct NativeEngine
    {
        using Node = UnitOrdersStruct;
        using Unit = UnitStruct;
        struct Request
        {
            unsigned action = 0, kind = 0;
            Unit* target = nullptr;
            Position_Dword position = {};
            int param1 = 0, param2 = 0, param3 = 0;
            int rotation = -1;
            unsigned flags = 1;
        };

        Node* Head(Unit& unit) { return unit.UnitOrders; }
        Node* Next(Node* node) { return node->NextOrder; }
        bool Boundary(Node* node) { return (node->Order_State & 0x1000u) != 0; }
        void SetBoundary(Node* node, bool on)
        {
            if (on) node->Order_State |= 0x1000u;
            else node->Order_State &= ~0x1000u;
        }

        PrependQueue::CurrentKind SnapshotResume(Unit& unit, Node* old, Request& out)
        {
            if (old->Unit_ptr != &unit || (old->Order_State & (0x40000u | 0x8000u))
                || !LiveTarget(World(), old->AttackTargat))
                return PrependQueue::CurrentKind::Unsupported;
            out.kind = KindFor(old->COBHandler_index, unit, old->AttackTargat, old->Pos);
            if (!out.kind || out.kind == ordertype::STOP || !(old->Order_State & 1u)
                || !LiveTarget(World(), old->AttackTargat))
                return PrependQueue::CurrentKind::Unsupported;
            out.action = old->COBHandler_index;
            out.target = old->AttackTargat;
            out.position = old->Pos;
            out.param1 = ReadOrderParameter(old, 0x36);
            out.param2 = ReadOrderParameter(old, 0x3a);
            out.param3 = ReadOrderParameter(old, 0x3e);
            if (out.kind == ordertype::BUILD)
            {
                // Factory production is a separate sub-build queue, not a
                // mobile constructor's interruptible battlefield command.
                if (unit.UnitType->bmcode == 0) return PrependQueue::CurrentKind::Unsupported;
                if (out.target)
                {
                    // Native cancellation leaves even a just-created frame
                    // alive. Repair resumes construction at any progress,
                    // including Nanoframe==1 before the first build tick.
                    out.action = ActionFor(ordertype::REPAIR, unit, out.target, out.position);
                    if (!out.action) return PrependQueue::CurrentKind::Unsupported;
                    out.kind = ordertype::REPAIR;
                    out.param1 = out.param2 = out.param3 = 0;
                }
                else if (CUnitRotate* rotation = CUnitRotate::GetInstance())
                    out.rotation = rotation->TakeOrderRotation(old);
            }
            return PrependQueue::CurrentKind::Resume;
        }

        void* AllocateRaw() { return NativeAllocate(sizeof(Node)); }
        void FreeRaw(void* raw)
        {
            if (CUnitRotate* rotation = CUnitRotate::GetInstance()) rotation->ClearOrderRotation(raw);
            NativeFree(raw);
        }
        Node* Construct(Unit& unit, void* raw, const Request& request)
        {
            // Registering a rotation can allocate. Do that before native target
            // references are constructed, and before the transaction mutates
            // the queue. The native constructor itself does not allocate.
            try
            {
                if (request.rotation >= 0)
                    if (CUnitRotate* rotation = CUnitRotate::GetInstance())
                        rotation->TagOrderRotation(raw, request.rotation);
            }
            catch (...) { return nullptr; }
            Node* node = NativeConstruct(static_cast<Node*>(raw), request.action,
                request.target, &request.position, request.param1, request.param2, request.param3);
            node->Unit_ptr = &unit;
            node->Order_State |= request.flags;
            return node;
        }
        void DestroyUnlinked(Unit& unit, Node* node)
        {
            node->Unit_ptr = &unit;
            node->Order_State |= 0x10000u; // Non-current: release refs without stopping the unit.
            NativeDestroy(node);
            FreeRaw(node);
        }
        void Remove(Unit& unit, Node* node)
        {
            // Cancellation can call the running build handler; retain its
            // captured footprint/yardmap rotation until native cleanup returns.
            NativeRemove(&unit, node);
            if (CUnitRotate* rotation = CUnitRotate::GetInstance()) rotation->ClearOrderRotation(node);
        }
        void InsertBefore(Unit& unit, Node* node, Node* before) { NativeInsert(&unit, node, before); }
    };

    PrependQueue::Result InsertNative(NativeEngine& engine, UnitStruct& unit,
        const NativeEngine::Request& request, PrependQueue::Cursor<NativeEngine>& cursor)
    {
        mutating = true;
        __try { return PrependQueue::Insert(engine, unit, request, cursor); }
        __finally { mutating = false; }
    }

    struct Entry
    {
        UnitStruct* unit;
        short type;
        UnitDefStruct* definition;
        PlayerStruct* owner;
        PrependQueue::Cursor<NativeEngine> cursor;
        bool failed;
        bool appendNative;
    };
    struct Context
    {
        TAdynmemStruct* world;
        int player;
        bool warned = false;
        std::vector<Entry> entries;
        Context() : world(World()), player(world ? world->LocalHumanPlayer_PlayerID : -1) {}
    };
    Context* context = nullptr;
    bool requested = false;

    void Warn(const char* reason)
    {
        if (failureLogs < 32) { ++failureLogs; IDDrawSurface::OutptFmtTxt("[QueuePrepend] refused: %s", reason); }
        if (context && !context->warned)
        {
            context->warned = true;
            char message[] = "Cannot prepend this command while the unit is performing its current action.";
            SendText(message, 0);
        }
    }

    bool SameEntry(const Entry& entry, TAdynmemStruct* ta)
    {
        return LiveOwned(ta, entry.unit) && entry.unit->UnitID == entry.type
            && entry.unit->UnitType == entry.definition && entry.unit->Owner_PlayerPtr0 == entry.owner;
    }

    struct CallSite { unsigned address; unsigned char expected[5]; unsigned char patch[5] = {}; };
    CallSite sites[] = {
        {0x0048D1F4u, {0xE8,0xC7,0xDD,0xFA,0xFF}}, // Native battlefield/megamap commands.
        {0x004197A9u, {0xE8,0x12,0x18,0x02,0x00}}  // Native mobile build placement.
    };
    std::vector<std::unique_ptr<SingleHook>> hooks;

    bool CodeMatches(unsigned address, const unsigned char* expected)
    {
        __try { return std::memcmp(reinterpret_cast<void*>(address), expected, 5) == 0; }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }
}

namespace PrependOrders
{
    bool Active() { return requested && context; }

    bool ModifierDown()
    {
        if (!installed || !enabled || !Playable(World()) || TextInputOpen(World())) return false;
        if (modifier && !(GetAsyncKeyState(modifier) & 0x8000)) return false;
        if ((modifierMask & 1u) && !(GetAsyncKeyState(VK_CONTROL) & 0x8000)) return false;
        if ((modifierMask & 2u) && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) return false;
        if ((modifierMask & 4u) && !(GetAsyncKeyState(VK_MENU) & 0x8000)) return false;
        return modifier || modifierMask;
    }
    bool ModifierKey(UINT key)
    {
        return installed && enabled && ((modifier && key == modifier)
            || ((modifierMask & 1u) && key == VK_CONTROL)
            || ((modifierMask & 2u) && key == VK_SHIFT)
            || ((modifierMask & 4u) && key == VK_MENU));
    }

    bool ModifierForMouse(unsigned keys)
    {
        if (!installed || !enabled || !Playable(World()) || TextInputOpen(World())) return false;
        if (modifier && !(GetAsyncKeyState(modifier) & 0x8000)) return false;
        // TA queues native clicks for its next input update. The Ctrl/Shift
        // state belongs to that saved click, even if either key is now up.
        if ((modifierMask & 1u) && !(keys & MK_CONTROL)) return false;
        if ((modifierMask & 2u) && !(keys & MK_SHIFT)) return false;
        if ((modifierMask & 4u) && !(GetAsyncKeyState(VK_MENU) & 0x8000)) return false;
        return modifier || modifierMask;
    }
    bool CommandMessage(UINT message)
    {
        return message == WM_LBUTTONDOWN || message == WM_LBUTTONUP || message == WM_LBUTTONDBLCLK
            || message == WM_RBUTTONDOWN || message == WM_RBUTTONUP || message == WM_RBUTTONDBLCLK;
    }

    bool KeyMessage(UINT message, WPARAM key)
    {
        // Chord members remain native: selection, Ctrl+Shift+letter shortcuts
        // and factory batch counts must still receive both modifier events.
        if (!modifier || !installed || !enabled || !Playable(World()) || TextInputOpen(World())) return false;
        return ((message == WM_KEYDOWN || message == WM_KEYUP
            || message == WM_SYSKEYDOWN || message == WM_SYSKEYUP) && key == modifier)
            || (modifier == VK_SPACE && message == WM_CHAR && key == ' ');
    }

    Batch::Batch(bool active, bool reuseActive) : m_lock(inputMutex), m_previous(context), m_previousRequested(requested), m_context(nullptr)
    {
        // Grid/megamap helpers can invoke a native mouse handler repeatedly
        // inside one command. Keep its anchor so that the block is not reversed.
        if (reuseActive && requested) return;
        requested = installed && enabled && active;
        context = nullptr;
        if (requested)
        {
            try { m_context = new Context; context = static_cast<Context*>(m_context); }
            catch (...) { Warn("cannot allocate input batch"); }
        }
    }
    Batch::~Batch()
    {
        delete static_cast<Context*>(m_context);
        context = static_cast<Context*>(m_previous);
        requested = m_previousRequested;
    }

    void __stdcall Dispatch(unsigned action, int shift, UnitStruct* unit, UnitStruct* target,
        const Position_Dword* position, int param1, int param2)
    {
        std::lock_guard<std::recursive_mutex> lock(inputMutex);
        if (!installed || !enabled || !requested)
        {
            NativeSend(action, shift, unit, target, position, param1, param2);
            return;
        }
        TAdynmemStruct* ta = World();
        if (mutating || !context || context->world != ta || !Playable(ta)
            || context->player != ta->LocalHumanPlayer_PlayerID || !LiveOwned(ta, unit)
            || !LiveTarget(ta, target)) { Warn("invalid command context"); return; }

        Entry* entry = nullptr;
        for (Entry& existing : context->entries)
            if (existing.unit == unit) { entry = &existing; break; }
        if (!entry)
        {
            try
            {
                Entry fresh = {unit, unit->UnitID, unit->UnitType, unit->Owner_PlayerPtr0, {}, false, false};
                context->entries.push_back(fresh); entry = &context->entries.back();
            }
            catch (...) { Warn("cannot allocate unit cursor"); return; }
        }
        if (entry->failed) return;
        if (!SameEntry(*entry, ta)) { entry->failed = true; Warn("unit identity changed"); return; }
        if (!entry->appendNative && !entry->cursor.initialized && Idle(*unit)) entry->appendNative = true;
        if (entry->appendNative)
        {
            // There is nothing to suspend. Keep native append semantics for
            // this unit's whole gesture, including every grid/mex command.
            NativeSend(action, 1, unit, target, position, param1, param2);
            return;
        }

        NativeEngine::Request request;
        // Native callers only initialize the low script-index byte.
        request.action = action & 0xffu; request.target = target;
        request.position = position ? *position : Position_Dword{};
        request.param1 = param1; request.param2 = param2;
        request.kind = KindFor(request.action, *unit, target, request.position);
        if (!request.kind || request.kind == ordertype::STOP) { Warn("unsupported urgent action"); return; }
        request.flags = 1u | 0x2000u; // Fresh explicit user command; never an append boundary.
        if (request.kind == ordertype::BUILD)
            if (CUnitRotate* rotation = CUnitRotate::GetInstance()) request.rotation = rotation->GetRotation();
        NativeEngine engine;
        // Separate SEH envelope resets the guard even if a native handler
        // raises an exception caught by the outer game window procedure.
        const PrependQueue::Result result = InsertNative(engine, *unit, request, entry->cursor);
        if (result != PrependQueue::Result::Inserted) { entry->failed = true; Warn("unsupported current order or full/invalid queue"); return; }
        if (successLogs < 32)
        {
            ++successLogs;
            IDDrawSurface::OutptFmtTxt("[QueuePrepend] inserted unit=%u action=%u", unit->UnitInGameIndex, action);
        }
    }

    void Install()
    {
        if (installed || !MyConfig) return;
        enabled = MyConfig->GetIniBool("QueuePrepend", TRUE) != FALSE;
        int key = MyConfig->GetIniInt("QueuePrependKey", 0);
        if (key && (key < 8 || key > 255)) key = 0;
        int mask = MyConfig->GetIniInt("QueuePrependModifiers", key ? 0 : 3);
        if (mask < 0 || mask > 7 || (!key && !mask)) mask = key ? 0 : 3;
        modifier = static_cast<unsigned>(key);
        modifierMask = static_cast<unsigned>(mask);
        if (!enabled) return;
        const CallSite helpers[] = {
            {0x0043A0C0u,{0x83,0xEC,0x0C,0x8A,0x44}},
            {0x00439F80u,{0x8B,0x44,0x24,0x04,0x56}},
            {0x0043AC60u,{0x56,0x8B,0x74,0x24,0x0C}},
            {0x0043A1F0u,{0x53,0x56,0x8B,0xF1,0x57}}
        };
        for (const CallSite& site : helpers) if (!CodeMatches(site.address, site.expected)) return;
        for (const CallSite& site : sites) if (!CodeMatches(site.address, site.expected)) return;
        try
        {
            hooks.reserve(sizeof(sites)/sizeof(sites[0]));
            for (CallSite& site : sites)
            {
                site.patch[0] = 0xE8;
                const std::int32_t relative = static_cast<std::int32_t>(reinterpret_cast<std::uintptr_t>(&Dispatch)-site.address-5);
                std::memcpy(site.patch+1, &relative, sizeof(relative));
                hooks.emplace_back(new SingleHook(site.address, 5, INLINE_UNPROTECTEVINMENT, site.patch));
                if (!CodeMatches(site.address, site.patch)) { hooks.clear(); return; }
            }
        }
        catch (...) { hooks.clear(); return; }
        installed = true;
        IDDrawSurface::OutptTxt("[QueuePrepend] build=ctrl-shift-v4");
    }
}
#else
namespace PrependOrders
{
    bool Active() { return false; }
    bool ModifierDown() { return false; }
    bool ModifierForMouse(unsigned) { return false; }
    bool ModifierKey(UINT) { return false; }
    bool KeyMessage(UINT, WPARAM) { return false; }
    bool CommandMessage(UINT) { return false; }
    void Install() {}
    Batch::Batch(bool, bool) : m_previous(nullptr), m_previousRequested(false), m_context(nullptr) {}
    Batch::~Batch() {}
    void __stdcall Dispatch(unsigned action, int shift, UnitStruct* unit, UnitStruct* target,
        const Position_Dword* position, int param1, int param2)
    {
        using Send = void (__stdcall*)(unsigned, int, UnitStruct*, UnitStruct*, const Position_Dword*, int, int);
        reinterpret_cast<Send>(0x0043AFC0u)(action, shift, unit, target, position, param1, param2);
    }
}
#endif
