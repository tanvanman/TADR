#pragma once

#include "tamem.h"

inline bool ResourceBarsGameState(int state)
{
    return state == gameingstate::MULTI || state == gameingstate::SKIRMISH;
}

inline bool ResourceBarsAI(const PlayerStruct& player)
{
    return player.My_PlayerType == Player_LocalAI || player.My_PlayerType == Player_RemoteAI;
}

inline bool ResourceBarsAlliedAI(const TAdynmemStruct& ta, int original, int slot)
{
    if (original < 0 || original >= 10 || slot < 0 || slot >= 10 || original == slot)
        return false;
    const PlayerStruct& player = ta.Players[slot];
    return ResourceBarsAI(player) && player.Name[0] && player.PlayerInfo &&
        !(player.PlayerInfo->PropertyMask & WATCH) &&
        ta.Players[original].AllyFlagAry[slot] && player.AllyFlagAry[original];
}
