#include "config.h"

#if FAST_SIMULATION_ENABLE

#include "PathSearch.h"

#include <climits>
#include <cstring>
#include <intrin.h>

// Every function is a port of the engine function it names, in its order of operations, DISASSEMBLED
// from the stock TotalA.exe (objdump -d -M intel).

namespace PathSearch
{
namespace
{
	__forceinline int32_t Add(int32_t a, int32_t b) { return (int32_t)((uint32_t)a + (uint32_t)b); }
	__forceinline int32_t Sub(int32_t a, int32_t b) { return (int32_t)((uint32_t)a - (uint32_t)b); }
	__forceinline int32_t Mul(int32_t a, int32_t b) { return (int32_t)((uint32_t)a * (uint32_t)b); }

	// (a * b) >> 16 in 64 bits, its low dword: the engine's __allmul then __allshr.
	__forceinline int32_t MulShr16(int32_t a, int32_t b) { return (int32_t)(uint32_t)(uint64_t)(((int64_t)a * b) >> 16); }

	// The directions, the bytes of 0x4FD670 and 0x4FD678: 0 is -z (heading 0), d is heading d * 0x2000.
	const int8_t kDX[8] = { 0, -1, -1, -1, 0, 1, 1, 1 };
	const int8_t kDZ[8] = { -1, -1, 0, 1, 1, 1, 0, -1 };
	// The turn penalty by the turn k (the bytes of 0x4FCA10) and the step by the direction
	// (0x40E66B..0x40E688), both written into the object at every search start.
	const uint8_t kTurnPen[8] = { 0, 40, 60, 80, 100, 80, 60, 40 };
	const uint8_t kStep[8] = { 16, 22, 16, 22, 16, 22, 16, 22 };

	// ---- the goals --------------------------------------------------------------------------------

	// in_goal (vtable +0x14): the point 0x44D290, dx^2 + dz^2 <= rc2 from the goal cell, imul and add
	// wrapping, the compare signed; the annulus 0x44D7C0, rin2 <= d^2 <= rout2; the ring 0x44DCB0,
	// only the rectangle's border, in the branch order of the engine's code.
	bool InGoal(const Goal& g, int32_t x, int32_t z)
	{
		switch (g.kind)
		{
		case GOAL_POINT:
		case GOAL_ANNULUS:
		{
			const int32_t dx = Sub(x, g.gx), dz = Sub(z, g.gz), d2 = Add(Mul(dx, dx), Mul(dz, dz));
			if (g.kind == GOAL_POINT)
				return d2 <= g.rc2;
			return d2 <= g.rout2 && d2 >= g.rin2;
		}
		case GOAL_RING:
			if (x == g.x0 || x == g.x1)
			{
				if (z < g.z0)
					return z == g.z1 && x >= g.x0 && x <= g.x1;
				if (z <= g.z1)
					return true;
			}
			if (z != g.z0 && z != g.z1)
				return false;
			return x >= g.x0 && x <= g.x1;
		default:
			return false;
		}
	}

	// |v| as cdq; xor; sub computes it: INT_MIN stays INT_MIN.
	__forceinline int32_t Abs(int32_t v) { return v < 0 ? Sub(0, v) : v; }

	// The heuristic (vtable +0x1C), in the engine's mixed units: 18 a straight cell and 7 more a
	// diagonal one against the steps' 16 and 22, less a radius in px. The point 0x44D350: 18 max + 7 min
	// of |x - gx| and |z - gz|, less r, 0 below it. The annulus 0x44D840: the same sum h, then h - rout
	// above rout, rin - h below rin, else 0. The ring 0x44DD00: outside the rectangle on both axes
	// 16 max + 6 min of the axis distances, outside on one 16 times its distance, inside 16 times the
	// distance to the nearest side.
	__forceinline int32_t Heuristic(const Goal& g, int32_t x, int32_t z)
	{
		int32_t a, b, h;
		switch (g.kind)
		{
		case GOAL_POINT:
		case GOAL_ANNULUS:
			a = Abs(Sub(x, g.gx));
			b = Abs(Sub(z, g.gz));
			h = a > b ? Add(Mul(a, 18), Mul(b, 7)) : Add(Mul(a, 7), Mul(b, 18));
			if (g.kind == GOAL_POINT)
				return h < g.r ? 0 : Sub(h, g.r);
			if (h > g.rout)
				return Sub(h, g.rout);
			return h < g.rin ? Sub(g.rin, h) : 0;
		case GOAL_RING:
			if (x < g.x0 || x > g.x1)
			{
				a = x < g.x0 ? Sub(g.x0, x) : Sub(x, g.x1);
				if (z >= g.z0 && z <= g.z1)
					return Mul(a, 16);
				b = z < g.z0 ? Sub(g.z0, z) : Sub(z, g.z1);
				return a > b ? Add(Mul(a, 16), Mul(b, 6)) : Add(Mul(b, 16), Mul(a, 6));
			}
			if (z < g.z0)
				return Mul(Sub(g.z0, z), 16);
			if (z > g.z1)
				return Mul(Sub(z, g.z1), 16);
			a = Sub(x, g.x0);
			if (Sub(g.x1, x) <= a)
				a = Sub(g.x1, x);
			b = Sub(g.z1, z);
			if (Sub(z, g.z0) < b)
				b = Sub(z, g.z0);
			return Mul(a < b ? a : b, 16);
		default:
			return 0;
		}
	}

	// The goal cells (vtable +0x18), i from 0 while it answers true: the point's 0x44D0E0 one,
	// (gx, gz); the annulus's 0x44D560 one, (gx, gz + (rin + rout) / 32), the division truncating
	// and the add a word's; the ring's 0x44DA00, for x from x0 to x1 (x, z0) then (x, z1), then for z
	// from z0 + 1 to z1 - 1 (x0, z) then (x1, z).
	bool GoalCell(const Goal& g, int64_t i, int16_t* x, int16_t* z)
	{
		switch (g.kind)
		{
		case GOAL_POINT:
			if (i)
				return false;
			*x = g.gx;
			*z = g.gz;
			return true;
		case GOAL_ANNULUS:
			if (i)
				return false;
			*x = g.gx;
			*z = (int16_t)(uint16_t)((uint32_t)(uint16_t)g.gz + (uint32_t)(Add(g.rin, g.rout) / 32));
			return true;
		case GOAL_RING:
		{
			const int64_t nx = g.x1 >= g.x0 ? (int64_t)g.x1 - g.x0 + 1 : 0;
			const int64_t nz = (int64_t)g.z1 - g.z0 - 1 > 0 ? (int64_t)g.z1 - g.z0 - 1 : 0;
			if (i < 2 * nx)
			{
				*x = (int16_t)(g.x0 + (int32_t)(i >> 1));
				*z = (int16_t)(i & 1 ? g.z1 : g.z0);
				return true;
			}
			i -= 2 * nx;
			if (i >= 2 * nz)
				return false;
			*x = (int16_t)(i & 1 ? g.x1 : g.x0);
			*z = (int16_t)(g.z0 + 1 + (int32_t)(i >> 1));
			return true;
		}
		default:
			return false;
		}
	}

	// ---- the grid ---------------------------------------------------------------------------------

	int32_t GridCells(int32_t w, int32_t h) { return (int32_t)(((int64_t)w * h + 7) & ~(int64_t)7); }
	int32_t TouchWords(int32_t cells) { return (cells + 0xFF) >> 8; }
	size_t Align4(size_t n) { return (n + 3u) & ~(size_t)3u; }

	__forceinline bool Inside(const Scratch* s, int32_t x, int32_t z)
	{
		return (uint32_t)x < (uint32_t)s->w && (uint32_t)z < (uint32_t)s->h;
	}

	__forceinline int32_t CellIndex(const Scratch* s, int32_t x, int32_t z) { return z * s->w + x; }

	// The touched-block bit of cell c, as 0x40D8B0 and every inline copy set it.
	__forceinline void Touch(Scratch* s, int32_t c) { s->touch[(uint32_t)c >> 8] |= 1u << (((uint32_t)c >> 3) & 31); }

	// 0x40D900: b0 = 0 in all 8 cells of every touched block, and the bits cleared. A block is 8 cells
	// and `cells` a multiple of 8, so a block is inside whole or not at all.
	void ResetGrid(Scratch* s)
	{
		const int32_t n = TouchWords(s->cells);
		for (int32_t i = 0; i < n; i++)
		{
			uint32_t bits = s->touch[i];
			if (!bits)
				continue;
			s->touch[i] = 0;
			for (; bits; bits &= bits - 1)
			{
				unsigned long low;
				_BitScanForward(&low, bits);
				const int32_t c = (i << 8) + ((int32_t)low << 3);
				if (c < s->cells)
					for (int k = 0; k < 8; k++)
						s->grid[c + k].b0 = 0;
			}
		}
	}

	// THE READER'S VIEW, everything the engine's cell read 0x40D7B0 takes for one search: made once a
	// walk or an A* pass, so that a cell's read is its own two loads.
	struct Reader
	{
		const uint32_t* map;
		const uint16_t* explored;
		const Patch* patch;
		uint32_t cw, ch;            // the class's grid
		int32_t mw, mh;             // the feature grid's 32-px blocks
		int32_t ox, oz;             // the footprint's centre, in blocks
		uint32_t bit;
		int32_t weight;             // the heuristic's, 16.16
	};

	Reader ReaderOf(const Query& q)
	{
		Reader rd;
		rd.map = q.map;
		rd.explored = q.explored;
		rd.patch = q.patch;
		rd.cw = (uint32_t)q.w;
		rd.ch = (uint32_t)q.h;
		rd.mw = q.w >> 1;
		rd.mh = q.h >> 1;
		rd.ox = q.fx >> 2;
		rd.oz = q.fz >> 2;
		rd.bit = 1u << (q.seat & 31);
		rd.weight = q.weight;
		return rd;
	}

	// 0x40D7B0(x, z): 0 off the class's grid; 0 when the 32-px block at the footprint's centre,
	// ((x >> 1) + (fx >> 2), (z >> 1) + (fz >> 2)), is outside the feature grid's blocks; 2 when that
	// block's explored bit of the player is clear; else the class map's code, 0 blocked, 1 bad, 3 good.
	__forceinline int Code(const Reader& rd, int32_t x, int32_t z)
	{
		if ((uint32_t)x >= rd.cw || (uint32_t)z >= rd.ch)
			return 0;
		const int32_t bx = (x >> 1) + rd.ox, bz = (z >> 1) + rd.oz;
		if (bx >= rd.mw || bz >= rd.mh)
			return 0;
		if (rd.explored && !(rd.explored[bz * rd.mw + bx] & rd.bit))
			return 2;
		if (rd.patch)
		{
			const Patch& p = *rd.patch;
			const uint32_t px = (uint32_t)(x - p.x0), pz = (uint32_t)(z - p.z0);
			if (px < (uint32_t)p.w && pz < (uint32_t)p.h)
				return p.code[pz * (uint32_t)p.w + px] & 3;
		}
		return MapCode(rd.map, (int32_t)rd.cw, x, z);
	}

	__forceinline int32_t Weighed(const Reader& rd, const Goal& g, int32_t x, int32_t z)
	{
		return MulShr16(Heuristic(g, x, z), rd.weight);
	}

	// ---- set path and the search's end ------------------------------------------------------------

	// Set path 0x44F080 with no corners, on the search's own course: a goal the unit's cell is not in
	// is flagged no-path. The search is dropped (0x40E982..0x40E99B, 0x40E7AA..0x40E7C8,
	// 0x40EEC9..0x40EEE2).
	void Fail(Scratch* s, const Query& q)
	{
		if (!InGoal(q.goal, q.sx, q.sz))
			s->goalflags |= GOALF_NOPATH;
		s->searching = false;
	}

	// ---- the bug walk -----------------------------------------------------------------------------

	// A step of the walk onto (x, z) by direction d: touched, b1 = d, b0 bit 3; true on a goal cell.
	bool WalkOnto(Scratch* s, int32_t x, int32_t z, int d)
	{
		if (!Inside(s, x, z))
			return false;
		const int32_t ci = CellIndex(s, x, z);
		Touch(s, ci);
		Cell* c = &s->grid[ci];
		c->b1 = (uint8_t)d;
		c->b0 |= 8;
		return (c->b0 & 4) != 0;
	}

	// 0x40E3D5..0x40E43F: Q is on the m-line from P when, the axes turned so that the nearest goal
	// cell lies at gx, gz >= 0 from P, Q is on its x leg (0 < ax <= gx, az = 0) or on its z leg
	// (ax = gx, 0 < az <= gz): the L the straight walk follows, x first.
	bool OnMLine(const Scratch* s, int32_t qx, int32_t qz, int32_t px, int32_t pz)
	{
		int32_t gx = Sub(s->near_x, px), gz = Sub(s->near_z, pz), ax = Sub(qx, px), az = Sub(qz, pz);
		if (gx < 0)
		{
			gx = Sub(0, gx);
			ax = Sub(0, ax);
		}
		if (gz < 0)
		{
			gz = Sub(0, gz);
			az = Sub(0, az);
		}
		return (az == 0 && ax > 0 && ax <= gx) || (ax == gx && az > 0 && az <= gz);
	}

	// 0x40E160: a bug walk from the start toward the goal cell nearest it, whose answer is the A*'s
	// relaxation (a cell whose weighed heuristic is at most it counts as a goal cell). It answers 0 when
	// it steps onto a goal cell or the weighed heuristic falls to 0, else the least weighed heuristic
	// of every cell it stood on. A blocked start answers the start's.
	// STRAIGHT: toward the nearest goal cell along x first, then z; a blocked step starts the follow.
	// FOLLOW: two heads leave the blocked cell, A trying the eight directions from da - 2 upward, B
	// mirrored on the other side, a step each in turn. A head with no free
	// direction ends the walk; a head that reaches the m-line beyond P makes it P and the walk goes
	// straight again; the heads' meeting ends the walk. THE ENGINE'S, KEPT: each meeting test compares a
	// head's position before its step with the other's, and the second has no `started` term
	// (0x40E352..0x40E376, 0x40E4F6..0x40E512).
	// THE BOUND, which the engine has not: a follow in which the heads neither meet, stick nor reach the
	// m-line would never end. A head's state, its cell and its direction, takes at most 8 * w * h values,
	// so the walk ends such a follow after that many pairs of steps and answers its least heuristic.
	int32_t BugWalk(Scratch* s, const Query& q, const Reader& rd)
	{
		const Goal& g = q.goal;
		const int64_t bound = 8 * (int64_t)s->w * s->h;
		int32_t px = s->start_x, pz = s->start_z, best = Weighed(rd, g, px, pz);
		int d = 0;
		if (Code(rd, px, pz) < 1)
			return best;
		for (;;)
		{
			for (;;)
			{
				if (best == 0)
					return 0;
				const int32_t gx = Sub(s->near_x, px);
				d = gx < 0 ? 2 : gx > 0 ? 6 : (Sub(s->near_z, pz) <= 0 ? 0 : 4);
				const int32_t nx = px + kDX[d], nz = pz + kDZ[d];
				if (Code(rd, nx, nz) < 1)
					break;
				px = nx;
				pz = nz;
				if (WalkOnto(s, nx, nz, d))
					return 0;
				const int32_t h = Weighed(rd, g, nx, nz);
				if (h < best)
					best = h;
			}
			int32_t ax = px, az = pz, bx = px, bz = pz;
			int da = (d + 2) & 7, db = da;
			bool started = false;
			for (int64_t steps = 0;; steps++)
			{
				if (steps >= bound)
					return best;
				int lim = (da - 3) & 7;
				da = (da - 2) & 7;
				while (Code(rd, ax + kDX[da], az + kDZ[da]) < 1)
				{
					if (da == lim)
						return best;
					da = (da + 1) & 7;
				}
				if (ax == bx && az == bz && da == db && started)
					return best;
				ax += kDX[da];
				az += kDZ[da];
				started = true;
				if (WalkOnto(s, ax, az, da))
					return 0;
				if (OnMLine(s, ax, az, px, pz))
				{
					px = ax;
					pz = az;
					break;
				}
				int32_t h = Weighed(rd, g, ax, az);
				if (h < best)
					best = h;
				lim = (db + 3) & 7;
				db = (db + 2) & 7;
				while (Code(rd, bx - kDX[db], bz - kDZ[db]) < 1)
				{
					if (db == lim)
						return best;
					db = (db - 1) & 7;
				}
				if (bx == ax && bz == az && db == da)
					return best;
				bx -= kDX[db];
				bz -= kDZ[db];
				if (WalkOnto(s, bx, bz, db))
					return 0;
				if (OnMLine(s, bx, bz, px, pz))
				{
					px = bx;
					pz = bz;
					break;
				}
				h = Weighed(rd, g, bx, bz);
				if (h < best)
					best = h;
			}
		}
	}

	// ---- the heap ---------------------------------------------------------------------------------

	// 0x40F000(i): heap[i] moves up while its f is below its parent's, strictly.
	void SiftUp(Scratch* s, int32_t i)
	{
		if (i == 0)
			return;
		const HeapEntry x = s->heap[i];
		int32_t p = (i - 1) >> 1;
		if (x.f >= s->heap[p].f)
			return;
		do
		{
			s->heap[i] = s->heap[p];
			s->pool[s->heap[i].n].link = i;
			i = p;
			if (!i)
				break;
			p = (i - 1) >> 1;
		} while (x.f < s->heap[p].f);
		s->heap[i] = x;
		s->pool[x.n].link = i;
	}

	// 0x40F060(i): with both children present the right one is taken only when its f is below the
	// left's, so ties go left; a child moves up only when its f is below heap[i]'s.
	void SiftDown(Scratch* s, int32_t i)
	{
		const HeapEntry x = s->heap[i];
		for (;;)
		{
			const int32_t l = 2 * i + 1, r = 2 * i + 2;
			int32_t to;
			if (r < s->count)
				to = s->heap[r].f < s->heap[l].f ? r : l;
			else if (l < s->count)
				to = l;
			else
				break;
			const HeapEntry c = s->heap[to];
			if (c.f >= x.f)
				break;
			s->heap[i] = c;
			s->pool[c.n].link = i;
			i = to;
		}
		s->heap[i] = x;
		s->pool[x.n].link = i;
	}

	// The free 0x40F1E0 and its inline copies: LIFO.
	__forceinline void NodeFree(Scratch* s, uint32_t n)
	{
		s->pool[n].link = s->freehead;
		s->freehead = (int32_t)n;
	}

	// A node from the free list, else the pool's next (0x40DE86..0x40DEAB, 0x40E8DE..0x40E902); -1 past
	// the pool, which sets `overflow`.
	__forceinline int32_t NodeTake(Scratch* s)
	{
		const int32_t n = s->freehead;
		if (n != -1)
		{
			if ((uint32_t)n >= (uint32_t)s->used)
				return -1;
			s->freehead = s->pool[n].link;
			return n;
		}
		if (s->used >= s->nodes)
		{
			s->overflow = true;
			return -1;
		}
		return s->used++;
	}

	// The node at heap position `at` taken out and the heap's last moved into its place
	// (0x40EDE2..0x40EE03, 0x40DBEF..0x40DC17). THE ENGINE'S, KEPT: the moved entry is only sifted down,
	// so with `at` past the root it may stand below a parent of larger f, and a later pop is then not
	// the open set's least; the pops are the engine's for that. THE BOUND: `at` is read from the node.
	__forceinline void HeapFill(Scratch* s, int32_t at)
	{
		s->count -= 1;
		if (at < 0 || at >= s->count)
			return;
		s->heap[at] = s->heap[s->count];
		s->pool[s->heap[at].n].link = at;
		SiftDown(s, at);
	}

	// ---- A* ---------------------------------------------------------------------------------------

	// 0x40DA70(L, pcell, k): the neighbour of the popped node L in direction d = (pcell.b1 + k) & 7.
	// A NEW CELL is touched and read: blocked makes it dead unless the bug walk stood on it; else it
	// opens, as a goal cell when its weighed heuristic is at most the walk's relaxation, b1 = d, and
	// its node has g = turnpen[k] + step[d] + L.g + pen, plus 75 when k != 0 and L.straight < 5, pen 30
	// for a bad cell, straight 1 after a turn and L.straight + 1 ahead (a word), f = g + h. With the
	// lazy top standing the node overwrites it in place and sifts down; else it is pushed.
	// AN OPEN CELL is relaxed when the new g, with the cell's own pen, is below its node's: b1 = d, f
	// moved by the same amount, and the node sifted up. With the lazy top standing, a relaxed node that
	// displaces it from the root frees it and fills its place from the heap's last.
	// A CLOSED OR DEAD CELL is left.
	// THE ENGINE'S, KEPT: the node word is 16 bits, so past 65 536 live nodes a cell keeps a wrapped
	// index. THE BOUNDS: that index against the pool's live part, and the relaxed node's heap position
	// against the heap's count.
	__forceinline void Expand(Scratch* s, const Reader& rd, const Goal& g, const Node& L, int32_t pc, int k)
	{
		const int d = (s->grid[pc].b1 + k) & 7;
		const int32_t nx = L.x + kDX[d], nz = L.z + kDZ[d];
		const int32_t add = (k && L.straight < 5) ? 75 : 0;
		const int16_t straight = k ? 1 : (int16_t)(uint16_t)((uint16_t)L.straight + 1u);
		if (!Inside(s, nx, nz))
			return;
		const int32_t ci = CellIndex(s, nx, nz);
		Cell* c = &s->grid[ci];
		if ((c->b0 & 3) == 0)
		{
			Touch(s, ci);
			const int code = Code(rd, nx, nz);
			if (code < 1)
			{
				if (!(c->b0 & 8))
					c->b0 |= 3;
				return;
			}
			const int32_t h = Weighed(rd, g, nx, nz);
			c->b0 |= h <= s->relax ? 5 : 1;
			c->b1 = (uint8_t)d;
			const int16_t pen = code > 1 ? 0 : 30;
			const int32_t gg = Add(Add(Add(Add(kTurnPen[k], kStep[d]), L.g), pen), add);
			int32_t n;
			if (s->lazy)
				n = (int32_t)s->heap[0].n;
			else if ((n = NodeTake(s)) < 0)
				return;
			Node& N = s->pool[n];
			N.x = (int16_t)nx;
			N.z = (int16_t)nz;
			N.g = gg;
			N.f = Add(gg, h);
			N.pen = pen;
			N.straight = straight;
			if (s->lazy)
			{
				s->heap[0].f = N.f;
				SiftDown(s, 0);
				s->lazy = 0;
			}
			else
			{
				N.link = s->count;
				s->heap[s->count].f = N.f;
				s->heap[s->count].n = (uint32_t)n;
				s->count += 1;
				SiftUp(s, s->count - 1);
			}
			c->node = (uint16_t)n;
		}
		else if ((c->b0 & 3) == 1)
		{
			const int32_t n = c->node;
			if (n >= s->used)
				return;
			Node& N = s->pool[n];
			const int32_t gg = Add(Add(Add(Add(kTurnPen[k], kStep[d]), L.g), N.pen), add);
			if (gg >= N.g)
				return;
			c->b1 = (uint8_t)d;
			N.f = Add(N.f, Sub(gg, N.g));
			N.g = gg;
			N.straight = straight;
			const int32_t hi = N.link;
			// The entry's copy of f. A cell whose 16-bit node word names another node may have no entry of
			// this node's to store into.
			if ((uint32_t)hi < (uint32_t)s->count && s->heap[hi].n == (uint32_t)n)
				s->heap[hi].f = N.f;
			// the lazy top as it stood before the sift: the relaxed node may displace it from the root
			const uint32_t top = s->heap[0].n;
			if (hi > 0 && hi < s->count)
				SiftUp(s, hi);
			if (s->lazy)
			{
				const int32_t at = s->pool[top].link;
				if (at == 0)
					return;
				s->lazy = 0;
				NodeFree(s, top);
				HeapFill(s, at);
			}
		}
	}

	// The found search's path, 0x40E050: back from the found cell by each cell's b1 to the start, a
	// corner kept wherever b1 changes, in a ring of 64 -- so the 64 corners nearest the start survive --
	// and the start last; then the corners from the start on, each the cell's footprint centre in px,
	// ((2x + fx) << 3, (2z + fz) << 3) as words, handed to set path, which keeps the first 20.
	// THE BOUND: the walk back stops after `nodes` steps or off the grid, and a b1 is taken & 7.
	void Extract(Scratch* s, const Query& q, Result* r)
	{
		const int kRing = 64;
		int16_t ring[kRing][2];
		int16_t cx = s->found_x, cz = s->found_z;
		int32_t n = 1, steps = 0;
		int d = Inside(s, cx, cz) ? (int8_t)s->grid[CellIndex(s, cx, cz)].b1 : 0;
		ring[0][0] = cx;
		ring[0][1] = cz;
		while (!(cx == s->start_x && cz == s->start_z))
		{
			if (!Inside(s, cx, cz) || ++steps > s->nodes)
				break;
			const int e = (int8_t)s->grid[CellIndex(s, cx, cz)].b1;
			if (e != d)
			{
				ring[n & (kRing - 1)][0] = cx;
				ring[n & (kRing - 1)][1] = cz;
				n++;
				d = e;
			}
			cx = (int16_t)(uint16_t)((uint16_t)cx - (uint16_t)kDX[d & 7]);
			cz = (int16_t)(uint16_t)((uint16_t)cz - (uint16_t)kDZ[d & 7]);
		}
		ring[n & (kRing - 1)][0] = s->start_x;
		ring[n & (kRing - 1)][1] = s->start_z;
		n++;
		const int32_t m = n < kWaypoints ? n : kWaypoints;
		for (int32_t i = 0; i < m; i++)
		{
			const int16_t* c = ring[(n - 1 - i) & (kRing - 1)];
			r->pts[i][0] = (int16_t)(uint16_t)(((uint32_t)(2 * c[0] + q.fx)) << 3);
			r->pts[i][1] = (int16_t)(uint16_t)(((uint32_t)(2 * c[1] + q.fz)) << 3);
		}
		r->n = m;
		s->found = true;
		s->searching = false;
	}

	// The scheduler's in-search branch, 0x40ED90..0x40EEE2: an empty open set fails the search; else
	// pops, one cost each, until the cost reaches 100 or the open set empties (the next pass fails it).
	// A pop frees the lazy top left by the previous one and copies the new top into L, which stays at
	// heap[0] as the lazy top. A goal cell ends the search with its path. Otherwise the cell closes and
	// the directions -fan .. +fan around L's are expanded, then fan is 2. THE ENGINE'S, KEPT: the start
	// node's fan of 4 expands direction 4 twice, as k = -4 and k = 4; the second relaxes nothing.
	void AStarPass(Scratch* s, const Query& q, const Reader& rd, Result* r)
	{
		if (s->count - s->lazy == 0)
		{
			Fail(s, q);
			return;
		}
		for (;;)
		{
			if (s->count - s->lazy == 0)
				return;
			s->cost = Add(s->cost, 1);
			if (s->lazy)
			{
				const uint32_t top = s->heap[0].n;
				const int32_t at = s->pool[top].link;
				s->lazy = 0;
				NodeFree(s, top);
				HeapFill(s, at);
			}
			const Node L = s->pool[s->heap[0].n];
			s->lazy = 1;
			if (!Inside(s, L.x, L.z))
			{
				Fail(s, q);
				return;
			}
			const int32_t pc = CellIndex(s, L.x, L.z);
			if (s->grid[pc].b0 & 4)
			{
				s->found_x = L.x;
				s->found_z = L.z;
				Extract(s, q, r);
				return;
			}
			s->grid[pc].b0 = 2;
			for (int k = -s->fan; k <= s->fan; k++)
				Expand(s, rd, q.goal, L, pc, k & 7);
			s->fan = 2;
			if (s->cost >= 100)
				return;
		}
	}

	// 0x40E630(goal) after its watermark: the grid reset; each goal cell on the grid marked a goal and
	// the one nearest the start kept (d^2 an imul sum that wraps, THE ENGINE'S, KEPT, the first of equals
	// winning). A start in the goal flags WALKIN and fails; a start off the grid flags WALKOUT and fails.
	// Then the bug walk: 0 flags WALKIN, anything else WALKOUT, and a walk that got no nearer than the
	// start's own weighed heuristic fails. Otherwise the A* starts with the start node: g 0, f the
	// start's weighed heuristic, straight 100, b1 the searcher's heading rounded to a direction, fan 4.
	// THE ENGINE'S, KEPT: the start node's pen word is uninitialised stack in the engine and nothing
	// reads it; here it is 0.
	void SearchStart(Scratch* s, const Query& q, const Reader& rd)
	{
		const Goal& g = q.goal;
		int32_t best = INT_MAX;
		int16_t x, z;
		s->start_x = q.sx;
		s->start_z = q.sz;
		ResetGrid(s);
		for (int64_t i = 0; GoalCell(g, i, &x, &z); i++)
		{
			if (Inside(s, x, z))
			{
				const int32_t ci = CellIndex(s, x, z);
				s->grid[ci].b0 = 4;
				Touch(s, ci);
			}
			const int32_t ddz = Sub(s->start_z, z), ddx = Sub(s->start_x, x);
			const int32_t d2 = Add(Mul(ddx, ddx), Mul(ddz, ddz));
			if (d2 < best)
			{
				s->near_x = x;
				s->near_z = z;
				best = d2;
			}
		}
		if (InGoal(g, s->start_x, s->start_z))
		{
			s->goalflags |= GOALF_WALKIN;
			Fail(s, q);
			return;
		}
		const int32_t h0 = Weighed(rd, g, s->start_x, s->start_z);
		if (!Inside(s, s->start_x, s->start_z))
		{
			s->goalflags |= GOALF_WALKOUT;
			Fail(s, q);
			return;
		}
		const int32_t relax = BugWalk(s, q, rd);
		s->relax = relax;
		if (relax == 0)
			s->goalflags |= GOALF_WALKIN;
		else
		{
			s->goalflags |= GOALF_WALKOUT;
			if (relax >= h0)
			{
				Fail(s, q);
				return;
			}
		}
		s->freehead = -1;
		s->used = 0;
		s->lazy = 0;
		const int32_t ci = CellIndex(s, s->start_x, s->start_z);
		Touch(s, ci);
		Cell* c = &s->grid[ci];
		c->b0 |= 1;
		c->b1 = (uint8_t)((((uint32_t)q.heading + 0x1000u) >> 13) & 7);
		const int32_t n = NodeTake(s);
		Node& N = s->pool[n];
		N.link = 0;
		N.x = s->start_x;
		N.z = s->start_z;
		N.g = 0;
		N.f = h0;
		N.pen = 0;
		N.straight = 100;
		s->heap[0].f = h0;
		s->heap[0].n = (uint32_t)n;
		s->count = 1;
		c->node = (uint16_t)n;
		s->fan = 4;
	}
}

size_t Bytes(int32_t w, int32_t h, int32_t nodes)
{
	if (w <= 0 || h <= 0 || (int64_t)w * h > 0x1000000 || nodes < 1 || (int64_t)nodes > (int64_t)w * h)
		return 0;
	const int32_t cells = GridCells(w, h);
	return Align4((size_t)cells * sizeof(Cell)) + 4 * (size_t)TouchWords(cells) + (size_t)nodes * sizeof(Node) +
		(size_t)nodes * sizeof(HeapEntry);
}

bool Init(Scratch* s, void* mem, size_t bytes, int32_t w, int32_t h, int32_t nodes)
{
	const size_t need = Bytes(w, h, nodes);
	if (!need || !mem || bytes < need)
		return false;
	unsigned char* p = static_cast<unsigned char*>(mem);
	std::memset(s, 0, sizeof *s);
	s->w = w;
	s->h = h;
	s->cells = GridCells(w, h);
	// every cell new and no block touched; the pool and the heap are written before they are read
	std::memset(mem, 0, Align4((size_t)s->cells * sizeof(Cell)) + 4 * (size_t)TouchWords(s->cells));
	s->nodes = nodes;
	s->grid = reinterpret_cast<Cell*>(p);
	p += Align4((size_t)s->cells * sizeof(Cell));
	s->touch = reinterpret_cast<uint32_t*>(p);
	p += 4 * (size_t)TouchWords(s->cells);
	s->pool = reinterpret_cast<Node*>(p);
	p += (size_t)nodes * sizeof(Node);
	s->heap = reinterpret_cast<HeapEntry*>(p);
	s->freehead = -1;
	return true;
}

void Search(Scratch* s, const Query& q, Result* r)
{
	std::memset(r, 0, sizeof *r);
	r->used = -1;
	if (!q.map || q.w != s->w || q.h != s->h || q.goal.kind == GOAL_NONE || q.fx < 0 || q.fz < 0)
	{
		r->kind = Refused;
		return;
	}
	const Reader rd = ReaderOf(q);
	s->searching = true;
	s->found = false;
	s->overflow = false;
	s->goalflags = 0;
	s->used = -1;
	s->relax = 0;
	SearchStart(s, q, rd);
	while (s->searching && !s->overflow)
	{
		s->cost = 0;
		AStarPass(s, q, rd, r);
		r->pops = Add(r->pops, s->cost);
	}
	r->used = s->used;
	r->relax = s->relax;
	r->goalflags = s->goalflags;
	if (s->overflow)
	{
		r->kind = Overflow;
		r->n = 0;
		std::memset(r->pts, 0, sizeof r->pts);
	}
	else if (s->found)
	{
		r->kind = Found;
		r->found_x = s->found_x;
		r->found_z = s->found_z;
	}
	else if (r->used >= 0)
		r->kind = NoPath;
	else
		r->kind = !InGoal(q.goal, q.sx, q.sz) && !Inside(s, q.sx, q.sz) ? OffGrid : NoSearch;
}
}

#endif
