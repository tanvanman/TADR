#pragma once

struct ProjectileStruct;
struct UnitStruct;

// Allied projectile pass-through, contributed by TAG_Venom.
// Build-time simulation rule: every peer and replay viewer must use matching builds.
namespace AlliedProjectileCollision
{
	void Install();
	void Shutdown();
	void AbortIfInstallFailed();
	bool IsInstalled();

	// Always preserves native same-owner pass-through. When installed, also
	// checks the firing player's current alliance towards the candidate owner.
	bool ShouldPassThrough(const ProjectileStruct* projectile, const UnitStruct* candidate);
}
