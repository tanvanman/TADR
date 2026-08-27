#include "WeaponTags.h"
#include "WeaponTdfHook.h"
#include "tamem.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace WeaponTags {

namespace detail {

const char* g_baseArray = nullptr;
uint32_t    g_baseTags[kBaseCapacity] = { 0 };

// The weapon array is embedded in TAdynmemStruct, which is allocated once at
// startup, so this address is stable for the life of the process.
static const char* ResolveBaseArray()
{
    if (!g_baseArray)
    {
        TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511DE8;
        if (taPtr)
            g_baseArray = (const char*)&taPtr->Weapons[0];
    }
    return g_baseArray;
}

} // namespace detail

// Weapons in WeaponIdOverflow's heap array. Off the fast path; the buffer is
// allocated once and reused, so this never grows without bound.
static std::unordered_map<const void*, uint32_t> s_overflowTags;

uint32_t GetSlow(const void* pWeaponDef)
{
    const char* base = detail::ResolveBaseArray();
    if (base)
    {
        const size_t d = (size_t)((const char*)pWeaponDef - base);
        if (d < (size_t)(detail::kBaseCapacity * detail::kStride))
            return detail::g_baseTags[d / detail::kStride];
    }

    std::unordered_map<const void*, uint32_t>::const_iterator it =
        s_overflowTags.find(pWeaponDef);
    return it == s_overflowTags.end() ? 0u : it->second;
}

void SetSlow(const void* pWeaponDef, uint32_t tags)
{
    const char* base = detail::ResolveBaseArray();
    if (base)
    {
        const size_t d = (size_t)((const char*)pWeaponDef - base);
        if (d < (size_t)(detail::kBaseCapacity * detail::kStride))
        {
            detail::g_baseTags[d / detail::kStride] = tags;
            return;
        }
    }
    s_overflowTags[pWeaponDef] = tags;
}

// -------------------------------------------------------------------------

namespace {

struct KeyBinding
{
    std::string key;
    uint32_t    tag;
};

std::vector<KeyBinding>& bindings()
{
    static std::vector<KeyBinding> s_bindings;
    return s_bindings;
}

bool s_handlerInstalled = false;

struct InstallHook
{
    uint32_t  tag;
    InstallFn fn;
};

std::vector<InstallHook>& installHooks()
{
    static std::vector<InstallHook> s_installHooks;
    return s_installHooks;
}

uint32_t s_firedTags = 0;

void FireInstallHooks(uint32_t tags)
{
    const uint32_t pending = tags & ~s_firedTags;
    if (!pending)
        return;
    s_firedTags |= pending;

    std::vector<InstallHook>& h = installHooks();
    for (size_t i = 0; i < h.size(); ++i)
    {
        if (h[i].tag & pending)
            h[i].fn();
    }
}

// One handler for all bound keys, not one per key: the tag word is assigned rather
// than OR-ed, so every key has to be evaluated before the store. Assigning is what
// makes a wipe hook unnecessary -- a weapon's tags are always exactly what its own
// TDF says on this load, and slots a later load leaves unpopulated are unreachable
// anyway (LoadWeapons_Tdf clears WeaponName[0], and weapons resolve by name). That
// matters because WeaponIdOverflow already owns the only convenient wipe site,
// LoadWeapons_Tdf's prologue at 0x42E310.
void OnWeaponTdfParsed(const WeaponTdfHook::Context& ctx)
{
    uint32_t tags = 0;
    const std::vector<KeyBinding>& b = bindings();
    for (size_t i = 0; i < b.size(); ++i)
    {
        if (ctx.getInt(b[i].key.c_str()) & 1)
            tags |= b[i].tag;
    }

    // WeaponIdOverflow's EAX substitution at 0x42E468 has already redirected
    // ctx.pWeaponDef into the heap array for ID >= 256, so this key is correct for
    // overflow weapons too.
    SetSlow(ctx.pWeaponDef, tags);

    if (tags)
        FireInstallHooks(tags);
}

} // namespace

void OnFirstUse(Tag tag, InstallFn fn)
{
    InstallHook entry;
    entry.tag = (uint32_t)tag;
    entry.fn  = fn;
    installHooks().push_back(entry);
}

void RegisterKey(const char* tdfKey, Tag tag)
{
    KeyBinding binding;
    binding.key = tdfKey;
    binding.tag = (uint32_t)tag;
    bindings().push_back(binding);

    if (!s_handlerInstalled)
    {
        s_handlerInstalled = true;
        WeaponTdfHook::Register(&OnWeaponTdfParsed);
    }
}

} // namespace WeaponTags
