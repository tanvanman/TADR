#pragma once

#include <cstdint>

struct WeaponStruct;

// Adds heap-backed weapon slots above TA's hard-coded Weapons[256] array.
//
// TA's TAdynmemStruct holds a fixed 256-entry weapon table at offset 0x2CF3
// (stride 0x115). The TDF format already lets a mod author write `ID=N` per
// weapon, but for N >= 256 the engine writes past the array into
// PROJECTILES_ARRAY_p — silent corruption.
//
// This module provides:
//   * a parallel heap-allocated overflow array for slots [256, kMaxCapacity),
//   * GetDef(id)  — id <  256: &taPtr->Weapons[id];
//                   id >= 256: &overflow[id - 256] (or nullptr if not Installed),
//   * GetId(def)  — slot index for a weapon pointer in either array; -1 otherwise.
//
// The TDF loader hook that populates the overflow array lives in this module
// too (added in a follow-up phase). For now the storage exists and is empty.
//
// Network-side: TA's WEAPON_FIRED_0D packet carries only the low byte of ID,
// so the engine cannot natively transmit overflow weapons. WeaponFiredExt
// (ChatHijackId::WeaponFiredExt) carries the full uint16 for IDs >= 256.

namespace WeaponIdOverflow {

    constexpr int kBaseCapacity = 256;
    constexpr int kMaxCapacity  = 4096;

    // Allocate the overflow array and ready it for population. Idempotent.
    void Install();

    // Free the overflow array. Safe to call multiple times.
    void Shutdown();

    // Map id -> WeaponStruct*. Returns nullptr if id is out of range or the
    // overflow slot has not been populated.
    WeaponStruct* GetDef(uint16_t id);

    // Map WeaponStruct* -> id. Returns -1 if the pointer is not in either
    // the base array or the overflow array.
    int GetId(const WeaponStruct* def);

    // Number of overflow slots that have been populated (0..kMaxCapacity-256).
    int GetOverflowCount();
}
