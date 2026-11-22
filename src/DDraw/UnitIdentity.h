#pragma once

// UnitIdentity -- diagnosis and repair of unit-identity divergence between clients, and the crash
// it causes in ReceiveWeaponFired (game 190441, INT_DIVIDE_BY_ZERO at 0x0049CE6A inside
// UNITS_FireProjectile_Ballistic).
//
// UnitWeapons[n].p_Weapon is a pure function of UnitID -- sole writer UNITS_StartWeaponsScripts
// @0x0049E070, reached only from the two create paths -- so a wrong p_Weapon means this client's
// copy of the unit is a different TYPE than the owner's. ReceiveWeaponFired @0x0049D270 then
// divides by zero: it picks the projectile branch from the packet's weapon id at 0x0049D42A while
// the callees divide by the weaponvelocity of the LOCAL unit's UnitWeapons[pkt[0x23]].p_Weapon,
// and an unarmed slot points at the all-zero WeaponsTypedefArray[0]. TA's own local firing path
// @0x0049D742 reads the mask from p_Weapon, the object it then uses, and is immune.
//
// Two parts: WEAPONFIRE_DISPATCH_FROM_SLOT steers that branch from p_Weapon instead (config.h),
// and observe-only breadcrumbs plus an audit make the divergence visible without a crash (see the
// TRACE_CAT_* notes in TABugFix.h).
//
// No per-tick cost: the hooks fire only on network unit creation, the engine's own ghost
// detection, and remote weapon fire; the audit walk runs every kAuditPeriodTicks.

namespace UnitIdentity
{
	void Install();
	void Shutdown();

	// Live counters, exposed for the crash report / debug pipe.
	struct Stats
	{
		unsigned morphs;          // CreateFromNetwork onto an occupied slot, type CHANGED
		unsigned recreates;       // same, but same type in and out -- benign, not divergence
		unsigned ghosts;          // owner says slot empty, we have a live unit
		unsigned weaponMismatch;  // packet weapon != slot weapon in ReceiveWeaponFired
		unsigned countDrift;      // audit found nNumUnits != walked live count
		unsigned peerMismatch;    // audit digest disagreed with the block's owner
		unsigned twoCBad;         // 0x2C dirty-loop entry with impossible fields
	};
	const Stats& GetStats();
}
