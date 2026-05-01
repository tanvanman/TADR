#include "WeaponIdOverflow.h"
#include "WeaponTdfHook.h"
#include "tamem.h"
#include "hook/hook.h"
#include "iddrawsurface.h"

#include <cstring>
#include <cstdlib>
#include <cstdio>

static_assert(sizeof(WeaponStruct) == 0x115,
              "WeaponStruct must be exactly 0x115 bytes — TA expects this stride");

namespace {

// ---- storage ----------------------------------------------------------------

// Raw allocation base (whatever the heap returns) and the aligned slot pointer
// inside it. The aligned base is chosen so that
//   ((char*)g_alignedBase - (char*)&taPtr->Weapons[0]) % 0x115 == 0
// which makes EAX-substitution work: the engine's hard-coded
//     EBP = g_TAMainStruct + EAX*0x115 + 0x2CF3
// computation in LoadWeaponTdf can be redirected into our overflow array
// just by replacing EAX with a synthetic int E (see EntryHookProc below).
char*         g_allocBase   = nullptr;
WeaponStruct* g_alignedBase = nullptr;

// One bit per slot. True once LoadWeaponTdf has fully populated the slot.
constexpr int kOverflowSlots = WeaponIdOverflow::kMaxCapacity - WeaponIdOverflow::kBaseCapacity;
bool g_populated[kOverflowSlots] = { false };
int  g_populatedCount = 0;

InlineSingleHook* g_eaxSubstHook   = nullptr;
InlineSingleHook* g_nameLookupHook = nullptr;
InlineSingleHook* g_loadWipeHook   = nullptr;

// ---- helpers ---------------------------------------------------------------

WeaponStruct* baseArrayPtr()
{
    TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
    return taPtr ? &taPtr->Weapons[0] : nullptr;
}

// Find an aligned slot pointer inside g_allocBase such that the offset from
// the base array divides evenly by sizeof(WeaponStruct). Always succeeds when
// the allocation has at least sizeof(WeaponStruct)-1 padding bytes available.
WeaponStruct* computeAligned(char* alloc, WeaponStruct* arrayBase)
{
    const auto stride = (uintptr_t)sizeof(WeaponStruct);  // 0x115
    const uintptr_t a  = (uintptr_t)alloc;
    const uintptr_t b  = (uintptr_t)arrayBase;
    const uintptr_t r  = (a - b) % stride;
    const uintptr_t fix = r ? (stride - r) : 0;
    return (WeaponStruct*)(alloc + fix);
}

// Lazily allocate the overflow buffer the first time an overflow ID is seen.
// Returns false if g_TAMainStruct is not yet ready (in which case the engine
// would have crashed long before getting here, but be defensive anyway).
bool ensureAllocated()
{
    if (g_alignedBase)
        return true;
    WeaponStruct* arrayBase = baseArrayPtr();
    if (!arrayBase)
        return false;
    const size_t total = (size_t)kOverflowSlots * sizeof(WeaponStruct)
                         + (sizeof(WeaponStruct) - 1);
    g_allocBase = (char*)std::calloc(1, total);
    if (!g_allocBase)
        return false;
    g_alignedBase = computeAligned(g_allocBase, arrayBase);
    return true;
}

// Synthetic EAX value E that, when fed into the engine's
//   EBP = g_TAMainStruct + EAX*0x115 + 0x2CF3
// formula, lands EBP at &g_alignedBase[id - kBaseCapacity].
int computeSyntheticEAX(uint16_t id)
{
    WeaponStruct* arrayBase = baseArrayPtr();
    const ptrdiff_t deltaBytes = (char*)g_alignedBase - (char*)arrayBase;
    const int delta = (int)(deltaBytes / (ptrdiff_t)sizeof(WeaponStruct));
    return delta + (int)(id - WeaponIdOverflow::kBaseCapacity);
}

// ---- the LoadWeaponTdf entry hook -------------------------------------------

// Patch site at 0x42E468 (6 bytes). Disasm:
//   8B 15 E8 1D 51 00   MOV EDX, dword ptr [g_TAMainStruct]
// EAX at this point is the just-returned ID from TdfFile::GetInt("ID", -1)
// at 0x42E463. Subsequent instructions (0x42E46E..0x42E489) compute
//   EBP = g_TAMainStruct + EAX*0x115 + 0x2CF3
// which is the slot pointer the rest of LoadWeaponTdf writes into. By
// substituting EAX here we redirect the slot pointer into our overflow
// buffer. The 6 displaced bytes are reloaded from the trampoline by
// INLINE_5BYTESLAGGERJMP after our router returns.
constexpr DWORD kEntryHookAddr = 0x0042E468u;
constexpr DWORD kEntryHookLen  = 6u;

int __stdcall EntryHookProc(PInlineX86StackBuffer pBuf)
{
    const int id = (int)pBuf->Eax;
    if (id < 0 || id < WeaponIdOverflow::kBaseCapacity)
        return 0;  // base array path — let the engine proceed unchanged
    if (id >= WeaponIdOverflow::kMaxCapacity)
    {
        char msg[96];
        std::snprintf(msg, sizeof(msg),
            "WeaponIdOverflow: ID=%d exceeds kMaxCapacity=%d, dropping",
            id, WeaponIdOverflow::kMaxCapacity);
        IDDrawSurface::OutptTxt(msg);
        // Substitute slot 0 to neutralise the rest of LoadWeaponTdf — slot 0
        // is the null/sentinel weapon and gets clobbered, which is benign so
        // long as no weapon defines ID=0 in TDF.
        pBuf->Eax = 0;
        return 0;
    }
    if (!ensureAllocated())
        return 0;  // alloc failed — let it OOB-crash as it would have anyway
    pBuf->Eax = (DWORD)computeSyntheticEAX((uint16_t)id);
    return 0;
}

// ---- WeaponName2WeaponTypeDef extension -------------------------------------

// Patch site at 0x49E5F3 (5 bytes, the engine's "name not found" path):
//   31 C0      XOR EAX, EAX        ; EAX = NULL
//   5F         POP EDI
//   5E         POP ESI
//   5B         POP EBX
//   C2 04 00   RET 4
// Reached only when the linear scan over Weapons[256] failed to match. EBX
// still holds the original name pointer (POP EBX is at 0x49E5F7, inside our
// patch but executed *only* by the LAGGERJMP trampoline if we return 0).
//
// Behaviour:
//   * No overflow match -> return 0; the trampoline runs the displaced bytes
//     (XOR EAX,EAX; POP EDI/ESI/EBX), control resumes at RET 4 (0x49E5F8) and
//     the function returns NULL exactly as before.
//   * Overflow match -> set EAX = matched WeaponStruct*, redirect rtnAddr to
//     a custom epilogue that pops the function-prologue-saved registers and
//     returns. We can't let the displaced bytes run because XOR EAX,EAX would
//     clobber our match pointer.
constexpr DWORD kNameLookupHookAddr = 0x0049E5F3u;
constexpr DWORD kNameLookupHookLen  = 5u;

// Custom epilogue. Equivalent to the engine's RET path at 0x49E5F5..0x49E5F8
// (POP EDI; POP ESI; POP EBX; RET 4) but preserves EAX. Used only when we've
// found an overflow match and need to return it to the caller.
__declspec(naked) static void __stdcall NameLookupOverflowEpilogue()
{
    __asm
    {
        pop edi
        pop esi
        pop ebx
        ret 4
    }
}

int __stdcall NameLookupHookProc(PInlineX86StackBuffer pBuf)
{
    if (!g_alignedBase || g_populatedCount == 0)
        return 0;  // nothing to search — fall through to engine's NULL return

    const char* name = (const char*)pBuf->Ebx;
    if (!name || !*name)
        return 0;

    for (int k = 0; k < kOverflowSlots; ++k)
    {
        if (!g_populated[k])
            continue;
        WeaponStruct* slot = &g_alignedBase[k];
        if (slot->WeaponName[0] && _stricmp(slot->WeaponName, name) == 0)
        {
            pBuf->Eax = (DWORD)slot;
            pBuf->rtnAddr_Pvoid = (LPVOID)&NameLookupOverflowEpilogue;
            return X86STRACKBUFFERCHANGE;
        }
    }
    return 0;
}

// ---- LoadWeapons_Tdf wipe-on-entry hook -------------------------------------

// Patch site at 0x42E310 (6 bytes, function prologue):
//   81 EC 20 01 00 00   SUB ESP, 0x120
//
// LoadWeapons_Tdf is the engine's whole-array wipe-and-load entry point. When
// it fires we know the engine is about to (a) zero the base Weapons[256] array
// and (b) reload all *.tdf files. We mirror that by clearing our overflow
// populated bitmap and zeroing the buffer so a subsequent mod load doesn't
// see stale entries.
//
// This is the lightweight alternative to a full Release_WeaponTypeArray hook.
// It does NOT free per-slot heap allocations (pDamageArray, ModelDebris) so a
// mod restart leaks those, but the WeaponStruct overflow buffer is reused.
// A full free hook against Release_WeaponTypeArray (0x42F3A0) is left for a
// later phase if leak pressure becomes noticeable.
constexpr DWORD kLoadWipeHookAddr = 0x0042E310u;
constexpr DWORD kLoadWipeHookLen  = 6u;

int __stdcall LoadWipeHookProc(PInlineX86StackBuffer /*pBuf*/)
{
    if (g_alignedBase)
    {
        std::memset(g_alignedBase, 0, (size_t)kOverflowSlots * sizeof(WeaponStruct));
    }
    std::memset(g_populated, 0, sizeof(g_populated));
    g_populatedCount = 0;
    return 0;
}

// Post-parse handler — fires at the existing WeaponTdfHook site (0x42E4AB)
// after LoadWeaponTdf has set EBP and copied the weapon name. We use it to
// mark the overflow slot populated so GetDef can return it.
void OnWeaponTdfParsed(const WeaponTdfHook::Context& ctx)
{
    const int id = WeaponIdOverflow::GetId((const WeaponStruct*)ctx.pWeaponDef);
    if (id < WeaponIdOverflow::kBaseCapacity)
        return;
    const int k = id - WeaponIdOverflow::kBaseCapacity;
    if (k >= 0 && k < kOverflowSlots && !g_populated[k])
    {
        g_populated[k] = true;
        ++g_populatedCount;
        // The engine's LoadWeapons_Tdf init loop sets ID = (slot & 0xFF) for
        // base-array slots 0..255 only; overflow slots stay at calloc 0.
        // ID == 0 is the NOWEAPON sentinel — multiple engine paths early-out
        // on (pWeapon->ID == 0): UNITS_StartWeaponsScripts gates cStateMask
        // bit 1 (the "armed" bit) on it, AI_RecalcUnitBuildPriorities skips
        // it for build scoring, etc. So we stamp 0xFF.
        //
        // 0xFF collides with base[255]'s natural byte ID (the engine sets
        // base[i].ID = i for all base slots). The ONLY engine path that
        // compares byte IDs across weapons is Receive_AofEDamage's 4-tuple
        // (xyz + byteID) projectile match — for a base[255] AoE to false-
        // match an overflow projectile, both would have to land at the
        // exact same xyz on the same tick. Vanishingly unlikely. If a mod
        // ever needs perfect isolation, leave base[255] empty.
        WeaponStruct* slot = (WeaponStruct*)ctx.pWeaponDef;
        slot->ID = 0xFF;
    }
}

} // namespace

namespace WeaponIdOverflow {

void Install()
{
    if (g_eaxSubstHook)
        return;
    g_eaxSubstHook = new InlineSingleHook(
        kEntryHookAddr, kEntryHookLen,
        INLINE_5BYTESLAGGERJMP,
        (InlineX86HookRouter)EntryHookProc);
    g_nameLookupHook = new InlineSingleHook(
        kNameLookupHookAddr, kNameLookupHookLen,
        INLINE_5BYTESLAGGERJMP,
        (InlineX86HookRouter)NameLookupHookProc);
    g_loadWipeHook = new InlineSingleHook(
        kLoadWipeHookAddr, kLoadWipeHookLen,
        INLINE_5BYTESLAGGERJMP,
        (InlineX86HookRouter)LoadWipeHookProc);
    WeaponTdfHook::Register(OnWeaponTdfParsed);
}

void Shutdown()
{
    delete g_eaxSubstHook;     g_eaxSubstHook   = nullptr;
    delete g_nameLookupHook;   g_nameLookupHook = nullptr;
    delete g_loadWipeHook;     g_loadWipeHook   = nullptr;
    if (g_allocBase)
    {
        std::free(g_allocBase);
        g_allocBase   = nullptr;
        g_alignedBase = nullptr;
    }
    std::memset(g_populated, 0, sizeof(g_populated));
    g_populatedCount = 0;
}

WeaponStruct* GetDef(uint16_t id)
{
    if (id < kBaseCapacity)
    {
        WeaponStruct* base = baseArrayPtr();
        return base ? &base[id] : nullptr;
    }
    if (id >= kMaxCapacity || !g_alignedBase)
        return nullptr;
    const int k = id - kBaseCapacity;
    if (!g_populated[k])
        return nullptr;
    return &g_alignedBase[k];
}

int GetId(const WeaponStruct* def)
{
    if (!def)
        return -1;
    WeaponStruct* base = baseArrayPtr();
    if (base)
    {
        const auto delta = (const char*)def - (const char*)base;
        if (delta >= 0
            && delta < (ptrdiff_t)(kBaseCapacity * sizeof(WeaponStruct))
            && (delta % (ptrdiff_t)sizeof(WeaponStruct)) == 0)
        {
            return (int)(delta / (ptrdiff_t)sizeof(WeaponStruct));
        }
    }
    if (g_alignedBase)
    {
        const auto delta = (const char*)def - (const char*)g_alignedBase;
        if (delta >= 0
            && delta < (ptrdiff_t)(kOverflowSlots * sizeof(WeaponStruct))
            && (delta % (ptrdiff_t)sizeof(WeaponStruct)) == 0)
        {
            return kBaseCapacity + (int)(delta / (ptrdiff_t)sizeof(WeaponStruct));
        }
    }
    return -1;
}

int GetOverflowCount()
{
    return g_populatedCount;
}

} // namespace WeaponIdOverflow
