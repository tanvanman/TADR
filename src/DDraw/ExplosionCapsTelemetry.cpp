#include "ExplosionCapsTelemetry.h"

#include <windows.h>

#include "tamem.h"
#include "iddrawsurface.h"
#include "GameTickHook.h"

namespace
{
	// g_ModelInstanceSlots: 100 DWORD slots at 0x00511DF0..0x00511F80, initialised by
	// InitExplode @ 0x00420620 and consumed by ExplodeEffect @ 0x00421620 to track
	// piece-flying animations. When all 100 are non-zero, ExplodeEffect drops the
	// effect silently -- the visible symptom is "fewer explosions" in heavy combat.
	const DWORD MODEL_SLOTS_BASE  = 0x00511DF0u;
	const int   MODEL_SLOTS_COUNT = 100;

	// TAdynmemStruct.Explosions[300] at offset 0x1491F (NumExplosions at 0x1491B).
	// When NumExplosions reaches 300, new explosions are not added to the array.
	const int   EXPLOSIONS_CAP = 300;

	// Log a periodic summary every 30s of game time (TA runs at 30 ticks/sec).
	const int   LOG_INTERVAL_TICKS = 30 * 30;

	int  s_peakNumExplosions = 0;
	int  s_peakModelSlots    = 0;
	int  s_explosionsSaturatedTicks = 0;
	int  s_modelSlotsSaturatedTicks = 0;
	bool s_explosionsSaturated = false;
	bool s_modelSlotsSaturated = false;
	int  s_lastLogGameTime  = -1;
	int  s_lastSeenGameTime = -1;
	int  s_tickCount = 0;

	int CountModelSlotsInUse()
	{
		const DWORD* slots = reinterpret_cast<const DWORD*>(MODEL_SLOTS_BASE);
		int n = 0;
		for (int i = 0; i < MODEL_SLOTS_COUNT; ++i)
		{
			if (slots[i] != 0) ++n;
		}
		return n;
	}

	void ResetPeaks(int gameTime)
	{
		s_peakNumExplosions = 0;
		s_peakModelSlots    = 0;
		s_explosionsSaturatedTicks = 0;
		s_modelSlotsSaturatedTicks = 0;
		s_explosionsSaturated = false;
		s_modelSlotsSaturated = false;
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

		const int numExplosions = taPtr->NumExplosions;
		const int slotsInUse    = CountModelSlotsInUse();

		if (numExplosions > s_peakNumExplosions) s_peakNumExplosions = numExplosions;
		if (slotsInUse    > s_peakModelSlots)    s_peakModelSlots    = slotsInUse;
		if (numExplosions >= EXPLOSIONS_CAP)     ++s_explosionsSaturatedTicks;
		if (slotsInUse    >= MODEL_SLOTS_COUNT)  ++s_modelSlotsSaturatedTicks;

		// Saturation transitions -- log once per enter/exit, not every tick while held.
		const bool explNowSat = (numExplosions >= EXPLOSIONS_CAP);
		const bool slotNowSat = (slotsInUse    >= MODEL_SLOTS_COUNT);
		if (explNowSat && !s_explosionsSaturated)
		{
			IDDrawSurface::OutptFmtTxt(
				"[ExplCaps] WARN NumExplosions saturated: %d/%d (t=%d)",
				numExplosions, EXPLOSIONS_CAP, gameTime);
		}
		else if (!explNowSat && s_explosionsSaturated)
		{
			IDDrawSurface::OutptFmtTxt(
				"[ExplCaps] NumExplosions left saturation: %d/%d (t=%d, sat_ticks_total=%d)",
				numExplosions, EXPLOSIONS_CAP, gameTime, s_explosionsSaturatedTicks);
		}
		if (slotNowSat && !s_modelSlotsSaturated)
		{
			IDDrawSurface::OutptFmtTxt(
				"[ExplCaps] WARN ModelInstanceSlots saturated: %d/%d (t=%d) -- ExplodeEffect calls now dropping",
				slotsInUse, MODEL_SLOTS_COUNT, gameTime);
		}
		else if (!slotNowSat && s_modelSlotsSaturated)
		{
			IDDrawSurface::OutptFmtTxt(
				"[ExplCaps] ModelInstanceSlots left saturation: %d/%d (t=%d, sat_ticks_total=%d)",
				slotsInUse, MODEL_SLOTS_COUNT, gameTime, s_modelSlotsSaturatedTicks);
		}
		s_explosionsSaturated = explNowSat;
		s_modelSlotsSaturated = slotNowSat;

		// Periodic summary. Only emits if any activity has been observed (peak > 0) so
		// idle pre-game ticks don't pollute the log.
		if (s_lastLogGameTime < 0 ||
		    gameTime - s_lastLogGameTime >= LOG_INTERVAL_TICKS)
		{
			if (s_peakNumExplosions > 0 || s_peakModelSlots > 0)
			{
				IDDrawSurface::OutptFmtTxt(
					"[ExplCaps] t=%d expl=%d/%d (peak=%d, sat=%d ticks) "
					"slots=%d/%d (peak=%d, sat=%d ticks)",
					gameTime,
					numExplosions, EXPLOSIONS_CAP, s_peakNumExplosions, s_explosionsSaturatedTicks,
					slotsInUse, MODEL_SLOTS_COUNT, s_peakModelSlots, s_modelSlotsSaturatedTicks);
			}
			s_lastLogGameTime = gameTime;
		}
	}
}

void ExplosionCapsTelemetry::Install()
{
	static bool installed = false;
	if (installed) return;
	installed = true;

	IDDrawSurface::OutptTxt(
		"[ExplCaps] telemetry installed (caps: NumExplosions=300, ModelInstanceSlots=100)");
	GameTickHook::GetInstance()->addCallback(&OnGameTick);
}
