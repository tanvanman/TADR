#pragma once

// Empirical telemetry for TotalA.exe's expanded projectile and explosion capacities.
// Logs new high-water marks, saturation transitions, and a periodic summary to
// tdrawlog.txt via IDDrawSurface::OutptFmtTxt. Resets peaks when a new game starts.
//
// Also reports the relocated auxiliary debris/effect workspace so stress tests can
// establish whether flying model pieces become a bottleneck.
//
// Install once during DLL init after LimitCrack/TABugFixing have been constructed.
namespace ExplosionCapsTelemetry
{
    void Install();
}
