#include "RepairRateFix.h"
#include "config.h"
#include "tamem.h"
#include "hook/hook.h"

#include <cstdint>
#include <cstdlib>
#include <intrin.h>   // _ReturnAddress()

// The table is sized by dividing the engine's unit-array byte span by this stride,
// and the engine's own index math is `Begin + 0x118 * UnitInGameIndex`. If the
// struct ever stops matching, that division silently mis-sizes the table.
static_assert(sizeof(UnitStruct) == 0x118,
              "UnitStruct must be exactly 0x118 bytes -- the engine's unit array stride");

// Sanity bounds on the two balance multipliers, not just documentation: a typo
// that turns "3" into "30" or "0" is a real failure mode for a value meant to
// be hand-edited by a non-programmer before a rebuild (see config_escalation.h).
//   - 0 or negative silently reproduces the exact bug this whole module exists
//     to prevent: the resource gate still runs (energy drains), ApplyHeal's
//     callers still see a truthy return (nano-beam still draws), but `whole`
//     is <= 0 every tick so no HP ever moves. Indistinguishable in-game from
//     "repair completely broken" while every other signal says it's working.
//   - A very large value is still safe arithmetically (see the int64_t
//     overflow note below) but is almost certainly a fat-fingered extra
//     digit, not an intended balance value.
static_assert(REPAIR_RATE_FIX_REPAIR_MULTIPLIER >= 1 && REPAIR_RATE_FIX_REPAIR_MULTIPLIER <= 100,
              "REPAIR_RATE_FIX_REPAIR_MULTIPLIER out of sane range [1,100] -- 0/negative silently "
              "disables healing while still draining energy and drawing the nano-beam; see comment");
static_assert(REPAIR_RATE_FIX_SELFHEAL_MULTIPLIER >= 1 && REPAIR_RATE_FIX_SELFHEAL_MULTIPLIER <= 100,
              "REPAIR_RATE_FIX_SELFHEAL_MULTIPLIER out of sane range [1,100] -- 0/negative silently "
              "disables self-heal while still draining energy and drawing the nano-beam; see comment");

namespace {

// ---- accumulator table -------------------------------------------------------
//
// Keyed by the repairer's UnitInGameIndex (Unit+0xA8, int16), which is the unit's
// position in the engine's flat unit array:
//     Unit* u = BeginUnitsArray_p + 0x118 * UnitInGameIndex
// VERIFIED two ways -- from the binary, and empirically against live captures where
// Commander(1), Sumo(3), MAK(11) and MAK2(12) all resolved to an identical array
// base. Index 0 is reserved/invalid.
//
// Index keying rather than pointer keying, for three reasons:
//
//  1. EXPLOIT. The fallback path below heals at vanilla's ceil() rate, which is
//     FASTER than the fixed rate. With a pointer-keyed table, slots were claimed
//     permanently and the table was shared by every player, so a single player could
//     deliberately exhaust it with throwaway repairers and revert the whole match to
//     the old (faster) behaviour. One slot per live unit makes exhaustion
//     unreachable by construction.
//  2. GROWTH. A pointer-keyed table never recycles: dead units' slots leak, and
//     Install() runs once per process, so entries survived across matches. Index
//     slots are reused automatically when the engine reuses the array position --
//     confirmed in-game, and type-agnostic (a MAK inherited a dead Sumo's index).
//  3. DETERMINISM. The index is engine-assigned and transmitted in network packets,
//     so it is identical across clients by construction. Pointer keying only
//     survived desync review because slot assignment depended on call *sequence*;
//     this removes that argument entirely.
//
// TWO SLOTS PER UNIT, not one. The passive HealTime regen path shares this same
// function and calls it with BuilderPtr == Healed (verified: "push esi; push esi" at
// the 0x48AF94 call site). So one unit can legitimately have two distinct heal
// targets live in the same tick -- itself, plus whatever it is repairing. With a
// single slot the two calls would evict each other every tick, permanently zeroing
// the accumulator and collapsing the fix to floor() for that unit.
struct AccSlot
{
    void*   target;
    int32_t acc;
};

constexpr int kSlotsPerUnit = 2;

AccSlot*    g_table = nullptr;
int         g_tableUnits = 0;      // capacity in units; slot count is x kSlotsPerUnit
UnitStruct* g_cachedArrayBase = nullptr;
int         g_fallbackHits = 0;    // canary: see AccumulatorFor()

SingleHook* g_healHook = nullptr;

// (Re)size the table to match the engine's current unit array. Allocation must be
// lazy: Install() runs at DLL_PROCESS_ATTACH, long before the engine has allocated
// the array, so there is nothing to measure at that point. Follows the same
// ensureAllocated() shape as WeaponIdOverflow.cpp.
//
// Begin/End are re-checked on every call (two loads and a compare) so that a
// between-match reallocation resizes the table rather than silently leaving live
// indices out of range -- which would quietly re-open the exploit in reason 1 above.
bool EnsureTable()
{
    TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
    if (!taPtr)
        return false;

    UnitStruct* begin = taPtr->BeginUnitsArray_p;
    UnitStruct* end   = taPtr->EndOfUnitsArray_p;
    if (!begin || !end || end <= begin)
        return false;

    if (g_table && begin == g_cachedArrayBase)
        return true;   // unchanged since last check -- common case

    const ptrdiff_t bytes = (const char*)end - (const char*)begin;
    const int units = (int)(bytes / (ptrdiff_t)sizeof(UnitStruct));
    if (units <= 0)
        return false;

    std::free(g_table);
    g_table = (AccSlot*)std::calloc((size_t)units * kSlotsPerUnit, sizeof(AccSlot));
    if (!g_table)
    {
        g_tableUnits = 0;
        g_cachedArrayBase = nullptr;
        return false;
    }
    g_tableUnits = units;
    g_cachedArrayBase = begin;
    return true;
}

// Returns the accumulator cell for this (repairer, target) pair, zeroing it when the
// repairer switches to a different target (the banked fraction belongs to the old
// job and must not follow it to a new one).
//
// Returns nullptr only when the table is unavailable or the index is out of range.
// Callers MUST then fall back to vanilla ceil(), never floor(): over-healing is
// vanilla's own 25-year-old shipped behaviour, whereas floor() would silently heal
// nothing while still draining energy and drawing the nano-beam.
int32_t* AccumulatorFor(UnitStruct* repairer, void* target)
{
    if (!repairer || !EnsureTable())
    {
        ++g_fallbackHits;
        return nullptr;
    }

    const int idx = repairer->UnitInGameIndex;   // Unit+0xA8
    if (idx <= 0 || idx >= g_tableUnits)         // index 0 is reserved/invalid
    {
        ++g_fallbackHits;
        return nullptr;
    }

    AccSlot* pair = &g_table[(size_t)idx * kSlotsPerUnit];

    for (int i = 0; i < kSlotsPerUnit; ++i)
        if (pair[i].target == target)
            return &pair[i].acc;

    // No slot holds this target. Claim the first one that is free, else slot 0.
    // Claiming resets the accumulator, which is also what makes index recycling
    // after a unit's death safe: the new occupant's first call re-tags the slot.
    for (int i = 0; i < kSlotsPerUnit; ++i)
    {
        if (pair[i].target == nullptr)
        {
            pair[i].target = target;
            pair[i].acc = 0;
            return &pair[i].acc;
        }
    }

    pair[0].target = target;
    pair[0].acc = 0;
    return &pair[0].acc;
}

// ---- vanilla calls -------------------------------------------------------------
//
// Local one-off typedefs rather than additions to the shared tafunctions.h table,
// since only this file calls these two addresses (matches the convention already
// used for single-use engine calls in ConstructionKickout.cpp).

typedef int (__stdcall* _RepairHasEnoughResources)(void* resourceBlock, float energyCost);
_RepairHasEnoughResources RepairHasEnoughResources = (_RepairHasEnoughResources)0x00401180u;

typedef int (__stdcall* _ApplyHeal)(void* repairer, void* target, int amount, int type, int unused);
_ApplyHeal ApplyHeal = (_ApplyHeal)0x00489BB0u;

// ---- repair vs. self-heal discrimination ---------------------------------------
//
// The five vanilla call sites into 0x41BD10 split into two groups that want
// different multipliers: four active-repair orders (ground / self / no-move /
// VTOL, all sourcing t from WorkerTime/30) and one passive-regen path (t from
// HealTime, call site 0x48AF96). `repairer == target` does NOT distinguish
// them -- Order_SelfRepair (0x402518) also has repairer == target, so a unit
// ordered to repair itself would be misclassified as passive regen by that
// test alone.
//
// What DOES distinguish them, with no new hook and no mutable state: this
// function is reached from all five sites via a `jmp` (not `call`) at 0x41BD10
// -- SingleHook/INLINE_SINGLEJMP patches 0x41BD10's own first 5 bytes with an
// unconditional jmp here. A `jmp` pushes nothing, so the return address still
// on the stack when this function's prologue runs is exactly whichever
// vanilla `call 0x41BD10` the CPU actually executed. That return address is
// call-site address + 5 (the length of a `call rel32`), a compile-time
// constant identical on every client running this build -- no pointer-value
// dependence, satisfies the determinism rule in ENGINE_NOTES SS10 trivially.
//
// Deliberately NOT a second inline hook at 0x48AF96: that would need a naked
// trampoline to preserve/replay the original `call` instruction with correct
// register and stack state, which is real risk (relocation of a relative
// call, naked-function ABI edge cases) for no benefit over reading data that
// is already sitting on the stack at this function's own entry.
constexpr uintptr_t kPassiveRegenReturnAddr = 0x48AF9Bu;   // 0x48AF96 (the call) + 5 bytes

// If a unit is simultaneously the target of an Order_SelfRepair order AND
// eligible for passive regen in the same tick, both calls have repairer ==
// target == that unit and so land in the SAME accumulator slot (AccSlot is
// keyed on repairer index + target pointer, not on which call path invoked
// it). This is safe, not a bug: the exact-remainder accumulator is agnostic
// to where a (num, buildTime) contribution came from, and buildTime is
// necessarily identical between the two calls here (it's read from the
// target's UnitDef, and target is the same unit both times) -- see the
// accumulator math below. Two different multipliers threading through one
// slot in the same tick still sums exactly.

// ---- energy cost: vanilla-identical ceil, unchanged ---------------------------
//
// Vanilla computes max(1, ceil(BuildCostEnergy * t / BuildTime)) with a chained x87
// sequence at 80-bit intermediate precision, finished by a call into vanilla's own
// __ftol at 0x4E43A0. That calculation is intentionally NOT reimplemented with plain
// C++ float arithmetic: this is a lockstep multiplayer engine, and modern MSVC
// defaults to SSE2 for float/double math on x86, whose intermediate precision can
// differ from the original 1990s x87 codegen in rare boundary cases -- a desync
// risk not worth taking. There is no isolated vanilla subroutine left to call into
// for just this piece (the ceil was inlined in the very function this hook
// replaces), so the safest port reproduces the exact instruction sequence and
// calls into 0x4E43A0 for the truncation itself, exactly as the CE reference does.
int32_t ComputeVanillaEnergyCeil(float buildCostEnergy, float t, int32_t buildTime)
{
    static const float kOne = 1.0f;
    static const float kNegOne = -1.0f;
    int result;

    __asm
    {
        fild dword ptr [buildTime]
        fld  dword ptr [buildCostEnergy]
        fmul dword ptr [t]
        fsub dword ptr [kOne]
        fdiv st(0), st(1)
        fsub dword ptr [kNegOne]
        mov  eax, 0x4E43A0
        call eax                  ; vanilla __ftol: truncates st(0) toward zero -> eax, pops st(0)
        fstp st(0)                ; discard the buildTime value left on the FPU stack
        mov  result, eax
    }

    return (result < 1) ? 1 : result;
}

// ---- the replacement function --------------------------------------------------
//
// Signature matches the original exactly, confirmed against the full disassembly:
//     HealUnit_HealTimeWay(UnitStruct* BuilderPtr, UnitStruct* Healed, float Amount)
// arg1 = repairer, arg2 = target, all on the stack (12 bytes) with stdcall cleanup
// ret 0xC -- an ordinary MSVC __stdcall function produces exactly this, so no naked
// function or manual prologue is needed. Replacing this one address covers all five
// vanilla call sites (ground / self / no-move / VTOL repair, plus passive HealTime
// regen) automatically, since they all funnel through here.
//
// The vanilla function was audited instruction by instruction: it is exactly
// full-HP check -> compute HP and energy -> resource gate -> apply. There are NO
// state-mask tests, cloak checks, under-construction guards, timer writes or other
// side effects to preserve.
//
// DELIBERATE DIVERGENCES FROM VANILLA (all intentional; see the port plan):
//   1. HP is an exact remainder accumulator instead of max(1, ceil(...)). The fix.
//   2. buildTime <= 0 returns 0. Vanilla has no such guard -- its float divide
//      yields infinity, __ftol maps that to INT_MIN, and max(1,.) heals 1. Here the
//      arithmetic is integer, so an unguarded divide would fault. Unreachable for
//      real units; the guard exists purely so a corrupt UNITINFO cannot crash.
//   3. Sub-1-HP ticks apply nothing (they bank instead). Still returns 1 so the
//      nano-beam does not flicker. The fix.
//   4. maxHp * t is integer here, floating point in vanilla. Strictly more exact --
//      float loses precision past 2^24, reachable only at factory-tier t.
//   5. The applied amount is clamped to 0..65535 (see below). Vanilla does not
//      clamp, which is a latent bug in vanilla rather than a behaviour to copy.
//   6. HP delivered is scaled by REPAIR_RATE_FIX_REPAIR_MULTIPLIER or
//      REPAIR_RATE_FIX_SELFHEAL_MULTIPLIER (config_escalation.h) depending on
//      which of the two the call turns out to be -- see the discrimination
//      comment below. A balance buff, not a fix; energy cost is untouched on
//      purpose, per Wotan.
int __stdcall HealUnit_HealTimeWay_Fixed(void* repairer, void* target, float t)
{
    UnitStruct* tgt = static_cast<UnitStruct*>(target);
    UnitDefStruct* def = tgt->UnitType;   // Unit+0x92

    // The engine reads/writes nMaxHP as a full dword (VERIFIED from disassembly),
    // even though this repo's tamem.h declares the field as unsigned short -- read
    // the same 4 bytes the engine does (which also covers the adjacent `data8`
    // field as the high word) rather than the narrower typed accessor, matching
    // vanilla's own signed 32-bit compare/multiply on this value.
    int32_t maxHp     = *reinterpret_cast<const int32_t*>(&def->nMaxHP);
    int32_t buildTime = static_cast<int32_t>(def->buildtime);
    int32_t curHp     = tgt->Health;   // Unit+0x108; `short` -> `int32_t` sign-extends, matching vanilla's `movsx`

    if (curHp >= maxHp)
        return 0;                          // already at full HP
    if (buildTime <= 0)
        return 0;                          // divide guard

    int32_t tInt = static_cast<int32_t>(t);   // t is always integral at every known call site (see CLAUDE.md)

    int32_t energy = ComputeVanillaEnergyCeil(def->buildcostenergy, t, buildTime);

    // Unit+0xBC: unnamed resource-accounting block inside UnitStruct::data17[50] in
    // this repo's tamem.h -- reached by raw offset rather than restructuring that
    // struct (its layout beyond this one field is unverified and other code may
    // depend on its declared size).
    void* resourceBlock = reinterpret_cast<char*>(repairer) + 0xBC;

    // The resource gate MUST run, and MUST succeed, before the accumulator below is
    // touched: otherwise a boundary-crossing tick that fails the energy check would
    // silently discard banked HP fraction. (This was a real bug in the first
    // live-tested version of this fix; the ordering here is load-bearing, not
    // stylistic.)
    if (!RepairHasEnoughResources(resourceBlock, static_cast<float>(energy)))
        return 0;                          // gate failed: nothing banked, nothing lost

    if (tInt <= 0)
        return 1;                          // gate passed, nothing owed this tick -- two of
                                            // the five callers gate nano-beam drawing on the
                                            // return value, so this must still be 1

    const bool isPassiveSelfHeal =
        (reinterpret_cast<uintptr_t>(_ReturnAddress()) == kPassiveRegenReturnAddr);
    const int32_t multiplier = isPassiveSelfHeal
        ? REPAIR_RATE_FIX_SELFHEAL_MULTIPLIER
        : REPAIR_RATE_FIX_REPAIR_MULTIPLIER;

    // The multiplier scales the numerator, not the final `whole`, so the
    // exact-remainder accumulator below banks fractional HP at the multiplied
    // rate too -- e.g. 3 ticks that would each round to 0.9 HP bank as
    // 3x0.9=2.7 (delivering 1 HP on tick 3) rather than 3x floor(0.9)=0 every
    // tick forever. Energy is computed separately, above, from the
    // unmultiplied buildCostEnergy/t/buildTime -- this only changes HP
    // delivered per unit of energy already spent, never the energy cost
    // itself.
    //
    // Overflow: maxHp is a dword (up to ~2^31, see the nMaxHP comment above),
    // tInt reaches 640 at factory tier, multiplier is bounded to 100 by the
    // static_asserts above. Worst case ~2^31 * 640 * 100 ~= 1.4e14, four
    // orders of magnitude under int64_t's ~9.2e18 ceiling -- no overflow at
    // any value the static_asserts allow, real mod data or not.
    int64_t num   = static_cast<int64_t>(maxHp) * static_cast<int64_t>(tInt)
                   * static_cast<int64_t>(multiplier);
    int32_t whole = static_cast<int32_t>(num / buildTime);
    int32_t rem   = static_cast<int32_t>(num % buildTime);

    int32_t* acc = AccumulatorFor(static_cast<UnitStruct*>(repairer), target);
    if (acc)
    {
        *acc += rem;
        if (*acc >= buildTime)
        {
            *acc -= buildTime;
            whole += 1;
        }
    }
    else
    {
        // INVARIANT: no accumulator available -> fall back to vanilla ceil, never
        // floor. This is the safety property that makes table exhaustion degrade to
        // (already-shipped, already-live) vanilla behaviour instead of the "beam
        // draws, energy drains, HP doesn't move" failure mode a small round-robin
        // table can hit under heavy load.
        if (rem > 0)
            whole += 1;
        if (whole < 1)
            whole = 1;
    }

    if (whole <= 0)
        return 1;

    // 16-bit clamp. Two VERIFIED facts combine into a real hazard here
    // (ENGINE_NOTES SS25.3/SS25.8, not this file's own claim): the amount this
    // call passes ends up truncated into a 16-bit word in the net damage
    // packet (0x489C71), so a value outside 0..65535 wraps to something much
    // smaller; and the store that applies it, `HP = min(HP + amount, nMaxHP)`
    // at 0x489D7A, compares UNSIGNED against a dword nMaxHP -- documented as a
    // live full-heal hazard when HP is ever negative (T5 proved -16608 is
    // reachable). Vanilla has the same 16-bit truncation; it is simply
    // unreachable there because vanilla's amount is always small. Guard
    // explicitly rather than relying on that: maxHp is read as a dword
    // spanning nMaxHP plus the adjacent field, and factory-tier t (640)
    // combined with a multiplier up to 100 is not structurally small.
    if (whole > 65535)
        whole = 65535;

    // DmgType 0x0A is the raw path -- amount applied verbatim, no scaling. All
    // OTHER damage types route through armour lookup, EXCEPT that damage
    // >= 30000 bypasses armour entirely (it doesn't clamp the amount -- it
    // skips the *scaling*, at 0x489BD1; that bypass exists so the engine's
    // universal kill token, always exactly 30000, can't be blocked by
    // armour). Not relevant to this call: type 0x0A never reaches that check.
    ApplyHeal(repairer, target, whole, 0x0A, 0);
    return 1;
}

} // namespace

namespace RepairRateFix {

void Install()
{
    if (g_healHook)
        return;

    // No allocation here on purpose. This runs at DLL_PROCESS_ATTACH, before the
    // engine has allocated its unit array, so there is nothing to size against yet
    // -- EnsureTable() does it on first use and re-checks every call thereafter.
    g_healHook = new SingleHook(0x0041BD10u, 5, INLINE_SINGLEJMP,
        reinterpret_cast<LPBYTE>(&HealUnit_HealTimeWay_Fixed));
}

void Shutdown()
{
    delete g_healHook;
    g_healHook = nullptr;

    std::free(g_table);
    g_table = nullptr;
    g_tableUnits = 0;
    g_cachedArrayBase = nullptr;
    g_fallbackHits = 0;
}

} // namespace RepairRateFix
