#pragma once

// Faster enemies-in-range search. CLASS A: the same list, members and order, purely faster.
//
// THE FUNCTION. 0x40AD80, __stdcall(int player, const int* point, int radius, int unused,
// UnitVector* out), `ret 0x14`, lists every enemy unit a player can see within `radius` of
// `point`. Its four call sites: the weapon target finder 0x40B7B0 (at 0x40B82F and 0x40B85F),
// 0x401CE0 (at 0x401D2C) and the guard order 0x4021F0 (at 0x402379); each passes a zeroed
// vector and the seat byte at unit+0xFF. It reads the player's record from the table at
// 0x5119C0 and walks two unit vectors in it:
//   * +0x05: every enemy unit the player sees (begin +0x09, end +0x0D);
//   * +0x15: the player's radar contacts (begin +0x19, end +0x1D), walked only when the
//     record's +0x79 is non-zero (a targeting facility is active) AND `out` is still empty
//     after the first walk.
// For each unit it squares dx = point[0] - XPos and dz = point[2] - YPos (16.16 dwords) in 64
// bits through two CRT helpers each (__allmul 0x4E4400, then a shift by 32 at 0x4E43D0), adds
// the two high dwords and compares the sum, signed, with radius * radius (32-bit). A unit in
// range that is alive (UnitStateMask 0x10000000) and not dying (0x4000) is appended to `out`.
//
// WHY IT MATTERS. The target finder draws TA's random stream over this list, so its members AND
// THEIR ORDER are simulation state, and the four helper calls an enemy are most of its cost.
// MEASURED with the check build below (3.9.02 exe, OTA), on the same calls: the engine's
// function took 4.54x the cycles of this one over 271 985 calls of a skirmish with four AIs,
// and 3.89x over 266 878 calls of a 262-unit-a-side battle with targeting facilities,
// where 255 593 of the calls also walked the radar contacts.
//
// WHAT THIS DOES. Install() puts a 5-byte JMP at 0x40AD80 to FindCandidates(), which reads the
// same fields in the same order and does the same arithmetic inline (__emul high dwords; the sum
// and radius * radius wrap in 32 bits). Appends store in place while the vector has room, as the
// engine's vector insert 0x408F30 does, and otherwise call 0x408F30. Both of the engine's appends
// (0x408F30 in the first walk, the inlined one in the radar walk) grow a full vector to
// size + max(1, size) through 0x4B4F10 (DISASSEMBLED), so the vector, capacity included, is the
// engine's.
//
// A seat without a record gets an empty list and one log line, where the engine would read a
// null record and crash; a seat outside 0..9, where the engine would read past the table, the
// same. Stock never asks for either.
//
// SAFE ACROSS BUILDS AND PEERS. Install() compares the engine's whole function (0x40AD80 ..
// 0x40B0C2, 0x342 bytes) with the stock bytes this file mirrors and patches only on a match
// (FunctionPatch); on a mismatch it logs the first differing byte and leaves the engine alone.
// Because the results are identical, a peer running this build and one without it stay in sync.
//
// CHECK BUILD. TDRAW_TARGET_CANDIDATES_VERIFY=1 also runs the engine's own function beside this
// one, compares the lists, counts radar walks and times both; tdrawlog.txt gets the totals
// periodically and on Shutdown(). Developer measurements only; release builds leave it 0.
//
// Other code at these addresses: none in tdraw or the recorder (the function, its four callers,
// 0x408F30 and the table at 0x5119C0 are not hooked or patched elsewhere).

namespace TargetCandidates
{
	// Compares the function with the stock bytes and, only on a match, installs the jump. Idempotent.
	void Install();

	// Restores the engine's 5 bytes. Safe to call even if Install() patched nothing.
	void Shutdown();
}
