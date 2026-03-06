#pragma once
#include <cstdint>

#pragma pack(1)
static const char*     TAFGAMESTATE_SHMEM_NAME = "TAFGameState";
static const uint32_t  TAFGAMESTATE_MAGIC       = 0x54414601u;

struct TAFGameState {
    uint32_t magic;                    // TAFGAMESTATE_MAGIC when data is valid
    uint32_t sequenceNumber;           // seqlock: odd=writing, even=stable, 0=never written
    uint8_t  playerAllyFlags[10][10];  // playerAllyFlags[i][j] != 0 => slot i is allied with slot j
                                       // mirrors PlayerStruct.AllyFlagAry; irrelevant if playerActive[i]==0
    uint8_t  playerAllyTeam[10];       // PlayerStruct.AllyTeam (0-4 = explicit team, 5 = none)
    uint8_t  playerActive[10];         // 1 = slot occupied and player alive, 0 = empty/eliminated
    uint32_t playerDirectPlayId[10];   // PlayerStruct.DirectPlayID
    int32_t  playerWinLoseTime[10];    // PlayerStruct.WinLoseTime (0 = still playing)
    int16_t  playerUnitsNumber[10];    // PlayerStruct.UnitsNumber
    uint8_t  playerRaceSide[10];       // PlayerInfoStruct.RaceSide (faction index, 0=ARM 1=CORE ...)
    uint16_t playerPropertyMask[10];   // PlayerInfoStruct.PropertyMask (WATCH=0x40, HUMANPLAYER=0x80, PLAYERCHEATING=0x2000)
    uint8_t  playerInfoType[10];       // PlayerInfoStruct.PlayerType (0=none,1=LocalHuman,2=LocalAI,3=RemoteHuman,4=RemoteAI)
    uint8_t  playerMyType[10];         // PlayerStruct.My_PlayerType (same enum as playerInfoType)
};
#pragma pack()
