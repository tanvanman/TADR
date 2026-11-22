#pragma once

// Preserve all player-controlled unit settings during voluntary ownership transfer.
// Both peers need this patch to preserve settings over the network.
namespace ShareUnitSettings
{
    void Install();
    void Shutdown();

    // Called by the existing GiveUnit entry/return envelope, after the share
    // delay guard has accepted the call and before any rotation redirection.
    // A stack keeps nested transfers from overwriting the caller classification.
    void __cdecl BeginGive(unsigned originalReturnAddress);
    unsigned __cdecl EndGive();
}
