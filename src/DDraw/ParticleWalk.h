#pragma once

// The particle layers' tick in one pass: the same calls on the same particles in the same order, and
// each layer left with the same pointers in [begin, end) and the same end; purely faster. (The
// unused capacity past the end holds other stale pointers than the engine's shifts leave there.)
//
// THE FUNCTION. 0x471EB0, cdecl, no argument, called once a tick from the game loop's step
// (0x4955BF in 0x495490). It walks the ten particle layers of the table at *(main+0x38D77), 16
// bytes each, a vector of particle-object pointers with begin at +4 and end at +8. For each object
// in order it calls slot 3 of the object's vtable (thiscall, nonzero when the object has nothing
// left to show). On nonzero it calls slot 0 with 1 (the deleting destructor), moves every later
// pointer of the layer down one place and takes 4 off the end, and the next object is then the one
// at the same place; otherwise it calls slot 1, the update. The end is read again at every turn.
// DISASSEMBLED, 0x471EB0..0x471F3E.
//
// WHY IT MATTERS. Removing k objects from a layer of n moves the rest of the layer k times, k*n
// moves a tick. A big battle fills a layer with tens of thousands of objects (smoke, trails), so
// the shifting becomes a large part of the step.
//
// WHAT THIS DOES. Install() puts a 5-byte JMP at 0x471EB0 to WalkLayers(), which makes the engine's
// three calls on the same objects in the same order and writes each surviving pointer to its final
// place once, then sets the layer's end. Each layer ends with the same pointers in the same order
// and the same end; begin and capacity are never touched.
//
// THE INVARIANT IT RESTS ON. While a layer is walked, the places between the last survivor kept and
// the object being visited hold stale pointers, and the end is lowered only when the layer is done:
// states the engine's own walker never shows. So nothing the three slots reach may read or change
// the layer table, nor read a layer (it would meet the stale places). The table pointer main+0x38D77
// is named at nineteen places in the image: its
// allocation 0x471DCB and release 0x471DEC / 0x471E97, this walker 0x471EBB, the two draws
// 0x471F4B and 0x471F9C, and the thirteen emitters 0x471FD9..0x472C58. Following direct calls from
// every slot of the seven particle classes' vtables (0x4FD588, 0x4FD5A8, 0x4FD5B8, 0x4FD5D8,
// 0x4FD5F8, 0x4FD618, 0x4FD638) and of 0x4FD580 reaches 207 functions and none of those nineteen
// places (DISASSEMBLED). That walk does not follow indirect calls, and does not see tdraw's own
// code inside it: TeamColorNanolathe's hooks at 0x4739E6 (in 0x4739B0, called from slot 1 0x472EB0)
// and 0x473F3B, which use only their own tag table and the main struct pointer and do not name the
// layer table -- a change to them must keep it so. The check build below tests the moves at run
// time as well.
//
// SAFE ACROSS BUILDS AND PEERS. Install() compares the engine's whole walker (0x471EB0 .. 0x471F3E,
// 0x8F bytes) with the stock function this file mirrors and patches only on a match
// (FunctionPatch); on a mismatch it logs the first differing byte and leaves the engine alone.
// Because the calls and the layers are identical, a peer running this build and one without it
// stay in sync.
//
// CHECK BUILD. TDRAW_PARTICLE_WALK_VERIFY=1 runs, in turn for 64 ticks each, the engine's own
// walker (timed), this one (timed), this one with checks, and this one (timed) again; the checked
// ticks run the same walk as the release build. They test after every call that the layer table,
// its begin, end and capacity are unchanged -- a slot that moves the layer, not one that only reads
// it -- and at the end that each layer holds what the engine's walker would leave given the same
// answers. tdrawlog.txt gets the totals periodically and on Shutdown(), with each engine block
// compared against the mean of the two timed blocks either side of it. Developer measurements
// only; the switch is in config.h and release builds leave it 0.
//
// Other code at these addresses: none in tdraw or the recorder (the walker, its call at 0x4955BF
// and the table at main+0x38D77 are not hooked or patched elsewhere).

namespace ParticleWalk
{
	// Verifies the fingerprint and, only on a match, installs the jump. Idempotent.
	void Install();

	// Restores the engine's 5 bytes. Safe to call even if Install() patched nothing.
	void Shutdown();
}
