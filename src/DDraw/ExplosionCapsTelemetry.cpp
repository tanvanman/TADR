#include "ExplosionCapsTelemetry.h"

#include <windows.h>

#include "tamem.h"
#include "iddrawsurface.h"
#include "GameTickHook.h"
#include "EngineLimits.h"

namespace
{
	// Log a periodic summary every 30s of game time (TA runs at 30 ticks/sec).
	const int   LOG_INTERVAL_TICKS = 30 * 30;

	int  s_peakNumExplosions = 0;
	int  s_peakModelSlots    = 0;
	int  s_peakProjectiles   = 0;
	int  s_peakAuxEffectSlots = 0;
	int  s_explosionsSaturatedTicks = 0;
	int  s_modelSlotsSaturatedTicks = 0;
	int  s_auxSlotsSaturatedTicks = 0;
	bool s_explosionsSaturated = false;
	bool s_modelSlotsSaturated = false;
	bool s_auxSlotsSaturated = false;
	int  s_lastLogGameTime  = -1;
	int  s_lastSeenGameTime = -1;
	int  s_tickCount = 0;

	int CountModelSlotsInUse()
	{
		const unsigned long* slots = EngineLimits::GetModelEffectSlots();
		int n = 0;
		for (int i = 0; i < EngineLimits::MODEL_EFFECT_LIMIT; ++i)
		{
			if (slots[i] != 0) ++n;
		}
		return n;
	}

	int CountAuxEffectSlotsInUse()
	{
		const unsigned char* slots = EngineLimits::GetAuxEffectSlots();
		int n = 0;
		for (int i = 0; i < EngineLimits::AUX_EFFECT_LIMIT; ++i)
		{
			if (slots[i * EngineLimits::AUX_EFFECT_SIZE] != 0xFF) ++n;
		}
		return n;
	}

	void ResetPeaks(int gameTime)
	{
		s_peakNumExplosions = 0;
		s_peakModelSlots    = 0;
		s_peakProjectiles   = 0;
		s_peakAuxEffectSlots = 0;
		s_explosionsSaturatedTicks = 0;
		s_modelSlotsSaturatedTicks = 0;
		s_auxSlotsSaturatedTicks = 0;
		s_explosionsSaturated = false;
		s_modelSlotsSaturated = false;
		s_auxSlotsSaturated = false;
		s_lastLogGameTime = gameTime;
		s_tickCount = 0;
	}

	void OnGameTick(int gameTime)
	{
		TAdynmemStruct* taPtr = *reinterpret_cast<TAdynmemStruct**>(0x00511de8);
		if (!taPtr) return;

		// New game detection: GameTime monotonically increases within a single match
		// and resets to 0 on a new one. Reset peaks so per-game numbers don't conflate.
		if (gameTime < s_lastSeenGameTime)
		{
			IDDrawSurface::OutptTxt("[ExplCaps] new game detected -- resetting peaks");
			ResetPeaks(gameTime);
		}
		s_lastSeenGameTime = gameTime;
		++s_tickCount;

		const int numExplosions = EngineLimits::GetExplosionCount();
		const int slotsInUse    = CountModelSlotsInUse();
		const int projectiles   = taPtr->NumProjectiles;
		const int auxSlotsInUse = CountAuxEffectSlotsInUse();

		if (numExplosions > s_peakNumExplosions) s_peakNumExplosions = numExplosions;
		if (slotsInUse    > s_peakModelSlots)    s_peakModelSlots    = slotsInUse;
		if (projectiles > s_peakProjectiles) s_peakProjectiles = projectiles;
		if (auxSlotsInUse > s_peakAuxEffectSlots) s_peakAuxEffectSlots = auxSlotsInUse;
		if (numExplosions >= EngineLimits::EXPLOSION_LIMIT) ++s_explosionsSaturatedTicks;
		if (slotsInUse >= EngineLimits::MODEL_EFFECT_LIMIT) ++s_modelSlotsSaturatedTicks;
		if (auxSlotsInUse >= EngineLimits::AUX_EFFECT_LIMIT) ++s_auxSlotsSaturatedTicks;

		// Saturation transitions -- log once per enter/exit, not every tick while held.
		const bool explNowSat = (numExplosions >= EngineLimits::EXPLOSION_LIMIT);
		const bool slotNowSat = (slotsInUse >= EngineLimits::MODEL_EFFECT_LIMIT);
		const bool auxNowSat = (auxSlotsInUse >= EngineLimits::AUX_EFFECT_LIMIT);
		if (explNowSat && !s_explosionsSaturated)
		{
			IDDrawSurface::OutptFmtTxt(
				"[ExplCaps] WARN NumExplosions saturated: %d/%d (t=%d)",
				numExplosions, EngineLimits::EXPLOSION_LIMIT, gameTime);
		}
		else if (!explNowSat && s_explosionsSaturated)
		{
			IDDrawSurface::OutptFmtTxt(
				"[ExplCaps] NumExplosions left saturation: %d/%d (t=%d, sat_ticks_total=%d)",
				numExplosions, EngineLimits::EXPLOSION_LIMIT, gameTime, s_explosionsSaturatedTicks);
		}
		if (slotNowSat && !s_modelSlotsSaturated)
		{
			IDDrawSurface::OutptFmtTxt(
				"[ExplCaps] WARN ModelInstanceSlots saturated: %d/%d (t=%d) -- ExplodeEffect calls now dropping",
				slotsInUse, EngineLimits::MODEL_EFFECT_LIMIT, gameTime);
		}
		else if (!slotNowSat && s_modelSlotsSaturated)
		{
			IDDrawSurface::OutptFmtTxt(
				"[ExplCaps] ModelInstanceSlots left saturation: %d/%d (t=%d, sat_ticks_total=%d)",
				slotsInUse, EngineLimits::MODEL_EFFECT_LIMIT, gameTime, s_modelSlotsSaturatedTicks);
		}
		if (auxNowSat && !s_auxSlotsSaturated)
		{
			IDDrawSurface::OutptFmtTxt(
				"[ExplCaps] WARN auxiliary effect slots saturated: %d/%d (t=%d)",
				auxSlotsInUse, EngineLimits::AUX_EFFECT_LIMIT, gameTime);
		}
		else if (!auxNowSat && s_auxSlotsSaturated)
		{
			IDDrawSurface::OutptFmtTxt(
				"[ExplCaps] auxiliary effect slots left saturation: %d/%d (t=%d, sat_ticks_total=%d)",
				auxSlotsInUse, EngineLimits::AUX_EFFECT_LIMIT, gameTime, s_auxSlotsSaturatedTicks);
		}
		s_explosionsSaturated = explNowSat;
		s_modelSlotsSaturated = slotNowSat;
		s_auxSlotsSaturated = auxNowSat;

		// Periodic summary. Only emits if any activity has been observed (peak > 0) so
		// idle pre-game ticks don't pollute the log.
		if (s_lastLogGameTime < 0 ||
		    gameTime - s_lastLogGameTime >= LOG_INTERVAL_TICKS)
		{
			if (s_peakProjectiles > 0 || s_peakNumExplosions > 0 ||
				s_peakModelSlots > 0 || s_peakAuxEffectSlots > 0)
			{
				IDDrawSurface::OutptFmtTxt(
					"[ExplCaps] t=%d proj=%d/%d (peak=%d) expl=%d/%d (peak=%d, sat=%d ticks) "
					"model=%d/%d (peak=%d, sat=%d ticks) aux=%d/%d (peak=%d, sat=%d ticks)",
					gameTime,
					projectiles, EngineLimits::PROJECTILE_LIMIT, s_peakProjectiles,
					numExplosions, EngineLimits::EXPLOSION_LIMIT, s_peakNumExplosions, s_explosionsSaturatedTicks,
					slotsInUse, EngineLimits::MODEL_EFFECT_LIMIT, s_peakModelSlots, s_modelSlotsSaturatedTicks,
					auxSlotsInUse, EngineLimits::AUX_EFFECT_LIMIT, s_peakAuxEffectSlots,
					s_auxSlotsSaturatedTicks);
			}
			s_lastLogGameTime = gameTime;
		}
	}
}

void ExplosionCapsTelemetry::Install()
{
	static bool installed = false;
	if (installed) return;
	if (!EngineLimits::IsInstalled())
	{
		IDDrawSurface::OutptTxt(
			"[ExplCaps] telemetry disabled because engine limits were not installed");
		return;
	}
	installed = true;

	IDDrawSurface::OutptFmtTxt(
		"[ExplCaps] telemetry installed (caps: projectiles=%d, explosions=%d, model-effects=%d, aux-effects=%d)",
		EngineLimits::PROJECTILE_LIMIT, EngineLimits::EXPLOSION_LIMIT,
		EngineLimits::MODEL_EFFECT_LIMIT, EngineLimits::AUX_EFFECT_LIMIT);
	GameTickHook::GetInstance()->addCallback(&OnGameTick);
}
