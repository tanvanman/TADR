#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "config.h"
#include "LabBuildCycles.h"

#if LAB_BUILD_CYCLES_ENABLE
#include "LabBuildCycleMenu.h"
#include "GameTickHook.h"
#include "oddraw.h"
#include "tamem.h"
#include "tafunctions.h"
#include "iddrawsurface.h"
#include "hook/hook.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <unordered_map>

static_assert(sizeof(UnitStruct) == 0x118, "Native unit stride");
static_assert(sizeof(UnitOrdersStruct) == 0x56, "Native order stride");
static_assert(offsetof(UnitStruct, UnitOrders) == 0x5c, "Factory production list");
static_assert(offsetof(UnitStruct, UnitType) == 0x92, "Native unit type");
static_assert(offsetof(UnitStruct, UnitInGameIndex) == 0xa8, "Native unit slot");
static_assert(offsetof(UnitOrdersStruct, State) == 0x05, "Production phase");
static_assert(offsetof(UnitOrdersStruct, Unit_ptr) == 0x0e, "Production owner");
static_assert(offsetof(UnitOrdersStruct, BuildUnitID) == 0x36, "Production type");
static_assert(offsetof(UnitOrdersStruct, BuildCount) == 0x3a, "Production batch count");
static_assert(offsetof(UnitOrdersStruct, NextOrder) == 0x4a, "Native queue link");

namespace
{
    // BuildingBuild phase 4, after FinishConstruction, clearing the target,
    // and decrementing the batch count. EDI = lab; ESI = production order.
    // The displaced CALL still refreshes the native menu with the new count.
    // Neither cancellation (wake bit 2), failed construction (bit 8), nor
    // stockpiled weapons reach this site. Verified in ESC and ProTA exes.
    constexpr unsigned kCompletedCall = 0x00402B3Cu;
    constexpr unsigned char kCompletedBytes[] = {0xE8,0x0F,0x96,0x01,0x00};
    constexpr unsigned char kBuildingEntryBytes[] = {0x8A,0x44,0x24,0x0C,0x83,0xEC,0x30,0xA8};
    constexpr unsigned char kAdjustBytes[] = {0x53,0x55,0x8B,0x6C,0x24,0x18,0x56,0x85,0xED,0x57,0x7E,0x70};

    // Unit slots are recycled, even into the same unit type. InitUnitFromType
    // is the shared creation/transfer initializer, so clearing here provides
    // a real lifetime boundary instead of guessing from a cached pointer.
    // Hook at function entry: the first argument remains [ESP+4].
    constexpr unsigned kInitUnit = 0x00485A40u;
    constexpr unsigned char kInitBytes[] = {0x53,0x56,0x8B,0x74,0x24,0x0C};
    constexpr unsigned kMaximumSlots = 65535u;
    constexpr unsigned kMaximumQueue = 4096u;
    using NativeAdjust = void (__stdcall*)(unsigned, UnitStruct*, int, int);
    NativeAdjust const AdjustBuildCount = reinterpret_cast<NativeAdjust>(0x0043B0B0u);

    struct Entry
    {
        UnitStruct* unit;
        UnitDefStruct* type;
        PlayerStruct* owner;
        void* script;
        Object3doStruct* object;
    };

    std::unordered_map<unsigned,Entry> g_enabled;
    std::unique_ptr<InlineSingleHook> g_completedHook;
    std::unique_ptr<InlineSingleHook> g_initHook;
    TAdynmemStruct* g_world = nullptr;
    UnitStruct* g_bank = nullptr;
    UnitDefStruct* g_definitions = nullptr;
    int g_lastTick = -1;
    bool g_installed = false;
    bool g_callbackRegistered = false;

    bool Slot(TAdynmemStruct* ta, const UnitStruct* unit, unsigned& index)
    {
        if (!ta || !ta->BeginUnitsArray_p || !ta->EndOfUnitsArray_p || !unit) return false;
        const auto begin=reinterpret_cast<std::uintptr_t>(ta->BeginUnitsArray_p);
        const auto end=reinterpret_cast<std::uintptr_t>(ta->EndOfUnitsArray_p);
        const auto value=reinterpret_cast<std::uintptr_t>(unit);
        if (end < begin || (end-begin)%sizeof(UnitStruct)
            || (end-begin)/sizeof(UnitStruct) >= kMaximumSlots
            || value < begin || value > end || (value-begin)%sizeof(UnitStruct)) return false;
        index=static_cast<unsigned>((value-begin)/sizeof(UnitStruct));
        return index != 0;
    }

    TAdynmemStruct* LocalWorld()
    {
        if (!DataShare || DataShare->TAProgress != TAInGame || DataShare->PlayingDemo
            || DataShare->ehaOff == 1) return nullptr;
        TAdynmemStruct* ta=*TAmainStruct_PtrPtr;
        if (!ta || !ta->UnitDef || ta->UNITINFOCount < 2 || ta->UNITINFOCount > kMaximumSlots
            || ta->GameTime < 0) return nullptr;
        const unsigned local=static_cast<unsigned char>(ta->LocalHumanPlayer_PlayerID);
        if (local >= 10) return nullptr;
        const PlayerStruct& player=ta->Players[local];
        if (!player.PlayerActive || player.My_PlayerType != Player_LocalHuman
            || !player.PlayerInfo || (player.PlayerInfo->PropertyMask & WATCH)) return nullptr;
        return ta;
    }

    bool Lab(TAdynmemStruct* ta, UnitStruct* unit, unsigned& index)
    {
        if (!Slot(ta,unit,index) || static_cast<unsigned short>(unit->UnitInGameIndex) != index)
            return false;
        const unsigned local=static_cast<unsigned char>(ta->LocalHumanPlayer_PlayerID);
        const unsigned type=static_cast<unsigned short>(unit->UnitID);
        if (!type || type >= ta->UNITINFOCount || unit->UnitType != &ta->UnitDef[type]
            || unit->Owner_PlayerPtr0 != &ta->Players[local] || unit->cOwnerID != local
            || !(unit->UnitSelected & 0x10000000u) || (unit->UnitSelected & 0x4000u)
            || !(unit->UnitSelected & 0x20000000u) || unit->Nanoframe != 0.0f
            || static_cast<unsigned short>(unit->Health) == 0 || unit->IsUnit != 0) return false;
        const UnitDefStruct& def=*unit->UnitType;
        // Human build menus can differ from the AI CANBUILD list. The native
        // menu page count identifies a build menu without excluding factories
        // whose AI-only list is empty.
        return def.bmcode == 0 && (def.UnitTypeMask_0 & builder) && def.GUINum > 0;
    }

    bool SameEntry(TAdynmemStruct* ta, unsigned index, const Entry& entry)
    {
        unsigned current=0;
        return Lab(ta,entry.unit,current) && current == index
            && entry.unit->UnitType == entry.type && entry.unit->Owner_PlayerPtr0 == entry.owner
            && entry.unit->UnkPTR2 == entry.script && entry.unit->Object3do == entry.object;
    }

    void ResetState()
    {
        g_enabled.clear();
        g_world=nullptr;
        g_bank=nullptr;
        g_definitions=nullptr;
        g_lastTick=-1;
    }

    TAdynmemStruct* RefreshState()
    {
        TAdynmemStruct* ta=g_installed ? LocalWorld() : nullptr;
        if (!ta) { ResetState(); return nullptr; }
        if (ta != g_world || ta->BeginUnitsArray_p != g_bank || ta->UnitDef != g_definitions
            || ta->GameTime < g_lastTick) ResetState();
        g_world=ta;
        g_bank=ta->BeginUnitsArray_p;
        g_definitions=ta->UnitDef;
        g_lastTick=ta->GameTime;
        for (auto it=g_enabled.begin(); it!=g_enabled.end();)
            if (!SameEntry(ta,it->first,it->second)) it=g_enabled.erase(it);
            else ++it;
        return ta;
    }

    UnitStruct* ResolveSelection()
    {
        TAdynmemStruct* ta=RefreshState();
        if (!ta || (ta->GameStateMask & 3) == 3 || (ta->GameStateMask & 0x60)) return nullptr;
        PlayerStruct& owner=ta->Players[static_cast<unsigned char>(ta->LocalHumanPlayer_PlayerID)];
        unsigned first=0,last=0;
        if (!Slot(ta,owner.Units,first) || !Slot(ta,owner.UnitsAry_End,last) || first > last)
            return nullptr;
        UnitStruct* selected=nullptr;
        for (unsigned index=first; index<=last; ++index)
        {
            UnitStruct* unit=&ta->BeginUnitsArray_p[index];
            if (unit->Owner_PlayerPtr0 != &owner || !(unit->UnitSelected & UnitSelected_State)) continue;
            if (selected) return nullptr;
            selected=unit;
        }
        unsigned slot=0;
        return selected && Lab(ta,selected,slot) ? selected : nullptr;
    }

    // Only inspect the native primary list; rally orders and stockpiles have
    // their own lists/types. Bounded walking also rejects cycles/corruption
    // before handing the list to the engine's unbounded tail search.
    bool CanAppend(UnitStruct* unit, UnitOrdersStruct* completed)
    {
        if (!completed || unit->UnitOrders != completed || completed->Unit_ptr != unit
            || completed->State != 4 || completed->AttackTargat || completed->BuildCount < 0)
            return false;
        UnitOrdersStruct* tail=nullptr;
        unsigned walked=0;
        for (UnitOrdersStruct* node=completed; node; node=node->NextOrder)
        {
            if (++walked > kMaximumQueue || node->Unit_ptr != unit) return false;
            tail=node;
        }
        if (tail->COBHandler_index == completed->COBHandler_index
            && tail->BuildUnitID == completed->BuildUnitID)
        {
            const int count=tail->BuildCount;
            if (count < 0 || count == (std::numeric_limits<int>::max)()) return false;
        }
        return true;
    }

    void Completed(UnitStruct* unit, UnitOrdersStruct* order)
    {
        TAdynmemStruct* ta=RefreshState();
        unsigned slot=0;
        if (!ta || !Lab(ta,unit,slot) || g_enabled.find(slot) == g_enabled.end()
            || !order || !order->COBHandler_index || !order->BuildUnitID
            || order->BuildUnitID >= ta->UNITINFOCount || !CanAppend(unit,order)) return;
        // Native menu queue append, without click sounds. Tail merging keeps
        // the original production order: F5,C1 -> F4,C1,F1 -> ... -> C1,F5.
        // Off simply stops appending; the remaining native queue drains.
        AdjustBuildCount(order->COBHandler_index,unit,static_cast<int>(order->BuildUnitID),1);
    }

    void CompletedCaught(UnitStruct* unit, UnitOrdersStruct* order)
    {
        try { Completed(unit,order); }
        catch (...)
        {
            // An optional refill must not unwind through the engine's tick
            // dispatcher (for example, if its order allocation fails).
            g_enabled.clear();
        }
    }

    void CompletedSafe(UnitStruct* unit, UnitOrdersStruct* order)
    {
        __try { CompletedCaught(unit,order); }
        __except (EXCEPTION_EXECUTE_HANDLER) { g_enabled.clear(); }
    }

    int __stdcall CompletionProc(PInlineX86StackBuffer buffer)
    {
        CompletedSafe(reinterpret_cast<UnitStruct*>(buffer->Edi),
            reinterpret_cast<UnitOrdersStruct*>(buffer->Esi));
        return 0; // Preserve the original call and all registers/stack.
    }

    void ForgetUnit(UnitStruct* unit)
    {
        unsigned slot=0;
        if (Slot(g_world,unit,slot)) g_enabled.erase(slot);
    }

    int __stdcall InitUnitProc(PInlineX86StackBuffer buffer)
    {
        __try
        {
            UnitStruct* unit=*reinterpret_cast<UnitStruct**>(buffer->Esp+4u);
            ForgetUnit(unit);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
        return 0;
    }

    void RefreshSafe()
    {
        __try { RefreshState(); }
        __except (EXCEPTION_EXECUTE_HANDLER) { ResetState(); }
    }

    void OnGameTick(int)
    {
        if (!g_installed) return;
        RefreshSafe();
        LabBuildCycleMenu::Update();
    }

    bool Bytes(unsigned address, const unsigned char* expected, std::size_t size)
    {
        unsigned char actual[16]={};
        SIZE_T read=0;
        return size <= sizeof(actual) && ReadProcessMemory(GetCurrentProcess(),
            reinterpret_cast<const void*>(address),actual,size,&read) && read == size
            && std::memcmp(actual,expected,size) == 0;
    }

    bool SelectionStateImpl(bool& enabled)
    {
        UnitStruct* unit=ResolveSelection();
        if (!unit) { enabled=false; return false; }
        unsigned slot=0;
        enabled=Slot(g_world,unit,slot) && g_enabled.find(slot) != g_enabled.end();
        return true;
    }

    bool EnabledImpl(UnitStruct* unit)
    {
        TAdynmemStruct* ta=RefreshState();
        unsigned slot=0;
        return ta && Lab(ta,unit,slot) && g_enabled.find(slot) != g_enabled.end();
    }

    bool SelectionStateSafe(bool& enabled)
    {
        // Checked iterators in MSVC Debug have destructors. Keep them outside
        // SEH wrappers too, so Debug and Release use the same safe boundaries.
        __try { return SelectionStateImpl(enabled); }
        __except (EXCEPTION_EXECUTE_HANDLER) { enabled=false; return false; }
    }

    bool Toggle()
    {
        UnitStruct* unit=ResolveSelection();
        unsigned slot=0;
        if (!unit || !Slot(g_world,unit,slot)) return false;
        const auto it=g_enabled.find(slot);
        if (it != g_enabled.end()) g_enabled.erase(it);
        else g_enabled.emplace(slot,Entry{unit,unit->UnitType,unit->Owner_PlayerPtr0,
            unit->UnkPTR2,unit->Object3do});
        return true;
    }

    bool ToggleSafe()
    {
        __try { return Toggle(); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }
}

namespace LabBuildCycles
{
    void Install()
    {
        if (g_installed) return;
        if (!Bytes(kCompletedCall,kCompletedBytes,sizeof(kCompletedBytes))
            || !Bytes(0x00402640u,kBuildingEntryBytes,sizeof(kBuildingEntryBytes))
            || !Bytes(0x0043B0B0u,kAdjustBytes,sizeof(kAdjustBytes))
            || !Bytes(kInitUnit,kInitBytes,sizeof(kInitBytes)))
        {
            IDDrawSurface::OutptTxt("[LabBuildCycles] incompatible engine; left disabled");
            return;
        }
        try
        {
            ResetState();
            g_initHook.reset(new InlineSingleHook(kInitUnit,sizeof(kInitBytes),
                INLINE_5BYTESLAGGERJMP,InitUnitProc));
            g_completedHook.reset(new InlineSingleHook(kCompletedCall,sizeof(kCompletedBytes),
                INLINE_5BYTESLAGGERJMP,CompletionProc));
            if (!g_callbackRegistered)
            {
                GameTickHook::GetInstance()->addCallback(OnGameTick);
                g_callbackRegistered=true;
            }
            g_installed=true;
            if (!LabBuildCycleMenu::Install()) { Shutdown(); return; }
        }
        catch (...) { Shutdown(); return; }
        IDDrawSurface::OutptTxt("[LabBuildCycles] build=native-cycle-v3");
    }

    void Shutdown()
    {
        g_installed=false;
        LabBuildCycleMenu::Shutdown();
        g_completedHook.reset();
        g_initHook.reset();
        ResetState();
    }

    bool SelectionState(bool& enabled) { return SelectionStateSafe(enabled); }

    bool ToggleSelection()
    {
        try { return ToggleSafe(); }
        catch (...) { return false; }
    }

    bool IsEnabled(UnitStruct* unit)
    {
        __try { return EnabledImpl(unit); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }
}
#else
namespace LabBuildCycles
{
    void Install() {}
    void Shutdown() {}
    bool SelectionState(bool& enabled) { enabled=false; return false; }
    bool ToggleSelection() { return false; }
    bool IsEnabled(UnitStruct*) { return false; }
}
#endif
