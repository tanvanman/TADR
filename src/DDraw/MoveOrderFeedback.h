#pragma once

#include <Windows.h>
#include <ddraw.h>
#include <cstdint>

struct TAdynmemStruct;
struct UnitStruct;
struct _Position_Dword;

namespace MoveOrderFeedback
{
    // Instance fingerprint, not a monotonic generation. Native init assigns a
    // random birth salt at unit+0xAA; opaque instance addresses are never dereferenced.
    struct ActorIdentity
    {
        short type;
        unsigned short birthSalt;
        std::uintptr_t definition, script, model;
    };
    void Install();
    // One guarded native user-order CALL; optionally delegates queue prepending.
    void __stdcall Dispatch(unsigned action, int shift, UnitStruct* unit,
        UnitStruct* target, const _Position_Dword* position, int param1, int param2);
    // Presentation snapshots only; neither entry point retains a unit/order pointer.
    void Record(TAdynmemStruct* world, unsigned unitIndex, const ActorIdentity& identity,
        const _Position_Dword& origin, const _Position_Dword& destination);
    void Cancel(TAdynmemStruct* world, unsigned unitIndex, const _Position_Dword& destination);
    void Draw(LPDIRECTDRAWSURFACE surface);
}
