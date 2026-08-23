#pragma once

// Ports the reverse-engineered repair-rate fix documented in CLAUDE.md into a real
// hook. See ai-reference/repair_fix_v2.CEA for the Cheat Engine reference this was
// ported from, and CLAUDE.md for the full formula derivation and live-test log.
//
// HealUnit_HealTimeWay (0x41BD10) rounds each repairer's fractional HP contribution
// UP, per repairer, per tick, with no pooling -- so several cheap repairers out-heal
// one expensive repairer with identical total Build Power. This module replaces that
// function wholesale with a version that carries the exact integer remainder forward
// per (repairer, target) pair instead of re-rounding it away every tick.
//
// Energy cost is deliberately left as vanilla ceil (unchanged) -- see CLAUDE.md.
//
// HP delivered is additionally scaled by REPAIR_RATE_FIX_REPAIR_MULTIPLIER (active
// repair, four order call sites) or REPAIR_RATE_FIX_SELFHEAL_MULTIPLIER (passive
// HealTime regen, one call site) -- both share this one hooked function, but
// which multiplier applies is decided per call via a return-address check, not
// by anything visible in this header. Two knobs, not one, because a flat
// multiplier lands very differently on the two mechanics: RepairRateFix's own
// rounding fix already cut passive regen 1-2 orders of magnitude for
// slow-BuildTime units, so repair and self-heal need independent tuning. Both
// are balance buffs layered on top of the rounding fix, both are
// Escalation-only, and both require REPAIR_RATE_FIX_ENABLE 1 (config.h fails
// the build otherwise) -- see config_escalation.h.

namespace RepairRateFix {

    // Installs the hook and allocates the per-repairer accumulator table (sized to
    // the same UnitLimit the engine itself is configured with). Idempotent.
    void Install();

    // Restores the original function and frees the accumulator table. Safe to call
    // multiple times.
    void Shutdown();

}
