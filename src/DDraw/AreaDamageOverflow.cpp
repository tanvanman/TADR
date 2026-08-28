#include "AreaDamageOverflow.h"

#include <windows.h>

#include <cstddef>
#include <cstring>

#include "config.h"
#include "iddrawsurface.h"
#include "tamem.h"
#include "GameTickHook.h"

namespace
{
	// ---------------------------------------------------------------- addresses ---
	// All `[bin]` VERIFIED against TotalA.exe, Escalation GOLD 10.2.0, 1,178,624 bytes,
	// md5 1e677a7f92c79b5ab35440853d822c17. ImageBase 0x400000, no ASLR.
	// (Version read from the PE resource, not assumed: FileVersion 10.2.0.0. Earlier
	// revisions said "10.1 GOLD"; 10.1 and 10.2 are code-identical apart from 8 bytes --
	// version strings, one minor-version immediate, the PE checksum -- so every address
	// here is valid for both and only the label was wrong. ENGINE_NOTES.md §27.)

	const DWORD kTaPtrAddr            = 0x00511DE8u;

	// Every struct offset this module needs is already named AND static_assert-guarded in
	// tamem.h (see the "Layout guards" block there). Duplicating them as raw constants
	// here would swap a compile-time guarantee for a comment -- and it is exactly what hid
	// the off-map double-damage bug: UnitStruct::pSortBucket sits four fields from
	// SizeFootZ, so reading the footprint through `unit + 0x7E` put the one field that
	// answers "is this unit even on the map" out of sight. Typed access, not offsets.
	//
	// The one field tamem.h does not name is the transport back-pointer at UnitStruct+0x86
	// (`mov eax,[esi+0x86]` @0x004867BA, the same field TransportedExplosions uses). It
	// falls inside `data15`, so pin it to that rather than to a bare literal.
	static_assert(offsetof(UnitStruct, data15) == 0x86,
		"UnitStruct+0x86 BeingTransportedByUnit_p (unnamed in tamem.h, lives in data15)");

	inline UnitStruct* CarrierOf(const UnitStruct* u)
	{
		return *reinterpret_cast<UnitStruct* const*>(&u->data15[0]);
	}

	// Patch sites.
	const DWORD kSlotDispatchAddr     = 0x0049A214u;  // 29 bytes, .. 0x0049A230
	const DWORD kResolveIndexAddr     = 0x0049A231u;  // vanilla index -> UnitStruct*
	const DWORD kTestVictimAddr       = 0x0049A24Eu;  // test esi,esi / je skip-cell
	const DWORD kLoopBoundAddr        = 0x0049A41Au;  // 83 F8 01  ->  cmp eax, N-1
	const DWORD kLoopBoundImmAddr     = 0x0049A41Cu;  // the immediate byte itself
	const DWORD kAreaDamageFn         = 0x0049A120u;  // Weapon_ApplyAreaDamageAndBroadcast
	// Both call sites, byte-pattern and xref verified: AreaOfEffectDamage has exactly two
	// callers, Weapon_DetonateOrImpact+0x1F9 and Weapon_ApplyBurnWeaponAtPos+0x49.
	//
	// ORDERING DEPENDENCY -- ZeroDamageMapWeapons (ddraw.cpp:248) installs an
	// InlineSingleHook at 0x0049A0A7 length 7, a region that CONTAINS kAreaDamageCallA.
	// It works today only because ddraw.cpp installs this module first (line 228), so that
	// hook copies an already-patched `call AreaDamageWrapper` into its trampoline and
	// InlineHook.cpp's X86RedirectOpcodeToNewBase relocates the rel32. If those two
	// Install() calls are ever reordered, this module's signature check at kAreaDamageCallA
	// sees ZeroDamageMapWeapons' E9 displacement and disables itself -- safe, but silent.
	// Do not reorder them without reading both modules.
	const DWORD kAreaDamageCallA      = 0x0049A0A9u;
	const DWORD kAreaDamageCallB      = 0x0049A109u;

	// ------------------------------------------------------------ original bytes ---
	// Byte-signature validation. If any of these do not match, the binary is not the
	// build this module was written against and we disable rather than corrupt it.

	const BYTE kSlotDispatchBytes[] = {
		0x85, 0xC0,                    // test eax, eax
		0x74, 0x0D,                    // je   0x0049A225
		0x66, 0x8B, 0x47, 0x02,        // mov  ax, [edi+2]      ; slot B
		0x66, 0x85, 0xC0,              // test ax, ax
		0x75, 0x10,                    // jne  0x0049A231
		0x33, 0xF6,                    // xor  esi, esi
		0xEB, 0x29,                    // jmp  0x0049A24E
		0x66, 0x8B, 0x07,              // mov  ax, [edi]        ; slot A
		0x66, 0x85, 0xC0,              // test ax, ax
		0x75, 0x04,                    // jne  0x0049A231
		0x33, 0xF6,                    // xor  esi, esi
		0xEB, 0x1D                     // jmp  0x0049A24E
	};
	const BYTE kLoopBoundBytes[]  = { 0x83, 0xF8, 0x01 };            // cmp eax, 1
	const BYTE kCallABytes[]      = { 0xE8, 0x72, 0x00, 0x00, 0x00 };
	const BYTE kCallBBytes[]      = { 0xE8, 0x12, 0x00, 0x00, 0x00 };

	static_assert(sizeof(kSlotDispatchBytes) == 29, "slot dispatch region is 29 bytes");
	static_assert(offsetof(UnitStruct, UnitInGameIndex) == 0xA8, "UnitStruct.UnitInGameIndex");
	static_assert(offsetof(UnitStruct, UnitSelected)    == 0x110, "UnitStruct.UnitStateMask");

	// ------------------------------------------------------------------- tuning ---

	// Airborne occupants tracked per cell in the DLL-side index.
	// Total per-cell loop iterations N = 2 + kOverflowSlots.
	//
	// CAPACITY, precisely -- this is NOT "6 on top of vanilla's 2". IndexUnit inserts
	// EVERY airborne unit on a cell unconditionally, including the one that already holds
	// vanilla's native air slot; the per-explosion dedup collapses that repeat downstream,
	// not at insert time. So k aircraft on one cell consume k of the kOverflowSlots
	// entries, and the reachable total is 6 aircraft per cell -- occasionally 7, when the
	// unit that lost the index race happens to be the one holding vanilla's slot B and so
	// arrives through selector 1 anyway. Aircraft beyond that are still unreachable: a
	// known residual, counted at runtime by g_saturationEvents below so the question stays
	// a measurement rather than a guess. Raising the ceiling costs 2 bytes/cell.
	//
	// Memory: the index costs (4 + 2*kOverflowSlots) bytes per map cell. At 6 slots that
	// is 16 bytes/cell -- 4.8 MB on a 418x712 map, 16 MB on 1024x1024.
	const int kOverflowSlots = 6;

	// Vanilla's two cell slots plus the overflow slots. Patched into the loop bound at
	// 0x0049A41A; selectors 0 and 1 still return vanilla's own `cell+0` / `cell+2` reads
	// byte-for-byte, so the common path is unchanged and only selectors >= 2 are new.
	const int kLoopIterations = 2 + kOverflowSlots;
	static_assert(kLoopIterations >= 2 && kLoopIterations <= 127,
		"loop bound is a signed byte immediate in `cmp eax, imm8`");

	// (UnitStateMask & 3) == 2 means airborne. Two shipped modules already rely on
	// this exact test (NotToAir.cpp:57, SurfaceFire.cpp:183) and T1 confirmed it live:
	// a CORMUAT reads 1 while grounded and flips to 2 the moment it lifts off.
	const DWORD kStateMaskLayerBits = 3u;
	const DWORD kStateMaskAirborne  = 2u;

	// ------------------------------------------------------------------- state ---

	struct OverflowCell
	{
		unsigned int   stamp;                    // == g_stamp if valid this tick
		unsigned short slot[kOverflowSlots];
	};

	bool           g_installed        = false;
	bool           g_active           = false;   // patches applied and index usable
	bool           g_savedValid       = false;   // g_saved* hold real original bytes

	OverflowCell*  g_index            = NULL;
	int            g_indexCellCount   = 0;
	int            g_indexWidth       = 0;
	int            g_indexHeight      = 0;
	const FeatureStruct* g_indexCellBase = NULL; // engine cell base the index was built for
	unsigned int   g_stamp            = 0;       // bumped per rebuild; 0 is never valid

	// Per-explosion victim dedup, keyed by UnitInGameIndex. A generation counter
	// rather than a bitmap so it is re-entrancy safe (see AreaDamageWrapper) and needs
	// no clearing between explosions.
	unsigned int*  g_hitGen           = NULL;
	int            g_hitGenCount      = 0;
	unsigned int   g_genCounter       = 0;
	unsigned int   g_currentGen       = 0;

	// Diagnostics.
	unsigned int   g_saturationEvents = 0;   // a unit could not be indexed: cell full
	unsigned int   g_lastReportedSat  = 0;
	int            g_lastGameTime     = -1;
	int            g_airUnitsLastTick = 0;

	// The game tick the index was last rebuilt for. NOT the same as g_lastGameTime,
	// which only exists to spot a new game (gameTime going backwards). See OnGameTick.
	int            g_indexBuiltForTick = -1;

	BYTE           g_savedSlotDispatch[sizeof(kSlotDispatchBytes)];
	BYTE           g_savedLoopBound[sizeof(kLoopBoundBytes)];
	BYTE           g_savedCallA[sizeof(kCallABytes)];
	BYTE           g_savedCallB[sizeof(kCallBBytes)];

	// ------------------------------------------------------------------ helpers ---

	inline TAdynmemStruct* Ta()
	{
		return *reinterpret_cast<TAdynmemStruct**>(kTaPtrAddr);
	}

	bool WriteCode(DWORD address, const void* data, size_t length)
	{
		DWORD oldProtect = 0;
		if (!VirtualProtect(reinterpret_cast<void*>(address), length,
			PAGE_EXECUTE_READWRITE, &oldProtect))
		{
			IDDrawSurface::OutptFmtTxt(
				"[AreaDamageOverflow] VirtualProtect failed at 0x%08X", address);
			return false;
		}
		std::memcpy(reinterpret_cast<void*>(address), data, length);
		VirtualProtect(reinterpret_cast<void*>(address), length, oldProtect, &oldProtect);
		FlushInstructionCache(GetCurrentProcess(),
			reinterpret_cast<void*>(address), length);
		return true;
	}

	bool HasExpectedBytes(DWORD address, const BYTE* expected, size_t length)
	{
		if (std::memcmp(reinterpret_cast<const void*>(address), expected, length) == 0)
			return true;

		IDDrawSurface::OutptFmtTxt(
			"[AreaDamageOverflow] disabled: unexpected TotalA.exe bytes at 0x%08X",
			address);
		return false;
	}

	// ------------------------------------------------------- the occupant provider ---
	//
	// Replaces vanilla's two-slot dispatch. Called once per (cell, slot selector).
	//
	//   slot 0        -> cell+0x00, vanilla slot A, unchanged
	//   slot 1        -> cell+0x02, vanilla slot B, unchanged
	//   slot 2..N-1   -> the (slot-2)'th airborne unit indexed on this cell, else 0
	//
	// Returning 0 makes vanilla take its existing "empty, skip this slot" path, so no
	// new control flow is introduced. Every returned unit is marked against the current
	// explosion generation, so no unit is ever returned twice for one blast.

	unsigned int __stdcall GetOccupantIndex(int slot, const FeatureStruct* cell)
	{
		// Vanilla already NULL-checks the cell at 0x0049A206, but a new consumer of
		// HeightMap_GetCell's NULL return is the single most reachable crash this
		// module could ship, so check independently.
		if (!cell)
			return 0;

		unsigned int index = 0;

		if (slot == 0)
		{
			index = cell->occupyingUnitNumber;      // vanilla slot A, cell+0x00
		}
		else if (slot == 1)
		{
			index = cell->airborneUnitNumber;       // vanilla slot B, cell+0x02
		}
		else
		{
			// Cheapest, most selective test first. The widened loop bound means slots
			// 2..N-1 run on EVERY tile of EVERY explosion in the game; with nothing
			// airborne the index is never written, so without this the module would pay
			// a guaranteed cold miss into a multi-megabyte array precisely when it has
			// nothing to say. One compare instead.
			if (g_airUnitsLastTick == 0)
				return 0;

			const int k = slot - 2;
			if (!g_index || k < 0 || k >= kOverflowSlots)
				return 0;

			// The index is addressed by cell ordinal, so it is only meaningful if the
			// engine's cell array is still the one we indexed against. A map change
			// between rebuild and use would otherwise read the wrong cell entirely.
			const TAdynmemStruct* ta = Ta();
			if (!ta || ta->FeatureMap != g_indexCellBase)
				return 0;

			const ptrdiff_t cellOrdinal = cell - g_indexCellBase;
			if (cellOrdinal < 0 || cellOrdinal >= g_indexCellCount)
				return 0;

			const OverflowCell& oc = g_index[cellOrdinal];
			if (oc.stamp != g_stamp)      // not rebuilt this tick -- degrade to vanilla
				return 0;

			index = oc.slot[k];
		}

		if (index == 0)
			return 0;

		// Per-explosion dedup. Bounds-checked against the LIVE unit array size, which
		// was measured at 10000 slots -- not the 4096 an earlier note assumed. An
		// unchecked index here would be an out-of-bounds write on every large game.
		if (!g_hitGen || static_cast<int>(index) >= g_hitGenCount)
			return 0;

		// Generation 0 means "no explosion bracket is active", which can only happen if
		// AreaDamageWrapper was bypassed while the provider and the widened loop bound
		// stayed installed. g_hitGen is zero-filled, so dedupping against 0 would suppress
		// EVERY victim on first sight -- splash silently vanishing instead of degrading to
		// vanilla. Fail open: hand the victim back undedupped.
		if (g_currentGen == 0)
			return index;

		// `>=`, not `==`. A nested explosion (an outer blast killing a unit whose death
		// explosion detonates before the outer call returns) runs at a HIGHER generation,
		// so a victim it marked would compare unequal to the outer generation and take the
		// outer blast a second time. Not reachable on this binary -- the damage path
		// (Weapon_ApplyScaledDamageToTarget -> Unit_ApplyDamageAndBroadcast ->
		// Unit_ApplyNetDamagePacket) has no route back to AreaOfEffectDamage, so death is
		// deferred to the unit tick -- but `>=` closes it for free without depending on
		// that staying true.
		if (g_hitGen[index] >= g_currentGen)
		{
#if AREA_DAMAGE_OVERFLOW_FIX_DEDUP_CAP
			return 0;
#else
			// Restricted mode: only suppress duplicates arriving through the overflow
			// slots, leaving vanilla's slot A/B behaviour -- including its 20-victim
			// cap bug -- bit-for-bit intact.
			if (slot >= 2)
				return 0;
#endif
		}
		else
		{
			g_hitGen[index] = g_currentGen;
		}

		return index;
	}

	// ------------------------------------------------------------- the tick rebuild ---

	void FreeIndex()
	{
		delete[] g_index;
		g_index = NULL;
		g_indexCellCount = 0;
		g_indexWidth = 0;
		g_indexHeight = 0;
		g_indexCellBase = NULL;
		g_stamp = 0;
	}

	bool EnsureIndex(int width, int height, const FeatureStruct* cellBase)
	{
		if (g_index && g_indexWidth == width && g_indexHeight == height
			&& g_indexCellBase == cellBase)
			return true;

		FreeIndex();

		const int cellCount = width * height;
		if (width <= 0 || height <= 0 || cellCount <= 0)
			return false;

		g_index = new (std::nothrow) OverflowCell[cellCount];
		if (!g_index)
		{
			IDDrawSurface::OutptFmtTxt(
				"[AreaDamageOverflow] index allocation FAILED for %dx%d (%u KB) "
				"-- overflow slots disabled, vanilla behaviour retained",
				width, height,
				static_cast<unsigned>((cellCount * sizeof(OverflowCell)) / 1024));
			return false;
		}

		std::memset(g_index, 0, cellCount * sizeof(OverflowCell));
		g_indexCellCount = cellCount;
		g_indexWidth = width;
		g_indexHeight = height;
		g_indexCellBase = cellBase;
		g_stamp = 0;   // stamps restart; 0 is reserved as "never valid"

		IDDrawSurface::OutptFmtTxt(
			"[AreaDamageOverflow] index allocated for %dx%d map: %d cells, %u KB, "
			"%d overflow slots/cell",
			width, height, cellCount,
			static_cast<unsigned>((cellCount * sizeof(OverflowCell)) / 1024),
			kOverflowSlots);
		return true;
	}

	bool EnsureHitGen(const TAdynmemStruct* ta)
	{
		const UnitStruct* begin = ta->BeginUnitsArray_p;
		const UnitStruct* end   = ta->EndOfUnitsArray_p;
		if (!begin || !end || end <= begin)
			return false;

		const int count = static_cast<int>(end - begin);
		if (count <= 0)
			return false;

		if (g_hitGen && count <= g_hitGenCount)
			return true;

		delete[] g_hitGen;
		g_hitGen = new (std::nothrow) unsigned int[count];
		if (!g_hitGen)
		{
			g_hitGenCount = 0;
			IDDrawSurface::OutptTxt(
				"[AreaDamageOverflow] dedup table allocation FAILED -- module inert");
			return false;
		}
		std::memset(g_hitGen, 0, count * sizeof(unsigned int));
		g_hitGenCount = count;

		IDDrawSurface::OutptFmtTxt(
			"[AreaDamageOverflow] dedup table sized for %d unit slots", count);
		return true;
	}

	// Insert one airborne unit into every cell of its claimed footprint rect.
	//
	// The rect is XGridPos/YGridPos (anchor) plus SizeFootX/SizeFootZ -- the exact fields
	// the engine's own claim loops read, so the index covers precisely the cells vanilla
	// itself claims, rather than a rect re-derived from UnitDef footprint or world
	// position which could disagree at the margins.
	//
	// Unlike vanilla's claim loop, this CLAMPS to the map. Vanilla walks fw*fh cells
	// with raw pointer arithmetic and no bounds check; clamping can only ever index
	// fewer cells than vanilla claims, never more, so the victim set stays a superset
	// of today's without ever writing outside the index.
	void IndexUnit(const UnitStruct* unit, unsigned short unitIndex, int width, int height)
	{
		const int anchorX = unit->XGridPos;
		const int anchorZ = unit->YGridPos;
		const int footW   = unit->SizeFootX;
		const int footH   = unit->SizeFootZ;

		if (footW <= 0 || footH <= 0)
			return;

		int x0 = anchorX, z0 = anchorZ;
		int x1 = anchorX + footW, z1 = anchorZ + footH;
		if (x0 < 0) x0 = 0;
		if (z0 < 0) z0 = 0;
		if (x1 > width)  x1 = width;
		if (z1 > height) z1 = height;

		for (int z = z0; z < z1; ++z)
		{
			OverflowCell* row = g_index + static_cast<ptrdiff_t>(z) * width;
			for (int x = x0; x < x1; ++x)
			{
				OverflowCell& oc = row[x];
				if (oc.stamp != g_stamp)
				{
					oc.stamp = g_stamp;
					for (int i = 0; i < kOverflowSlots; ++i)
						oc.slot[i] = 0;
				}

				int i = 0;
				for (; i < kOverflowSlots; ++i)
				{
					if (oc.slot[i] == 0)
					{
						oc.slot[i] = unitIndex;
						break;
					}
					if (oc.slot[i] == unitIndex)
						break;      // already present, nothing to do
				}
				if (i == kOverflowSlots)
				{
					++g_saturationEvents;
				}
			}
		}
	}

	void OnGameTick(int gameTime)
	{
		if (!g_active)
			return;

		TAdynmemStruct* ta = Ta();
		if (!ta)
			return;

		// New game: reset diagnostics and force a fresh index.
		if (gameTime < g_lastGameTime)
		{
			g_saturationEvents = 0;
			g_lastReportedSat = 0;
			g_indexBuiltForTick = -1;
			FreeIndex();
		}
		g_lastGameTime = gameTime;

		// GameTickHook does NOT fire once per game tick. It is an InlineSingleHook at
		// 0x004969CB, inside Game_MainLoopTick -- the main loop, which iterates faster
		// than the simulation advances. Measured live 2026-08-27 at roughly 9x-15x per
		// tick, and the multiplier varies with frame rate.
		//
		// Evidence, from the A3 session log: 7 airborne CORMUATs (FootprintX/Z = 5, read
		// from unitse\cormuat.fbi, the only copy across all mounted archives) share a
		// 5x5 = 25 cell block. With 6 overflow slots exactly ONE unit can fail to insert
		// per cell, so the ceiling is 25 saturation events per rebuild. The log recorded
		// a steady 225/tick early and ~374/tick later -- 9x and 15x the ceiling.
		//
		// Without this guard the entire index is rebuilt on every one of those calls:
		// the whole 10000-slot unit array walked and every airborne footprint re-stamped,
		// 9-15 times per tick instead of once. That is pure waste, it multiplies the
		// module's measured per-tick cost by the same factor, and it inflates
		// g_saturationEvents by that factor so the diagnostic cannot be read as an
		// absolute count.
		//
		// Skipping is safe, not merely cheap: `[bin]` VERIFIED that GameTime is written
		// only by Game_RunSimulationStep @0x00495490 (the write is at 0x004954C0), and
		// that Game_RunSimulationStep is the ONLY route from Game_MainLoopTick to
		// UnitMotion_Tick -> UnitMotion_ApplyDeltaAndRelink, which is where XGridPos /
		// YGridPos change. So if gameTime has not advanced the simulation has not
		// stepped, no unit has moved, and a rebuild would reproduce the identical index
		// byte for byte. (Caveat on the provenance of that claim: the reachability walk
		// covered direct E8 call edges to depth 7; indirect vtable dispatch was not
		// enumerated, and this engine does use it -- SpatialQueryCb_ClaimFootprint is
		// reached through vftable 0x004FD660. That particular callback is itself invoked
		// from inside the simulation step, but the general limitation stands.)
		if (gameTime == g_indexBuiltForTick && g_index != NULL)
			return;

		const int width  = ta->FeatureMapSizeX;
		const int height = ta->FeatureMapSizeY;
		const FeatureStruct* cellBase = ta->FeatureMap;
		if (width <= 0 || height <= 0 || !cellBase)
			return;                       // not in a game yet

		if (!EnsureHitGen(ta))
			return;
		if (!EnsureIndex(width, height, cellBase))
		{
			// No index means no overflow slots. Zeroing this makes the provider's first
			// test reject every slot >= 2 call immediately, so the widened loop bound
			// costs a compare per (tile, slot) instead of walking into the null checks.
			g_airUnitsLastTick = 0;
			return;
		}

		// Bump the stamp. Every cell whose stamp does not match is treated as empty,
		// which is what makes "rebuild from scratch" free -- no 5 MB memset per tick.
		// Skip 0, which EnsureIndex uses to mean "never written".
		if (++g_stamp == 0)
			++g_stamp;

		UnitStruct* begin = ta->BeginUnitsArray_p;
		UnitStruct* end   = ta->EndOfUnitsArray_p;
		if (!begin || !end || end <= begin)
			return;

		int airUnits = 0;
		for (UnitStruct* unit = begin; unit < end; ++unit)
		{
			// Skip empty pool slots. The unit array is a fixed pool (measured: 10000
			// slots) and the overwhelming majority are unused at any moment, marked by
			// a null UnitType.
			if (!unit->UnitType)
				continue;

			if ((unit->UnitSelected & kStateMaskLayerBits) != kStateMaskAirborne)
				continue;

			// Cargo is not on the map. Without this, every Roach riding inside a
			// transport would start taking splash it has never taken, which would be a
			// large and entirely unintended nerf to transports.
			if (CarrierOf(unit))
				continue;

			// Off-map units are not on the map either, and this one is load-bearing.
			// Unit_LinkToSpatialGrid @0x0047CC30 refuses to stamp a unit whose footprint
			// is not wholly inside the map -- `X < 0 || Z < 0 || X + SizeFootX >= TilesWidth
			// || Z + SizeFootZ >= TilesHeight` (0x0047CC57..0x0047CCA3) -- and parks it in
			// OffMapBucket_p instead. Note the `>=`: that catches the ENTIRE last tile row
			// and column, not just genuinely off-map units.
			//
			// OffMapAircraft.cpp already damages everything in that bucket from its own
			// AreaOfEffectDamage hook, and its comment says its victim set and the engine's
			// are disjoint "because the engine's tile scan is clamped to the map". That was
			// true while the only victim source was the engine's own stamps. It is NOT true
			// of this index, which is built from unit coordinates and CLAMPS rather than
			// skips -- so without this line every aircraft on the map's last row or column
			// would take splash twice, once from each module. Escalation runs both
			// (OFFMAP_AIRCRAFT_TARGETABLE_MARGIN_TILES 32, AREA_DAMAGE_OVERFLOW_ENABLE 1).
			if (ta->OffMapBucket_p && unit->pSortBucket == ta->OffMapBucket_p)
				continue;

			const unsigned short unitIndex =
				static_cast<unsigned short>(unit->UnitInGameIndex);
			if (unitIndex == 0 || static_cast<int>(unitIndex) >= g_hitGenCount)
				continue;   // 0 is the grid's "empty" sentinel and cannot be indexed

			IndexUnit(unit, unitIndex, width, height);
			++airUnits;
		}
		g_airUnitsLastTick = airUnits;

		g_indexBuiltForTick = gameTime;   // see the multi-fire guard above

		// Report overflow saturation the first time it happens and then only when it
		// grows by an order of magnitude, so the log records that the residual limit
		// was reached without becoming a per-tick spam source.
		if (g_saturationEvents > 0 &&
			(g_lastReportedSat == 0 || g_saturationEvents >= g_lastReportedSat * 10))
		{
			IDDrawSurface::OutptFmtTxt(
				"[AreaDamageOverflow] NOTE %u cell-insert saturations (>%d airborne "
				"units on one cell); units beyond that remain unreachable. air=%d t=%d",
				g_saturationEvents, kOverflowSlots, airUnits, gameTime);
			g_lastReportedSat = g_saturationEvents;
		}
	}

	// ------------------------------------------------------------------- wrapper ---
	//
	// Wraps Weapon_ApplyAreaDamageAndBroadcast to bracket each explosion with its own
	// dedup generation. Save/restore rather than a plain increment because a blast can
	// kill a unit whose death explosion may run before the outer call returns; with a
	// bare counter the outer explosion would lose its dedup state and start hitting
	// units twice. Saving and restoring makes the nesting correct either way, without
	// needing to prove whether that recursion actually happens.

	typedef void (__stdcall* AreaDamageFn)(void*, void*);

	void __stdcall AreaDamageWrapper(void* a, void* b)
	{
		const unsigned int saved = g_currentGen;
		g_currentGen = ++g_genCounter;
		if (g_currentGen == 0)                 // wrapped; 0 collides with a fresh table
			g_currentGen = ++g_genCounter;

		reinterpret_cast<AreaDamageFn>(kAreaDamageFn)(a, b);

		g_currentGen = saved;
	}

	// --------------------------------------------------------------- patch builders ---

	bool PatchSlotDispatch()
	{
		BYTE code[sizeof(kSlotDispatchBytes)];
		std::memset(code, 0x90, sizeof(code));      // nop padding

		// Explicit indices rather than i++ with `i` also read on the right-hand side,
		// which would be unsequenced.
		//
		//   0x0049A214  57           push edi          ; arg2 = cell
		//   0x0049A215  50           push eax          ; arg1 = slot selector
		//   0x0049A216  E8 rel32     call GetOccupantIndex   ; __stdcall, cleans 8
		//   0x0049A21B  85 C0        test eax, eax
		//   0x0049A21D  75 12        jne  0x0049A231   ; vanilla index -> UnitStruct*
		//   0x0049A21F  33 F6        xor  esi, esi
		//   0x0049A221  EB 2B        jmp  0x0049A24E   ; test esi -> skip this slot
		//   0x0049A223  90 x14       padding
		//
		// __stdcall preserves ebx/esi/edi/ebp; vanilla recomputes eax/ecx/edx from
		// 0x0049A231 onward, and eax is exactly our return value, so nothing live is
		// clobbered across the call.
		code[0] = 0x57;
		code[1] = 0x50;
		code[2] = 0xE8;
		*reinterpret_cast<DWORD*>(&code[3]) =
			reinterpret_cast<DWORD>(&GetOccupantIndex) - (kSlotDispatchAddr + 7);
		code[7] = 0x85; code[8] = 0xC0;
		code[9] = 0x75;
		code[10] = static_cast<BYTE>(kResolveIndexAddr - (kSlotDispatchAddr + 11));
		code[11] = 0x33; code[12] = 0xF6;
		code[13] = 0xEB;
		code[14] = static_cast<BYTE>(kTestVictimAddr - (kSlotDispatchAddr + 15));

		return WriteCode(kSlotDispatchAddr, code, sizeof(code));
	}

	bool PatchCallSite(DWORD site)
	{
		BYTE code[5];
		code[0] = 0xE8;
		*reinterpret_cast<DWORD*>(&code[1]) =
			reinterpret_cast<DWORD>(&AreaDamageWrapper) - (site + 5);
		return WriteCode(site, code, sizeof(code));
	}
}

namespace AreaDamageOverflow
{
	void Install()
	{
		if (g_installed)
			return;
		g_installed = true;

#if !AREA_DAMAGE_OVERFLOW_ENABLE
		return;
#else
		if (!HasExpectedBytes(kSlotDispatchAddr, kSlotDispatchBytes, sizeof(kSlotDispatchBytes))
			|| !HasExpectedBytes(kLoopBoundAddr, kLoopBoundBytes, sizeof(kLoopBoundBytes))
			|| !HasExpectedBytes(kAreaDamageCallA, kCallABytes, sizeof(kCallABytes))
			|| !HasExpectedBytes(kAreaDamageCallB, kCallBBytes, sizeof(kCallBBytes)))
		{
			return;
		}

		std::memcpy(g_savedSlotDispatch, reinterpret_cast<void*>(kSlotDispatchAddr),
			sizeof(g_savedSlotDispatch));
		std::memcpy(g_savedLoopBound, reinterpret_cast<void*>(kLoopBoundAddr),
			sizeof(g_savedLoopBound));
		std::memcpy(g_savedCallA, reinterpret_cast<void*>(kAreaDamageCallA),
			sizeof(g_savedCallA));
		std::memcpy(g_savedCallB, reinterpret_cast<void*>(kAreaDamageCallB),
			sizeof(g_savedCallB));
		g_savedValid = true;

		// Order matters: the wrapper and the provider must both be live before the loop
		// bound is widened, or the widened loop would read overflow slots through
		// vanilla's dispatch and resolve garbage indices into unit pointers.
		if (!PatchCallSite(kAreaDamageCallA) || !PatchCallSite(kAreaDamageCallB))
		{
			IDDrawSurface::OutptTxt(
				"[AreaDamageOverflow] disabled: could not patch call sites");
			Shutdown();
			return;
		}
		if (!PatchSlotDispatch())
		{
			IDDrawSurface::OutptTxt(
				"[AreaDamageOverflow] disabled: could not patch slot dispatch");
			Shutdown();
			return;
		}

		const BYTE bound = static_cast<BYTE>(kLoopIterations - 1);
		if (!WriteCode(kLoopBoundImmAddr, &bound, 1))
		{
			IDDrawSurface::OutptTxt(
				"[AreaDamageOverflow] disabled: could not patch loop bound");
			Shutdown();
			return;
		}

		g_active = true;
		GameTickHook::GetInstance()->addCallback(OnGameTick);

		IDDrawSurface::OutptFmtTxt(
			"[AreaDamageOverflow] installed: %d slots/cell (2 vanilla + %d overflow), "
			"air-only, dedup-cap-fix=%d",
			kLoopIterations, kOverflowSlots, AREA_DAMAGE_OVERFLOW_FIX_DEDUP_CAP);
#endif
	}

	void Shutdown()
	{
		// Only ever restore bytes we actually captured. Without this guard, a Shutdown
		// after a failed byte-signature check would splatter a zero-filled buffer over
		// live engine code -- turning a safe self-disable into a guaranteed crash.
		if (g_savedValid)
		{
			WriteCode(kSlotDispatchAddr, g_savedSlotDispatch, sizeof(g_savedSlotDispatch));
			WriteCode(kLoopBoundAddr, g_savedLoopBound, sizeof(g_savedLoopBound));

			// kAreaDamageCallA sits INSIDE the 7-byte region ZeroDamageMapWeapons hooks
			// (0x0049A0A7). Today Shutdown() only runs from Install()'s own failure paths,
			// which execute before that hook is installed, so the bytes there are still
			// ours. If that ever stops being true, restoring blind would write 5 bytes
			// into the middle of somebody else's E9 rel32. Restore only what we still own.
			if (std::memcmp(reinterpret_cast<const void*>(kAreaDamageCallA),
					g_savedCallA, sizeof(g_savedCallA)) != 0
				&& *reinterpret_cast<const BYTE*>(kAreaDamageCallA) == 0xE8)
			{
				WriteCode(kAreaDamageCallA, g_savedCallA, sizeof(g_savedCallA));
			}
			else if (*reinterpret_cast<const BYTE*>(kAreaDamageCallA) != 0xE8)
			{
				IDDrawSurface::OutptFmtTxt(
					"[AreaDamageOverflow] NOT restoring 0x%08X: another module owns those "
					"bytes now (first byte 0x%02X, expected 0xE8)",
					kAreaDamageCallA,
					*reinterpret_cast<const BYTE*>(kAreaDamageCallA));
			}

			WriteCode(kAreaDamageCallB, g_savedCallB, sizeof(g_savedCallB));
			g_savedValid = false;
		}
		g_active = false;
		g_airUnitsLastTick = 0;

		FreeIndex();
		delete[] g_hitGen;
		g_hitGen = NULL;
		g_hitGenCount = 0;
	}
}
