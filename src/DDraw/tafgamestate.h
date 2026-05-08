#pragma once
#include <cstdint>

#pragma pack(1)
static const char*     TAFGAMESTATE_SHMEM_NAME = "TAFGameState";
static const uint32_t  TAFGAMESTATE_MAGIC       = 0x54414603u;  // ...02 = added KillEvent ring; ...03 = added tiebreakerWinnerDplayId

// IMPORTANT: per-slot arrays here are indexed by TA's local Players[0..9] order
// (each peer puts itself at slot 0). Do NOT cross-reference these arrays by
// lobby slot — resolve the player via playerDirectPlayId[xslot] instead.

// KillEvent flags. Emitted on every commander-death edge, not just dgun-victim.
static const uint16_t TAF_KILL_FLAG_VICTIM_COMMANDER = 0x0001u;
static const uint16_t TAF_KILL_FLAG_KILLER_COMMANDER = 0x0002u;
static const uint16_t TAF_KILL_FLAG_PRESUMED_DGUN    = 0x0004u;
static const uint16_t TAF_KILL_FLAG_SELF_DESTRUCT    = 0x0008u;  // excluded from
                                                                 // tiebreaker rule 2

struct TAFKillEvent {
    int64_t  wallClockMs;     // ms since unix epoch (GetSystemTimeAsFileTime)
    uint32_t victimDplayId;   // 0 if unknown
    uint32_t killerDplayId;   // 0 if unknown
    uint16_t flags;
    uint16_t _pad;
};  // 20 bytes

static const int TAF_KILL_RING_SIZE = 16;

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

    // Ring buffer of commander-death events. Consumers track killRingHead and
    // read newer events modulo TAF_KILL_RING_SIZE. At 30 Hz scan rate, the
    // 16-slot ring covers >0.5 s — well above forwarding latency.
    uint32_t     killRingHead;                  // monotonically increasing
    TAFKillEvent killRing[TAF_KILL_RING_SIZE];

    // Tiebreaker decision computed locally on mutual-elim end-game. 0 = undecided
    // (game ongoing, or normal non-mutual end). Each peer's exporter runs the same
    // rules over its local kill ring and reaches the same answer; consumers
    // (gpgnet4ta) read this and skip their own tiebreaker.
    uint32_t     tiebreakerWinnerDplayId;
};
#pragma pack()
