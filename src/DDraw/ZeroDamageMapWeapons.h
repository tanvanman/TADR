#pragma once

struct ProjectileStruct;

namespace ZeroDamageMapWeapons
{
	void Install();
	void Shutdown();
	bool ShouldHideProjectileMarker(const ProjectileStruct* projectile);
}
