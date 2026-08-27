#include "OffMapAircraft.h"

#include <windows.h>

#include <climits>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <memory>

#include "tamem.h"
#include "iddrawsurface.h"
#include "hook/hook.h"

namespace
{
	// Addresses and byte guards read from Ghidra and checked against a clean
	// D:\games\GOG\Total Annihilation\TotalA.exe. Rationale for each site: OFFMAP_AIRCRAFT.md.

	// (B1) CheckUnitInPlayerLOS body, past the own-unit and cloak early-outs, with all four
	// callee-saved registers pushed -- so the function's own epilogues are legal redirect
	// targets. EBX = pUnit, ESI = pPlayer. Fixes every caller at once, incl. UpdateHotUnits.
	const DWORD ADDR_CheckUnitInPlayerLOS_Body = 0x00465AFDu;
	const DWORD ADDR_CheckLOS_ReturnTrue       = 0x00465AD9u;  // MOV EAX,1 / POP x4 / RET 8
	const DWORD ADDR_CheckLOS_ReturnFalse      = 0x00465AF1u;  // XOR EAX,EAX / POP x4 / RET 8

	// (B2) ProjectileUnitCollisionDetection is wrapped, not hooked: its one call site (in
	// ProjectilesEngine) is redirected to OffMapCollisionDetect below, which calls the
	// original. So everything patched inside it still runs -- audited against
	// prota4.8-patches.txt, the Escalation binary diff and tdraw's own hooks.
	const DWORD ADDR_ProjCollision_Fn       = 0x0049B090u;
	const DWORD ADDR_ProjCollision_CallSite = 0x0049BD88u;
	const DWORD ADDR_Position2Grid          = 0x004815A0u;

	// (B3) AreaOfEffectDamage, just after MOV EBP,ESP, so args are at [EBP+8]/[EBP+0xC].
	// Not the entry (0x0049a120) -- StackTelemetry.cpp claims that, one router per address.
	const DWORD ADDR_AreaOfEffectDamage_Body = 0x0049A12Cu;

	const BYTE BYTES_CheckLOSBody[]     = { 0x8B, 0x83, 0x92, 0x00, 0x00, 0x00 };
	const BYTE BYTES_CheckLOSTrue[]     = { 0xB8, 0x01, 0x00, 0x00, 0x00, 0x5F, 0x5E, 0x5D, 0x5B };
	const BYTE BYTES_CheckLOSFalse[]    = { 0x33, 0xC0, 0x5F, 0x5E, 0x5D, 0x5B };
	// CALL rel32 0x0049bd88 -> 0x0049b090. Guarding the call site also catches somebody else
	// having already redirected it.
	const BYTE BYTES_ProjCallSite[]     = { 0xE8, 0x03, 0xF3, 0xFF, 0xFF };
	const BYTE BYTES_ProjCollisionFn[]  = { 0x83, 0xEC, 0x18, 0x53, 0x55, 0x56 };
	const BYTE BYTES_AreaOfEffectBody[] = { 0x33, 0xC0, 0x53, 0x89, 0x84, 0x24, 0x94, 0x00, 0x00, 0x00 };

	// Engine functions we call back into.
	typedef void (__stdcall *WeaponsProjectileDamageFn)(ProjectileStruct* proj, UnitStruct* target);
	// NOTE the third argument: Ghidra's signature for MakeDamage_Weapon has only two
	// parameters, so the decompiler renders the falloff multiply as a bare __ftol() and every
	// caller looks like it passes nothing. The real ABI is __stdcall/RET 0xC with the edge
	// ratio as a float (AreaOfEffectDamage pushes it at 0x49a3f2, the direct-hit path pushes
	// the constant 0x3f800000 at 0x49a05b).
	typedef int  (__stdcall *MakeDamageWeaponFn)(ProjectileStruct* proj, UnitStruct* target, float edgeRatio);

	typedef void (__stdcall *CollisionDetectFn)(WeaponStruct* weaponDef, ProjectileStruct* proj);
	typedef int  (__stdcall *Position2GridFn)(const int* posFx);

	const WeaponsProjectileDamageFn WEAPONS_ProjectileDamage =
		reinterpret_cast<WeaponsProjectileDamageFn>(0x00499EB0u);
	const MakeDamageWeaponFn MakeDamage_Weapon =
		reinterpret_cast<MakeDamageWeaponFn>(0x00499CD0u);
	const CollisionDetectFn OriginalCollisionDetect =
		reinterpret_cast<CollisionDetectFn>(ADDR_ProjCollision_Fn);
	const Position2GridFn Position2Grid =
		reinterpret_cast<Position2GridFn>(ADDR_Position2Grid);

	// UnitStateMask bits (UnitStruct::UnitSelected, +0x110).
	const DWORD UNITSTATE_MOVESTATE_MASK = 0x00000003u;
	const DWORD UNITSTATE_MOVESTATE_FLY  = 0x00000002u;
	const DWORD UNITSTATE_PENDING_DEATH  = 0x00004000u;
	const DWORD UNITSTATE_ALIVE          = 0x10000000u;

	// WeaponTypeMask bit 22. WEAPONS_ProjectileDamage sets `detonated` itself at 0x00499f32
	// for every weapon EXCEPT these -- noexplode rounds are pass-through by design.
	const DWORD WEAPONMASK_NOEXPLODE = 0x00400000u;

	const BYTE PROJSTATE_DETONATED = 0x02u;   // WeaponProjectile::nState (+0x69) bit 1

	// Backstop against a corrupt list; the bucket normally holds a handful of aircraft.
	const int OFFMAP_LIST_SCAN_CAP = 4096;

	// Off-map there is no terrain to detonate on and TimeToDeath is only set for
	// weapontimer/twophase weapons, so the margin is all that bounds a round fired outward.
	// Inert at the shipped margins (3-32); here so widening one cannot become a pool leak.
	const int OFFMAP_PROJECTILE_MAX_LIFE_TICKS = 450;   // 15 s at 30 Hz

	// MallocProjectileAry @0x00499a30 allocates exactly 300 slots and TA cannot fire once
	// they are all live, so stop spending them on off-map rounds before it gets that far.
	const int PROJECTILE_POOL_SIZE      = 300;
	const int PROJECTILE_POOL_HIGHWATER = 270;

	// 0 = feature off. See OFFMAP_AIRCRAFT_TARGETABLE_MARGIN_TILES in config_*.h.
	int s_marginTiles = 0;

	std::unique_ptr<InlineSingleHook> s_hookCheckLos;
	std::unique_ptr<InlineSingleHook> s_hookAreaOfEffect;
	// The 5-byte CALL rel32 written over 0x0049bd88. SingleHook keeps the pointer, so the
	// buffer has to outlive it.
	std::unique_ptr<SingleHook> s_hookProjCallSite;
	BYTE s_projCallSitePatch[5] = { 0 };

	TAdynmemStruct* Ta()
	{
		return *reinterpret_cast<TAdynmemStruct**>(0x00511DE8);
	}

	// ------------------------------------------------------------------
	// Raw-offset accessors for fields tamem.h does not name usefully.
	// ------------------------------------------------------------------

	// UnitStruct position, 16.16 fixed point: [0] = x, [1] = y (altitude), [2] = z (map depth).
	// tamem.h names these in TA's screen convention (XPos__/ZPos__/YPos__); Ghidra uses the
	// 3-D convention. Same bytes, transposed names.
	const int* UnitPosFx(const UnitStruct* u)
	{
		return reinterpret_cast<const int*>(reinterpret_cast<const BYTE*>(u) + 0x6A);
	}

	// UNITINFO axis-aligned bounding box, 16.16 fixed point, in the same axis order:
	// [0..2] = min x/y/z (+0x15E/+0x162/+0x166), [3..5] = max x/y/z (+0x16A/+0x16E/+0x172).
	// tamem.h splits this region into misleading __X_Width/X_Width shorts.
	const int* UnitDefBoundsFx(const UnitDefStruct* d)
	{
		return reinterpret_cast<const int*>(reinterpret_cast<const BYTE*>(d) + 0x15E);
	}

	// WeaponProjectile::Position_Curnt, 16.16 fixed point, x/y/z.
	const int* ProjPosFx(const ProjectileStruct* p)
	{
		return reinterpret_cast<const int*>(reinterpret_cast<const BYTE*>(p) + 0x04);
	}

	BYTE* ProjState(ProjectileStruct* p)
	{
		return reinterpret_cast<BYTE*>(p) + 0x69;
	}

	// WeaponProjectile::CreationGameTime (+0x42), in game ticks.
	DWORD ProjCreationGameTime(const ProjectileStruct* p)
	{
		return *reinterpret_cast<const DWORD*>(reinterpret_cast<const BYTE*>(p) + 0x42);
	}

	bool IsAirborne(const UnitStruct* u)
	{
		return (u->UnitSelected & UNITSTATE_MOVESTATE_MASK) == UNITSTATE_MOVESTATE_FLY;
	}

	bool IsAliveAndTargetable(const UnitStruct* u)
	{
		return u != NULL
			&& u->UnitType != NULL
			&& (u->UnitSelected & UNITSTATE_ALIVE) != 0
			&& (u->UnitSelected & UNITSTATE_PENDING_DEATH) == 0;
	}

	// The engine's own off-map test: UNITS_RebuildFootPrint parks the unit in a bucket that is
	// not part of the sort grid, and caches that bucket in the unit.
	bool IsOffMap(const UnitStruct* u)
	{
		const TAdynmemStruct* ta = Ta();
		return ta != NULL
			&& ta->OffMapBucket_p != NULL
			&& u->pSortBucket == ta->OffMapBucket_p;
	}

	// ------------------------------------------------------------------
	// The targetable margin
	// ------------------------------------------------------------------

	// Chebyshev distance in tiles from a tile rect to the map rect; 0 if they touch at all.
	// Square band, so corners behave like edges.
	int TilesOutsideMap(int tileX0, int tileZ0, int tileX1, int tileZ1)
	{
		const TAdynmemStruct* ta = Ta();
		if (!ta) return INT_MAX;

		const int lastX = ta->FeatureMapSizeX - 1;
		const int lastZ = ta->FeatureMapSizeY - 1;

		int dx = 0;
		if (tileX1 < 0)          dx = -tileX1;
		else if (tileX0 > lastX) dx = tileX0 - lastX;

		int dz = 0;
		if (tileZ1 < 0)          dz = -tileZ1;
		else if (tileZ0 > lastZ) dz = tileZ0 - lastZ;

		return dx > dz ? dx : dz;
	}

	// Measured by footprint, the rect UNITS_RebuildFootPrint stamps. Reads 0 for a unit only
	// PARTIALLY off the map -- which the engine's whole-footprint guard already treats as
	// fully off-map, so any positive margin covers that case.
	bool UnitWithinMargin(const UnitStruct* u)
	{
		const int x0 = u->XGridPos;
		const int z0 = u->YGridPos;
		return TilesOutsideMap(x0, z0, x0 + u->SizeFootX - 1, z0 + u->SizeFootZ - 1)
			<= s_marginTiles;
	}

	bool ProjectileWithinMargin(const ProjectileStruct* proj)
	{
		const int* pos = ProjPosFx(proj);
		const int tileX = (pos[0] >> 16) >> 4;
		const int tileZ = (pos[2] >> 16) >> 4;
		return TilesOutsideMap(tileX, tileZ, tileX, tileZ) <= s_marginTiles;
	}

	// ------------------------------------------------------------------
	// (B1) Line of sight, evaluated at the nearest in-map cell.
	// ------------------------------------------------------------------

	// Mirrors CheckUnitInPlayerLOS's index arithmetic (0x00465b6a-0x00465b93) including the
	// altitude shear -- TA keeps visibility in projected space, so the row is sampled alt/2
	// world units north. Clamping instead of failing its unsigned bounds test is what makes
	// an off-map aircraft inherit the nearest border cell's visibility.
	bool ClampedLosVisible(const PlayerStruct* pPlayer, const UnitStruct* u)
	{
		const TAdynmemStruct* ta = Ta();
		if (!ta || !pPlayer) return false;

		const int width  = static_cast<int>(pPlayer->LOS_Tilewidth);
		const int height = static_cast<int>(pPlayer->LOS_Tileheight);
		if (width <= 0 || height <= 0) return false;

		const int* pos = UnitPosFx(u);
		const int worldX = pos[0] >> 16;
		const int worldY = pos[1] >> 16;   // altitude
		const int worldZ = pos[2] >> 16;   // map depth

		int col = worldX >> 5;
		int row = (worldZ - (worldY >> 1)) >> 5;

		// Prefer the sheared row (where the engine looks and the unit is drawn). If it is off
		// the array, the unit's true row beats clamping: in bounds for anything on the map, and
		// it is where the unit actually is.
		if (row < 0 || row >= height)
		{
			const int trueRow = worldZ >> 5;
			if (trueRow >= 0 && trueRow < height) row = trueRow;
		}

		if (col < 0) col = 0; else if (col >= width)  col = width - 1;
		if (row < 0) row = 0; else if (row >= height) row = height - 1;

		const int index = row * width + col;

		if ((ta->LosType & 2) == 2)
		{
			const unsigned char* los = pPlayer->LOS_MEMORY_p;
			return los != NULL && los[index] != 0;
		}

		// Fog-of-war variant. The engine indexes MAPPED_MEMORY with the VIEW player's bit
		// here, not pPlayer's -- mirrored deliberately so the answer matches what the engine
		// would have produced for an in-map unit at the same cell.
		const unsigned short* mapped = ta->MAPPED_MEMORY_p;
		if (!mapped) return false;
		const unsigned int bit = 1u << (static_cast<unsigned>(ta->LOS_Sight_PlayerID) & 0x1Fu);
		return (static_cast<unsigned int>(mapped[index]) & bit) != 0;
	}

	// True when the engine's LOS index falls outside the array, so its unsigned bounds test at
	// 0x00465b89 says "not visible" to everyone. Off-map units, and also on-map aircraft near
	// the top edge -- the altitude shear pushes the row negative (measured: ARMATLAS at alt 275
	// lands at row -2 while 5.5 tiles inside the border).
	bool EngineLosIndexOutOfBounds(const PlayerStruct* pPlayer, const UnitStruct* u)
	{
		const int width  = static_cast<int>(pPlayer->LOS_Tilewidth);
		const int height = static_cast<int>(pPlayer->LOS_Tileheight);
		if (width <= 0 || height <= 0) return false;

		const int* pos = UnitPosFx(u);
		const int col = (pos[0] >> 16) >> 5;
		const int row = ((pos[2] >> 16) - ((pos[1] >> 16) >> 1)) >> 5;
		return col < 0 || col >= width || row < 0 || row >= height;
	}

	// CheckUnitInPlayerLOS body. EBX = pUnit, ESI = pPlayer.
	int __stdcall CheckLosProc(PInlineX86StackBuffer pBuf)
	{
		if (s_marginTiles <= 0) return 0;

		UnitStruct* u = reinterpret_cast<UnitStruct*>(pBuf->Ebx);
		const PlayerStruct* p = reinterpret_cast<const PlayerStruct*>(pBuf->Esi);
		if (!u || !p || !u->UnitType) return 0;
		if (!IsAirborne(u)) return 0;
		// Leave anything the engine can already answer alone.
		if (!IsOffMap(u) && !EngineLosIndexOutOfBounds(p, u)) return 0;
		// Beyond the margin the stock engine answer stands: invisible and unaimable.
		if (!UnitWithinMargin(u)) return 0;

		pBuf->rtnAddr_Pvoid = reinterpret_cast<LPVOID>(
			ClampedLosVisible(p, u) ? ADDR_CheckLOS_ReturnTrue : ADDR_CheckLOS_ReturnFalse);
		return X86STRACKBUFFERCHANGE;
	}

	// ------------------------------------------------------------------
	// (B2) Off-map projectiles
	// ------------------------------------------------------------------

	// The engine's airborne direct-hit rule (0x0049b2c9-0x0049b30d) without the tile array:
	// projectile on one of the unit's footprint tiles, other owner, altitude inside its Y
	// bounds. Not a proximity sphere, so an off-map aircraft is no easier to hit than one on
	// the map. Returns true if the bucket held ANY reachable enemy aircraft, hit or not --
	// that is what decides whether the round is worth keeping alive.
	bool ScanOffMapAircraft(ProjectileStruct* proj, UnitStruct** outVictim)
	{
		*outVictim = NULL;

		const TAdynmemStruct* ta = Ta();
		if (!ta || !ta->OffMapBucket_p) return false;

		const int* ppos = ProjPosFx(proj);
		const int projTileX = (ppos[0] >> 16) >> 4;
		const int projTileZ = (ppos[2] >> 16) >> 4;
		const int projY     = ppos[1];

		bool anyCandidate = false;
		int scanned = 0;
		for (UnitStruct* u = ta->OffMapBucket_p->pUnitListHead;
		     u != NULL && scanned < OFFMAP_LIST_SCAN_CAP;
		     u = u->pNextUnitInSortBucket, ++scanned)
		{
			if (!IsAliveAndTargetable(u) || !IsAirborne(u)) continue;
			// The engine's own owner test at 0x0049b2b6 / 0x0049b2fa: a projectile never
			// collides with a unit belonging to the player who fired it.
			if (u->cOwnerID == static_cast<unsigned char>(proj->myLos_PlayerID)) continue;
			if (!UnitWithinMargin(u)) continue;

			anyCandidate = true;

			if (projTileX < u->XGridPos || projTileX >= u->XGridPos + u->SizeFootX) continue;
			if (projTileZ < u->YGridPos || projTileZ >= u->YGridPos + u->SizeFootZ) continue;

			const int* upos = UnitPosFx(u);
			const int* bnd  = UnitDefBoundsFx(u->UnitType);
			if (projY < upos[1] + bnd[1]) continue;   // below the unit's box
			if (projY > upos[1] + bnd[4]) continue;   // above it

			*outVictim = u;
			return true;
		}
		return anyCandidate;
	}

	// Fire the projectile's warhead on a victim exactly as the engine's own tile-hit paths do
	// (0x0049b213 / 0x0049b275): WEAPONS_ProjectileDamage(proj, victim). For any anti-air
	// weapon -- all of them have areaofeffect > 0x10 -- that lands in AreaOfEffectDamage, and
	// the B3 hook below is what makes the blast reach the off-map bucket.
	void DetonateOn(ProjectileStruct* proj, UnitStruct* victim)
	{
		WEAPONS_ProjectileDamage(proj, victim);
		// Every weapon but "noexplode" gets the detonated bit set inside that call
		// (0x00499f32). Finish the job for the exceptions so nothing leaks.
		if ((proj->Weapon->WeaponTypeMask & WEAPONMASK_NOEXPLODE) != 0)
			*ProjState(proj) |= PROJSTATE_DETONATED;
	}

	// Is this projectile outside the map? Uses the engine's own Position2Grid so the answer
	// is byte-identical to the test the original function makes a few instructions later.
	bool ProjectileIsOffMap(const ProjectileStruct* proj)
	{
		return Position2Grid(ProjPosFx(proj)) == 0;
	}

	// Would keeping this off-map round alive cost more than it is worth? See
	// OFFMAP_PROJECTILE_MAX_LIFE_TICKS and PROJECTILE_POOL_HIGHWATER.
	bool OffMapProjectileMayLive(const ProjectileStruct* proj)
	{
		const TAdynmemStruct* ta = Ta();
		if (!ta) return false;
		if (!ProjectileWithinMargin(proj)) return false;
		if (ta->NumProjectiles >= PROJECTILE_POOL_HIGHWATER) return false;
		return ta->GameTime - static_cast<int>(ProjCreationGameTime(proj))
		       <= OFFMAP_PROJECTILE_MAX_LIFE_TICKS;
	}

	// ------------------------------------------------------------------
	// (B2) The wrapper the call site at 0x0049bd88 is redirected to.
	// ------------------------------------------------------------------
	void __stdcall OffMapCollisionDetect(WeaponStruct* weaponDef, ProjectileStruct* proj)
	{
		if (s_marginTiles <= 0 || !proj || !proj->Weapon)
		{
			OriginalCollisionDetect(weaponDef, proj);
			return;
		}

		if (ProjectileIsOffMap(proj))
		{
			// The engine's first act here is to mark the round detonated and drop it with no
			// damage call -- the bug. Keep it flying, but only while something is out there to
			// fly to, or a stray shot burns a pool slot for nothing.
			UnitStruct* victim = NULL;
			if (OffMapProjectileMayLive(proj) && ScanOffMapAircraft(proj, &victim))
			{
				if (victim) DetonateOn(proj, victim);
				return;                       // never reaches the engine -> never reaped
			}
			OriginalCollisionDetect(weaponDef, proj);   // reaped exactly as before
			return;
		}

		// Over the map: the engine gets first refusal (units, features, terrain -- all things
		// genuinely there). Only if it found nothing do we look for an aircraft bucketed
		// off-map yet standing on on-map tiles, which its tile lookups cannot find.
		OriginalCollisionDetect(weaponDef, proj);
		if ((*ProjState(proj) & PROJSTATE_DETONATED) != 0) return;

		// noexplode weapons never set `detonated` (0x00499ede), so there is no "already
		// handled" signal for them and acting anyway would double-damage every tick. No stock
		// AA weapon is noexplode, and they can still splash an off-map aircraft via B3.
		if ((proj->Weapon->WeaponTypeMask & WEAPONMASK_NOEXPLODE) != 0) return;

		UnitStruct* victim = NULL;
		ScanOffMapAircraft(proj, &victim);
		if (victim) DetonateOn(proj, victim);
	}

	// ------------------------------------------------------------------
	// (B3) Splash damage reaches the off-map bucket
	// ------------------------------------------------------------------

	// Distance in world units from a blast point to a unit's AABB, exactly as
	// AreaOfEffectDamage does it at 0x0049a2aa-0x0049a3a0: per axis
	// d = clamp0(boxMin - p, p - boxMax), squared and rooted. Inside the box gives 0.
	int DistanceToUnitBoxWorld(const int* blastPosFx, const UnitStruct* u)
	{
		const int* upos = UnitPosFx(u);
		const int* bnd  = UnitDefBoundsFx(u->UnitType);

		double sum = 0.0;
		for (int axis = 0; axis < 3; ++axis)
		{
			const int p    = blastPosFx[axis];
			const int lo   = upos[axis] + bnd[axis];
			const int hi   = upos[axis] + bnd[axis + 3];
			double d = 0.0;
			if (p < lo)      d = static_cast<double>(lo) - static_cast<double>(p);
			else if (p > hi) d = static_cast<double>(p)  - static_cast<double>(hi);
			sum += d * d;
		}

		// The engine keeps this in 16.16 and then takes the high word (MOVSX word [ESP+0x1a]).
		const double distWorld = std::sqrt(sum) / 65536.0;
		if (distWorld <= 0.0)    return 0;
		if (distWorld > 32767.0) return 32767;
		return static_cast<int>(distWorld);
	}

	// AreaOfEffectDamage body. [EBP+8] = WeaponProjectile*, [EBP+0xC] = explosion point.
	// The engine's tile scan is clamped to the map, so its victim set and the off-map bucket
	// are disjoint -- no dedup needed. Adding a discovery source cannot make damage wrong:
	// out-of-radius units are still rejected by the distance test below.
	int __stdcall AreaOfEffectProc(PInlineX86StackBuffer pBuf)
	{
		if (s_marginTiles <= 0) return 0;

		const TAdynmemStruct* ta = Ta();
		if (!ta || !ta->OffMapBucket_p || !ta->OffMapBucket_p->pUnitListHead) return 0;

		const DWORD* args = reinterpret_cast<const DWORD*>(pBuf->Ebp);
		ProjectileStruct* proj = reinterpret_cast<ProjectileStruct*>(args[2]);   // [EBP+8]
		const int* blastPos    = reinterpret_cast<const int*>(args[3]);          // [EBP+0xC]
		if (!proj || !proj->Weapon || !blastPos) return 0;

		const WeaponStruct* w = proj->Weapon;
		const int aoeRadius = static_cast<unsigned short>(w->AOE) >> 1;
		if (aoeRadius <= 0) return 0;

		const UnitStruct* attacker = proj->AttackerUnitPtr;

		int scanned = 0;
		for (UnitStruct* u = ta->OffMapBucket_p->pUnitListHead;
		     u != NULL && scanned < OFFMAP_LIST_SCAN_CAP;
		     u = u->pNextUnitInSortBucket, ++scanned)
		{
			if (u == attacker) continue;                 // mirrors the check at 0x0049a259
			if (!IsAliveAndTargetable(u)) continue;
			// Beyond the margin the stock engine result stands: no splash out there.
			if (!UnitWithinMargin(u)) continue;

			const int dist = DistanceToUnitBoxWorld(blastPos, u);
			if (dist >= aoeRadius) continue;

			// falloff = dist==0 ? 1.0 : (1-edgeEff)*(dist/radius - 1)^2 + edgeEff
			// edgeeffectiveness defaults to 0.0 (LoadWeaponTdf @0x0042e440).
			float ratio = 1.0f;
			if (dist != 0)
			{
				const float edgeEff = w->EdgeEffectivnes;
				const float t = static_cast<float>(dist) / static_cast<float>(aoeRadius) - 1.0f;
				ratio = (1.0f - edgeEff) * t * t + edgeEff;
			}

			// No friendly-fire exemption, same as the engine: owner only decides which
			// running total the damage lands in, and those totals feed
			// SetProjectileStateUnit's cosmetic "hit mostly friends" flag.
			MakeDamage_Weapon(proj, u, ratio);
		}
		return 0;
	}

	// ------------------------------------------------------------------
	// Installation
	// ------------------------------------------------------------------

	bool HasExpectedBytes(DWORD address, const BYTE* expected, size_t length)
	{
		if (std::memcmp(reinterpret_cast<const void*>(address), expected, length) == 0)
			return true;
		IDDrawSurface::OutptFmtTxt(
			"[OffMapAircraft] disabled: unexpected TotalA.exe bytes at 0x%08X", address);
		return false;
	}

	bool ValidateHookSites()
	{
		return HasExpectedBytes(ADDR_CheckUnitInPlayerLOS_Body, BYTES_CheckLOSBody,     sizeof(BYTES_CheckLOSBody))
			&& HasExpectedBytes(ADDR_CheckLOS_ReturnTrue,       BYTES_CheckLOSTrue,     sizeof(BYTES_CheckLOSTrue))
			&& HasExpectedBytes(ADDR_CheckLOS_ReturnFalse,      BYTES_CheckLOSFalse,    sizeof(BYTES_CheckLOSFalse))
			&& HasExpectedBytes(ADDR_ProjCollision_CallSite,    BYTES_ProjCallSite,     sizeof(BYTES_ProjCallSite))
			&& HasExpectedBytes(ADDR_ProjCollision_Fn,          BYTES_ProjCollisionFn,  sizeof(BYTES_ProjCollisionFn))
			&& HasExpectedBytes(ADDR_AreaOfEffectDamage_Body,   BYTES_AreaOfEffectBody, sizeof(BYTES_AreaOfEffectBody));
	}
}

void OffMapAircraft::Install(int marginTiles)
{
	static bool installed = false;
	if (installed) return;
	installed = true;

	// No runtime override, by design: the margin decides who can shoot what, so a per-machine
	// env var or ini key would be a mixed-fleet vector. Rebuild with the macro at 0 to
	// disable -- which is also how to A/B a crash against a stock engine.
	if (marginTiles <= 0)
	{
		// Distinctive literal so the DLL can be grepped to confirm the feature shipped,
		// even when it is switched off.
		IDDrawSurface::OutptTxt("[OffMapAircraft] present but disabled (targetable margin 0 tiles)");
		return;
	}

	if (!ValidateHookSites()) return;

	s_marginTiles = marginTiles;

	s_hookCheckLos.reset(new InlineSingleHook(
		ADDR_CheckUnitInPlayerLOS_Body, 6,  INLINE_5BYTESLAGGERJMP, CheckLosProc));
	// Redirect ProjectilesEngine's one and only call to ProjectileUnitCollisionDetection at
	// our wrapper. No trampoline, no displaced bytes: just a new rel32.
	s_projCallSitePatch[0] = 0xE8;
	{
		const DWORD target = reinterpret_cast<DWORD>(&OffMapCollisionDetect);
		const DWORD relative = target - (ADDR_ProjCollision_CallSite + 5u);
		std::memcpy(&s_projCallSitePatch[1], &relative, sizeof(relative));
	}
	s_hookProjCallSite.reset(new SingleHook(
		ADDR_ProjCollision_CallSite, sizeof(s_projCallSitePatch),
		INLINE_UNPROTECTEVINMENT, s_projCallSitePatch));
	s_hookAreaOfEffect.reset(new InlineSingleHook(
		ADDR_AreaOfEffectDamage_Body,   10, INLINE_5BYTESLAGGERJMP, AreaOfEffectProc));

	// Spot-check the call-site redirect actually took. A byte guard proves what was there
	// BEFORE; this proves what is there now, which is the part that decides whether any of
	// the projectile work runs at all.
	{
		const BYTE* cs = reinterpret_cast<const BYTE*>(ADDR_ProjCollision_CallSite);
		DWORD rel = 0;
		std::memcpy(&rel, cs + 1, sizeof(rel));
		const DWORD landsOn = ADDR_ProjCollision_CallSite + 5u + rel;
		if (cs[0] != 0xE8 || landsOn != reinterpret_cast<DWORD>(&OffMapCollisionDetect))
		{
			IDDrawSurface::OutptFmtTxt(
				"[OffMapAircraft] WARNING: call-site redirect did not take -- 0x%08X now lands "
				"on 0x%08X, wanted 0x%08X. Direct hits on off-map aircraft will not work.",
				ADDR_ProjCollision_CallSite, landsOn,
				reinterpret_cast<DWORD>(&OffMapCollisionDetect));
		}
	}

	IDDrawSurface::OutptFmtTxt(
		"[OffMapAircraft] installed -- off-map aircraft are visible, targetable and damageable "
		"within %d tile%s of the map edge", s_marginTiles, s_marginTiles == 1 ? "" : "s");
}
