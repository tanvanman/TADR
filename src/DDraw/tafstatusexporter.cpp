#include "tafstatusexporter.h"
#include <cstring>

CTAFStatusExporter::CTAFStatusExporter(TAdynmemStruct* dynmem)
    : m_TAdynmem(dynmem), m_hMap(NULL), m_pView(nullptr), m_prev{}
{
    m_hMap  = CreateFileMapping((HANDLE)0xFFFFFFFF, NULL, PAGE_READWRITE,
                                 0, sizeof(TAFGameState),
                                 TAFGAMESTATE_SHMEM_NAME);
    m_pView = m_hMap ? MapViewOfFile(m_hMap, FILE_MAP_ALL_ACCESS,
                                     0, 0, sizeof(TAFGameState))
                     : nullptr;
    if (m_pView)
        memset(m_pView, 0, sizeof(TAFGameState)); // magic=0 until first update
}

CTAFStatusExporter::~CTAFStatusExporter()
{
    if (m_pView) { UnmapViewOfFile(m_pView); m_pView = nullptr; }
    if (m_hMap)  { CloseHandle(m_hMap);       m_hMap  = NULL;   }
}

void CTAFStatusExporter::FrameUpdate()
{
    if (!m_pView || !m_TAdynmem) return;

    TAFGameState next = {};
    next.magic = TAFGAMESTATE_MAGIC;
    for (int i = 0; i < 10; i++) {
        const PlayerStruct& p = m_TAdynmem->Players[i];
        next.playerActive[i] = (p.PlayerActive != 0) ? 1u : 0u;
        if (p.PlayerActive != 0) {
            memcpy(next.playerAllyFlags[i], p.AllyFlagAry, 10);
            next.playerAllyTeam[i]    = static_cast<uint8_t>(p.AllyTeam);
            next.playerDirectPlayId[i] = static_cast<uint32_t>(p.DirectPlayID);
            next.playerWinLoseTime[i]  = static_cast<int32_t>(p.WinLoseTime);
            next.playerUnitsNumber[i]  = static_cast<int16_t>(p.UnitsNumber);
            next.playerMyType[i]       = static_cast<uint8_t>(p.My_PlayerType);
            if (p.PlayerInfo) {
                next.playerRaceSide[i]     = static_cast<uint8_t>(p.PlayerInfo->RaceSide);
                next.playerPropertyMask[i] = static_cast<uint16_t>(p.PlayerInfo->PropertyMask);
                next.playerInfoType[i]     = static_cast<uint8_t>(p.PlayerInfo->PlayerType);
            }
        }
        // inactive slots stay zeroed
    }

    bool unitsZeroCrossing = false;
    for (int i = 0; i < 10; i++) {
        if ((m_prev.playerUnitsNumber[i] > 0) != (next.playerUnitsNumber[i] > 0)) {
            unitsZeroCrossing = true;
            break;
        }
    }

    if (m_prev.magic != TAFGAMESTATE_MAGIC ||
        unitsZeroCrossing ||
        memcmp(m_prev.playerAllyFlags,    next.playerAllyFlags,    sizeof(next.playerAllyFlags))    != 0 ||
        memcmp(m_prev.playerAllyTeam,     next.playerAllyTeam,     sizeof(next.playerAllyTeam))     != 0 ||
        memcmp(m_prev.playerActive,       next.playerActive,       sizeof(next.playerActive))       != 0 ||
        memcmp(m_prev.playerDirectPlayId, next.playerDirectPlayId, sizeof(next.playerDirectPlayId)) != 0 ||
        memcmp(m_prev.playerWinLoseTime,  next.playerWinLoseTime,  sizeof(next.playerWinLoseTime))  != 0 ||
        memcmp(m_prev.playerRaceSide,     next.playerRaceSide,     sizeof(next.playerRaceSide))     != 0 ||
        memcmp(m_prev.playerPropertyMask, next.playerPropertyMask, sizeof(next.playerPropertyMask)) != 0 ||
        memcmp(m_prev.playerInfoType,     next.playerInfoType,     sizeof(next.playerInfoType))     != 0 ||
        memcmp(m_prev.playerMyType,       next.playerMyType,       sizeof(next.playerMyType))       != 0)
    {
        TAFGameState* shm = static_cast<TAFGameState*>(m_pView);

        // Seqlock write: odd sequenceNumber signals write-in-progress to readers
        uint32_t writingSeq = m_prev.sequenceNumber + 1;  // always odd (prev was even or 0)
        shm->sequenceNumber = writingSeq;
        MemoryBarrier();

        shm->magic = next.magic;
        memcpy(shm->playerAllyFlags,    next.playerAllyFlags,    sizeof(next.playerAllyFlags));
        memcpy(shm->playerAllyTeam,     next.playerAllyTeam,     sizeof(next.playerAllyTeam));
        memcpy(shm->playerActive,       next.playerActive,       sizeof(next.playerActive));
        memcpy(shm->playerDirectPlayId, next.playerDirectPlayId, sizeof(next.playerDirectPlayId));
        memcpy(shm->playerWinLoseTime,  next.playerWinLoseTime,  sizeof(next.playerWinLoseTime));
        memcpy(shm->playerUnitsNumber,  next.playerUnitsNumber,  sizeof(next.playerUnitsNumber));
        memcpy(shm->playerRaceSide,     next.playerRaceSide,     sizeof(next.playerRaceSide));
        memcpy(shm->playerPropertyMask, next.playerPropertyMask, sizeof(next.playerPropertyMask));
        memcpy(shm->playerInfoType,     next.playerInfoType,     sizeof(next.playerInfoType));
        memcpy(shm->playerMyType,       next.playerMyType,       sizeof(next.playerMyType));

        MemoryBarrier();
        shm->sequenceNumber = writingSeq + 1;  // even: write complete

        m_prev = next;
        m_prev.sequenceNumber = writingSeq + 1;
    }
}
