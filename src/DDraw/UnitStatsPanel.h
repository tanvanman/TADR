#pragma once

struct _OFFSCREEN;

// Native bottom-bar presentation, shared by the battlefield and megamap.
namespace UnitStatsPanel
{
    void Install();
    void Shutdown();
    void Draw(_OFFSCREEN* surface);
}
