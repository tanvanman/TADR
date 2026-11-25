#include "config.h"

#if FAST_SIMULATION_ENABLE

#include "Pathfinder.h"

#include "PathSearch.h"
#include "hook/hook.h"
#include "iddrawsurface.h"
#include "tamem.h"
#include "tafunctions.h"

#include <cstddef>
#include <cstring>
#include <memory>
#include <new>

#if TDRAW_PATHFIND_VERIFY
#include <vector>
#endif

using namespace PathSearch;

namespace
{
	// ---- engine facts (DISASSEMBLED from the stock TotalA.exe) ------------------------------------

	// The fields tamem.h names, at the offsets the engine code this mirrors reads them.
	static_assert(offsetof(TAdynmemStruct, PlayerCounters) == 0x2A3C, "the player count: the scheduler does nothing while it is 0");
	static_assert(offsetof(TAdynmemStruct, FeatureMapSizeX) == 0x14233, "the feature grid's width");
	static_assert(offsetof(TAdynmemStruct, MAPPED_MEMORY_p) == 0x14273, "the explored map");
	static_assert(offsetof(TAdynmemStruct, FeatureMap) == 0x14287, "the feature grid");
	static_assert(offsetof(TAdynmemStruct, BeginUnitsArray_p) == 0x14357, "the unit array");
	static_assert(offsetof(TAdynmemStruct, GameTime) == 0x38A47, "GameTime");
	static_assert(offsetof(TAdynmemStruct, Players) == 0x1B63, "the player records");
	static_assert(offsetof(PlayerStruct, Units) == 0x67 && offsetof(PlayerStruct, UnitsAry_End) == 0x6B, "a player's block");
	static_assert(offsetof(PlayerStruct, My_PlayerType) == 0x73 && offsetof(PlayerStruct, PlayerAryIndex) == 0x146, "a player's type");
	static_assert(offsetof(UnitStruct, XGridPos) == 0x76 && offsetof(UnitStruct, YGridPos) == 0x78, "a unit's cell");
	static_assert(offsetof(UnitStruct, SizeFootX) == 0x7E && offsetof(UnitStruct, UnitID) == 0xA6, "a unit's footprint and type");
	static_assert(offsetof(UnitStruct, UnitSelected) == 0x110, "a unit's state: bit 28 alive");
	static_assert(offsetof(FeatureStruct, FeatureDefIndex) == 0x08 && offsetof(FeatureStruct, FeatureDefDy) == 0x0A, "a cell");

	// What tamem.h has no field for.
	const size_t OFF_PATHFINDER = 0x14207;      // main+: the pathfinder object
	const size_t OFF_LOADFLAGS  = 0x38D75;      // main+: a word, 0 in play (zeroed at 0x49847E), non-zero for a load
	const size_t UNIT_MOVER     = 0x00;         // a unit's mover (tamem.h: IsUnit)
	const size_t UNIT_HEADING   = 0x66;         // a word
	// the pathfinder object [main + 0x14207]
	const size_t PF_USED = 0x0C, PF_W = 0x20, PF_H = 0x24, PF_FOUND = 0x34, PF_RELAX = 0x40, PF_WEIGHT = 0x50;
	const size_t PF_BASE_WEIGHT = 0x54, PF_UNIT = 0x58, PF_GOAL = 0x60, PF_CLASS = 0x64, PF_SEAT = 0x78;
	// the movement classes: 32 records of 0x20 bytes
	const DWORD VA_CLASSES = 0x00512358u;
	const size_t CLASS_STRIDE = 0x20u, CLASS_COUNT = 32u;
	const size_t CLASS_FOOT = 0x04;             // two shorts, x and z
	const size_t CLASS_DEPTH = 0x08;            // two shorts: max, then min water depth
	const size_t CLASS_SLOPE = 0x0C;            // four bytes: max slope, bad slope, max water slope, bad water slope
	const size_t CLASS_W = 0x10, CLASS_H = 0x14;        // its grid
	const size_t CLASS_MAP = 0x18;              // its pathing map
	const size_t CLASS_WATERMARK = 0x1C;
	// a mover: [+0] its course
	const size_t MOVER_CLASS = 0x04, MOVER_STAMP = 0x26, MOVER_FLAGS = 0x2E;   // stamp: its claim; flags bit 2: blocked
	// the local ground course (vtable 0x4FD458): +0x04 its goal, +0x08 its unit
	const DWORD VT_GROUND_COURSE = 0x004FD458u;
	const size_t COURSE_GOAL = 0x04, COURSE_UNIT = 0x08, COURSE_WP = 0x0C, COURSE_COUNT = 0x5C;
	const size_t COURSE_STAMP = 0x60;           // the GameTime of its last search
	const size_t COURSE_FLAGS = 0x64;           // bit 0 has a path, bit 1 wants a search
	// the ground goals, by vtable; [goal + 4] is the order node, its flags +0x4E
	const DWORD VT_GOAL_POINT = 0x004FD328u, VT_GOAL_ANNULUS = 0x004FD358u, VT_GOAL_RING = 0x004FD388u;
	const size_t NODE_FLAGS = 0x4E;

	// the sites and the engine functions called, each thiscall
	const DWORD VA_SCHEDULER  = 0x0040EB70u;    // (pf)
	const DWORD VA_SCHED_CALL = 0x00464F92u;    // the players phase's call of the scheduler
	const DWORD VA_INIT       = 0x0040E9E0u;    // (pf) the pathfinder's per-game init
	const DWORD VA_INIT_CALL  = 0x0044F6B3u;    //   its one call
	const DWORD VA_END_CALL   = 0x00491B76u;    // the level teardown 0x491B60's first call, before it frees an order or a unit,
	const DWORD VA_END_FIRST  = 0x004CED40u;    //   of this: (net object [main+0x10]), ret, its eax not read
	const DWORD VA_JOIN_CALL  = 0x004955E5u;    // the step's call, after the particles, of:
	const DWORD VA_STEP_COUNT = 0x00466580u;    //   (void) cdecl: a countdown word of main's, nothing of a unit
	const DWORD VA_SET_GOAL   = 0x0044F2A0u;    // (course; goal) the ground course's set goal
	const DWORD VA_REMARK     = 0x00440830u;    // (class; cell x | z << 16, size w | h << 16), ret 8
	const DWORD VA_SET_PATH   = 0x0044F080u;    // (course; points, n), ret 8
	const DWORD VA_GOAL_FLAG  = 0x0044CED0u;    // (goal; flags), ret 4
	typedef void (__thiscall *PfFn)(void* pf);
	typedef void* (__thiscall *InitFn)(void* pf);     // returns pf, which its caller stores at main+0x14207 (0x44F6BE)
	typedef void (__thiscall *RemarkFn)(void* cls, unsigned cell, unsigned size);
	typedef void (__thiscall *SetPathFn)(void* course, const void* points, int n);
	typedef void (__thiscall *GoalFlagFn)(void* goal, unsigned flags);

	// The engine code this file and PathSearch.cpp follow, each range fingerprinted (FNV-1a) as the stock
	// exe has it, each to the end of its last instruction: identical in the five distinct game exes of the
	// reference setup, retail 3.1 among them. The pathfinder's per-game init is
	// not among them: TADR writes its budget into it (0x40EAD6).
	struct Range { DWORD at, len, fnv; const char* what; };
	const Range kMirrored[] = {
		{ 0x0040D7B0u, 0x0C0u, 0xE3092F1Cu, "the cell read" },
		{ 0x0040D900u, 0x0BEu, 0x1992E3B2u, "the grid reset" },
		{ 0x0040DA70u, 0x48Cu, 0x8DA4AEFFu, "the expansion" },
		{ 0x0040E050u, 0x10Du, 0x887AC7DCu, "the extraction" },
		{ 0x0040E160u, 0x4C7u, 0x76409B27u, "the bug walk" },
		{ 0x0040E630u, 0x36Eu, 0x8311A81Au, "the search start" },
		{ 0x0040EB70u, 0x3A8u, 0xF0268F7Bu, "the scheduler and its A* passes" },
		{ 0x0040F000u, 0x109u, 0x57A8C0B4u, "the heap's sifts" },
		{ 0x0040F1E0u, 0x019u, 0xA43AFE72u, "the node free" },
		{ 0x00440830u, 0x0F2u, 0x75540B9Au, "the remark" },
		{ 0x00440AF0u, 0x0E1u, 0xB0AA805Cu, "the watermark" },
		{ 0x0047DFC0u, 0x223u, 0x0F808551u, "the rectangle rule" },
		{ 0x0047E1F0u, 0x0E0u, 0x3E07299Au, "the refresh rule" },
		{ 0x0044CED0u, 0x016u, 0x82E988B1u, "the goal flag" },
		{ 0x0044CF60u, 0x06Eu, 0x10A44B29u, "the point goal" },
		{ 0x0044D0E0u, 0x19Eu, 0x1432F527u, "the point's goal cells" },
		{ 0x0044D290u, 0x02Cu, 0x488F25DDu, "the point's in_goal" },
		{ 0x0044D350u, 0x051u, 0x8C931360u, "the point's heuristic" },
		{ 0x0044D3B0u, 0x08Au, 0xEDC5EF79u, "the annulus goal" },
		{ 0x0044D560u, 0x122u, 0xBB472DD9u, "the annulus's goal cells" },
		{ 0x0044D7C0u, 0x037u, 0x1B7FBC3Au, "the annulus's in_goal" },
		{ 0x0044D840u, 0x05Du, 0x72C9FE32u, "the annulus's heuristic" },
		{ 0x0044D8A0u, 0x050u, 0x82152F96u, "the ring goal" },
		{ 0x0044DA00u, 0x254u, 0xF078F445u, "the ring's goal cells" },
		{ 0x0044DCB0u, 0x047u, 0x416E424Cu, "the ring's in_goal" },
		{ 0x0044DD00u, 0x0B3u, 0x3EF8E359u, "the ring's heuristic" },
		{ 0x0044F080u, 0x07Au, 0x4966A020u, "set path" },
		{ 0x0044F260u, 0x024u, 0x5FE53B03u, "wants search" },
		{ 0x0044F2A0u, 0x1AFu, 0x8BA72277u, "set goal" },
	};

	// ---- the rules' numbers -----------------------------------------------------------------------

	// A tick chooses at most kCapSearches units, and begins no search once the pops of its ended
	// searches reach kCapPops: a search that finds no way pops every cell it can reach (MEASURED with
	// no such cap: 1 621 833 pops in one tick of the battle). kQuiet is the most units ready in a tick
	// that searches at the base weight (MEASURED: 2900 pops a search at the base, 510 at 3 times, 410
	// at 6 times).
	const unsigned kCapSearches = 64;
	const unsigned kCapPops     = 64000;
	const unsigned kQuiet       = 4;
	const unsigned kRetry       = 60;          // the engine's gate (0x44F271)
	const unsigned kThreads     = 4;           // the most threads that search, the scheduler's own counted
	const unsigned kHelpers     = kThreads - 1;
	const unsigned kJobs        = kCapSearches + kHelpers;     // a tick's, and the carried ones
	const int      kFootMax     = 64;          // the largest footprint, of a class or a unit, a patch is made for
	const int      kPatchMax    = (2 * kFootMax + 1) * (2 * kFootMax + 1);   // its entries: (fx + w + 1)(fz + h + 1)
	const int      kPool        = 65536;       // a scratch's nodes: the engine's node word is 16 bits
	const unsigned kSlots       = 65536;       // unit slots: a cell names its unit by a u16 slot
	const size_t   kCopyBytes   = 32u << 20;   // the copies' block, at most
	const unsigned kClassPages  = CLASS_COUNT + kHelpers;

	// ---- reading engine memory --------------------------------------------------------------------

	template <class T> __forceinline T Get(const void* p, size_t off)
	{
		return *reinterpret_cast<const T*>(static_cast<const char*>(p) + off);
	}

	template <class T> __forceinline void Put(void* p, size_t off, T v)
	{
		*reinterpret_cast<T*>(static_cast<char*>(p) + off) = v;
	}

	__forceinline TAdynmemStruct* Main() { return *TAmainStruct_PtrPtr; }

	__forceinline char* ClassRecord(unsigned i) { return reinterpret_cast<char*>(VA_CLASSES + i * CLASS_STRIDE); }

	// A class record's index among the 32, or -1 for a pointer that is none of them.
	int ClassIndex(const void* cls)
	{
		const size_t ci = (size_t)cls - VA_CLASSES;
		return (size_t)cls >= VA_CLASSES && !(ci % CLASS_STRIDE) && ci / CLASS_STRIDE < CLASS_COUNT ? (int)(ci / CLASS_STRIDE) : -1;
	}

	// A unit's cell and footprint as the remark 0x440830 takes them: two words packed in a dword each.
	__forceinline unsigned CellWord(const UnitStruct* u) { return *reinterpret_cast<const unsigned*>(&u->XGridPos); }
	__forceinline unsigned FootWord(const UnitStruct* u) { return *reinterpret_cast<const unsigned*>(&u->SizeFootX); }

	// THE UNIT ARRAY, bounded once: slots 0 .. n - 1, n at most kSlots.
	struct Units
	{
		UnitStruct* first;
		unsigned n;

		bool Open(const TAdynmemStruct* ta)
		{
			first = ta ? ta->BeginUnitsArray_p : nullptr;
			const UnitStruct* last = ta ? ta->EndOfUnitsArray_p : nullptr;
			n = first && last >= first ? (unsigned)(last - first) + 1 : 0;
			return n && n <= kSlots;
		}
		UnitStruct* At(unsigned slot) const { return slot < n ? first + slot : nullptr; }
		// a pointer's slot, or kSlots for one that is not on a slot of the array
		unsigned SlotOf(const void* p) const
		{
			const size_t off = (size_t)((const char*)p - (const char*)first);
			return p >= first && off % sizeof(UnitStruct) == 0 && off / sizeof(UnitStruct) < n ? (unsigned)(off / sizeof(UnitStruct)) : kSlots;
		}
	};

	// ---- the engine's cells, read as the class rule reads them ------------------------------------

	struct CellView
	{
		const FeatureStruct* grid;  // w x h
		const char* fdefs;
		int nfd, w, h, sea;
		Units units;
		const UnitStruct* absent;   // the unit the rule leaves out
		unsigned wm;                // the class's watermark
		int fx, fz, maxwaterdepth, minwaterdepth, maxslope, badslope, maxwaterslope, badwaterslope;
	};

	// The feature rule the engine's cell readers share: a feature blocks when its definition's mask has
	// bit 6, a wreck's footprint cell by its anchor's.
	bool FeatureBlocks(const CellView& v, int idx)
	{
		const FeatureStruct& cl = v.grid[idx];
		unsigned word = cl.FeatureDefIndex;
		if (word == 0xFFFFu)
			return false;
		if (word == 0xFFFEu)
		{
			const int a = idx - (cl.FeatureDefDy * v.w + cl.FeatureDefDx);
			if (a < 0)
				return true;
			word = v.grid[a].FeatureDefIndex;
			if (word >= 0xFFFBu)
				return false;
		}
		else if (word >= 0xFFFBu)
			return true;
		if ((int)word >= v.nfd || !v.fdefs)
			return true;
		return (v.fdefs[(size_t)word * sizeof(FeatureDefStruct) + 0xFE] >> 6 & 1) != 0;   // FeatureMask's bit 6
	}

	// The unit rule 0x47E10C..0x47E13F: the unit in the cell's slot blocks with no mover, or with a claim
	// stamp below the watermark. THE BOUND: the slot against the array, which the engine indexes unchecked;
	// a slot past it blocks. The absent unit is the searcher, whose stamp the engine holds at GameTime while
	// its own rectangle is recomputed (0x440B30), so it never blocks there.
	bool UnitBlocks(const CellView& v, const FeatureStruct& cl)
	{
		const unsigned slot = cl.occupyingUnitNumber;
		if (!slot)
			return false;
		const UnitStruct* unit = v.units.At(slot);
		if (!unit)
			return true;
		if (unit == v.absent)
			return false;
		const char* mover = Get<const char*>(unit, UNIT_MOVER);
		return !mover || Get<unsigned>(mover, MOVER_STAMP) < v.wm;
	}

	// One cell's answer of the rectangle rule 0x47DFC0: 0 blocked, 1 bad, 3 good (the loop body).
	int CellCode(const CellView& v, int idx)
	{
		const FeatureStruct& cl = v.grid[idx];
		const int mn = cl.minHeight2x2, mx = cl.maxHeight2x2;
		if (FeatureBlocks(v, idx) || UnitBlocks(v, cl))
			return 0;
		if (mn < v.sea - v.maxwaterdepth || mx > v.sea - v.minwaterdepth)
			return 0;
		const int d = (unsigned char)(mx - mn);
		if (mn < v.sea)
			return d <= v.badwaterslope ? 3 : d > v.maxwaterslope ? 0 : 1;
		return d <= v.badslope ? 3 : d > v.maxslope ? 0 : 1;
	}

	// The view of class record `cls` with `absent` left out; false when a value it rests on is out of its
	// bound.
	bool OpenCellView(CellView* v, const TAdynmemStruct* ta, const char* cls, const UnitStruct* absent)
	{
		std::memset(v, 0, sizeof *v);
		v->grid = ta->FeatureMap;
		v->w = ta->FeatureMapSizeX;
		v->h = ta->FeatureMapSizeY;
		v->fdefs = reinterpret_cast<const char*>(ta->FeatureDef);
		v->nfd = ta->NumFeatureDefs;
		if (v->nfd < 0 || v->nfd > 0xFFFA)
			v->nfd = 0;
		v->sea = ta->SeaLevel;
		if (!v->grid || !v->units.Open(ta) || v->w < 1 || v->h < 1 || (long long)v->w * v->h > 0x1000000)
			return false;
		v->absent = absent;
		v->wm = Get<unsigned>(cls, CLASS_WATERMARK);
		v->fx = Get<short>(cls, CLASS_FOOT);
		v->fz = Get<short>(cls, CLASS_FOOT + 2);
		v->maxwaterdepth = Get<short>(cls, CLASS_DEPTH);
		v->minwaterdepth = Get<short>(cls, CLASS_DEPTH + 2);
		v->maxslope = Get<unsigned char>(cls, CLASS_SLOPE);
		v->badslope = Get<unsigned char>(cls, CLASS_SLOPE + 1);
		v->maxwaterslope = Get<unsigned char>(cls, CLASS_SLOPE + 2);
		v->badwaterslope = Get<unsigned char>(cls, CLASS_SLOPE + 3);
		return v->fx >= 1 && v->fz >= 1 && v->fx <= kFootMax && v->fz <= kFootMax;
	}

	// THE RECTANGLE of a unit's entries that 0x440830 recomputes in a class's map, a grid of w x h with the
	// class footprint fx x fz: [x - fx, x + w] x [z - fz, z + h], the near ends held at 0 and the far ends at
	// the grid's. False for a footprint, the unit's or the class's, outside [0, kFootMax], a side past
	// 2 kFootMax + 1, and an empty rectangle.
	bool PatchRect(int w, int h, int fx, int fz, const UnitStruct* unit, unsigned char* code, Patch* p)
	{
		const int ux = unit->XGridPos, uz = unit->YGridPos, uw = unit->SizeFootX, uh = unit->SizeFootZ;
		if (uw < 0 || uh < 0 || uw > kFootMax || uh > kFootMax || fx < 0 || fz < 0 || fx > kFootMax || fz > kFootMax)
			return false;
		int x0 = ux - fx, z0 = uz - fz, x1 = uw + ux + 1, z1 = uh + uz + 1;
		if (x0 < 0)
			x0 = 0;
		if (z0 < 0)
			z0 = 0;
		if ((unsigned)x1 > (unsigned)w)
			x1 = w;
		if ((unsigned)z1 > (unsigned)h)
			z1 = h;
		// a far end past the grid's, or wrapped negative, is held at the grid's (0x44085E..0x4408A0, an
		// unsigned compare), so each side is bounded here too: at most 2 kFootMax + 1 for footprints in range
		if (x0 >= x1 || z0 >= z1 || x1 - x0 > 2 * kFootMax + 1 || z1 - z0 > 2 * kFootMax + 1)
			return false;
		p->x0 = x0;
		p->z0 = z0;
		p->w = x1 - x0;
		p->h = z1 - z0;
		p->code = code;
		return true;
	}

	// THE PATCH of a unit recomputed: each entry of its rectangle through the refresh rule 0x47E1F0 with
	// the unit left out -- the footprint's rectangle, then its ring in pinwheel order, each 0x47DFC0 -- 0 off
	// the grid (x < 0, z < 0, x + w >= W or z + h >= H), else the least of its cells' answers (CellCode),
	// which the engine's loop finds in another order: a rectangle has no other effect. Each cell those
	// rectangles read is answered once.
	bool PatchOf(const CellView& v, const UnitStruct* unit, unsigned char* code, Patch* p)
	{
		if (!PatchRect(v.w, v.h, v.fx, v.fz, unit, code, p))
			return false;
		const int x0 = p->x0, z0 = p->z0, x1 = p->x0 + p->w, z1 = p->z0 + p->h;
		// the cells read: [x0 - 1, x1 - 1 + fx] x [z0 - 1, z1 - 1 + fz], of which a rectangle on the grid
		// reaches only [0, W - 2] x [0, H - 2]; with both footprints at most kFootMax, a side is at most
		// (2 kFootMax + 1) + kFootMax + 1
		const int kSide = 2 * kFootMax + 1 + kFootMax + 1;
		unsigned char cells[kSide * kSide];
		const int cx0 = x0 - 1, cz0 = z0 - 1, cw = p->w + v.fx + 1, ch = p->h + v.fz + 1;
		for (int z = 0; z < ch; z++)
			for (int x = 0; x < cw; x++)
			{
				const int gx = cx0 + x, gz = cz0 + z;
				cells[z * cw + x] = gx >= 0 && gz >= 0 && gx < v.w - 1 && gz < v.h - 1 ? (unsigned char)CellCode(v, gz * v.w + gx) : 0;
			}
		// 0x47DFC0 over the answered cells
		auto rect = [&](int x, int z, int w, int h) -> int
		{
			if (x < 0 || z < 0 || x + w >= v.w || z + h >= v.h)
				return 0;
			int r = 3;
			for (int row = 0; row < h; row++)
				for (int col = 0; col < w; col++)
				{
					const int c = cells[(z + row - cz0) * cw + (x + col - cx0)];
					if (c < r && (r = c) == 0)
						return 0;
				}
			return r;
		};
		for (int z = z0; z < z1; z++)
			for (int x = x0; x < x1; x++)
			{
				int r = rect(x, z, v.fx, v.fz);
				if (r > 1)
					r = rect(x - 1, z - 1, v.fx + 1, 1) != 3 || rect(x + v.fx, z - 1, 1, v.fz + 1) != 3 ||
						rect(x, z + v.fz, v.fx + 1, 1) != 3 || rect(x - 1, z, 1, v.fz + 1) != 3 ? 1 : 3;
				code[(z - z0) * p->w + (x - x0)] = (unsigned char)r;
			}
		return true;
	}

	// THE PATCH of a unit as the class's map holds it, for the search that reads the map's own entries.
	bool PatchCopy(const char* cls, const UnitStruct* unit, unsigned char* code, Patch* p)
	{
		const uint32_t* map = Get<const uint32_t*>(cls, CLASS_MAP);
		const int w = Get<int>(cls, CLASS_W);
		if (!map || !PatchRect(w, Get<int>(cls, CLASS_H), Get<short>(cls, CLASS_FOOT), Get<short>(cls, CLASS_FOOT + 2), unit, code, p))
			return false;
		for (int z = 0; z < p->h; z++)
			for (int x = 0; x < p->w; x++)
				code[z * p->w + x] = (unsigned char)MapCode(map, w, p->x0 + x, p->z0 + z);
		return true;
	}

	// A ground course's goal is one of the three ground goals: the order handlers make those only for a
	// unit whose def has +0x241 bit 11 clear, which is the unit a ground course moves; the other goal
	// classes (0x4FD3B8, 0x4FD3F8) are the air movers'. Any other is counted and answered no path.
	bool GoalOf(const char* goal, Goal* g)
	{
		const DWORD vt = Get<DWORD>(goal, 0);
		std::memset(g, 0, sizeof *g);
		if (vt == VT_GOAL_POINT)
		{
			g->kind = GOAL_POINT;
			g->gx = Get<short>(goal, 0x08);
			g->gz = Get<short>(goal, 0x0A);
			g->r = Get<int>(goal, 0x0C);
			g->rc2 = Get<int>(goal, 0x10);
		}
		else if (vt == VT_GOAL_ANNULUS)
		{
			g->kind = GOAL_ANNULUS;
			g->gx = Get<short>(goal, 0x08);
			g->gz = Get<short>(goal, 0x0A);
			g->rin = Get<int>(goal, 0x0C);
			g->rout = Get<int>(goal, 0x10);
			g->rin2 = Get<int>(goal, 0x14);
			g->rout2 = Get<int>(goal, 0x18);
		}
		else if (vt == VT_GOAL_RING)
		{
			g->kind = GOAL_RING;
			g->x0 = Get<int>(goal, 0x08);
			g->x1 = Get<int>(goal, 0x0C);
			g->z0 = Get<int>(goal, 0x10);
			g->z1 = Get<int>(goal, 0x14);
		}
		else
			return false;
		return true;
	}

	// THE QUERY of `unit`'s search in class `cls` for `goal`: the goal, the unit's cell and heading, the
	// patch of `kind` (a fresh one into `code`; a given one already in `patch`), the maps the search is to read,
	// the player's explored bit and the weight. The release scheduler and the check build's oracle make it
	// alike, so the check holds the release query to the engine's.
	// WHAT THE SEARCH READS OF ITS OWN RECTANGLE, as the engine's watermark 0x440AF0 decides it for the
	// searcher: its claim stamp below the class's watermark from before this search's update (0x440B33),
	// the rectangle recomputed with it left out (0x440B45): PATCH_FRESH; its stamp at or above that but
	// below the new one -- the first search of the class in a tick, of a unit that has stood still since the
	// class's last watermark -- which the sweep leaves out as it holds the searcher's stamp at GameTime
	// (0x440B30) and nothing recomputes:
	// the map's own entries, PATCH_GIVEN, copied where the sweep has left them (PatchCopy); else none.
	enum PatchKind { PATCH_NONE, PATCH_FRESH, PATCH_GIVEN, PATCH_REFUSED };
	PatchKind PatchKindOf(const UnitStruct* unit, unsigned old_wm, unsigned new_wm)
	{
		const unsigned stamp = Get<unsigned>(Get<const char*>(unit, UNIT_MOVER), MOVER_STAMP);
		return stamp < old_wm ? PATCH_FRESH : stamp < new_wm ? PATCH_GIVEN : PATCH_NONE;
	}

	enum QueryMade { QUERY_MADE, QUERY_OTHER_GOAL, QUERY_REFUSED };
	QueryMade MakeQuery(const TAdynmemStruct* ta, const UnitStruct* unit, const char* cls, const char* goal, const uint32_t* map,
		const uint16_t* explored, int w, int h, unsigned char seat, int weight, PatchKind kind, Query* q, Patch* patch,
		unsigned char* code)
	{
		std::memset(q, 0, sizeof *q);
		if (!GoalOf(goal, &q->goal))
			return QUERY_OTHER_GOAL;
		CellView v;
		if (kind == PATCH_REFUSED || !OpenCellView(&v, ta, cls, unit) || v.w != w || v.h != h)
			return QUERY_REFUSED;
		if (kind == PATCH_FRESH && !PatchOf(v, unit, code, patch))
			return QUERY_REFUSED;
		if (kind != PATCH_NONE)
			q->patch = patch;
		q->map = map;
		q->w = w;
		q->h = h;
		q->fx = (int16_t)v.fx;
		q->fz = (int16_t)v.fz;
		q->explored = explored;
		q->seat = seat;
		q->weight = weight;
		q->sx = unit->XGridPos;
		q->sz = unit->YGridPos;
		q->heading = Get<unsigned short>(unit, UNIT_HEADING);
		return QUERY_MADE;
	}

	// ---- the work in hand -------------------------------------------------------------------------

	// One search: what the walk chose, its query, its result.
	enum JobState { JOB_QUEUED, JOB_RUNNING, JOB_DONE, JOB_SKIPPED };
	struct Job
	{
		UnitStruct* unit;
		char* mover;
		char* course;
		char* goal;
		char* cls;
		unsigned slot, stamp, ci;   // ci: the class's index among the 32, bounded for a whole job
		unsigned stamp0, kind;      // the course's stamp before the writes; the ready list it came from
		bool whole;                 // every value the search rests on is inside its bound
		bool ready;                 // the query is made: the search runs
		// JOB_QUEUED until one exchange takes it: a helper's, to JOB_RUNNING and then JOB_DONE once the
		// result is whole, or a set goal's, to JOB_SKIPPED -- which a helper that had claimed the job then
		// makes JOB_DONE too: JOB_DONE alone says a helper has left a claimed job.
		volatile LONG state;
		bool other;                 // its goal is of a kind the search has not
		unsigned char patch_kind;   // PatchKind: what the search reads of its own rectangle
		bool stale;                 // a set goal came while it ran: its result is not applied
		bool applied;               // its result is applied (a set goal's, before the join)
		bool pinned;                // it pins its pages of the copies, ex and cl
		unsigned char ex, cl;
		Patch patch;
		Query q;                    // its seat is set with the writes, the rest with the query
		Result r;
#if TDRAW_PATHFIND_VERIFY
		bool checked;               // `check` is the same search made on the engine's own maps
		Result check;
#endif
		unsigned char code[kPatchMax];
	};

	struct Tick;

	// A HANDING-OUT: tasks 0 .. n - 1, each claimed by one thread's exchange of `next`. The searches'
	// has `cap`: no task is claimed once the pops of its ended searches reach it. kRunClosed in `next`
	// ends the claiming for good.
	const LONG kRunClosed = 0x40000000;
	struct Run
	{
		void (*task)(Run* r, unsigned i, Scratch* s);
		Tick* e;                    // the tick's view of the engine; null in the searches'
		unsigned n, cap;
		volatile LONG next, pops;
		Job* job[kCapSearches];     // the searches': task i's job
	};

	// THE WORK IN HAND, a block made at install. A search may be carried past its tick, so this is not
	// empty between two ticks while one is: a helper is inside it.
	//   help[k]      the handing-out helper k is in, null when it is in none: THE ONE WORD the helpers'
	//                ordering rests on (RunSend, HelperMain).
	//   sync         the handing-out the scheduler's thread waits for inside the scheduler.
	//   bg           the searches' handing-outs: one a tick, taken where no helper is in it.
	//   tick         the tick's searches, not yet joined: in play they wait for the step's join, and the
	//                loader's call joins its own before it returns.
	//   job, free    the jobs, taken and given back on the scheduler's thread.
	//   carry        THE SEARCHES CARRIED: begun in an earlier tick and not ended when that tick was
	//                joined, in the order chosen; a helper is inside each, so kHelpers at most.
	//   ex_pin, cl_pin  the jobs that read each page of the copies.
	struct Machine
	{
		Run* volatile help[kHelpers];
		Run sync, bg[kThreads];
		Run* tick;
		Job job[kJobs];
		Job* free[kJobs];
		unsigned nfree;
		Job* carry[kHelpers];
		unsigned ncarry;
		unsigned char ex_pin[kThreads], cl_pin[kClassPages];
	};

	Machine* s_m;

	__forceinline void* volatile* HelpWord(unsigned k) { return reinterpret_cast<void* volatile*>(&s_m->help[k]); }

	// THE HELPERS: threads made at install. Helper k waits on s_wake[k] and answers on s_done. Never
	// closed.
	unsigned s_helpers;
	HANDLE s_wake[kHelpers];
	HANDLE s_done;

	// THE SCRATCHES, one a thread that searches: s_pf[0] the scheduler's thread's (and the check build's),
	// s_pf[k] helper k's, in one block ours alone to free, made again when the grid's size changes. One not
	// laid out has no pool.
	Scratch s_pf[kThreads];
	unsigned char* s_mem;

	// THE COPIES, what a search reads of the engine while the game goes on: 1 + s_helpers pages of the
	// explored map, then a page of a class's map for each class with a map and s_helpers more, made with
	// the scratches. A tick takes a free explored page and a free class page for each class it searches,
	// writes them after its writes, and each of its jobs PINS the two it reads until the scheduler's thread
	// ends the job: a page is written only while no job pins it. A carried job pins one of each and there
	// are s_helpers of them at most, so a tick always finds its pages. Null when the memory was not to be
	// had: the searches are then made inside the scheduler.
	unsigned char* s_frz;
	unsigned s_frz_class_pages;
	unsigned s_classes;             // the classes with a map the scratches and copies were made for

	// a class's map is two bits a cell, a dword a column of 16 rows; the explored map a word a 32-px
	// block; each copy on a 16-byte boundary
	size_t ClassBytes(int w, int h) { return (size_t)w * (size_t)((h + 15) >> 4) * 4u; }
	size_t ExploredBytes(int w, int h) { return (size_t)(w >> 1) * (size_t)(h >> 1) * 2u; }
	size_t Round16(size_t n) { return (n + 15u) & ~(size_t)15u; }
	size_t ExploredRegion(int w, int h) { return (1 + s_helpers) * Round16(ExploredBytes(w, h)); }
	unsigned char* ExploredPage(int w, int h, unsigned page) { return s_frz + page * Round16(ExploredBytes(w, h)); }
	unsigned char* ClassPage(int w, int h, unsigned page) { return s_frz + ExploredRegion(w, h) + page * Round16(ClassBytes(w, h)); }

	// WHAT A TICK CARRIES TO THE NEXT, beside the units' own records: per unit slot the stamp of its last
	// search that found a path, 0 for none -- that search's while it equals the course's stamp, so a slot's
	// next unit never inherits it -- and each kind's cursor, the slot served last.
	unsigned s_found[kSlots];
	unsigned s_cursor[2];

	// THE GAME: which scheduler serves it, chosen at its first tick with no engine search under way.
	enum Serving { SERVING_UNDECIDED, SERVING_OURS, SERVING_ENGINE };
	Serving s_serving;
	bool s_ours;                    // our scheduler serves this tick: the join and set goal act

	// how often each rare line has been said, at most four times
	unsigned s_said_dropped, s_said_failed, s_said_other;

	std::unique_ptr<SingleHook> s_sched_call, s_join_call, s_init_call, s_end_call;
	std::unique_ptr<InlineSingleHook> s_set_goal_hook;
	BYTE s_sched_rel[4], s_join_rel[4], s_init_rel[4], s_end_rel[4];

	// ---- the check build's measurements ------------------------------------------------------------

#if TDRAW_PATHFIND_VERIFY
	double s_us_per_count;

	double NowUs()
	{
		LARGE_INTEGER t;
		QueryPerformanceCounter(&t);
		return static_cast<double>(t.QuadPart) * s_us_per_count;
	}

	enum Phase { PHASE_ENGINE, PHASE_OURS };
	const unsigned kBlock         = 300u;   // ticks a block, at least
	const unsigned kProgressEvery = 8u;     // blocks between two "so far" lines

	// A block's or a whole phase's figures.
	struct Figures
	{
		Phase phase;
		unsigned ticks;
		double us;                  // on the game thread, the checks' own time taken out
		double helper_us;           // ours: the searches' time on the threads that search
		unsigned searches, found;
		unsigned waited, wait_max;  // searches whose wait was measured, from the gate opening to the start
		double wait_sum;
	};

	// why a tick's searches were not made beside the game
	enum Beside { BESIDE, BESIDE_NOT_PLAY, BESIDE_NO_HELPER, BESIDE_NO_COPIES, BESIDE_NO_PAGE, BESIDE_REASONS };

	struct Check
	{
		// the engine's searches with ours beside them
		unsigned compared, equal, differ, spanned, spanned_differ, overflow, refused, other_goal;
		unsigned patched, patch_entries, patch_differ, patch_refused, patch_kept, kept_stale, logged;
		// our searches beside the game, made again on the engine's own maps
		unsigned live, live_equal, live_differ, inside, carried, stale, skipped, weight[3], beside[BESIDE_REASONS];
	};

	Phase s_phase;
	unsigned s_phase_ticks;
	Figures s_block, s_total[2];
	std::vector<Figures> s_blocks;
	Check s_check;
	double s_check_us;              // the checks' time inside this tick's span
	volatile LONG64 s_search_counts;    // the searches' time, in performance counts, summed over the threads
	unsigned s_ready_since[kSlots]; // the tick a slot was first seen ready, 0 for none

	// the time of a check's own work, taken out of the tick's span
	struct CheckTime
	{
		double t0;
		CheckTime() : t0(NowUs()) {}
		~CheckTime() { s_check_us += NowUs() - t0; }
	};

	// the engine's search in hand, between its grid reset and its cleanup
	struct Oracle
	{
		const char* wm_cls;         // the watermark's last call: its class, its searcher and the class's watermark before it
		const UnitStruct* wm_unit;
		unsigned wm_old;
		bool open, found;
		const UnitStruct* unit;
		const char* course;
		const char* node;
		unsigned gametime, node_flags0;
		Result res;
		unsigned char patch[kPatchMax];
	} s_or;

	void NoteWait(unsigned slot, unsigned start)
	{
		if (slot >= kSlots)
			return;
		const unsigned since = s_ready_since[slot];
		s_ready_since[slot] = 0;
		if (!since || start < since)
			return;
		const unsigned waited = start - since;
		s_block.waited++;
		s_block.wait_sum += waited;
		if (waited > s_block.wait_max)
			s_block.wait_max = waited;
	}
#endif

	// the walk's measure of the waits, in the check build
	__forceinline void NotReady(unsigned slot)
	{
#if TDRAW_PATHFIND_VERIFY
		s_ready_since[slot] = 0;
#else
		(void)slot;
#endif
	}

	__forceinline void Ready(unsigned slot, unsigned now)
	{
#if TDRAW_PATHFIND_VERIFY
		if (!s_ready_since[slot])
			s_ready_since[slot] = now ? now : 1;
#else
		(void)slot;
		(void)now;
#endif
	}

	// ---- the scratches and the copies --------------------------------------------------------------

	void DueDrop();

	// The scratches and the copies for a w x h grid, made again when the grid's size or the count of
	// classes with a map changes: a scratch a thread that searches, or one alone when that much memory is
	// not to be had -- the searches are then the same ones, one after another; the copies only with every
	// helper's scratch. THE LIFETIME: every
	// search is ended first (DueDrop), so no helper reads what this frees.
	bool ScratchFor(int w, int h)
	{
		unsigned classes = 0;
		for (unsigned i = 0; i < CLASS_COUNT; i++)
			classes += Get<const void*>(ClassRecord(i), CLASS_MAP) != nullptr;
		if (s_mem && s_pf[0].pool && s_pf[0].w == w && s_pf[0].h == h && s_classes == classes)
			return true;
		DueDrop();
		delete[] s_mem;
		delete[] s_frz;
		s_mem = s_frz = nullptr;
		s_frz_class_pages = 0;
		std::memset(s_pf, 0, sizeof s_pf);
		const int nodes = (long long)w * h < kPool ? w * h : kPool;
		const size_t need = Round16(Bytes(w, h, nodes));
		if (!need)
			return false;
		unsigned n = 1 + s_helpers;
		if (need <= 0x7FFFFFFFu / n)
			s_mem = new (std::nothrow) unsigned char[need * n];
		if (!s_mem)
		{
			n = 1;
			s_mem = new (std::nothrow) unsigned char[need];
		}
		if (!s_mem)
			return false;
		for (unsigned i = 0; i < n; i++)
			Init(&s_pf[i], s_mem + i * need, need, w, h, nodes);
		s_classes = classes;
		if (n > 1)
		{
			const unsigned pages = classes + s_helpers;
			const size_t bytes = ExploredRegion(w, h) + pages * Round16(ClassBytes(w, h));
			if (bytes <= kCopyBytes && (s_frz = new (std::nothrow) unsigned char[bytes]) != nullptr)
				s_frz_class_pages = pages;
		}
		return true;
	}

	// ---- the tick's view of the engine ------------------------------------------------------------

	struct Tick
	{
		TAdynmemStruct* ta;
		Units units;
		unsigned now;
		int w, h;                   // the pathfinder's grid
		const unsigned short* explored;
		int weight;
		unsigned wm;                // the tick's watermark, every class's: max(now, 30) - 30 (0x440B03)
		bool play;                  // no load in hand: the game thread runs this tick
		bool late;                  // the searches read the tick's copies, beside the game
		Job** job;                  // the tick's jobs, in the order chosen
		unsigned nj;
		unsigned ncls;              // the classes its whole jobs search, each once,
		unsigned char cls[CLASS_COUNT];     // and the pages of their copies and of the explored map
		unsigned char cl_page[CLASS_COUNT], ex_page;
		unsigned nstill;            // THE STILL UNITS (Walk), ascending by slot
		unsigned nblock;            // the served players' blocks of the unit array, ascending
		struct { UnitStruct* b0; UnitStruct* b1; unsigned char seat; } block[10];
	};

	// Player p's block of the unit array when the engine's scheduler serves the player
	// (0x40EC45..0x40EC62: a record in use, of type 1, 2 or 3, its +0x146 not 10). THE BOUND: both ends on
	// slots of the array.
	bool ServedBlock(const Tick& e, unsigned p, UnitStruct** b0, UnitStruct** b1)
	{
		const PlayerStruct& rec = e.ta->Players[p];
		const unsigned char type = (unsigned char)rec.My_PlayerType;
		if (!rec.PlayerActive || type < 1 || type > 3 || (unsigned char)rec.PlayerAryIndex == 10)
			return false;
		*b0 = rec.Units;
		*b1 = rec.UnitsAry_End;
		return e.units.SlotOf(*b0) < kSlots && e.units.SlotOf(*b1) < kSlots && *b1 >= *b0;
	}

	// The tick's view of the engine. TICK_NO_GAME with no game set up, which the engine's scheduler would do
	// nothing with either; TICK_REFUSED, with `*why`, for a game whose unit array, explored map or grid is
	// outside what the tables here hold: the engine's scheduler serves such a game (Dispatch).
	enum TickView { TICK_NO_GAME, TICK_OPEN, TICK_REFUSED };
	TickView OpenTick(Tick* e, const char** why)
	{
		std::memset(e, 0, sizeof *e);
		e->ta = Main();
		if (!e->ta || !e->ta->PlayerCounters)
			return TICK_NO_GAME;
		const char* pf = Get<const char*>(e->ta, OFF_PATHFINDER);
		if (!pf || !e->ta->BeginUnitsArray_p || e->ta->EndOfUnitsArray_p < e->ta->BeginUnitsArray_p)
			return TICK_NO_GAME;
		e->explored = e->ta->MAPPED_MEMORY_p;
		e->now = (unsigned)e->ta->GameTime;
		e->w = Get<int>(pf, PF_W);
		e->h = Get<int>(pf, PF_H);
		e->weight = Get<int>(pf, PF_BASE_WEIGHT);
		e->wm = (e->now > 30 ? e->now : 30) - 30;
		e->play = Get<unsigned short>(e->ta, OFF_LOADFLAGS) == 0;
		*why = !e->units.Open(e->ta) ? "the unit array is longer than the table of stamps"
			: !e->explored ? "there is no explored map"
			: e->w < 1 || e->h < 1 || !ScratchFor(e->w, e->h) ? "no scratch could be made for the grid"
			: nullptr;
		if (*why)
			return TICK_REFUSED;
		// the blocks in the unit array's order, whatever the players' order, so a ready list ascends by slot
		for (unsigned p = 0; p < 10; p++)
		{
			UnitStruct* b0;
			UnitStruct* b1;
			if (!ServedBlock(*e, p, &b0, &b1))
				continue;
			unsigned i = e->nblock++;
			for (; i && e->block[i - 1].b0 > b0; i--)
				e->block[i] = e->block[i - 1];
			e->block[i].b0 = b0;
			e->block[i].b1 = b1;
			e->block[i].seat = (unsigned char)p;
		}
		return TICK_OPEN;
	}

	// The unit's local ground course when it wants a search, as the engine's examination reads it
	// (0x40ED33..0x40ED4D: a type, a mover, the mover's class) and its wants-search 0x44F260 tests bit 1;
	// no other course class ever answers yes (their slot is 0x44EFF0, `xor eax, eax`).
	__forceinline char* WantingCourse(const UnitStruct* unit)
	{
		const char* mover;
		char* course;
		if (!unit->UnitID || !(mover = Get<const char*>(unit, UNIT_MOVER)) || !Get<const char*>(mover, MOVER_CLASS) ||
			!(course = Get<char*>(mover, 0)) || Get<DWORD>(course, 0) != VT_GROUND_COURSE ||
			!(Get<unsigned char>(course, COURSE_FLAGS) & 2))
			return nullptr;
		return course;
	}

	// THE WALK, one pass over the unit array. s_ready[0] takes the units served first -- a new goal (the
	// stamp is 0), and a unit that has followed its path to its end -- and s_ready[1] the others whose last
	// search began kRetry ticks ago: slots of the served players' blocks, ascending, which the cursors'
	// rotation rests on.
	// THE STILL UNITS are what a class's sweep marks from (ClassTick): every slot from 1 on whose unit is
	// alive (+0x110 bit 28), has a mover and a claim stamp below the tick's watermark. Nothing inside the
	// tick's call writes a unit's alive bit or its mover, and a mover's stamp only for the length of a sweep
	// that puts it back (ClassTick), so the list holds for every class the tick sweeps.
	unsigned short s_ready[2][kSlots], s_still_slot[kSlots];
	unsigned s_still_stamp[kSlots];

	void Walk(Tick* e, unsigned* n)
	{
		unsigned b = 0;
		n[0] = n[1] = 0;
		for (unsigned slot = 0; slot < e->units.n; slot++)
		{
			UnitStruct* u = e->units.first + slot;
			const char* const mover = Get<const char*>(u, UNIT_MOVER);
			if (slot && mover && (u->UnitSelected & 0x10000000u) && Get<unsigned>(mover, MOVER_STAMP) < e->wm)
			{
				s_still_slot[e->nstill] = (unsigned short)slot;
				s_still_stamp[e->nstill++] = Get<unsigned>(mover, MOVER_STAMP);
			}
			while (b < e->nblock && u > e->block[b].b1)
				b++;
			if (b >= e->nblock || u < e->block[b].b0)
				continue;
			const char* course = WantingCourse(u);
			if (!course)
			{
				NotReady(slot);
				continue;
			}
			const unsigned stamp = Get<unsigned>(course, COURSE_STAMP);
			unsigned kind;
			if (!stamp)
				kind = 0;
			else if (!(Get<unsigned char>(mover, MOVER_FLAGS) & 4) && Get<int>(course, COURSE_COUNT) < 2 && s_found[slot] == stamp)
				kind = 0;
			else if (e->now >= stamp + kRetry)     // the engine's gate as 0x44F271 compares it
				kind = 1;
			else
			{
				NotReady(slot);
				continue;
			}
			s_ready[kind][n[kind]++] = (unsigned short)slot;
			Ready(slot, e->now);
		}
	}

	// The class's watermark, and the marking of the units that have just become obstacles: 0x440AF0's own
	// (0x440B03..0x440B18, 0x440B4E..0x440BB8) -- the alive units with a mover whose stamp is in [the class's
	// old watermark, the new one), in slot order, taken from the walk's still units -- with the searcher left
	// out as the engine leaves it, its stamp held at GameTime for the sweep (0x440B30, put back at 0x440BC4):
	// it is not marked, and no rectangle the sweep recomputes counts it. The watermark moves once a tick, so
	// the sweep runs for the first search of a class in a tick and for no other. Returns the old watermark.
	unsigned ClassTick(const Tick& e, char* cls, const Job* j)
	{
		const unsigned old = Get<unsigned>(cls, CLASS_WATERMARK);
		Put<unsigned>(cls, CLASS_WATERMARK, e.wm);
		if (e.wm == old)
			return old;
		const unsigned stamp = Get<unsigned>(j->mover, MOVER_STAMP);
		Put<unsigned>(j->mover, MOVER_STAMP, e.now);
		for (unsigned i = 0; i < e.nstill; i++)
			if (s_still_stamp[i] >= old && s_still_slot[i] != j->slot)
			{
				const UnitStruct* u = e.units.first + s_still_slot[i];
				reinterpret_cast<RemarkFn>(VA_REMARK)(cls, CellWord(u), FootWord(u));
			}
		Put<unsigned>(j->mover, MOVER_STAMP, stamp);
		return old;
	}

	// THE WRITES of one search. The stamp is the engine's wants-search's (0x44F27E), held off 0, which here
	// reads as a new goal. THE BOUNDS a search rests on: the class record among the 32, its map of the
	// pathfinder's grid and of the cells' grid, a goal, a player. A unit one of them refuses is not searched
	// and gets the result of a search that found nothing.
	void JobWrites(const Tick& e, Job* j, unsigned slot)
	{
		std::memset(j, 0, offsetof(Job, code));
		j->slot = slot;
		j->unit = e.units.first + slot;
		j->mover = Get<char*>(j->unit, UNIT_MOVER);
		j->course = Get<char*>(j->mover, 0);
		j->goal = Get<char*>(j->course, COURSE_GOAL);
		j->cls = Get<char*>(j->mover, MOVER_CLASS);
		j->stamp = e.now ? e.now : 1;
		j->stamp0 = Get<unsigned>(j->course, COURSE_STAMP);
		Put<unsigned>(j->course, COURSE_STAMP, j->stamp);
		j->q.seat = 10;
		for (unsigned b = 0; b < e.nblock; b++)
			if (j->unit >= e.block[b].b0 && j->unit <= e.block[b].b1)
				j->q.seat = e.block[b].seat;
		const int ci = ClassIndex(j->cls);
		if (j->q.seat > 9 || !j->goal || ci < 0 || !Get<const void*>(j->cls, CLASS_MAP) || Get<int>(j->cls, CLASS_W) != e.w ||
			Get<int>(j->cls, CLASS_H) != e.h || e.ta->FeatureMapSizeX != e.w || e.ta->FeatureMapSizeY != e.h)
			return;
		j->whole = true;
		j->ci = (unsigned)ci;
		const PatchKind kind = PatchKindOf(j->unit, ClassTick(e, j->cls, j), e.wm);
		j->patch_kind = (unsigned char)(kind == PATCH_GIVEN && !PatchCopy(j->cls, j->unit, j->code, &j->patch) ? PATCH_REFUSED : kind);
		// an obstacle: its rectangle is recomputed in the map with the unit in it, as the cleanup 0x440BE0
		// leaves it; the search reads its patch there
		if (kind != PATCH_NONE)
			reinterpret_cast<RemarkFn>(VA_REMARK)(j->cls, CellWord(j->unit), FootWord(j->unit));
	}

	// The unit put back as the walk found it: the course's stamp before the writes.
	__forceinline void JobPutBack(Job* j) { Put<unsigned>(j->course, COURSE_STAMP, j->stamp0); }

	// THE QUERY of one job, made from the engine's memory while the scheduler's thread is inside the
	// scheduler, on the tick's copies when the searches end at the join, else the engine's own maps. A job
	// with no query is not searched.
	void JobReady(const Tick& e, Job* j)
	{
		j->r.kind = Refused;
		if (!j->whole)
			return;
		const uint32_t* map = e.late ? reinterpret_cast<const uint32_t*>(ClassPage(e.w, e.h, j->cl)) : Get<const uint32_t*>(j->cls, CLASS_MAP);
		const uint16_t* explored = e.late ? reinterpret_cast<const uint16_t*>(ExploredPage(e.w, e.h, j->ex)) : e.explored;
		switch (MakeQuery(e.ta, j->unit, j->cls, j->goal, map, explored, e.w, e.h, j->q.seat, e.weight, (PatchKind)j->patch_kind,
			&j->q, &j->patch, j->code))
		{
		case QUERY_MADE:
			j->ready = true;
			break;
		case QUERY_OTHER_GOAL:
			j->other = true;
			break;
		default:
			break;
		}
	}

	// One of the tick's copies: 0 the explored map, k the map of class e.cls[k - 1], each into the page the
	// tick took for it. THE BOUND on each length: a whole job of the class has held the class's grid, the
	// cells' and the pathfinder's to one size (JobWrites), and the bytes copied are the ones the engine's
	// reader 0x40D7B0 indexes for a grid of that size -- the engine's own allocations, ((h + 15) >> 4) w
	// dwords (0x44097E..0x4409B8) and w h / 2 bytes (0x483CF6).
	void CopyPage(const Tick& e, unsigned k)
	{
		if (!k)
			std::memcpy(ExploredPage(e.w, e.h, e.ex_page), e.explored, ExploredBytes(e.w, e.h));
		else
		{
			const unsigned ci = e.cls[k - 1];
			std::memcpy(ClassPage(e.w, e.h, e.cl_page[ci]), Get<const void*>(ClassRecord(ci), CLASS_MAP), ClassBytes(e.w, e.h));
		}
	}

	// ---- THE TASKS HANDED OUT ---------------------------------------------------------------------
	//
	// To the helpers and, where it waits for them, the scheduler's thread. A tick makes two handing-outs:
	//   THE FIRST, s_m->sync, begun and ended inside the scheduler (RunSend, RunWait): the jobs' queries
	//     and the tick's copies. Its tasks read engine memory.
	//   THE SECOND, the searches, one of s_m->bg. Its tasks read their own job, the pages that job pins and
	//     their thread's scratch, and write that job's result and that scratch.
	// THE ORDERING the helpers rest on is one word a helper, help[k]:
	//   - only the scheduler's thread gives a handing-out, by exchanging null for it (RunSend) and then
	//     setting the helper's event; helper k puts null back as the last thing it does (HelperMain), and
	//     no other thread does but the giver itself, for a helper whose event could not be set. So between
	//     a helper's null and the next exchange it touches nothing of ours, and a handing-out no helper's
	//     word names has no helper in it;
	//   - the scheduler's thread writes a handing-out's record before it gives it, and rewrites one only
	//     where no word names it: sync after RunWait, which returns only once none does, and a bg record
	//     where it finds none (BgTake).
	// s_done is set after each null a helper stores; RunWait and RunDrain look at the words again after
	// each wait, so a signal left over costs one more look, and none is missed: a helper still named owes
	// the signal.
	// IN THE FIRST the scheduler's thread, the game thread (helpers are woken only in play), does not leave
	// RunWait while a word names sync, so while a helper reads engine memory the engine's state is what
	// the tick's writes left. sync names the scheduler's frame there, so nothing may unwind past it:
	// SchedTick is left by its return alone.
	// IN THE SECOND the game thread goes on and a search reads NO engine memory. Its job is given back
	// (JobFree) by the scheduler's thread alone, and only where no helper can touch it again: it has read
	// the job's state as JOB_DONE, the helper's last write to a job it claimed; or the exchange that closed
	// the claiming showed that no thread claimed it; or every helper's word is null (DueDrop). The pages it
	// pins are written only while no job pins them; its scratch is freed only after DueDrop. So what the
	// engine does meanwhile cannot reach a search, and a search reaches nothing the engine reads until the
	// scheduler's thread applies its result (JobResult).

	void RunTake(Run* r, Scratch* s)
	{
		for (;;)
		{
			const LONG i = r->next;
			if (i >= (LONG)r->n || (r->cap && (unsigned)r->pops >= r->cap))
				return;
			if (InterlockedCompareExchange(&r->next, i + 1, i) == i)
				r->task(r, (unsigned)i, s);
		}
	}

	DWORD WINAPI HelperMain(LPVOID number)
	{
		const unsigned k = (unsigned)(size_t)number;
		// a wait that fails was given nothing; s_wake[k] is never closed, so none does
		while (WaitForSingleObject(s_wake[k], INFINITE) == WAIT_OBJECT_0)
		{
			Run* const r = s_m->help[k];
			if (r)
				RunTake(r, &s_pf[1 + k]);
			InterlockedExchangePointer(HelpWord(k), nullptr);
			SetEvent(s_done);
		}
		return 0;
	}

	// Gives `r` to helpers in none, `want` at most. One whose event cannot be set was not woken: its word
	// goes back.
	void RunSend(Run* r, unsigned want)
	{
		for (unsigned k = 0; k < s_helpers && want; k++)
		{
			if (InterlockedCompareExchangePointer(HelpWord(k), r, nullptr))
				continue;
			if (SetEvent(s_wake[k]))
				want--;
			else
				InterlockedExchangePointer(HelpWord(k), nullptr);
		}
	}

	bool RunIdle()
	{
		for (unsigned k = 0; k < s_helpers; k++)
			if (!s_m->help[k])
				return true;
		return false;
	}

	bool RunNamed(const Run* r)
	{
		for (unsigned k = 0; k < s_helpers; k++)
			if (s_m->help[k] == r)
				return true;
		return false;
	}

	// The scheduler's thread takes tasks of `r` too, and returns once no helper is in it: every task
	// claimed has ended.
	void RunWait(Run* r)
	{
		RunTake(r, &s_pf[0]);
		while (RunNamed(r))
			WaitForSingleObject(s_done, INFINITE);
	}

	// Returns once no helper is in any handing-out.
	void RunDrain()
	{
		for (unsigned k = 0; k < s_helpers; k++)
			while (s_m->help[k])
				WaitForSingleObject(s_done, INFINITE);
	}

	// A searches' record no helper is in: of kThreads, with kHelpers helpers, one.
	Run* BgTake()
	{
		for (unsigned i = 0; i < kThreads; i++)
			if (!RunNamed(&s_m->bg[i]))
				return &s_m->bg[i];
		return nullptr;
	}

	void TaskReady(Run* r, unsigned i, Scratch*)
	{
		if (i < r->e->nj)
			JobReady(*r->e, r->e->job[i]);
		else
			CopyPage(*r->e, i - r->e->nj);
	}

	// JOB_DONE is the helper's last touch of the job, written for one a set goal skipped too.
	void TaskSearch(Run* r, unsigned i, Scratch* s)
	{
		Job* j = r->job[i];
		if (InterlockedCompareExchange(&j->state, JOB_RUNNING, JOB_QUEUED) == JOB_QUEUED)
		{
#if TDRAW_PATHFIND_VERIFY
			LARGE_INTEGER t0, t1;
			QueryPerformanceCounter(&t0);
#endif
			if (j->ready)
				Search(s, j->q, &j->r);
#if TDRAW_PATHFIND_VERIFY
			QueryPerformanceCounter(&t1);
			InterlockedExchangeAdd64(&s_search_counts, t1.QuadPart - t0.QuadPart);
#endif
			InterlockedExchangeAdd(&r->pops, (LONG)j->r.pops);
		}
		InterlockedExchange(&j->state, JOB_DONE);
	}

	Job* JobNew() { return s_m->nfree ? s_m->free[--s_m->nfree] : nullptr; }

	// Ends a job no helper is in: its pages are free of it.
	void JobFree(Job* j)
	{
		if (j->pinned)
		{
			s_m->ex_pin[j->ex]--;
			s_m->cl_pin[j->cl]--;
			j->pinned = false;
		}
		s_m->free[s_m->nfree++] = j;
	}

	// The job's unit as the walk found it: its slot still holds that unit, mover, class and ground course,
	// and the course's stamp is the search's. THE BOUND that makes a result's writes land on the object it
	// was found for, or nowhere: a course made since has the stamp 0 its constructor writes, and the
	// scheduler alone writes another, a later tick's.
	// THE LIFETIME of what it follows: a unit's course and mover are freed only by the unit's teardown,
	// which destroys the course (0x43DD10), frees the mover and then zeroes the unit's +0 and its type +0xA6
	// (0x486DA9..0x486DC7), on this same thread. So a unit gone since the search fails the type or the
	// mover test here before any pointer of it is followed (DISASSEMBLED).
	bool JobCourseStands(const Job* j)
	{
		Units units;
		if (!units.Open(Main()) || units.At(j->slot) != j->unit || !j->unit->UnitID)
			return false;
		const char* mover = Get<const char*>(j->unit, UNIT_MOVER);
		if (!mover || mover != j->mover || Get<const char*>(mover, MOVER_CLASS) != j->cls)
			return false;
		const char* course = Get<const char*>(mover, 0);
		return course && course == j->course && Get<DWORD>(course, 0) == VT_GROUND_COURSE &&
			Get<unsigned>(course, COURSE_STAMP) == j->stamp;
	}

	// ...and the goal is the search's while the job is not stale: every engine path that changes or frees a
	// course's goal enters set goal first, and set goal's entry marks the course's job (GoalTouch).
	bool JobStands(const Job* j)
	{
		return !j->stale && JobCourseStands(j) && Get<const char*>(j->course, COURSE_GOAL) == j->goal;
	}

#if TDRAW_PATHFIND_VERIFY
	bool SameResult(const Result& a, const Result& b)
	{
		return a.kind == b.kind && a.goalflags == b.goalflags && a.n == b.n && !std::memcmp(a.pts, b.pts, sizeof a.pts) &&
			a.pops == b.pops && a.used == b.used && a.relax == b.relax && a.found_x == b.found_x && a.found_z == b.found_z;
	}
#endif

	// THE RESULT of one job whose unit still stands as searched, through the engine's own set path
	// 0x44F080: a path's corners, or none -- and with none it ORs 0x40 into the order when the unit is not
	// in its goal (0x44F089..0x44F0B9), which is why that bit is left to it. The search's other flags go
	// through the goal's 0x44CED0, as the search start ORs them.
	void JobResult(Job* j)
	{
		const unsigned flags = j->r.goalflags & ~GOALF_NOPATH;
		const bool found = j->r.kind == Found && j->r.n >= 1 && j->r.n <= kWaypoints;
		if (j->applied)
			return;
		j->applied = true;
		if (j->stale)
		{
			// A SEARCH A SET GOAL OVERTOOK. Its path is for the goal that was; what it tells is whether the
			// unit was on its way, and set goal's rule for such a unit (BeforeSetGoal) is then applied late:
			// the stamp goes to 0 and the walk serves the new goal. One that found nothing leaves the
			// engine's rule.
#if TDRAW_PATHFIND_VERIFY
			s_check.stale++;
#endif
			if (found && JobCourseStands(j))
				Put<unsigned>(j->course, COURSE_STAMP, 0);
			return;
		}
		if (!JobStands(j))
		{
			if (s_said_dropped++ < 4)
				IDDrawSurface::OutptFmtTxt("[Pathfinder] slot %u no longer holds the unit searched for: its result is not applied", j->slot);
			return;
		}
		if (flags && j->goal)
			reinterpret_cast<GoalFlagFn>(VA_GOAL_FLAG)(j->goal, flags);
		reinterpret_cast<SetPathFn>(VA_SET_PATH)(j->course, found ? j->r.pts : nullptr, found ? j->r.n : 0);
		s_found[j->slot] = found ? j->stamp : 0;
		if (j->other && s_said_other++ < 4)
			IDDrawSurface::OutptFmtTxt("[Pathfinder] a goal of a kind the search has not (vtable %#x): no path", Get<DWORD>(j->goal, 0));
		if (j->r.kind != Found && j->r.kind != NoSearch && j->r.kind != NoPath && j->r.kind != OffGrid && s_said_failed++ < 4)
			IDDrawSurface::OutptFmtTxt("[Pathfinder] a search not made (kind %d, slot %u, class %p): no path", j->r.kind, j->slot,
				static_cast<void*>(j->cls));
#if TDRAW_PATHFIND_VERIFY
		s_block.searches++;
		s_block.found += found;
		NoteWait(j->slot, j->stamp);
		if (!j->checked)
			s_check.inside++;
		else if (s_check.live++, SameResult(j->r, j->check))
			s_check.live_equal++;
		else if (s_check.live_differ++ < 8)
			IDDrawSurface::OutptFmtTxt("[Pathfinder] VERIFY: DIFFERENT from the same search on the engine's maps: slot %u, kind %d/%d, "
				"n %d/%d, pops %d/%d", j->slot, j->r.kind, j->check.kind, j->r.n, j->check.n, j->r.pops, j->check.pops);
#endif
	}

	// A search not ended when its tick is joined is CARRIED: its helper goes on, and the job waits in
	// s_m->carry for a later join. A helper is inside each carried job, so the list holds them; were it
	// full, the job is waited for here.
	void JobCarry(Job* j)
	{
		if (s_m->ncarry == kHelpers)
		{
			while (j->state != JOB_DONE)
				WaitForSingleObject(s_done, INFINITE);
			JobResult(j);
			JobFree(j);
			return;
		}
		s_m->carry[s_m->ncarry++] = j;
#if TDRAW_PATHFIND_VERIFY
		s_check.carried++;
#endif
	}

	// The carried searches that have ended: their results, in the order they were chosen.
	void CarriedReap()
	{
		unsigned k = 0;
		for (unsigned i = 0; i < s_m->ncarry; i++)
		{
			Job* j = s_m->carry[i];
			if (j->state != JOB_DONE)
			{
				s_m->carry[k++] = j;
				continue;
			}
			JobResult(j);
			JobFree(j);
		}
		s_m->ncarry = k;
	}

	// THE TICK'S JOIN, on the scheduler's thread, WITH NO WAIT: the claiming ends (kRunClosed), and each job
	// is one of three. Not claimed: no thread will touch it, and it is put back as the walk found it, to be
	// chosen again. Claimed and ended: its result is applied, in the order chosen. Claimed and not ended:
	// carried. The cursors move to the last unit claimed of each kind.
	void TickEnd()
	{
		Run* r = s_m->tick;
		if (!r)
			return;
		s_m->tick = nullptr;
		unsigned claimed = (unsigned)InterlockedExchange(&r->next, kRunClosed);
		if (claimed > r->n)
			claimed = r->n;
		for (unsigned i = 0; i < r->n; i++)
		{
			Job* j = r->job[i];
			if (i >= claimed)
			{
				if (!j->applied && JobStands(j))
					JobPutBack(j);
				JobFree(j);
				continue;
			}
			s_cursor[j->kind] = j->slot;
			if (j->state != JOB_DONE)
			{
				JobCarry(j);
				continue;
			}
			JobResult(j);
			JobFree(j);
		}
	}

	void DueJoin()
	{
		CarriedReap();
		TickEnd();
	}

	// Ends every search and drops every job, results and all: a new game, scratches about to be freed.
	// THE LIFETIME: it returns once no helper is in any handing-out. No stamp is put back: a new game makes
	// its units again.
	void DueDrop()
	{
		if (!s_m)
			return;
		if (s_m->tick)
			InterlockedExchange(&s_m->tick->next, kRunClosed);
		RunDrain();
		if (s_m->tick)
			for (unsigned i = 0; i < s_m->tick->n; i++)
				JobFree(s_m->tick->job[i]);
		for (unsigned i = 0; i < s_m->ncarry; i++)
			JobFree(s_m->carry[i]);
		s_m->tick = nullptr;
		s_m->ncarry = 0;
	}

	// The tick's pages: a free explored page, and a free class page for each class it searches; each whole
	// job pins the two it reads. THE BOUND: a carried job pins one of each, there are s_helpers of them at
	// most, and the copies hold s_helpers pages more of each kind than a tick takes; false were it not so.
	bool PagesTake(Tick* e)
	{
		unsigned long long taken = 0;
		unsigned k = 0;
		while (k <= s_helpers && s_m->ex_pin[k])
			k++;
		if (k > s_helpers)
			return false;
		e->ex_page = (unsigned char)k;
		for (unsigned i = 0; i < e->ncls; i++)
		{
			for (k = 0; k < s_frz_class_pages && (s_m->cl_pin[k] || (taken >> k & 1)); k++)
				;
			if (k >= s_frz_class_pages)
				return false;
			taken |= 1ull << k;
			e->cl_page[e->cls[i]] = (unsigned char)k;
		}
		for (unsigned i = 0; i < e->nj; i++)
		{
			Job* j = e->job[i];
			if (!j->whole)
				continue;
			j->ex = e->ex_page;
			j->cl = e->cl_page[j->ci];
			j->pinned = true;
			s_m->ex_pin[j->ex]++;
			s_m->cl_pin[j->cl]++;
		}
		return true;
	}

#if TDRAW_PATHFIND_VERIFY
	// Each search of the tick handed to the helpers made again here, on the engine's own maps as the tick's
	// writes left them -- what the copies hold -- on this thread's scratch, which no helper uses.
	void CheckOnLiveMaps(const Tick& e)
	{
		CheckTime time;
		for (unsigned i = 0; i < e.nj; i++)
		{
			Job* j = e.job[i];
			if (!j->ready)
				continue;
			Query q = j->q;
			q.map = Get<const uint32_t*>(j->cls, CLASS_MAP);
			q.explored = e.explored;
			Search(&s_pf[0], q, &j->check);
			j->checked = true;
		}
	}
#endif

	// In the engine's scheduler's place, once a tick. THE ORDERING its steps rest on: everything from the
	// walk to the searches' handing-out runs inside this one call on the thread that runs the simulation,
	// and nothing it calls makes, moves or frees a unit, a mover, a course or a goal -- 0x440830 writes a
	// class's map -- so a pointer the walk read is the same object in every later step of the call.
	// False, with every search dropped, for a game the tables here cannot hold (OpenTick).
	bool SchedTick()
	{
		// the carried searches that have ended give their results here, and a tick the step did not join is
		// joined
		DueJoin();
		Tick e;
		const char* why = nullptr;
		switch (OpenTick(&e, &why))
		{
		case TICK_NO_GAME:
			return true;
		case TICK_REFUSED:
			DueDrop();
			IDDrawSurface::OutptFmtTxt("[Pathfinder] the engine's scheduler serves the rest of this game: %s", why);
			return false;
		default:
			break;
		}
		// Helpers are woken only IN PLAY, when the scheduler's thread is the game thread: the load's word
		// main+0x38D75 is 0. THE ORDERING: the loader goes on to its call of the players phase only once it
		// has read bit 3 of that word (0x4975D6..0x4975F1), which the game thread sets (0x498576) and nothing
		// clears until the game thread zeroes the word at its entry into play (0x49847E), after it has read
		// the loader's last store. So in the loader's call the word is not 0, and where it is 0 no loader
		// lives. In that call every task runs on the loader's own thread, as the engine's searches did.
		// IN PLAY WITH EVERY HELPER'S WORD SET -- each inside a carried search, or not yet out of a record the
		// join has closed -- no helper could take a search, so the tick chooses none and the walk is not made:
		// the units stay ready. Only this thread sets a word, so a record found free stays free.
		const unsigned helpers = e.play && s_helpers && s_pf[s_helpers].pool ? s_helpers : 0;
		Run* t = helpers && !RunIdle() ? nullptr : BgTake();
		if (!t)
			return true;
		unsigned n[2], at[2], taken[2] = { 0, 0 }, nj = 0;
		Walk(&e, n);
		// THE WEIGHT, by the tick's load: the engine's three (0x40EBBF..0x40EC06)
		const unsigned wi = n[0] + n[1] > kCapSearches ? 2 : n[0] + n[1] > kQuiet ? 1 : 0;
		e.weight *= wi == 2 ? 6 : wi == 1 ? 3 : 1;
#if TDRAW_PATHFIND_VERIFY
		s_check.weight[wi]++;
#endif
		for (unsigned kind = 0; kind < 2; kind++)
			for (at[kind] = 0; at[kind] < n[kind] && s_ready[kind][at[kind]] <= s_cursor[kind]; at[kind]++)
				;
		while (nj < kCapSearches && s_m->nfree && taken[0] + taken[1] < n[0] + n[1])
		{
			Job* j = JobNew();
			const unsigned kind = taken[0] < n[0] ? 0 : 1;
			const unsigned slot = s_ready[kind][(at[kind] + taken[kind]++) % n[kind]];
			JobWrites(e, j, slot);
			j->kind = kind;
			t->job[nj++] = j;
		}
		if (!nj)
			return true;
		e.job = t->job;
		e.nj = nj;
		unsigned seen = 0;
		for (unsigned i = 0; i < nj; i++)
			if (t->job[i]->whole && !(seen >> t->job[i]->ci & 1))
			{
				seen |= 1u << t->job[i]->ci;
				e.cls[e.ncls++] = (unsigned char)t->job[i]->ci;
			}
		// BESIDE THE GAME: in play, a helper to search, and pages of the tick's copies
		const int beside = !e.play ? 1 : !helpers ? 2 : !s_frz ? 3 : !PagesTake(&e) ? 4 : 0;
		e.late = beside == 0;
#if TDRAW_PATHFIND_VERIFY
		s_check.beside[beside]++;
#endif
		// the first handing-out, ended here; this thread takes tasks too, so one helper fewer than tasks
		Run* r = &s_m->sync;
		r->task = TaskReady;
		r->e = &e;
		r->n = nj + (e.late && e.ncls ? 1 + e.ncls : 0);
		r->cap = 0;
		r->pops = 0;
		r->next = 0;
		RunSend(r, helpers < r->n - 1 ? helpers : r->n - 1);
		RunWait(r);
		// the searches: beside the game the helpers', and the joins find them ended or carry them; else this
		// thread's too, ended here
		t->task = TaskSearch;
		t->e = nullptr;
		t->n = nj;
		t->cap = kCapPops;
		t->pops = 0;
		t->next = 0;
		s_m->tick = t;
		if (e.late)
		{
			RunSend(t, helpers < nj ? helpers : nj);
#if TDRAW_PATHFIND_VERIFY
			CheckOnLiveMaps(e);
#endif
		}
		else
		{
			RunSend(t, helpers < nj - 1 ? helpers : nj - 1);
			RunWait(t);
		}
		// IN PLAY THE RESULTS WAIT FOR THE STEP'S JOIN, wherever the searches were made. The loader's call
		// ends its own.
		if (!e.play)
			TickEnd();
		return true;
	}

	// A SET GOAL ON A COURSE WITH A SEARCH DUE, before the engine's function runs. It replaces the goal a
	// result is for, and reads the stamp a result writes beside. THE ORDERING that keeps a result with its
	// goal: this runs at set goal's entry, and every engine path that changes or frees a course's goal
	// enters set goal first (the order's destructor 0x43A1F0 and the four goal installers 0x4388D0,
	// 0x438930, 0x438A00, 0x438AD0), so a job's goal is the course's until the job is marked here.
	//   - a search not begun is taken from the helpers by one exchange of its state, and the unit put back
	//     as the walk found it: it is chosen again, for the new goal;
	//   - a search ended has its result applied now, as a join would have;
	//   - a search running is marked stale: its path is for the goal that was (JobResult).
	void GoalTouch(const char* course)
	{
		Run* r = s_m->tick;
		const unsigned n = (r ? r->n : 0) + s_m->ncarry;
		for (unsigned i = 0; i < n; i++)
		{
			Job* j = i < s_m->ncarry ? s_m->carry[i] : r->job[i - s_m->ncarry];
			if (j->course != course || j->applied || j->stale)
				continue;
			if (InterlockedCompareExchange(&j->state, JOB_SKIPPED, JOB_QUEUED) == JOB_QUEUED)
			{
				if (JobStands(j))
					JobPutBack(j);
				j->applied = true;
#if TDRAW_PATHFIND_VERIFY
				s_check.skipped++;
#endif
			}
			else if (j->state == JOB_DONE)
				JobResult(j);
			else
				j->stale = true;
		}
	}

	// THE STEP'S JOIN, in place of the step's call of 0x466580 (0x4955E5), which it then makes. The step
	// runs from the players phase's return to this call in a straight line (0x495564..0x4955E5), so a tick
	// whose scheduler handed searches out reaches it. It waits for nothing.
	void __cdecl TickJoin()
	{
		if (s_ours && (s_m->tick || s_m->ncarry))
		{
#if TDRAW_PATHFIND_VERIFY
			const double t0 = NowUs();
#endif
			DueJoin();
#if TDRAW_PATHFIND_VERIFY
			s_block.us += NowUs() - t0;
#endif
		}
		reinterpret_cast<void (__cdecl*)()>(VA_STEP_COUNT)();
	}

	// The ground course's set goal 0x44F2A0, at its entry. The function's own rule zeroes the stamp when
	// the course's last search began 10 ticks ago or more (0x44F420..0x44F436), and the walk then serves
	// the unit at once. Here the same is done inside those 10 ticks for a unit whose last search found a
	// path: A UNIT ON ITS WAY THAT IS SENT ELSEWHERE. The function only ever zeroes the stamp, so the 0
	// stands. A unit whose last search found nothing keeps the engine's rule: an order handler answers such
	// a search with a new goal within a tick or two, for as long as the unit stays where it is, and each
	// would be a search. THE BOUND: the course's unit against the unit array, as a slot of the table.
	// FIRST the course's search due, if it has one (GoalTouch).
	int __stdcall BeforeSetGoal(PInlineX86StackBuffer regs)
	{
		char* course = reinterpret_cast<char*>(regs->Ecx);
		if (!s_ours || !course)
			return 0;
		if (s_m->tick || s_m->ncarry)
			GoalTouch(course);
		const TAdynmemStruct* ta = Main();
		if (!ta || !*reinterpret_cast<const DWORD*>(regs->Esp + 4) || Get<DWORD>(course, 0) != VT_GROUND_COURSE)
			return 0;
		const unsigned stamp = Get<unsigned>(course, COURSE_STAMP);
		if (stamp <= (unsigned)ta->GameTime - 10u)
			return 0;
		Units units;
		const unsigned slot = units.Open(ta) ? units.SlotOf(Get<const void*>(course, COURSE_UNIT)) : kSlots;
		if (slot < kSlots && s_found[slot] == stamp)
			Put<unsigned>(course, COURSE_STAMP, 0);
		return 0;
	}

	// ---- which scheduler serves the game ----------------------------------------------------------

	bool EngineIdle(const void* pf) { return !Get<const void*>(pf, PF_UNIT); }

#if TDRAW_PATHFIND_VERIFY
	void LogTimes(const char* when);
	void Census(const TAdynmemStruct* ta);

	void CloseBlock()
	{
		s_block.phase = s_phase;
		s_block.helper_us = static_cast<double>(InterlockedExchange64(&s_search_counts, 0)) * s_us_per_count;
		Figures& t = s_total[s_phase];
		t.ticks += s_block.ticks;
		t.us += s_block.us;
		t.helper_us += s_block.helper_us;
		t.searches += s_block.searches;
		t.found += s_block.found;
		t.waited += s_block.waited;
		t.wait_sum += s_block.wait_sum;
		if (s_block.wait_max > t.wait_max)
			t.wait_max = s_block.wait_max;
		s_blocks.push_back(s_block);
		s_block = Figures();
		std::memset(s_ready_since, 0, sizeof s_ready_since);
		if (s_blocks.size() % kProgressEvery == 0)
			LogTimes("so far");
	}

	// The phase of this tick: a block ends after kBlock ticks, the engine's at the first tick it has no
	// search under way, ours once every search it carries has ended and been applied.
	void NextPhase(void* pf)
	{
		if (++s_phase_ticks < kBlock)
			return;
		if (s_phase == PHASE_ENGINE)
		{
			if (!EngineIdle(pf))
				return;
			CloseBlock();
			s_phase = PHASE_OURS;
		}
		else
		{
			DueJoin();
			// each carried search's helper sets s_done after it has left the search
			while (s_m->ncarry)
			{
				WaitForSingleObject(s_done, INFINITE);
				CarriedReap();
			}
			CloseBlock();
			s_phase = PHASE_ENGINE;
		}
		s_phase_ticks = 0;
	}
#endif

	// In the engine's scheduler's place at the players phase's call (0x464F92), thiscall on the
	// pathfinder object. Ours serves a game from its first tick with no engine search under way, until a
	// tick finds the game past the tables here (SchedTick).
	void __fastcall Dispatch(void* pf)
	{
		if (s_serving == SERVING_UNDECIDED && EngineIdle(pf))
			s_serving = SERVING_OURS;
#if TDRAW_PATHFIND_VERIFY
		if (s_serving == SERVING_OURS)
			NextPhase(pf);
		s_check_us = 0;
		const double t0 = NowUs();
#endif
		s_ours = s_serving == SERVING_OURS;
#if TDRAW_PATHFIND_VERIFY
		s_ours = s_ours && s_phase == PHASE_OURS;
#endif
		if (s_ours && !SchedTick())
		{
			s_serving = SERVING_ENGINE;
			s_ours = false;
		}
		if (!s_ours)
		{
#if TDRAW_PATHFIND_VERIFY
			Census(Main());
#endif
			reinterpret_cast<PfFn>(VA_SCHEDULER)(pf);
		}
#if TDRAW_PATHFIND_VERIFY
		if (s_serving == SERVING_OURS)
		{
			s_block.us += NowUs() - t0 - s_check_us;
			s_block.ticks++;
		}
#endif
	}

	// Every search of the game dropped and the game's tables cleared; `next` serves until the next game's
	// init.
	void GameReset(Serving next)
	{
		DueDrop();
		std::memset(s_found, 0, sizeof s_found);
		std::memset(s_cursor, 0, sizeof s_cursor);
		s_serving = next;
		s_ours = false;
#if TDRAW_PATHFIND_VERIFY
		s_phase = PHASE_ENGINE;
		s_phase_ticks = 0;
		std::memset(s_ready_since, 0, sizeof s_ready_since);
		s_block = Figures();
		s_or.open = false;
#endif
	}

	// A NEW GAME, at the pathfinder's per-game init, in place of its one call (0x44F6B3) on the loader
	// thread: the next game is chosen for again.
	void* __fastcall PathfinderInit(void* pf)
	{
		void* const made = reinterpret_cast<InitFn>(VA_INIT)(pf);
		GameReset(SERVING_UNDECIDED);
		return made;
	}

	// THE GAME'S END, in place of the level teardown's first call (0x491B76), on the game thread. THE
	// LIFETIME: the teardown then frees the orders and the units (0x485980 at 0x491B95), whose courses'
	// set goals would otherwise meet searches of ours; here every search is dropped first and our
	// scheduler, the join and the observer stand aside until the next game's init. The game thread makes
	// the next loader thread (0x4982CA, CreateThread) only after the teardown has returned.
	void __fastcall GameEnds(void* net)
	{
		GameReset(SERVING_ENGINE);
		reinterpret_cast<PfFn>(VA_END_FIRST)(net);
	}

	// ---- the check build's oracle: our search beside each of the engine's ------------------------

#if TDRAW_PATHFIND_VERIFY
	const DWORD VA_GRID_RESET     = 0x0040D900u;
	const DWORD RA_RESET_SEARCH   = 0x0040E690u;    // the reset's return into the search start 0x40E630
	const DWORD VA_EXTRACT        = 0x0040E050u;
	const DWORD VA_CLEANUP        = 0x00440BE0u;
	const DWORD RA_END_OFFGRID    = 0x0040E7C0u;    // the cleanup's return addresses, each an exit of a search
	const DWORD RA_END_NOSEARCH   = 0x0040E998u;
	const DWORD RA_END_SCHEDULER  = 0x0040EEDFu;

	const DWORD VA_WATERMARK      = 0x00440AF0u;    // (class; unit), called once, at 0x40E655 in the search start

	std::unique_ptr<InlineSingleHook> s_watermark_hook, s_reset_hook, s_extract_hook, s_cleanup_hook;

	// The engine's watermark, at its entry: the class's watermark before the search's update, which decides
	// what the searcher reads of its own rectangle (PatchKindOf).
	int __stdcall BeforeWatermark(PInlineX86StackBuffer regs)
	{
		if (s_ours)
			return 0;
		s_or.wm_cls = reinterpret_cast<const char*>(regs->Ecx);
		s_or.wm_unit = *reinterpret_cast<const UnitStruct* const*>(regs->Esp + 4);
		s_or.wm_old = s_or.wm_cls ? Get<unsigned>(s_or.wm_cls, CLASS_WATERMARK) : 0;
		return 0;
	}

	// The engine's grid reset inside a search start: the watermark has run, the search has read nothing
	// yet. Our search runs here on the same inputs, its query made as the release scheduler makes it, and
	// a fresh patch is compared with the entries the engine has just recomputed; a given one is the map's.
	int __stdcall BeforeReset(PInlineX86StackBuffer regs)
	{
		if (s_ours || *reinterpret_cast<const DWORD*>(regs->Esp) != RA_RESET_SEARCH)
			return 0;
		CheckTime time;
		s_or.open = false;
		const TAdynmemStruct* ta = Main();
		const char* pf = ta ? Get<const char*>(ta, OFF_PATHFINDER) : nullptr;
		if (!pf)
			return 0;
		const UnitStruct* unit = Get<const UnitStruct*>(pf, PF_UNIT);
		const char* cls = Get<const char*>(pf, PF_CLASS);
		const char* goal = Get<const char*>(pf, PF_GOAL);
		const int w = cls ? Get<int>(cls, CLASS_W) : 0, h = cls ? Get<int>(cls, CLASS_H) : 0;
		const uint32_t* map = cls ? Get<const uint32_t*>(cls, CLASS_MAP) : nullptr;
		const unsigned char seat = Get<unsigned char>(pf, PF_SEAT);
		if (!unit || !goal || ClassIndex(cls) < 0 || !map || !Get<const char*>(unit, UNIT_MOVER) || w != Get<int>(pf, PF_W) ||
			h != Get<int>(pf, PF_H) || !ta->MAPPED_MEMORY_p || !ScratchFor(w, h) || seat > 9 || s_or.wm_unit != unit ||
			s_or.wm_cls != cls)
		{
			s_check.refused++;
			return 0;
		}
		Query q;
		Patch patch;
		PatchKind kind = PatchKindOf(unit, s_or.wm_old, Get<unsigned>(cls, CLASS_WATERMARK));
		if (kind == PATCH_GIVEN && !PatchCopy(cls, unit, s_or.patch, &patch))
			kind = PATCH_REFUSED;
		switch (MakeQuery(ta, unit, cls, goal, map, ta->MAPPED_MEMORY_p, w, h, seat, Get<int>(pf, PF_WEIGHT), kind, &q, &patch,
			s_or.patch))
		{
		case QUERY_OTHER_GOAL:
			s_check.other_goal++;
			return 0;
		case QUERY_REFUSED:
			s_check.patch_refused++;
			return 0;
		default:
			break;
		}
		if (kind == PATCH_GIVEN)
		{
			// what a fresh patch would have read instead: the map's entries the engine's search reads here and
			// no recompute matches
			static unsigned char fresh[kPatchMax];
			CellView v;
			Patch fp;
			s_check.patch_kept++;
			if (OpenCellView(&v, ta, cls, unit) && PatchOf(v, unit, fresh, &fp))
				for (int i = 0; i < fp.w * fp.h; i++)
					s_check.kept_stale += fresh[i] != s_or.patch[i];
		}
		if (kind == PATCH_FRESH)
		{
			s_check.patched++;
			for (int z = 0; z < patch.h; z++)
				for (int x = 0; x < patch.w; x++)
				{
					const int ours = s_or.patch[z * patch.w + x], theirs = MapCode(map, w, patch.x0 + x, patch.z0 + z);
					s_check.patch_entries++;
					if (ours != theirs && s_check.patch_differ++ < 8)
						IDDrawSurface::OutptFmtTxt("[Pathfinder] VERIFY: PATCH entry %d,%d: ours %d, the map's %d", patch.x0 + x,
							patch.z0 + z, ours, theirs);
				}
		}
		Search(&s_pf[0], q, &s_or.res);
		s_or.unit = unit;
		s_or.course = Get<const char*>(Get<const char*>(unit, UNIT_MOVER), 0);
		s_or.node = Get<const char*>(goal, 4);
		s_or.node_flags0 = s_or.node ? Get<unsigned>(s_or.node, NODE_FLAGS) : 0;
		s_or.gametime = (unsigned)ta->GameTime;
		s_or.found = false;
		s_or.open = true;
		s_block.searches++;
		Units units;
		if (units.Open(ta))
			NoteWait(units.SlotOf(unit), s_or.gametime);
		return 0;
	}

	int __stdcall BeforeExtract(PInlineX86StackBuffer)
	{
		if (!s_ours)
			s_or.found = true;
		return 0;
	}

	// The class's cleanup at a search's end: the engine has set the path, or none.
	int __stdcall BeforeCleanup(PInlineX86StackBuffer regs)
	{
		const DWORD ra = *reinterpret_cast<const DWORD*>(regs->Esp);
		if (s_ours || !s_or.open || *reinterpret_cast<const UnitStruct* const*>(regs->Esp + 4) != s_or.unit)
			return 0;
		s_or.open = false;
		if (ra != RA_END_OFFGRID && ra != RA_END_NOSEARCH && ra != RA_END_SCHEDULER)
			return 0;
		CheckTime time;
		const TAdynmemStruct* ta = Main();
		const char* pf = Get<const char*>(ta, OFF_PATHFINDER);
		const Result& r = s_or.res;
		if (r.kind == Overflow)
		{
			s_check.overflow++;
			return 0;
		}
		const int want = ra == RA_END_OFFGRID ? OffGrid : ra == RA_END_NOSEARCH ? NoSearch : s_or.found ? Found : NoPath;
		const bool spanned = (unsigned)ta->GameTime != s_or.gametime;
		const char* course = s_or.course;
		const char* why = nullptr;
		if (want != r.kind)
			why = "the exit";
		else if (s_or.node && Get<unsigned>(s_or.node, NODE_FLAGS) != (s_or.node_flags0 | r.goalflags))
			why = "the order's flags";
		else if ((want == Found || want == NoPath) && (Get<int>(pf, PF_USED) != r.used || Get<int>(pf, PF_RELAX) != r.relax))
			why = "the pool's use or the bug walk's answer";
		else if (want == Found && course)
		{
			const int n = Get<int>(course, COURSE_COUNT);
			if (Get<short>(pf, PF_FOUND) != r.found_x || Get<short>(pf, PF_FOUND + 2) != r.found_z)
				why = "the goal cell";
			else if (n != r.n || n < 0 || n > kWaypoints || std::memcmp(course + COURSE_WP, r.pts, (size_t)n * 4))
				why = "the path";
			else if (!(Get<unsigned char>(course, COURSE_FLAGS) & 1))
				why = "the path's flag";
		}
		else if (want != Found && course && (Get<unsigned char>(course, COURSE_FLAGS) & 3))
			why = "the path's flags after no path";
		if (spanned)
		{
			// The engine's search spanned ticks: the maps changed between its passes, where ours read one
			// tick's maps whole. Counted apart.
			s_check.spanned++;
			if (why)
				s_check.spanned_differ++;
		}
		else
		{
			s_check.compared++;
			if (!why)
				s_check.equal++;
			else if (s_check.differ++, s_check.logged++ < 8)
				IDDrawSurface::OutptFmtTxt("[Pathfinder] VERIFY: DIFFERENT in %s: engine exit %d used %d relax %d count %d, ours exit %d "
					"used %d relax %d count %d pops %d (unit cell %d,%d, GameTime %u)", why, want, Get<int>(pf, PF_USED),
					Get<int>(pf, PF_RELAX), course ? Get<int>(course, COURSE_COUNT) : -1, r.kind, r.used, r.relax, r.n, r.pops,
					s_or.unit->XGridPos, s_or.unit->YGridPos, s_or.gametime);
		}
		if (want == Found)
			s_block.found++;
		return 0;
	}

	// Once an engine tick, before its scheduler: the units ready by the engine's gate, for the waits.
	void Census(const TAdynmemStruct* ta)
	{
		CheckTime time;
		Units units;
		if (!units.Open(ta))
			return;
		const unsigned now = (unsigned)ta->GameTime;
		for (unsigned slot = 1; slot < units.n; slot++)
		{
			const char* course = WantingCourse(units.first + slot);
			if (!course || now < Get<unsigned>(course, COURSE_STAMP) + kRetry)
				NotReady(slot);
			else
				Ready(slot, now);
		}
	}

	void LogTimes(const char* when)
	{
		const Check& c = s_check;
		IDDrawSurface::OutptFmtTxt("[Pathfinder] VERIFY %s: the engine's searches with ours beside them: %u compared, %u equal, %u DIFFERENT; "
			"%u spanned ticks (%u of them different: the engine's read maps that changed between its passes); %u overflowed, "
			"%u refused, %u of a goal kind ours has not; patch: %u searches, %u entries, %u DIFFERENT, %u refused a view or a patch, "
			"%u read the map's own entries, which the engine does not recompute for a searcher still since the last watermark "
			"(%u of their entries not what a recompute gives)",
			when, c.compared, c.equal, c.differ, c.spanned, c.spanned_differ, c.overflow, c.refused, c.other_goal, c.patched,
			c.patch_entries, c.patch_differ, c.patch_refused, c.patch_kept, c.kept_stale);
		IDDrawSurface::OutptFmtTxt("[Pathfinder] VERIFY %s: our searches beside the game made again on the engine's maps: %u, %u equal, "
			"%u DIFFERENT; %u made inside the scheduler; %u carried past their tick, %u a set goal overtook running and %u not begun; "
			"ticks at the base weight %u, at 3 times %u, at 6 times %u; ticks searched beside the game %u, inside the scheduler: %u not "
			"in play, %u with no helper, %u with no copies, %u with no page",
			when, c.live, c.live_equal, c.live_differ, c.inside, c.carried, c.stale, c.skipped, c.weight[0], c.weight[1], c.weight[2],
			c.beside[BESIDE], c.beside[BESIDE_NOT_PLAY], c.beside[BESIDE_NO_HELPER], c.beside[BESIDE_NO_COPIES], c.beside[BESIDE_NO_PAGE]);
		// each engine block against the mean of our blocks either side of it, per tick
		unsigned slower = 0, pairs = 0;
		for (size_t b = 1; b + 1 < s_blocks.size(); b++)
		{
			const Figures& x = s_blocks[b];
			const Figures& before = s_blocks[b - 1];
			const Figures& after = s_blocks[b + 1];
			if (x.phase != PHASE_ENGINE || !x.ticks || !before.ticks || !after.ticks)
				continue;
			pairs++;
			if (x.us / x.ticks > (before.us / before.ticks + after.us / after.ticks) / 2)
				slower++;
		}
		const Figures& e = s_total[PHASE_ENGINE];
		const Figures& o = s_total[PHASE_OURS];
		IDDrawSurface::OutptFmtTxt("[Pathfinder] VERIFY %s: the engine's scheduler %.1f us a tick over %u ticks, %.1f searches a tick "
			"(%.1f found), a unit's wait from its gate opening to its search %.1f ticks (most %u); ours %.1f us a tick on the game "
			"thread over %u ticks and %.1f us a tick of searches on the threads that search, %.1f searches a tick (%.1f found), wait "
			"%.1f ticks (most %u); the engine's slower than our blocks either side in %u of %u",
			when, e.ticks ? e.us / e.ticks : 0.0, e.ticks, e.ticks ? double(e.searches) / e.ticks : 0.0,
			e.ticks ? double(e.found) / e.ticks : 0.0, e.waited ? e.wait_sum / e.waited : 0.0, e.wait_max,
			o.ticks ? o.us / o.ticks : 0.0, o.ticks, o.ticks ? o.helper_us / o.ticks : 0.0,
			o.ticks ? double(o.searches) / o.ticks : 0.0, o.ticks ? double(o.found) / o.ticks : 0.0,
			o.waited ? o.wait_sum / o.waited : 0.0, o.wait_max, slower, pairs);
	}
#endif

	// ---- install ----------------------------------------------------------------------------------

	uint32_t Fnv(const BYTE* p, DWORD n)
	{
		uint32_t h = 0x811C9DC5u;
		while (n--)
			h = (h ^ *p++) * 0x01000193u;
		return h;
	}

	// The first mirrored range that is not the stock one, null when all are.
	const char* NotStock()
	{
		for (const Range& r : kMirrored)
			if (Fnv(reinterpret_cast<const BYTE*>(r.at), r.len) != r.fnv)
				return r.what;
		return nullptr;
	}

	bool CallsTo(DWORD site, DWORD target)
	{
		const BYTE* p = reinterpret_cast<const BYTE*>(site);
		return p[0] == 0xE8 && site + 5 + *reinterpret_cast<const LONG*>(p + 1) == target;
	}

	// SingleHook reports nothing: the bytes it meant to write, read back where they went.
	bool Placed(const SingleHook* hook, DWORD len)
	{
		return std::memcmp(hook->m_AddrToHook, hook->m_NewBytes_Pbyte, len) == 0;
	}

	// The call at `site` sent to `to`: its rel32 rewritten.
	bool Redirect(std::unique_ptr<SingleHook>& hook, BYTE* rel, DWORD site, const void* to)
	{
		*reinterpret_cast<LONG*>(rel) = (LONG)(reinterpret_cast<DWORD>(to) - (site + 5));
		hook.reset(new SingleHook(site + 1, 4, INLINE_UNPROTECTEVINMENT, rel));
		FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<LPCVOID>(site), 5);
		return Placed(hook.get(), 4);
	}

	// An observer at `site`'s entry, whose first `len` bytes are whole instructions.
	bool Observe(std::unique_ptr<InlineSingleHook>& hook, DWORD site, DWORD len, InlineX86HookRouter router)
	{
		hook.reset(new InlineSingleHook(site, len, INLINE_5BYTESLAGGERJMP, router));
		FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<LPCVOID>(site), len);
		return Placed(hook.get(), 5);
	}

	// The machine's processor cores, counted once each whatever hardware threads they carry; 0 when the
	// system does not say.
	unsigned CoresCount()
	{
		SYSTEM_LOGICAL_PROCESSOR_INFORMATION info[1024];
		DWORD bytes = sizeof info;
		unsigned cores = 0;
		if (!GetLogicalProcessorInformation(info, &bytes))
			return 0;
		for (unsigned i = 0; i < bytes / sizeof info[0]; i++)
			if (info[i].Relationship == RelationProcessorCore)
				cores++;
		return cores;
	}

	// THE NUMBER OF THREADS that search: the machine's cores less one, left to the render thread, and at
	// most kThreads; the scheduler's own thread is one of them, so the helpers are one fewer. Cores, not
	// hardware threads: a search that shares a core waits on its sibling. The number decides how long the
	// searches take and nothing of what they find. A helper that cannot be made is one thread fewer.
	unsigned HelpersStart()
	{
		const unsigned cores = CoresCount();
		unsigned want = cores > 1 ? cores - 1 : 1;
		if (want > kThreads)
			want = kThreads;
		if (--want == 0)
			return cores;
		s_done = CreateEventA(nullptr, FALSE, FALSE, nullptr);
		if (!s_done)
			return cores;
		for (unsigned k = 0; k < want; k++)
		{
			// helper k waits on s_wake[k] and searches on s_pf[1 + k]; numbered as they are made, with no gap
			s_wake[s_helpers] = CreateEventA(nullptr, FALSE, FALSE, nullptr);
			if (!s_wake[s_helpers])
				continue;
			HANDLE t = CreateThread(nullptr, 0, HelperMain, reinterpret_cast<LPVOID>((size_t)s_helpers), 0, nullptr);
			if (!t)
			{
				CloseHandle(s_wake[s_helpers]);
				s_wake[s_helpers] = nullptr;
				continue;
			}
			CloseHandle(t);
			s_helpers++;
		}
		return cores;
	}

	void RemoveAll()
	{
		s_sched_call.reset();
		s_join_call.reset();
		s_init_call.reset();
		s_end_call.reset();
		s_set_goal_hook.reset();
#if TDRAW_PATHFIND_VERIFY
		s_watermark_hook.reset();
		s_reset_hook.reset();
		s_extract_hook.reset();
		s_cleanup_hook.reset();
#endif
	}
}

namespace Pathfinder
{
	void Install()
	{
		if (s_sched_call)
			return;
		const char* changed = NotStock();
		if (!changed && (!CallsTo(VA_SCHED_CALL, VA_SCHEDULER) || !CallsTo(VA_JOIN_CALL, VA_STEP_COUNT) || !CallsTo(VA_INIT_CALL, VA_INIT) ||
			!CallsTo(VA_END_CALL, VA_END_FIRST)))
			changed = "a call it redirects";
#if TDRAW_PATHFIND_VERIFY
		static const BYTE kCleanup[5] = { 0x8B, 0x44, 0x24, 0x04, 0x56 };   // 0x440BE0: mov eax, [esp+4]; push esi
		if (!changed && std::memcmp(reinterpret_cast<const void*>(VA_CLEANUP), kCleanup, sizeof kCleanup))
			changed = "the class's cleanup";
#endif
		if (changed)
		{
			IDDrawSurface::OutptFmtTxt("[Pathfinder] %s is not the stock code; not installed, the engine's scheduler runs", changed);
			return;
		}
		s_m = new (std::nothrow) Machine();
		if (!s_m)
		{
			IDDrawSurface::OutptFmtTxt("[Pathfinder] no memory for the scheduler's records; not installed");
			return;
		}
		for (unsigned i = 0; i < kJobs; i++)
			s_m->free[i] = &s_m->job[i];
		s_m->nfree = kJobs;
		const unsigned cores = HelpersStart();
		// the observers and the join act only while our scheduler serves a tick, and the init's and the
		// teardown's redirects only reset ours, so the scheduler's redirect goes in last
		bool placed = Observe(s_set_goal_hook, VA_SET_GOAL, 5, BeforeSetGoal);      // mov eax, [0x511DE8]
#if TDRAW_PATHFIND_VERIFY
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);
		s_us_per_count = 1e6 / static_cast<double>(frequency.QuadPart);
		placed = placed && Observe(s_watermark_hook, VA_WATERMARK, 5, BeforeWatermark) &&     // mov eax, [0x511DE8]
			Observe(s_reset_hook, VA_GRID_RESET, 6, BeforeReset) && Observe(s_extract_hook, VA_EXTRACT, 6, BeforeExtract) &&
			Observe(s_cleanup_hook, VA_CLEANUP, sizeof kCleanup, BeforeCleanup);
#endif
		placed = placed && Redirect(s_init_call, s_init_rel, VA_INIT_CALL, reinterpret_cast<const void*>(&PathfinderInit)) &&
			Redirect(s_end_call, s_end_rel, VA_END_CALL, reinterpret_cast<const void*>(&GameEnds)) &&
			Redirect(s_join_call, s_join_rel, VA_JOIN_CALL, reinterpret_cast<const void*>(&TickJoin)) &&
			Redirect(s_sched_call, s_sched_rel, VA_SCHED_CALL, reinterpret_cast<const void*>(&Dispatch));
		if (!placed)
		{
			RemoveAll();
			IDDrawSurface::OutptFmtTxt("[Pathfinder] a site did not read back; not installed, the engine's scheduler runs");
			return;
		}
		IDDrawSurface::OutptFmtTxt("[Pathfinder] installed%s: our scheduler at the players phase's call 0x464F92, set goal 0x44F2A0 "
			"watched, joined at the step's call 0x4955E5; at most %u searches and %u pops a tick, on %u threads (%u cores)",
#if TDRAW_PATHFIND_VERIFY
			" with VERIFY (blocks of the engine's scheduler and ours in turn)",
#else
			"",
#endif
			kCapSearches, kCapPops, 1 + s_helpers, cores);
	}

	void Shutdown()
	{
		if (!s_sched_call)
			return;
		RemoveAll();
#if TDRAW_PATHFIND_VERIFY
		LogTimes("total");
#endif
	}
}

#endif
