#pragma once

#include <cstdint>

// Extended per-weapon boolean tags, read from Weapons/*.tdf keys that vanilla TA
// does not understand. Shared by any module that needs to mark a weapon and test
// that mark from an inline hook.
//
// Not WeaponTypeMask: all 32 bits are already named by the engine. A module that
// can claim a real bit should (NotToAir sets WTM_NotToAir and reads it straight
// out of EAX, which no table beats).
//
// Not WeaponTypedef::ID: not unique. WeaponIdOverflow stamps every heap overflow
// slot with ID = 0xFF, which also collides with base[255]. The def pointer is the
// only sound key.

namespace WeaponTags {

enum Tag : uint32_t
{
    SurfaceFire     = 1u << 0,   // "surfacefire"
    NotToUnderwater = 1u << 1,   // "nottounderwater"
    NotOverWater    = 1u << 2,   // "notoverwater"
    NotOverLand     = 1u << 3,   // "notoverland"
};

// Bind a TDF key to a tag bit. Call at DLL attach; all bindings share one
// WeaponTdfHook handler.
void RegisterKey(const char* tdfKey, Tag tag);

// Run `fn` once, the first time a weapon carrying `tag` is loaded, so a module can
// defer installing its hooks until a mod actually uses the tag. Vanilla OTA uses
// none of these, and every one of them patches a combat hot path.
//
// Fires from inside WeaponTdfHook's router, during TDF load and long before any
// combat tick, so patching code memory is safe. `fn` must not call RegisterKey or
// WeaponTdfHook::Register -- that would mutate a vector being iterated.
typedef void (*InstallFn)();
void OnFirstUse(Tag tag, InstallFn fn);

uint32_t GetSlow(const void* pWeaponDef);
void     SetSlow(const void* pWeaponDef, uint32_t tags);

namespace detail {

    const uintptr_t kStride       = 0x115;   // sizeof(WeaponStruct)
    const int       kBaseCapacity = 256;     // TA's hard-coded Weapons[] bound

    // Exposed only so Get() can inline; treat as private.
    extern const char* g_baseArray;          // &TAdynmem->Weapons[0], cached
    extern uint32_t    g_baseTags[kBaseCapacity];

} // namespace detail

// The SurfaceFire gates call this inside UnitAutoAim_CheckUnitWeapon -- per
// candidate target, per weapon slot, per tick -- so keep it a subtract, an
// unsigned compare, a multiply-shift and a load.
inline uint32_t Get(const void* pWeaponDef)
{
    const char* base = detail::g_baseArray;
    if (base)
    {
        // Unsigned wrap makes one compare cover both ends: a pointer below the
        // array underflows and falls through to GetSlow.
        const size_t d = (size_t)((const char*)pWeaponDef - base);
        if (d < (size_t)(detail::kBaseCapacity * detail::kStride))
            return detail::g_baseTags[d / detail::kStride];
    }
    return GetSlow(pWeaponDef);
}

inline bool Has(const void* pWeaponDef, Tag tag)
{
    return (Get(pWeaponDef) & (uint32_t)tag) != 0;
}

} // namespace WeaponTags
