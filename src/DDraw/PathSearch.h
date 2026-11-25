#pragma once

#include <cstddef>
#include <cstdint>

// One ground unit's path search, whole, on maps it only reads: the engine's search start 0x40E630
// after its watermark, the A* passes of the scheduler's in-search branch (0x40ED90..0x40EEE2) to
// their end, and the extraction 0x40E050. The pops, their order and every result are the engine's;
// each function in PathSearch.cpp names the engine code it follows, DISASSEMBLED from the stock
// TotalA.exe. Arithmetic the engine does in 32 bits wraps here too (it goes through uint32_t).
//
// What the engine's search start writes to the shared map before it reads it -- the searcher's own
// rectangle recomputed with the searcher left out, when the searcher is an obstacle -- is here a
// Patch the caller hands over: the search reads those entries from it and writes nothing shared.
// The search writes its Scratch and its Result and nothing else, so searches on different scratches
// run side by side (Pathfinder.cpp).
//
// Where it differs from the engine's in how, not in what: a heap entry carries a copy of its node's
// f, so a sift stays inside the heap; a cell's read takes the map, the grids' sizes, the explored map
// and the patch from a view made once a walk or an A* pass; the grid's reset steps from one touched
// block to the next. The scratch's pool is at most 65 536 nodes, the engine's node word being 16 bits:
// a search that wants more answers Overflow with no result.
namespace PathSearch
{
	// The goal kinds, by the id each goal vtable's slot +0x08 returns.
	enum GoalKind { GOAL_NONE = 0, GOAL_POINT = 4, GOAL_ANNULUS = 5, GOAL_RING = 6 };

	// What the search ORs into the goal's order node +0x4E (the engine's 0x44CED0).
	const uint32_t GOALF_NOPATH  = 0x040;   // no path while the unit is not in its goal (set path, 0x44F0A4)
	const uint32_t GOALF_WALKIN  = 0x100;   // the start is in the goal, or the bug walk reached it
	const uint32_t GOALF_WALKOUT = 0x200;   // the start is off the grid, or the bug walk did not reach it

	const int kWaypoints = 20;              // set path 0x44F080 keeps at most 20 corners

	// One ground goal: the point (vtable 0x4FD328), the annulus (0x4FD358) or the ring around a
	// rectangle (0x4FD388), each with its own fields as the engine stores them.
	struct Goal
	{
		int32_t kind;
		int16_t gx, gz;             // point and annulus +0x08, +0x0A: the goal cell
		int32_t r, rc2;             // point +0x0C, +0x10: the radius in px, (r / 16)^2
		int32_t rin, rout;          // annulus +0x0C, +0x10
		int32_t rin2, rout2;        // annulus +0x14, +0x18
		int32_t x0, x1, z0, z1;     // ring +0x08, +0x0C, +0x10, +0x14, cells
	};

	// A private view of a rectangle of the class's map: the search reads the code
	// code[(z - z0) * w + (x - x0)] for an entry inside it, in place of the map's.
	struct Patch
	{
		int32_t x0, z0, w, h;
		const uint8_t* code;
	};

	struct Query
	{
		const uint32_t* map;        // the class's map [class + 0x18]: two bits a cell, a dword a column of 16
		int32_t w, h;               // the class's grid, which is the pathfinder's and the feature grid's
		int16_t fx, fz;             // the class's footprint
		const Patch* patch;         // null for none
		const uint16_t* explored;   // main+0x14273: a u16 per 32-px block, (w >> 1) a row, bit p player p's
		uint8_t seat;               // the player whose explored bit is read
		int32_t weight;             // the heuristic's weight, 16.16
		int16_t sx, sz;             // the searcher's cell, unit +0x76, +0x78
		uint16_t heading;           // unit +0x66
		Goal goal;
	};

	enum Kind
	{
		Found = 1,      // n corners from the start on
		NoSearch,       // the start is in the goal, or the bug walk got no nearer than the start
		OffGrid,        // the start is off the grid
		NoPath,         // the open set emptied
		Overflow,       // the scratch's pool is too small: no result
		Refused         // no map, no goal, or a map that is not the scratch's size
	};

	struct Result
	{
		Kind kind;
		uint32_t goalflags;         // GOALF_*, NOPATH where set path with no corners would set it
		int32_t n;
		int16_t pts[kWaypoints][2]; // world px (x, z), as set path stores them; zero past n
		int32_t pops;               // the A* pops: the engine's cost of the search past its start
		int32_t used;               // the pool's high-water mark [pf + 0x0C]; -1 with no A*
		int32_t relax;              // [pf + 0x40], the bug walk's answer; 0 when it did not run
		int16_t found_x, found_z;   // [pf + 0x34], the goal cell found
	};

	// The grid's cell, [pf + 0x1C] + 4 * (z * w + x).
	struct Cell
	{
		uint8_t b0;                 // bits 0-1: 0 new, 1 open, 2 closed, 3 dead; bit 2 a goal cell; bit 3 walked
		uint8_t b1;                 // the direction the cell was reached by
		uint16_t node;              // its open node
	};

	// The pool's 20-byte node.
	struct Node
	{
		int32_t link;               // its heap position; on the free list, the next free node
		int16_t x, z;
		int32_t g, f;
		int16_t pen;                // 30 for a bad cell
		int16_t straight;           // steps since the last turn
	};

	// A heap entry: the node, and a copy of its f, equal to the node's wherever a compare reads it.
	struct HeapEntry
	{
		int32_t f;
		uint32_t n;
	};

	// One thread's scratch: the engine's pathfinder object [main+0x14207] as one search uses it.
	struct Scratch
	{
		Node* pool;
		HeapEntry* heap;
		int32_t freehead, used, nodes, count, lazy;
		Cell* grid;
		int32_t w, h, cells;
		uint32_t* touch;            // a bit per 8 cells
		int16_t start_x, start_z, found_x, found_z;
		int32_t near_x, near_z;     // the goal cell nearest the start
		int32_t relax, fan;
		int32_t cost;               // the pops of the A* pass in hand
		bool searching, found, overflow;
		uint32_t goalflags;
	};

	// The bytes Init() needs for a w x h grid and a pool of `nodes`, 1 .. w * h; 0 when it refuses them.
	size_t Bytes(int32_t w, int32_t h, int32_t nodes);

	// Lays a scratch out in `mem`, every cell new. False for a size Bytes() refuses or too few bytes.
	bool Init(Scratch* s, void* mem, size_t bytes, int32_t w, int32_t h, int32_t nodes);

	// One search, its answer in `r`. `s` is any scratch of the map's size; a search leaves nothing in
	// it the next reads.
	void Search(Scratch* s, const Query& q, Result* r);

	// A class map's code of cell (x, z) on a grid w wide: two bits a cell, a dword a column of 16 rows.
	inline int MapCode(const uint32_t* map, int32_t w, int32_t x, int32_t z)
	{
		return (int)(map[((uint32_t)z >> 4) * (uint32_t)w + (uint32_t)x] >> (2 * (z & 15))) & 3;
	}
}
