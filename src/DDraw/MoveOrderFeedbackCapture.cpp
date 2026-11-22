#include "config.h"
#include "MoveOrderFeedback.h"
#if PREPEND_ORDERS_ENABLE
#include "PrependOrders.h"
#endif

#if DRAWN_MOVE_FORMATIONS_ENABLE
#include "oddraw.h"
#include "tamem.h"
#include "tafunctions.h"
#include "iddrawsurface.h"
#include "hook/hook.h"
#include <cstdint>
#include <cstring>
#include <memory>
#include <atomic>

namespace
{
    using SendFn = void (__stdcall*)(unsigned, int, UnitStruct*, UnitStruct*, const Position_Dword*, int, int);
    const auto NativeSend = reinterpret_cast<SendFn>(0x0043AFC0u);
    const auto NativeAction = reinterpret_cast<unsigned char* (__stdcall*)(unsigned*, unsigned,
        UnitStruct*, UnitStruct*, const Position_Dword*)>(0x0043F0E0u);

    const unsigned kCall = 0x0048D1F4u;
    const unsigned char kOriginal[5] = {0xE8,0xC7,0xDD,0xFA,0xFF};
    unsigned char callPatch[5] = {};
    std::unique_ptr<SingleHook> callHook;
    bool installed = false;
    const unsigned kMaxOrders = 8192;
    std::atomic<unsigned> diagnosticCount(0);

    struct Observation
    {
        TAdynmemStruct* world;
        UnitStruct* first;
        UnitStruct* last;
        UnitStruct* ownerFirst;
        UnitStruct* ownerLast;
        PlayerStruct* owner;
        UnitDefStruct* definition;
        int player;
        int tick;
        unsigned dpid;
        short type;
        unsigned unitIndex;
        unsigned action;
        unsigned before;
        bool prepend;
        bool beforeValid;
        const char* stage;
        const char* liveReason;
        bool diagnosticBounds;
        unsigned diagnosticFlags, diagnosticMask0, diagnosticMask1, diagnosticBmcode;
        float diagnosticNanoframe;
        MoveOrderFeedback::ActorIdentity identity;
        Position_Dword origin;
        Position_Dword destination;
    };

    bool Playable(TAdynmemStruct* ta)
    {
        if (!ta || !DataShare || DataShare->TAProgress != TAInGame
            || DataShare->PlayingDemo || DataShare->ehaOff) return false;
        const int player = ta->LocalHumanPlayer_PlayerID;
        return player >= 0 && player < 10 && ta->Players[player].PlayerInfo
            && !(ta->Players[player].PlayerInfo->PropertyMask & WATCH)
            && ta->Players[player].My_PlayerType == Player_LocalHuman;
    }

    bool InBank(UnitStruct* unit, UnitStruct* first, UnitStruct* last)
    {
        const std::uintptr_t p = reinterpret_cast<std::uintptr_t>(unit);
        const std::uintptr_t a = reinterpret_cast<std::uintptr_t>(first);
        const std::uintptr_t b = reinterpret_cast<std::uintptr_t>(last);
        return first && last && a <= b && p >= a && p <= b && (p-a)%sizeof(UnitStruct) == 0;
    }

    const char* LiveRejection(TAdynmemStruct* ta, UnitStruct* unit)
    {
        if (!ta) return "world";
        if (!DataShare) return "shared-state";
        if (DataShare->TAProgress != TAInGame) return "progress";
        if (DataShare->PlayingDemo) return "demo";
        if (DataShare->ehaOff) return "disabled";
        const int player = ta->LocalHumanPlayer_PlayerID;
        if (player < 0 || player >= 10) return "player-index";
        PlayerStruct* owner = &ta->Players[player];
        if (!owner->PlayerInfo) return "player-info";
        if (owner->PlayerInfo->PropertyMask & WATCH) return "watcher";
        if (owner->My_PlayerType != Player_LocalHuman) return "controller";
        if (!unit) return "unit-null";
        if (!InBank(unit, ta->BeginUnitsArray_p, ta->EndOfUnitsArray_p)) return "global-bank";
        if (!InBank(unit, owner->Units, owner->UnitsAry_End)) return "owner-bank";
        if (!unit->UnitID) return "empty-slot";
        if (!unit->UnitType) return "definition";
        if (unit->Owner_PlayerPtr0 != owner) return "owner";
        if (!(unit->UnitSelected & 0x10000000u)) return "not-alive";
        if (unit->UnitSelected & 0x4000u) return "pending-death";
        // Accepted native MOVE and its queued destination are authoritative.
        // Legacy constructor/capability fields are not lifetime checks.
        return nullptr;
    }
    bool LiveOwned(TAdynmemStruct* ta, UnitStruct* unit)
    {
        return !LiveRejection(ta,unit);
    }
    void CaptureLiveDiagnostics(TAdynmemStruct* ta, UnitStruct* unit, Observation& out)
    {
        // Optional diagnostics must not add eligibility requirements or keep
        // dereferencing native definitions after the bounded log allowance.
        if (diagnosticCount.load(std::memory_order_relaxed) >= 12) return;
        __try
        {
            if (!ta || !unit) return;
            const int player = ta->LocalHumanPlayer_PlayerID;
            if (player < 0 || player >= 10
                || !InBank(unit,ta->BeginUnitsArray_p,ta->EndOfUnitsArray_p)
                || !InBank(unit,ta->Players[player].Units,ta->Players[player].UnitsAry_End)) return;
            out.unitIndex = static_cast<unsigned>(unit-ta->BeginUnitsArray_p);
            out.type = unit->UnitID;
            out.diagnosticFlags = unit->UnitSelected;
            out.diagnosticNanoframe = unit->Nanoframe;
            out.diagnosticBounds = true;
            if (unit->UnitType)
            {
                out.diagnosticMask0 = unit->UnitType->UnitTypeMask_0;
                out.diagnosticMask1 = unit->UnitType->UnitTypeMask_1;
                out.diagnosticBmcode = unit->UnitType->bmcode;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    bool Nearby(const Position_Dword& a, const Position_Dword& b)
    {
        // Native Shift cancellation compares complete 16.16 X/Y coordinates.
        // Use unsigned arithmetic to match its wrap-safe +/-16 world-unit test.
        std::uint32_t ax, ay, bx, by;
        std::memcpy(&ax, &a, sizeof(ax));
        std::memcpy(&ay, reinterpret_cast<const unsigned char*>(&a)+8, sizeof(ay));
        std::memcpy(&bx, &b, sizeof(bx));
        std::memcpy(&by, reinterpret_cast<const unsigned char*>(&b)+8, sizeof(by));
        return ax-bx+0x100000u <= 0x200000u && ay-by+0x100000u <= 0x200000u;
    }

    bool CountMatches(UnitStruct& unit, unsigned action, UnitStruct* target,
        const Position_Dword& destination, unsigned& count)
    {
        count = 0;
        UnitOrdersStruct* checkpoint = unit.UnitOrders;
        unsigned power = 1, distance = 0, visited = 0;
        for (UnitOrdersStruct* order = unit.UnitOrders; order; )
        {
            if (++visited > kMaxOrders || order->Unit_ptr != &unit) return false;
            if (order->COBHandler_index == action && (!target || order->AttackTargat == target)
                && Nearby(order->Pos, destination)) ++count;
            order = order->NextOrder;
            ++distance;
            if (order && order == checkpoint) return false;
            if (distance == power) { checkpoint = order; power *= 2; distance = 0; }
        }
        return true;
    }

    bool CountProtected(UnitStruct& unit, unsigned action, UnitStruct* target,
        const Position_Dword& destination, unsigned& count)
    {
        __try { return CountMatches(unit,action,target,destination,count); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }

    bool Capture(unsigned action, UnitStruct* unit, UnitStruct* target,
        const Position_Dword* position, Observation& out)
    {
        // Observing presentation must never prevent native order delivery.
        __try
        {
            out.stage = "live";
            out.action = action & 0xffu;
            out.unitIndex = ~0u;
#if PREPEND_ORDERS_ENABLE
            out.prepend = PrependOrders::Active();
#endif
            TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
            CaptureLiveDiagnostics(ta,unit,out);
            out.liveReason = "native-read";
            out.liveReason = !position ? "position-null" : LiveRejection(ta,unit);
            if (out.liveReason) return false;
            out.unitIndex = static_cast<unsigned>(unit-ta->BeginUnitsArray_p);
            out.type = unit->UnitID;
            out.stage = "classification";
            if (!COBSciptHandler_Begin || !COBSciptHandler_End) return false;
            const COBHandle* begin = *COBSciptHandler_Begin;
            const COBHandle* end = *COBSciptHandler_End;
            action &= 0xffu; // Native GUI callers initialize only the low byte.
            if (!action || !begin || !end || end <= begin
                || action >= static_cast<unsigned>(end-begin)) return false;
            unsigned move = 0;
            Position_Dword probe = *position;
            const unsigned char* index = NativeAction(&move, ordertype::MOVE, unit, target, &probe);
            if (!index || *index != action) return false;
            out.world = ta;
            out.first = ta->BeginUnitsArray_p;
            out.last = ta->EndOfUnitsArray_p;
            out.player = ta->LocalHumanPlayer_PlayerID;
            out.owner = unit->Owner_PlayerPtr0;
            out.ownerFirst = out.owner->Units;
            out.ownerLast = out.owner->UnitsAry_End;
            out.dpid = out.owner->DirectPlayID;
            out.tick = ta->GameTime;
            out.definition = unit->UnitType;
            out.type = unit->UnitID;
            out.identity.type = unit->UnitID;
            std::memcpy(&out.identity.birthSalt, unit->data9, sizeof(out.identity.birthSalt));
            out.identity.definition = reinterpret_cast<std::uintptr_t>(unit->UnitType);
            out.identity.script = reinterpret_cast<std::uintptr_t>(unit->UnkPTR2);
            out.identity.model = reinterpret_cast<std::uintptr_t>(unit->Object3do);
            out.unitIndex = static_cast<unsigned>(unit-ta->BeginUnitsArray_p);
            out.action = action;
            out.origin = Position_Dword{};
            out.origin.x_ = unit->XPos__;
            out.origin.X = unit->XPos;
            out.origin.z_ = unit->ZPos__;
            out.origin.Z = unit->ZPos;
            out.origin.y_ = unit->YPos__;
            out.origin.Y = unit->YPos;
            out.destination = *position;
            out.stage = "before-invalid";
            out.beforeValid = CountProtected(*unit,action,target,out.destination,out.before);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }

    bool After(const Observation& observed, UnitStruct* unit, UnitStruct* target, unsigned& count)
    {
        __try
        {
            TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
            unsigned short birthSalt = 0;
            if (LiveOwned(ta,unit)) std::memcpy(&birthSalt, unit->data9, sizeof(birthSalt));
            return ta == observed.world && Playable(ta)
                && ta->LocalHumanPlayer_PlayerID == observed.player
                && ta->BeginUnitsArray_p == observed.first && ta->EndOfUnitsArray_p == observed.last
                && ta->GameTime >= observed.tick
                && LiveOwned(ta,unit) && unit->Owner_PlayerPtr0 == observed.owner
                && observed.owner->Units == observed.ownerFirst && observed.owner->UnitsAry_End == observed.ownerLast
                && observed.owner->DirectPlayID == observed.dpid
                && unit->UnitID == observed.type && unit->UnitType == observed.definition
                && birthSalt == observed.identity.birthSalt
                && reinterpret_cast<std::uintptr_t>(unit->UnkPTR2) == observed.identity.script
                && reinterpret_cast<std::uintptr_t>(unit->Object3do) == observed.identity.model
                && CountMatches(*unit,observed.action,target,observed.destination,count);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }

    void Present(const Observation& observed, bool cancel)
    {
        __try
        {
            if (cancel) MoveOrderFeedback::Cancel(observed.world,observed.unitIndex,observed.destination);
            else MoveOrderFeedback::Record(observed.world,observed.unitIndex,observed.identity,observed.origin,observed.destination);
        }
        // Let C++ exceptions unwind to the outer catch, but contain presentation
        // access violations independently of the native command that just ran.
        __except (GetExceptionCode() == 0xE06D7363u ? EXCEPTION_CONTINUE_SEARCH : EXCEPTION_EXECUTE_HANDLER) {}
    }

    void DiagnosticProtected(const Observation& observed, const char* stage, int shift, unsigned after)
    {
        __try
        {
            IDDrawSurface::OutptFmtTxt("[MoveOrderFeedback] stage=%s unit=%d action=%u type=%d prepend=%u shift=%d beforeValid=%u before=%u after=%u reason=%s bounded=%u flags=0x%08X nano=%g mask0=0x%08X mask1=0x%08X bmcode=%u",
                stage, static_cast<int>(observed.unitIndex), observed.action, observed.type,
                static_cast<unsigned>(observed.prepend), shift, static_cast<unsigned>(observed.beforeValid), observed.before, after,
                observed.liveReason ? observed.liveReason : "ok", static_cast<unsigned>(observed.diagnosticBounds),
                observed.diagnosticFlags, static_cast<double>(observed.diagnosticNanoframe),
                observed.diagnosticMask0, observed.diagnosticMask1, observed.diagnosticBmcode);
        }
        __except (GetExceptionCode() == 0xE06D7363u ? EXCEPTION_CONTINUE_SEARCH : EXCEPTION_EXECUTE_HANDLER) {}
    }
    void Diagnostic(const Observation& observed, const char* stage, int shift, unsigned after)
    {
        unsigned used = diagnosticCount.load(std::memory_order_relaxed);
        while (used < 12)
        {
            if (!diagnosticCount.compare_exchange_weak(used, used+1, std::memory_order_relaxed)) continue;
            try { DiagnosticProtected(observed,stage,shift,after); } catch (...) {}
            break;
        }
    }

    bool CodeMatches(const unsigned char* bytes)
    {
        __try { return std::memcmp(reinterpret_cast<const void*>(kCall),bytes,5) == 0; }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }
}

namespace MoveOrderFeedback
{
    void __stdcall Dispatch(unsigned action, int shift, UnitStruct* unit, UnitStruct* target,
        const Position_Dword* position, int param1, int param2)
    {
        Observation observed = {};
        const bool captured = Capture(action,unit,target,position,observed);
#if PREPEND_ORDERS_ENABLE
        PrependOrders::Dispatch(action,shift,unit,target,position,param1,param2);
#else
        NativeSend(action,shift,unit,target,position,param1,param2);
#endif
        unsigned after = 0;
        if (!captured) { Diagnostic(observed,observed.stage,shift,after); return; }
        if (!After(observed,unit,target,after)) { Diagnostic(observed,"after-invalid",shift,after); return; }
        const char* stage = "no-match";
        try
        {
            if (observed.prepend || shift)
            {
                if (!observed.beforeValid) stage = "before-invalid";
                else if (after > observed.before) { stage = "record"; Present(observed,false); }
                else if (after < observed.before) { stage = "cancel"; Present(observed,true); }
            }
            else if (after) { stage = "record"; Present(observed,false); }
        }
        catch (...) {} // Rendering allocation failure must not affect a sent order.
        Diagnostic(observed,stage,shift,after);
    }

    void Install()
    {
        if (installed) return;
        callPatch[0] = 0xE8;
        const std::int32_t relative = static_cast<std::int32_t>(reinterpret_cast<std::uintptr_t>(&Dispatch)-kCall-5);
        std::memcpy(callPatch+1,&relative,sizeof(relative));
        // Prepending installs this same wrapper when enabled. Otherwise (also
        // QueuePrepend=FALSE), this module owns the stock CALL itself. No chain
        // and no second hook object ever overwrite an existing owner.
        if (!CodeMatches(callPatch))
        {
            if (!CodeMatches(kOriginal)) return;
            try
            {
                std::unique_ptr<SingleHook> candidate(new SingleHook(kCall,5,INLINE_UNPROTECTEVINMENT,callPatch));
                if (!CodeMatches(callPatch)) return;
                callHook.swap(candidate);
            }
            catch (...) { return; }
        }
        installed = true;
        IDDrawSurface::OutptTxt("[MoveOrderFeedback] build=destinations-v3");
    }
}
#else
namespace MoveOrderFeedback
{
    void Install() {}
    void __stdcall Dispatch(unsigned action, int shift, UnitStruct* unit, UnitStruct* target,
        const _Position_Dword* position, int param1, int param2)
    {
#if PREPEND_ORDERS_ENABLE
        PrependOrders::Dispatch(action,shift,unit,target,position,param1,param2);
#else
        using Send = void (__stdcall*)(unsigned, int, UnitStruct*, UnitStruct*, const _Position_Dword*, int, int);
        reinterpret_cast<Send>(0x0043AFC0u)(action,shift,unit,target,position,param1,param2);
#endif
    }
}
#endif
