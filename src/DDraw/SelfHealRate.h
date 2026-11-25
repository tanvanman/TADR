#pragma once

// Passive self-heal, replacing the engine's HealTime regen block.
//
// FBI key SELFHEAL_RATE_FBI_KEY (config_escalation.h: "HealTime" in Escalation,
// matched case-insensitively), value N = HP per second as the player experiences it, up to 3
// decimals, parsed once per unit type with integer arithmetic: N points of
// weapon damage undone per second while the unit is armoured. Raw HP per
// second = N * DamageModifier (the Q16.16 value at UnitDef+0x1AA), so two units
// with the same N regenerate the same effective HP whatever DamageModifier they
// use to emulate their health. Applied every tick through an exact remainder
// accumulator; no energy cost; independent of BuildTime and of HealTime's bit
// pattern.
//
// A unit whose key is absent (or N <= 0) keeps the HealTime formula exactly as
// RepairRateFix computes it (gate (GameTime & HealTime & 0xFF) == 0,
// t = floor(HealTime * 16 / 15), HP = MaxDamage * t * SELFHEAL_MULTIPLIER /
// BuildTime), with no energy cost.
//
// Splice: a 6-byte jmp at 0x48AF37 (the regen block of
// Unit_TickAllPlayersScriptsAndWeapons), resuming at 0x48AF9B. The block runs
// for every unit of a local owner each tick (owner gate 0x48AEB5), so this code
// only touches the owner's own units; other clients learn their HP from 0x2C.
// SELFHEAL_RATE_ENABLE in config_*.h.

namespace SelfHealRate {

    // Verifies the exe bytes, runs the self-test, registers the FBI key and
    // installs the splice. Must run at DLL attach, before unit data is loaded.
    // On any failure it logs DISABLED and leaves the engine block untouched.
    void Install();

    // Restores the engine bytes and frees the accumulator table.
    void Shutdown();

}
