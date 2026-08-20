#include "WideHealth.h"
#include "tamem.h"
#include "hook/hook.h"

#include <cstdint>
#include <cstdlib>
#include <memory>
#include <vector>

// Both struct sizes are load-bearing for the index/stride math below -- if either
// stops matching, the table sizing and the type-index division both silently
// mis-size or mis-resolve. Same convention as RepairRateFix.cpp.
static_assert(sizeof(UnitStruct) == 0x118,
              "UnitStruct must be exactly 0x118 bytes -- the engine's unit array stride");
static_assert(sizeof(UnitDefStruct) == 0x249,
              "UnitDefStruct must be exactly 0x249 bytes -- the engine's UnitDef array stride");

namespace {

// ---- constants -----------------------------------------------------------------

// The highest raw MaxDamage any Escalation unit ships with today (`[hpi]` VERIFIED,
// see ENGINE_NOTES.md §25.5) -- also the highest value the 16-bit HP field and the
// 30,000 kill token can both safely hold at once. Any real-max at or below this
// needs no scaling at all (S == 1, proxy is an exact identity) -- verified true for
// 405 of 547 unit types in the current archives (verify_proxy_math.py).
constexpr int32_t kProxyCap = 24480;

// Upper bound on a raw FBI MaxDamage this module will accept as realMax. Not a
// gameplay limit -- a crash-safety one. `S = ceil(realMax / kProxyCap)` signed-
// overflows once realMax exceeds INT32_MAX - kProxyCap (~2.1477e9), and the
// division that follows (`proxyMax = realMax / S`) turns that overflow into a
// division-by-zero crash. Since def-load runs identically and deterministically
// on every client from the same mod archive, an unbounded realMax here means a
// single malformed FBI (a typo, or a deliberately crafted one) crashes every
// player in the lobby simultaneously, not just whoever loaded it first. Today's
// shipping archives top out at 24,480 (405 of 547 types need no scaling at
// all), so 100,000,000 -- ~24x the highest real value anywhere in the mod, and
// ~2,700x the largest documented Phase-3 migration target (4,080,000, `corms`)
// -- is generous headroom for any hand-edited or generated FBI while staying
// nowhere near the overflow boundary.
constexpr int32_t kRealMaxSaneCeiling = 100'000'000;

struct TypeInfo
{
    int32_t realMax;
    int32_t S;
    int32_t proxyMax;
};

// ---- realHP table: one int32 per live unit array SLOT ---------------------------
//
// Keyed by UnitInGameIndex (Unit+0xA8), exactly like RepairRateFix's accumulator
// table and for the same three reasons documented at the top of that file: index
// keying survives array-slot recycling (observed live, 2026-08-15 -- the same
// address held a corfav and later a corkrog), never depends on a pointer value
// (determinism), and is engine-assigned so every client resolves the same unit to
// the same slot by construction.
int32_t*    g_realHP = nullptr;
int         g_realHPCapacity = 0;      // capacity in units
UnitStruct* g_cachedUnitArrayBase = nullptr;

// ---- type table: one TypeInfo per UnitDef array SLOT -----------------------------
//
// Keyed by (def - UnitDef base) / sizeof(UnitDefStruct), the same array-index math
// findUnitInfoId() uses in the Lua test helper. Populated by the def-load hook as
// each unit type's FBI is parsed, so this is fully warm before any unit can spawn.
TypeInfo*       g_typeInfo = nullptr;
int             g_typeInfoCapacity = 0;
UnitDefStruct*  g_cachedTypeArrayBase = nullptr;

// Counts drift corrections -- see the header's "Graceful degradation" section.
// Not currently surfaced anywhere; a debug build could log it per type to find
// which unhooked path (build progress vs savegame vs transfer vs map-spawn) is
// actually firing in a given session.
int32_t g_driftCanary = 0;

// ---- TEMPORARY DIAGNOSTICS (added 2026-08-18) ------------------------------
// Verification step 5B failed: whScanTypes() reported 0 of 550 types recorded,
// i.e. WideHealth_DefLoadWrite never wrote to g_typeInfo for ANY type, while
// EnsureTables() demonstrably DID succeed later (both tables allocated, correct
// bases cached). Static analysis confirmed the hook site itself is right
// (0x42C39E stores the parsed "maxdamage" value, ebp is a def struct -- ebp+0x20
// is UnitName), so the failure is a RUNTIME condition inside the router, not a
// wrong address. These counters distinguish every early-return path so one test
// run answers which, instead of guessing.
//
// COST: plain int increments on a path that runs ~550 times TOTAL, once at match
// load. Not a per-tick path, not in the simulation loop -- zero measurable
// runtime impact. Remove once the cause is known.
int32_t g_dbgDefLoadCalls    = 0;   // router entered at all
int32_t g_dbgDefLoadNoTables = 0;   // bailed: EnsureTables() false
int32_t g_dbgDefLoadBadIndex = 0;   // bailed: GetTypeIndex() < 0
int32_t g_dbgDefLoadBadValue = 0;   // bailed: realMax <= 0 or > ceiling
int32_t g_dbgDefLoadOk       = 0;   // actually recorded a TypeInfo
DWORD   g_dbgLastTaPtr       = 0;   // [0x511DE8] as seen during def-load
DWORD   g_dbgLastEbp         = 0;   // the def pointer the router was handed
DWORD   g_dbgLastTypeBase    = 0;   // ta->UnitDef as seen during def-load
int32_t g_dbgLastTypeCount   = 0;   // ta->UNITINFOCount as seen during def-load
int32_t g_dbgLastEax         = 0;   // raw maxdamage value seen
DWORD   g_dbgLastUnitBegin   = 0;   // ta->BeginUnitsArray_p during def-load
DWORD   g_dbgLastUnitEnd     = 0;   // ta->EndOfUnitsArray_p   during def-load


std::vector<std::unique_ptr<InlineSingleHook>> g_hooks;

// (Re)size both tables to match the engine's current arrays. Both engine arrays are
// allocated after this DLL's DLL_PROCESS_ATTACH runs, so this must stay lazy and
// re-checked on every call -- exactly RepairRateFix::EnsureTable()'s reasoning,
// including the "a between-match reallocation must resize the table" half of it.
// THE TABLES ARE INDEPENDENT AND MUST BE ENSURED INDEPENDENTLY.
//
// This was one function until 2026-08-18, and coupling them was a real bug with
// a measured symptom: the def-load hook fired 549 times and bailed 549 times,
// leaving g_typeInfo completely empty, because the combined check also demanded
// the UNITS array -- which does not exist yet while FBIs are being parsed. No
// units have been created at that point in the load; there is nothing to size a
// unit table against, and there does not need to be. Def-load only ever touches
// the TYPE table.
//
// Measured evidence for the diagnosis (instrumented build, live skirmish):
//   entered=549  bail:no-tables=549  bail:bad-index=0  bail:bad-value=0  ok=0
//   during def-load: ta=0x2D03BF3, ta->UnitDef=0xA98DB38, UNITINFOCount=550
// ta and the type array were both valid and sane, which leaves the unit-array
// gate as the only one that could have failed.
//
// Also settled by that run, and worth recording because it was an open question
// (project CLAUDE.md, Open Question 4): ta->UnitDef during def-load and
// g_cachedTypeArrayBase after gameplay started were the SAME value
// (0xA98DB38 both times). The UnitDef array does NOT get relocated between FBI
// parse and play, so a populated type table survives -- EnsureTypeTable()'s
// realloc-on-base-change path will not silently wipe it.

// TYPE table only. Safe to call during FBI parsing, before any unit exists.
bool EnsureTypeTable()
{
    TAdynmemStruct* ta = *reinterpret_cast<TAdynmemStruct**>(0x00511de8);
    if (!ta)
        return false;

    UnitDefStruct* typeBase = ta->UnitDef;
    unsigned int typeCount = ta->UNITINFOCount;
    if (!typeBase || typeCount == 0 || typeCount > 5000)   // same sanity bound as the Lua helper
        return false;

    if (!g_typeInfo || typeBase != g_cachedTypeArrayBase)
    {
        std::free(g_typeInfo);
        g_typeInfo = static_cast<TypeInfo*>(std::calloc(static_cast<size_t>(typeCount), sizeof(TypeInfo)));
        if (!g_typeInfo)
        {
            g_typeInfoCapacity = 0;
            g_cachedTypeArrayBase = nullptr;
            return false;
        }
        g_typeInfoCapacity = static_cast<int>(typeCount);
        g_cachedTypeArrayBase = typeBase;
    }

    return true;
}

// UNIT table only. Cannot succeed until the engine has allocated the unit array,
// which is fine: every caller of this runs during gameplay, not during load.
bool EnsureUnitTable()
{
    TAdynmemStruct* ta = *reinterpret_cast<TAdynmemStruct**>(0x00511de8);
    if (!ta)
        return false;

    UnitStruct* unitBegin = ta->BeginUnitsArray_p;
    UnitStruct* unitEnd = ta->EndOfUnitsArray_p;
    if (!unitBegin || !unitEnd || unitEnd <= unitBegin)
        return false;

    if (!g_realHP || unitBegin != g_cachedUnitArrayBase)
    {
        const ptrdiff_t bytes = reinterpret_cast<const char*>(unitEnd) - reinterpret_cast<const char*>(unitBegin);
        const int units = static_cast<int>(bytes / static_cast<ptrdiff_t>(sizeof(UnitStruct)));
        if (units <= 0)
            return false;

        std::free(g_realHP);
        g_realHP = static_cast<int32_t*>(std::calloc(static_cast<size_t>(units), sizeof(int32_t)));
        if (!g_realHP)
        {
            g_realHPCapacity = 0;
            g_cachedUnitArrayBase = nullptr;
            return false;
        }
        g_realHPCapacity = units;
        g_cachedUnitArrayBase = unitBegin;
    }

    return true;
}

// Both, for the routers that genuinely index both tables. Same total work as the
// old combined function -- two pointer loads and a compare each on an already-warm
// path -- so this split costs nothing at runtime.
bool EnsureTables()
{
    const bool unitsOk = EnsureUnitTable();
    const bool typesOk = EnsureTypeTable();
    return unitsOk && typesOk;
}

// -1 means "not resolvable right now" -- callers must fall back to vanilla, never
// treat -1 as index 0 (a real, valid type).
int GetTypeIndex(UnitDefStruct* def)
{
    if (!def || !g_cachedTypeArrayBase)
        return -1;

    const ptrdiff_t byteOffset = reinterpret_cast<char*>(def) - reinterpret_cast<char*>(g_cachedTypeArrayBase);
    if (byteOffset < 0 || byteOffset % sizeof(UnitDefStruct) != 0)
        return -1;

    const ptrdiff_t index = byteOffset / static_cast<ptrdiff_t>(sizeof(UnitDefStruct));
    if (index < 0 || index >= g_typeInfoCapacity)
        return -1;

    return static_cast<int>(index);
}

// The proxy formula, `[game]` VERIFIED end-to-end (T4, T5) and exhaustively proven
// over all 547 shipping unit types and every realHP in 0..realMax
// (verify_proxy_math.py, 33,385,047 cases, zero failures). int64 intermediate:
// realHP * proxyMax peaks near 4.1e6 * 24480 ~= 1.0e11, outside int32.
inline int32_t DeriveProxy(int32_t realHP, const TypeInfo& info)
{
    if (realHP <= 0)
        return 0;
    if (info.realMax <= 0)   // corrupt/unresolved type table entry -- never divide by it
        return 0;

    int64_t p = (static_cast<int64_t>(realHP) * info.proxyMax) / info.realMax;
    if (p < 1) p = 1;                      // alive must never look dead (H1) -- load-bearing
    if (p > info.proxyMax) p = info.proxyMax;
    return static_cast<int32_t>(p);
}

// Every unit's UnitInGameIndex, bounds-checked against the live table. -1 on
// anything that isn't safe to index with.
inline int GetUnitIndex(UnitStruct* unit)
{
    if (!unit)
        return -1;
    const int idx = unit->UnitInGameIndex;   // Unit+0xA8, VERIFIED int16 (ENGINE_NOTES §4.1)
    if (idx <= 0 || idx >= g_realHPCapacity)  // index 0 is reserved/invalid, same as RepairRateFix
        return -1;
    return idx;
}

// ---- hook routers ----------------------------------------------------------------

// UnitDef_LoadFbiProperties's `maxdamage` store, 0x42C39E:
//     mov dword ptr [ebp+0x1FA], eax     ; eax = raw FBI MaxDamage, ebp = def base
// `[bin]` VERIFIED disassembly, 2026-08-15. This is the def-load pass: intercept the
// store, treat eax as realMax directly (Phase 3's data migration, not this module,
// is what makes MaxDamage large in the first place -- see WideHealth.h), compute
// S/proxyMax, record both in the type table, and overwrite eax so the ORIGINAL
// instruction (replayed after this router returns) writes proxyMax instead of the
// raw value. "Hook the store, not the load" -- same pattern as every other write
// site in this module.
int __stdcall WideHealth_DefLoadWrite(PInlineX86StackBuffer buf)
{
    // --- TEMPORARY DIAGNOSTICS: capture state BEFORE any early return ---
    ++g_dbgDefLoadCalls;
    g_dbgLastEbp = buf->Ebp;
    g_dbgLastEax = static_cast<int32_t>(buf->Eax);
    {
        TAdynmemStruct* dbgTa = *reinterpret_cast<TAdynmemStruct**>(0x00511de8);
        g_dbgLastTaPtr = reinterpret_cast<DWORD>(dbgTa);
        if (dbgTa)
        {
            g_dbgLastTypeBase  = reinterpret_cast<DWORD>(dbgTa->UnitDef);
            g_dbgLastTypeCount = static_cast<int32_t>(dbgTa->UNITINFOCount);
            // Proves WHICH EnsureTables() gate failed: if these are null/inverted
            // during def-load, the unit-array gate is confirmed as the culprit.
            g_dbgLastUnitBegin = reinterpret_cast<DWORD>(dbgTa->BeginUnitsArray_p);
            g_dbgLastUnitEnd   = reinterpret_cast<DWORD>(dbgTa->EndOfUnitsArray_p);
        }
    }
    // --- end diagnostics ---

    if (!EnsureTypeTable())
    {
        ++g_dbgDefLoadNoTables;
        return 0;   // type table not ready -- let the raw value through unscaled
    }

    UnitDefStruct* def = reinterpret_cast<UnitDefStruct*>(buf->Ebp);
    const int typeIdx = GetTypeIndex(def);
    if (typeIdx < 0)
    {
        ++g_dbgDefLoadBadIndex;
        return 0;
    }

    const int32_t realMax = static_cast<int32_t>(buf->Eax);
    if (realMax <= 0)
    {
        ++g_dbgDefLoadBadValue;
        return 0;   // nothing to scale (nanoframe defs, etc.) -- leave vanilla's value alone
    }
    if (realMax > kRealMaxSaneCeiling)
        return 0;   // refuse a value that risks overflowing S below -- leave vanilla's
                     // (truncated, but not crash-inducing) raw value alone instead

    TypeInfo info;
    info.realMax = realMax;
    info.S = (realMax + kProxyCap - 1) / kProxyCap;         // ceil(realMax / kProxyCap)
    if (info.S <= 0)
        return 0;   // defensive: should be unreachable given the ceiling check above,
                     // but this is the one division in the module that can crash the
                     // whole match if it's ever wrong, so it gets its own belt-and-
                     // suspenders guard rather than trusting the bound alone
    info.proxyMax = realMax / info.S;
    g_typeInfo[typeIdx] = info;
    ++g_dbgDefLoadOk;

    buf->Eax = static_cast<DWORD>(info.proxyMax);
    return 0;
}

// Unit_InitInstanceFields, main spawn branch, 0x485B1E:
//     mov ax, word ptr [edx+0x1FA]        ; edx = UnitDef, now holds proxyMax
//     mov word ptr [esi+0x108], ax        ; esi = new unit  <-- hook here
// `[bin]` VERIFIED. Vanilla's own store is already correct (it's writing the
// proxyMax the def-load hook put there) -- this router doesn't touch it, it only
// needs to ALSO set realHP = realMax for the new unit, and clear the slot first
// (H3: array indices are recycled, confirmed live 2026-08-15).
int __stdcall WideHealth_SpawnFullHealth(PInlineX86StackBuffer buf)
{
    if (!EnsureTables())
        return 0;

    UnitStruct* unit = reinterpret_cast<UnitStruct*>(buf->Esi);
    UnitDefStruct* def = reinterpret_cast<UnitDefStruct*>(buf->Edx);
    const int idx = GetUnitIndex(unit);
    const int typeIdx = GetTypeIndex(def);
    if (idx < 0 || typeIdx < 0)
        return 0;

    // H3: a single unconditional overwrite, not a read-modify-write -- this is what
    // makes recycled-slot correctness automatic. Whatever a previous occupant of
    // this index left behind (a dead unit's last realHP) is replaced outright, the
    // same way vanilla's own store unconditionally replaces the previous occupant's
    // proxy HP a few bytes away from this hook.
    g_realHP[idx] = g_typeInfo[typeIdx].realMax;
    return 0;
}

// Unit_InitInstanceFields, nanoframe branch, 0x485B37:
//     mov word ptr [esi+0x108], bx        ; bx = 0  <-- hook here
// `[bin]` VERIFIED. Same reasoning as the full-health branch, realHP = 0 instead.
int __stdcall WideHealth_SpawnNanoframe(PInlineX86StackBuffer buf)
{
    if (!EnsureUnitTable())   // only touches g_realHP
        return 0;

    UnitStruct* unit = reinterpret_cast<UnitStruct*>(buf->Esi);
    const int idx = GetUnitIndex(unit);
    if (idx < 0)
        return 0;

    g_realHP[idx] = 0;   // H3: clear the recycled slot (there is nothing to "set" after -- 0 either way)
    return 0;
}

// Unit_ApplyNetDamagePacket, heal-add write, 0x489D80:
//     movsx eax, word ptr [esi+0x108]     ; old proxy HP
//     ...
//     xor edx,edx / mov dx,[edi+5]        ; amount, real-space heal quantity
//     mov ecx,[ecx+0x1FA]                 ; proxyMax
//     add eax,edx / cmp eax,ecx / jb .. / mov eax,ecx   ; vanilla's own (proxy-space,
//                                                          therefore wrong) clamp
//     mov word ptr [esi+0x108], ax        ; <-- hook here
// `[bin]` VERIFIED, 2026-08-15. `amount` is real-space by construction: it is
// whatever the caller (vanilla repair, or RepairRateFix once wired up) decided to
// heal, computed against realMax, not proxyMax -- see WideHealth.h's scope note on
// the RepairRateFix integration that is NOT yet wired up. Vanilla's own clamped sum
// in eax is proxy-space and discarded; this router computes the real-space result
// itself and overwrites only the low word of eax, so the replayed store writes the
// correct proxy instead.
int __stdcall WideHealth_HealWrite(PInlineX86StackBuffer buf)
{
    UnitStruct* unit = reinterpret_cast<UnitStruct*>(buf->Esi);
    if (!EnsureTables())
        return 0;   // vanilla's own (proxy-only) math already sits in eax -- let it write

    const int idx = GetUnitIndex(unit);
    UnitDefStruct* def = unit->UnitType;   // Unit+0x92
    const int typeIdx = GetTypeIndex(def);
    if (idx < 0 || typeIdx < 0)
        return 0;

    const TypeInfo& info = g_typeInfo[typeIdx];
    if (info.realMax <= 0)
        return 0;

    // Drift check (H7) + graceful degradation for every write path this module
    // doesn't hook yet (build progress, savegame load, transfer, map-spawn, net
    // sync -- see WideHealth.h). If the live proxy doesn't match what our own
    // table implies, something else changed it; re-adopt rather than compound the
    // error silently.
    const int16_t liveProxy = *reinterpret_cast<int16_t*>(reinterpret_cast<char*>(unit) + 0x108);
    if (DeriveProxy(g_realHP[idx], info) != liveProxy)
    {
        ++g_driftCanary;
        g_realHP[idx] = static_cast<int64_t>(liveProxy) * info.S;
    }

    const int32_t amount = static_cast<int32_t>(buf->Edx & 0xFFFF);   // real-space heal amount

    int64_t realHP = static_cast<int64_t>(g_realHP[idx]) + amount;
    if (realHP > info.realMax) realHP = info.realMax;
    if (realHP < 0) realHP = 0;   // H1 defensive clamp -- structurally unreachable on this path, cheap anyway
    g_realHP[idx] = static_cast<int32_t>(realHP);

    const int32_t newProxy = DeriveProxy(g_realHP[idx], info);
    buf->Eax = (buf->Eax & 0xFFFF0000u) | (static_cast<DWORD>(newProxy) & 0xFFFFu);
    return 0;
}

// Unit_ApplyNetDamagePacket, damage-subtract write, 0x489EB5:
//     mov ax, word ptr [edi+5]            ; amount, real-space damage quantity
//     sub word ptr [esi+0x108], ax        ; <-- hook here (read-modify-write, no
//                                            separate load of the old proxy exists
//                                            to intercept)
// `[bin]` VERIFIED, 2026-08-15. This instruction is a memory-destination `sub`: the
// original bytes, replayed after this router returns, subtract whatever is in ax
// from the CURRENT value at [esi+0x108]. Rather than fight that, this router
// computes the correct old/new proxy itself and sets ax to the DELTA
// (oldProxy - newProxy) that makes vanilla's own subtract land exactly on the right
// number. That delta is always >= 0 on this path (damage only ever lowers realHP,
// and DeriveProxy is monotonic -- proven exhaustively), so no sign handling needed.
//
// No hook exists at the third write in this function, the zero-on-death at
// 0x489EF1 (`mov word ptr [esi+0x108], 0`) -- deliberately, not an oversight.
// Vanilla's own death test right after this site (`cmp word ptr [esi+0x108],0 /
// jg`) reads the proxy THIS hook just wrote, and DeriveProxy guarantees
// `proxy == 0 <=> realHP == 0` exactly (proven invariant, verify_proxy_math.py). So
// vanilla's own branch to the zero-write already agrees with real-space death in
// every case, and the zero-write itself is already the right value to write when it
// runs. Adding a third hook there would be redundant, not more correct.
int __stdcall WideHealth_DamageSubtractWrite(PInlineX86StackBuffer buf)
{
    UnitStruct* unit = reinterpret_cast<UnitStruct*>(buf->Esi);
    if (!EnsureTables())
        return 0;

    const int idx = GetUnitIndex(unit);
    UnitDefStruct* def = unit->UnitType;
    const int typeIdx = GetTypeIndex(def);
    if (idx < 0 || typeIdx < 0)
        return 0;

    const TypeInfo& info = g_typeInfo[typeIdx];
    if (info.realMax <= 0)
        return 0;

    const int16_t oldProxy = *reinterpret_cast<int16_t*>(reinterpret_cast<char*>(unit) + 0x108);
    if (DeriveProxy(g_realHP[idx], info) != oldProxy)
    {
        ++g_driftCanary;   // H7 -- see WideHealth_HealWrite for the full reasoning
        g_realHP[idx] = static_cast<int64_t>(oldProxy) * info.S;
    }

    // H10 fix, 2026-08-19. Self-destruct, kill-all-on-team (both type 3),
    // ownership transfer (type 4), and reverse-build's below-zero-progress kill
    // (type 9) all push exactly 0x7530 (30000) as an engine idiom for "kill this
    // unit, unconditionally" (CLAUDE.md Hard Rule 6). At MaxDamage <= 24480 that
    // always worked by construction -- 30000 necessarily exceeded the unit's own
    // real max. Under real 32-bit HP it doesn't, and nothing in this file checked
    // the damage TYPE before this fix, so widened units survived all four events
    // ([live] VERIFIED 2026-08-19, step 6D -- self-destruct at 180,000 real HP
    // left the unit alive at 150,000).
    //
    // [bin] VERIFIED 2026-08-19: every one of the four call sites (self-destruct
    // 0x402147, reverse-build's kill 0x41BC49, kill-all-on-team 0x486F94,
    // transfer 0x4886A4/0x4887D0) funnels through Unit_ApplyDamageAndBroadcast
    // (0x489BB0), which at 0x489C89 calls THIS function (Unit_ApplyNetDamage-
    // Packet, 0x489CE0) with a locally-built packet whose byte+8 is the caller's
    // own type argument, untouched by the armour/veterancy scaling in between
    // (stored from `bl` at 0x489C85, and `bl` still holds the original `ebx` type
    // parameter at that point). EDI holds that packet pointer for the ENTIRE
    // function body, confirmed by reading every instruction from entry (0x489CE9)
    // to this hook site (0x489EB5) -- it is never reassigned, so it is safe to
    // read here exactly as vanilla itself already does at 0x489EB1/0x489EFA.
    //
    // Deliberately NOT keyed on the packet's damage amount (`buf->Eax`): Hard
    // Rule 6 exists precisely because that value is post-armour AND
    // post-veterancy by the time it reaches here -- a vet-5 target sees 24,000
    // for the same 30,000 token (verified arithmetic: floor(30000*80/100)). The
    // type byte is the only value in this packet immune to that shrinkage.
    const uint8_t dmgType = *reinterpret_cast<const uint8_t*>(reinterpret_cast<const char*>(buf->Edi) + 8);
    const bool isKillToken = (dmgType == 3 || dmgType == 4 || dmgType == 9);

    // eax's upper 16 bits are NOT guaranteed clear here (only `mov ax,...` ran
    // before this hook fires) -- mask explicitly.
    const int32_t amount = isKillToken
        ? g_realHP[idx]   // drain to exactly 0 -- the TYPE is the kill signal,
                           // not whatever (possibly vet-shrunk) number rode along
        : static_cast<int32_t>(buf->Eax & 0xFFFF);

    int64_t realHP = static_cast<int64_t>(g_realHP[idx]) - amount;
    if (realHP < 0) realHP = 0;
    g_realHP[idx] = static_cast<int32_t>(realHP);

    const int32_t newProxy = DeriveProxy(g_realHP[idx], info);
    int32_t delta = oldProxy - newProxy;   // see comment above: usually >= 0, but NOT
                                            // provably so -- the H7 reconciliation just
                                            // above can set g_realHP[idx] from oldProxy
                                            // in a way that doesn't round-trip exactly
                                            // back through DeriveProxy (floor-division
                                            // rounding), so a small/zero-damage hit
                                            // landing right after a reconciliation can
                                            // make newProxy > oldProxy. An unclamped
                                            // negative delta here would mask into a huge
                                            // unsigned 16-bit value, and vanilla's
                                            // replayed `sub` would underflow the live
                                            // proxy -- reintroducing H1 (negative proxy
                                            // HP full-heals on the next heal tick)
                                            // through the one write path that wasn't
                                            // already guarded against it.
    if (delta < 0) delta = 0;

    buf->Eax = (buf->Eax & 0xFFFF0000u) | (static_cast<DWORD>(delta) & 0xFFFFu);
    return 0;
}

// MissionTick_Resurrect, 0x405226:
//     mov word ptr [edx+0x108], 1         ; edx = resurrected unit  <-- hook here
// `[bin]` VERIFIED. The hardcoded immediate is already correct at any scale --
// DeriveProxy(1, ...) == 1 always, by the same "alive never looks dead" invariant
// H1 relies on -- so this router doesn't touch eax/edx at all, it only mirrors the
// value into the real-space table.
int __stdcall WideHealth_ResurrectWrite(PInlineX86StackBuffer buf)
{
    if (!EnsureUnitTable())   // only touches g_realHP
        return 0;

    UnitStruct* unit = reinterpret_cast<UnitStruct*>(buf->Edx);
    const int idx = GetUnitIndex(unit);
    if (idx < 0)
        return 0;

    g_realHP[idx] = 1;
    return 0;
}

// H2 -- the kill-token hole. Unit_HandleKilledPacket forwards a damage TYPE read
// from the network packet (a small bit-manipulation sequence around 0x4867E2,
// `[bin]` seen but not fully mapped to named values) rather than a fixed constant,
// unlike the other six 30,000-token call sites. That's what ENGINE_NOTES §25.4
// already documents as "variable, from packet". Keying instakill on type alone
// therefore misses this site: a widened unit (realMax > 30000) receiving its own
// kill notification through this path would only lose 30,000 real HP, not die --
// exactly the survival T4 proved happens once a unit's raw HP crosses that number.
//
// Rather than resolve the exact argument-to-parameter mapping into the call at
// 0x4867FE (attempted, inconclusive -- the visible pushes don't cleanly add up to
// the 5-argument stdcall signature RepairRateFix.cpp already confirmed working for
// the same callee, and guessing at it is exactly the kind of unverified claim this
// project avoids), this hook sits AFTER that call returns, at 0x486810
// (`mov ecx, dword ptr [esi+0x8a]`, `[bin]` VERIFIED). `esi` is used consistently as
// "the unit being killed" throughout the surrounding code on both sides of the
// call (0x4867BA, 0x4867CA, 0x486804, and again here at 0x486810), independent of
// whatever the call's own arguments turn out to mean. This hook doesn't touch the
// call or its result at all -- it just forces realHP to 0 unconditionally once the
// kill has been dispatched, which is correct regardless of what type value the
// vanilla path used internally, and composes safely with WideHealth_DamageSubtractWrite
// firing earlier in the same call chain (whatever that hook computed, this one
// overrides it -- the last write wins, and 0 is always the right final answer here).
int __stdcall WideHealth_KillPacketPostCall(PInlineX86StackBuffer buf)
{
    if (!EnsureUnitTable())   // only touches g_realHP
        return 0;

    UnitStruct* unit = reinterpret_cast<UnitStruct*>(buf->Esi);
    const int idx = GetUnitIndex(unit);
    if (idx < 0)
        return 0;

    g_realHP[idx] = 0;
    return 0;
}

} // namespace

namespace WideHealth {

void Install()
{
    if (!g_hooks.empty())
        return;

    // No allocation here on purpose, same reasoning as RepairRateFix::Install():
    // this runs at DLL_PROCESS_ATTACH, before the engine has allocated either the
    // unit array or the UnitDef array. EnsureTables() sizes both lazily on first
    // use and re-checks every call thereafter.

    g_hooks.push_back(std::make_unique<InlineSingleHook>(
        0x0042C39Eu, 5, INLINE_5BYTESLAGGERJMP, WideHealth_DefLoadWrite));
    g_hooks.push_back(std::make_unique<InlineSingleHook>(
        0x00485B1Eu, 5, INLINE_5BYTESLAGGERJMP, WideHealth_SpawnFullHealth));
    g_hooks.push_back(std::make_unique<InlineSingleHook>(
        0x00485B37u, 5, INLINE_5BYTESLAGGERJMP, WideHealth_SpawnNanoframe));
    g_hooks.push_back(std::make_unique<InlineSingleHook>(
        0x00489D80u, 5, INLINE_5BYTESLAGGERJMP, WideHealth_HealWrite));
    g_hooks.push_back(std::make_unique<InlineSingleHook>(
        0x00489EB5u, 5, INLINE_5BYTESLAGGERJMP, WideHealth_DamageSubtractWrite));
    g_hooks.push_back(std::make_unique<InlineSingleHook>(
        0x00405226u, 5, INLINE_5BYTESLAGGERJMP, WideHealth_ResurrectWrite));
    g_hooks.push_back(std::make_unique<InlineSingleHook>(
        0x00486810u, 5, INLINE_5BYTESLAGGERJMP, WideHealth_KillPacketPostCall));
}

void Shutdown()
{
    g_hooks.clear();

    std::free(g_realHP);
    g_realHP = nullptr;
    g_realHPCapacity = 0;
    g_cachedUnitArrayBase = nullptr;

    std::free(g_typeInfo);
    g_typeInfo = nullptr;
    g_typeInfoCapacity = 0;
    g_cachedTypeArrayBase = nullptr;

    g_driftCanary = 0;
}

__int32 GetRealMax(UnitDefStruct* def)
{
    const int typeIdx = GetTypeIndex(def);
    if (typeIdx < 0)
        return -1;
    const TypeInfo& info = g_typeInfo[typeIdx];
    if (info.realMax <= 0)
        return -1;
    return info.realMax;
}

} // namespace WideHealth
