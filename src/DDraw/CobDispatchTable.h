#pragma once

// Replaces the COB bytecode interpreter's 28-node, 141-instruction opcode-
// dispatch compare ladder with a 256-entry jump table. Measured at 19-23% of
// main-thread wall time across three profile captures of real late-game
// matches. Full derivation, status and test results:
// ai-reference/simulation-performance/COB_DISPATCH_PROJECT.md and CLAUDE.md.
// Reproduce the static verification against the shipped binary with
// ai-reference/tools/exe/cob_dispatch.py --verify.
//
// CLASS A: bit-identical simulation output, purely faster. Same 57 opcodes
// accepted, same error path otherwise; EDX/EFLAGS proven dead at all 57
// handler entries; nothing keyed on pointers, timing, or thread identity; no
// new floating point.
//
// Raw byte splice, not an InlineSingleHook: the window has no call boundary
// or return path to preserve. Install() verifies the original 38 bytes match
// exactly before writing anything and refuses to patch on any mismatch --
// the only failure mode, and it fails safe.
//
// Gated behind COB_DISPATCH_TABLE_ENABLE (Escalation only; config.h defaults
// every other config to 0). No runtime toggle -- Install()/Shutdown() only.

namespace CobDispatchTable {

    // Verifies the 38-byte splice-window signature and, only on an exact match,
    // installs the jump table. Idempotent. Leaves the engine untouched on any
    // mismatch (wrong build, or a table entry found outside the VM region).
    // Also self-checks the write by reading the window back from live memory
    // and logs one line to tdrawlog.txt -- see CobDispatchTable.cpp for why.
    void Install();

    // Restores the original 38 bytes. Safe to call even if Install() never
    // patched anything.
    void Shutdown();

}
