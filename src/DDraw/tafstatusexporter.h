#pragma once
#include "tafgamestate.h"
#include "tamem.h"
#include <windows.h>

class CTAFStatusExporter
{
public:
    explicit CTAFStatusExporter(TAdynmemStruct* dynmem);
    ~CTAFStatusExporter();

    // Call once per rendered frame from IDDrawSurface::Unlock().
    void FrameUpdate();

private:
    TAdynmemStruct* m_TAdynmem;
    HANDLE          m_hMap;
    void*           m_pView;
    TAFGameState    m_prev;             // last-written snapshot for change detection
    int             m_framesSinceWrite; // heartbeat — force a write periodically so
                                        // gpgnet4ta's staleness check doesn't fall
                                        // back to packet inference during quiet play
};
