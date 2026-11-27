#pragma once

// The ground units' pathfinder: our scheduler in the engine's place, its searches on helper threads
// beside the game, each search the engine's own (PathSearch.h). Faster, and quicker to answer an
// order -- but NOT the engine's timing: see WHAT CHANGES.
//
// THE ENGINE'S. Once a tick the players phase calls the scheduler 0x40EB70 (from 0x464F92). It shares
// a budget of examinations and A* pops among the players (TADR's AISearchMapEntries, 66 650 by
// default, written at 0x40EAD6), walks each player's block of the unit array round-robin, and for a
// unit whose ground course wants a search (course +0x64 bit 1) and whose last search began 60 ticks
// ago or more (0x44F260) runs the search start 0x40E630 and A* passes until the budget is spent; a
// search may span ticks. Before a search the class's watermark 0x440AF0 marks the units that have
// stood still for 30 ticks as obstacles in the class's map and, for a searcher that was an obstacle
// already, recomputes its own rectangle without it; the class's cleanup 0x440BE0 puts it back. The heuristic's weight of a player
// rises the more slowly the budget serves its units. A path's corners reach the course through set
// path 0x44F080, the search's flags the goal's order through 0x44CED0. MEASURED on the battle below,
// budget 66 650: about 23 searches a tick, a unit waiting 48 ticks on average for its search, 4.5 ms
// a tick in the scheduler.
//
// OURS. The call at 0x464F92 goes to Dispatch(); the engine's scheduler does not run.
//   1. THE WALK, one pass over the unit array: every unit of a player the engine serves whose course
//      wants a search is READY when its stamp +0x60 is 0 (a new goal), or it is not blocked and has
//      fewer than two corners left of a path its last search found (it reached its path's end), or
//      its last search began 60 ticks ago or more (the engine's gate: a blocked unit, or one whose
//      search found nothing). The first two kinds are served before the third, each from its own
//      cursor on, so within a kind no unit starves and a jam never delays a new order.
//   2. THE TICK'S SEARCHES: at most kCapSearches units are chosen, with the engine's writes for each --
//      the course's stamp; the class's watermark and the marking of the units that have just become
//      obstacles, once a class a tick, the searcher left out, as 0x440AF0 does; for a searcher that is
//      an obstacle, its rectangle recomputed through the engine's 0x440830, as the cleanup leaves it. The searches
//      begin in the order chosen, none once the pops of the tick's ended ones reach kCapPops.
//   3. BESIDE THE GAME: each search reads a copy of its class's map and of the explored map taken
//      after the tick's writes, and its own rectangle through a patch where the engine's would read
//      it apart from the map: the engine's rule 0x47E1F0 over the engine's cells with the searcher
//      left out, or, for a searcher the engine's watermark does not recompute (it has just stood
//      still), the map's entries as the sweep left them. So it reads no engine memory and writes
//      nothing shared. Up to three helper threads search while the game thread goes on.
//   4. THE JOIN, at the step's call 0x4955E5, waits for nothing: an ended search's result goes through
//      the engine's own set path and flag call, in the order chosen; one not begun is put back; one
//      still running is carried to a later join. A set goal 0x44F2A0 on a course with a search due
//      meets it first, so a result never lands on a goal it was not found for.
//   The weight is the engine's base in a tick with at most kQuiet units ready, 3 times it while every
//   ready unit can begin this tick, 6 times it beyond: the engine's three weights, by the tick's load.
//
// WHAT CHANGES, and what does not. Each path is exactly the engine's search on that tick's maps (the
// check build compares them search by search). What differs is WHEN a unit is searched and at which
// of the engine's three weights: no budget, so about 30 searches a tick against 23 and a unit waits
// about 20 ticks against 48; and a long search's path may arrive a tick or more after the tick that
// chose it, which depends on the machine. Units answer orders sooner. A machine searches for its own
// players' units alone: only the local ground course (vtable 0x4FD458) asks for a search, a remote
// unit's ground proxy answering no (its slot is 0x44EFF0, `xor eax, eax`), and its moves reach the
// peers as the owner's. So a peer without this build is not desynced; each player's units follow the
// rules of that player's own build.
//
// THE BOUNDS. Every value read from engine memory is bounded before use: a player's block against the
// unit array, the class record against the 32 records, the grid's size against the class's and the
// pathfinder's, a cell's unit slot against the array's end, a feature definition against the count,
// the patch's rectangle against kPatchMax, a slot against kSlots. A result is applied only where its
// slot still holds the unit, mover, class and course searched for, with the search's stamp, and no
// set goal has come to the course since.
//
// THE ORDERING. Everything but the searches runs on the thread that runs the players phase -- the game
// thread in play, the loader once during a load -- inside the scheduler, the join or a set goal.
// Helpers read engine memory only in the tick's first handing-out (the queries and the copies), while
// that thread waits inside the scheduler; in the second, the searches, they read their own job and
// pages. Helpers are woken only in play (main+0x38D75 is 0). Pathfinder.cpp, THE TASKS HANDED OUT.
//
// A NEW GAME is the pathfinder's own: the game's setup makes the pathfinder object (0x44F6A0, from
// 0x4919CD) and calls its init 0x40E9E0 once, from 0x44F6B3; that call is ours too. Ours serves a
// game from its first tick with no engine search under way. THE GAME'S END is the level teardown
// 0x491B60's first call (0x491B76), also ours: every search is dropped there, before the teardown
// frees an order or a unit, and ours stands aside until the next init.
// A GAME OURS CANNOT SERVE -- a unit array past kSlots, no explored map, no memory for a scratch --
// is the engine's scheduler's from the tick that finds it on, and the log says so.
//
// SAFE ACROSS BUILDS. Every engine function this follows or calls -- the search, its goals, the
// scheduler, the watermark, the remark, the cell rules, set goal and set path -- is fingerprinted
// whole against the stock exe, and each call redirected is checked for its stock target; they are
// identical in the five distinct game exes of the reference setup, retail 3.1 among them. On any
// mismatch nothing is installed and the engine's scheduler runs.
//
// CHECK BUILD. TDRAW_PATHFIND_VERIFY=1 alternates blocks of kBlock ticks: the engine's scheduler, then
// ours. In the engine's blocks our search runs beside each of the engine's (observers at the watermark
// 0x440AF0, the grid reset 0x40D900, the extraction 0x40E050 and the cleanup 0x440BE0) and its result is compared field by field
// with what the engine wrote: the exit, the path, the goal cell, the pool's use, the bug walk's answer
// and the order's flags; a fresh patch is compared with the entries the engine has just recomputed. In our
// blocks each search made beside the game is made again on the game thread on the engine's own maps
// and the two results compared. Both kinds of block are timed on the game thread with the checks' own
// time taken out, and report searches a tick and the units' waits. tdrawlog.txt gets the totals
// periodically and on Shutdown(). Developer measurements only; release builds leave it 0.
//
// Other code at these addresses: none in tdraw. TADR writes the budget at 0x40EAD6, inside the init
// 0x40E9E0, which this calls unchanged; unitrotate.cpp hooks the teardown's entry 0x491B60, whose
// first five bytes the call at 0x491B76 is not among.

namespace Pathfinder
{
	// Checks every site and, only when all hold the stock code, installs. Idempotent.
	void Install();

	// Restores the engine's bytes. The helper threads are left: at process exit the system has
	// already ended them.
	void Shutdown();
}
