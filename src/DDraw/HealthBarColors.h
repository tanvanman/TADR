#pragma once

#include "config.h"

#if HEALTH_BAR_COLORS_ENABLE

// Five-colour unit health bar. Tested on Escalation only; the flag is 0 in every other config.
//
// Replaces Unit_DrawHealthBar (0x46A430, __stdcall(OFFSCREEN*, UnitStruct*, int centerX, int y)) with a
// 5-byte jump to a full reimplementation. The HP <= 0 early-out, the rectangles and the
// (HP << 5) / nMaxHP fill width are the engine's; only the fill colour changes, from thirds to 20 % steps
// of HP * 100 / nMaxHP:
//   80..100 slot 10, 60..79 slot 90, 40..59 slot 14, 20..39 slot 59, 0..19 slot 32.
// A slot indexes the colour table at ta + 0xDCB: guipal.pal, each entry remapped to the nearest
// palette.pal entry when a battle loads (Palette_RemapToClosestIndices 0x4AC7D0).
//
// The call site (0x469C4A..0x469CB9: a HUD option bit and an owner test) decides which units get a bar; this function only draws it. Nothing is simulated or sent.
//
// Other code on this function: the recorder (src/Recorder, eplayx.dll) only declares its address
// (TA_FunctionsU.pas) and never calls or patches it; ReloadBars (0x469CB1) and UnitStatusCounters (0x469CF9)
// hook the caller, outside this function.
//
// Install() patches only if the function's first 19 bytes match and reads the splice back; otherwise it
// logs DISABLED and leaves the engine's bar untouched.

namespace HealthBarColors
{
	// Idempotent.
	void Install();

	// Restores the engine's bytes. Safe to call more than once.
	void Shutdown();
}

#endif // HEALTH_BAR_COLORS_ENABLE
