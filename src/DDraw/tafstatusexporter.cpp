#include "tafstatusexporter.h"
#include <cstring>

// Force a write at least this often even if nothing changed, so consumers can detect
// the exporter is alive. Unlock fires roughly per render frame (~30 Hz).
static const int HEARTBEAT_FRAMES = 30;

CTAFStatusExporter::CTAFStatusExporter(TAdynmemStruct* dynmem)
    : m_TAdynmem(dynmem), m_hMap(NULL), m_pView(nullptr), m_prev{},
      m_framesSinceWrite(HEARTBEAT_FRAMES)
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

        next.playerActive[i]      = (p.PlayerActive != 0) ? 1u : 0u;
        next.playerUnitsNumber[i] = static_cast<int16_t>(p.UnitsNumber);

        if (p.PlayerActive != 0) {
            memcpy(next.playerAllyFlags[i], p.AllyFlagAry, 10);
            next.playerAllyTeam[i]     = static_cast<uint8_t>(p.AllyTeam);
            next.playerDirectPlayId[i] = static_cast<uint32_t>(p.DirectPlayID);
            if (p.PlayerInfo) {
                next.playerPropertyMask[i] = static_cast<uint16_t>(p.PlayerInfo->PropertyMask);
            }
        }
    }

    ++m_framesSinceWrite;
    const bool dataChanged =
        m_prev.magic != TAFGAMESTATE_MAGIC ||
        memcmp(m_prev.playerAllyFlags,    next.playerAllyFlags,    sizeof(next.playerAllyFlags))    != 0 ||
        memcmp(m_prev.playerAllyTeam,     next.playerAllyTeam,     sizeof(next.playerAllyTeam))     != 0 ||
        memcmp(m_prev.playerActive,       next.playerActive,       sizeof(next.playerActive))       != 0 ||
        memcmp(m_prev.playerUnitsNumber,  next.playerUnitsNumber,  sizeof(next.playerUnitsNumber))  != 0 ||
        memcmp(m_prev.playerPropertyMask, next.playerPropertyMask, sizeof(next.playerPropertyMask)) != 0 ||
        memcmp(m_prev.playerDirectPlayId, next.playerDirectPlayId, sizeof(next.playerDirectPlayId)) != 0;
    const bool heartbeatDue = m_framesSinceWrite >= HEARTBEAT_FRAMES;

    if (dataChanged || heartbeatDue)
    {
        TAFGameState* shm = static_cast<TAFGameState*>(m_pView);

        // Seqlock write: odd sequenceNumber signals write-in-progress to readers
        uint32_t writingSeq = m_prev.sequenceNumber + 1;
        shm->sequenceNumber = writingSeq;
        MemoryBarrier();

        shm->magic = next.magic;
        memcpy(shm->playerAllyFlags,    next.playerAllyFlags,    sizeof(next.playerAllyFlags));
        memcpy(shm->playerAllyTeam,     next.playerAllyTeam,     sizeof(next.playerAllyTeam));
        memcpy(shm->playerActive,       next.playerActive,       sizeof(next.playerActive));
        memcpy(shm->playerUnitsNumber,  next.playerUnitsNumber,  sizeof(next.playerUnitsNumber));
        memcpy(shm->playerPropertyMask, next.playerPropertyMask, sizeof(next.playerPropertyMask));
        memcpy(shm->playerDirectPlayId, next.playerDirectPlayId, sizeof(next.playerDirectPlayId));

        MemoryBarrier();
        shm->sequenceNumber = writingSeq + 1;

        m_prev = next;
        m_prev.sequenceNumber = writingSeq + 1;
        m_framesSinceWrite = 0;
    }
}
