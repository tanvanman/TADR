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

namespace RepairRateFix {

    // Installs the hook and allocates the per-repairer accumulator table (sized to
    // the same UnitLimit the engine itself is configured with). Idempotent.
    void Install();

    // Restores the original function and frees the accumulator table. Safe to call
    // multiple times.
    void Shutdown();

}
