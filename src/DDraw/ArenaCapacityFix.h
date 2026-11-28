#pragma once

// Gives the unit-picture arena (the CMemoryCache at [TAdynmemStruct+0x1437B], sized in GameData_LoadAll) the size its formula
// intends on a PC with 2 GB of RAM or more.
//
// Size = ScreenWidth * ScreenHeight * 2 * 1.3 * scale, scale = 1.0 for n <= 16 and min(n * 0.0625, 5.0) above, n = MB of RAM + 1,
// from DisplayContext+0x620 (dwTotalPhys as GlobalMemoryStatus stores it: a signed 32-bit field, saturated to 0xFFFFFFFF from
// 4 GB). The engine shifts it arithmetically, so a value with bit 31 set (2 GB and up) gives n <= 16: the smallest arena,
// 5.14 MB at 1920x1080 instead of the 25.7 MB it gives from 80 MB of RAM upward. Every unit picture lives in that ring; once the
// pictures on screen outgrow it, every unit is re-rasterized every frame.
//
// Raw 13-byte splice at 0x0042D41E: `cdq; and edx,0xFFFFF; add eax,edx; sar eax,0x14; inc eax` becomes `shr eax,0x14; inc eax;
// 9 x nop` (unsigned MB + 1). Identical for every value below 2^31 (cdq adds 0 there, sar equals shr); it differs only where
// bit 31 is set. Nothing jumps into the window (the five instructions end on a boundary at 0x42D42B).
//
// CLASS A: the pictures are built by the same code whether they are cached or rebuilt, so the output is bit-identical; render-only,
// client-local, no simulation state. Costs memory: 5 x the old arena (+20.6 MB at 1920x1080).
//
// Install() verifies the 13 original bytes and refuses to write on any mismatch, then reads the splice back from live memory and
// logs one line to tdrawlog.txt. Gated by ARENA_CAPACITY_FIX_ENABLE (Escalation only; every config_*.h defines it explicitly).

namespace ArenaCapacityFix {

    // Verifies the original bytes and, only on an exact match, writes the splice. Idempotent. Must run before GameData_LoadAll.
    void Install();

    // Restores the original bytes if this module wrote the splice.
    void Shutdown();

}
