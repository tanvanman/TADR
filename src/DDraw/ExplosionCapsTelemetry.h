#pragma once

// Empirical telemetry for TotalA.exe's hard-coded explosion-related capacity caps.
// Logs new high-water marks, saturation transitions, and a periodic summary to
// tdrawlog.txt via IDDrawSurface::OutptFmtTxt. Resets peaks when a new game starts.
//
// Caps observed:
//   * TAdynmemStruct.NumExplosions vs Explosions[300] cap at offset 0x1491F
//   * g_ModelInstanceSlots[100] at 0x00511DF0..0x00511F80 — used by ExplodeEffect
//     @0x00421620 for piece-flying effects; when full the call returns silently.
//
// Install once during DLL init after LimitCrack/TABugFixing have been constructed.
namespace ExplosionCapsTelemetry
{
    void Install();
}
