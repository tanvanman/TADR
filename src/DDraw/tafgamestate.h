#pragma once
#include <cstdint>

#pragma pack(1)
static const char*     TAFGAMESTATE_SHMEM_NAME = "TAFGameState";
static const uint32_t  TAFGAMESTATE_MAGIC       = 0x54414601u;

// IMPORTANT: per-slot arrays here are indexed by TA's local Players[0..9] order
// (each peer puts itself at slot 0). Do NOT cross-reference these arrays by
// lobby slot — resolve the player via playerDirectPlayId[xslot] instead.

struct TAFGameState {
    uint32_t magic;                    // TAFGAMESTATE_MAGIC when data is valid
    uint32_t sequenceNumber;           // seqlock: odd=writing, even=stable, 0=never written
    uint8_t  playerAllyFlags[10][10];  // playerAllyFlags[i][j] != 0 => slot i is allied with slot j
                                       // mirrors PlayerStruct.AllyFlagAry; irrelevant if playerActive[i]==0
    uint8_t  playerAllyTeam[10];       // PlayerStruct.AllyTeam (0-4 = explicit team, 5 = none)
    uint8_t  playerActive[10];         // 1 = slot occupied, 0 = empty.
                                       // SLOT OCCUPANCY ONLY — does NOT reflect elimination.
    int16_t  playerUnitsNumber[10];    // PlayerStruct.UnitsNumber (PlayerStruct+0x144) — live unit count
                                       // per the engine's local view of each player. Consumers derive
                                       // elimination via a max-seen-then-zero edge latch.
    uint16_t playerPropertyMask[10];   // PlayerInfoStruct.PropertyMask (WATCH=0x40, HUMANPLAYER=0x80, PLAYERCHEATING=0x2000)
    uint32_t playerDirectPlayId[10];   // PlayerStruct.DirectPlayID
};
#pragma pack()
