#pragma once

namespace EngineLimits
{
	const int PROJECTILE_LIMIT = 3000;
	const int EXPLOSION_LIMIT = 3000;
	const int MODEL_EFFECT_LIMIT = 1000;
	const int AUX_EFFECT_LIMIT = 3000;

	void Install();
	void Uninstall();
	void AbortIfInstallFailed();
	bool IsInstalled();
}
