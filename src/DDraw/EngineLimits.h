#pragma once

struct ExplosionStruct;

namespace EngineLimits
{
	const int PROJECTILE_LIMIT = 3000;
	const int EXPLOSION_LIMIT = 3000;
	const int MODEL_EFFECT_LIMIT = 1000;
	const int AUX_EFFECT_LIMIT = 3000;
	const int AUX_EFFECT_SIZE = 0x34;

	void Install();
	bool IsInstalled();

	int GetExplosionCount();
	ExplosionStruct* GetExplosions();
	const unsigned long* GetModelEffectSlots();
	const unsigned char* GetAuxEffectSlots();
}
