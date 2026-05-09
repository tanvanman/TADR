#pragma once
#include <cstdint>

#pragma pack(1)
static const char*     TAFGAMESTATE_SHMEM_NAME = "TAFGameState";
static const uint32_t  TAFGAMESTATE_MAGIC       = 0x54414604u;

// Per-slot arrays are indexed by TA's local Players[0..9] order (each peer puts
// itself at slot 0). Resolve players by playerDirectPlayId[slot], not lobby slot.

struct TAFGameState {
    uint32_t magic;                    // TAFGAMESTATE_MAGIC when data is valid
    uint32_t sequenceNumber;           // seqlock: odd=writing, even=stable, 0=never written
    uint8_t  playerAllyFlags[10][10];  // playerAllyFlags[i][j] != 0 => slot i is allied with slot j
    uint8_t  playerAllyTeam[10];       // PlayerStruct.AllyTeam (0-4 = explicit team, 5 = none)
    uint8_t  playerActive[10];         // 1 = slot occupied, 0 = empty (slot occupancy only)
    int16_t  playerUnitsNumber[10];    // engine's live unit count per slot. Consumers derive
                                       // elimination via a max-seen-then-zero edge latch.
    uint16_t playerPropertyMask[10];   // PlayerInfoStruct.PropertyMask (WATCH=0x40, HUMANPLAYER=0x80, PLAYERCHEATING=0x2000)
    uint32_t playerDirectPlayId[10];   // PlayerStruct.DirectPlayID
};
#pragma pack()
