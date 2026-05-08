// Lightweight per-frame profiler for tadr-ddraw.
//
// Usage:
//     PROFILE_SCOPE("Unlock.WhiteBoardLockBlit");
//     WhiteBoard->LockBlit(...);
//
// One PROFILE_SCOPE per source line. Each scope owns a static Counter that
// registers itself in a singly-linked list at first construction. The Scope
// RAII object samples QueryPerformanceCounter at ctor/dtor and adds the delta.
//
// PROFILE_FRAME() / PROFILE_DUMP_IF_DUE() should be called once per frame
// (end of IDDrawSurface::Unlock GUI-thread path). Every DUMP_INTERVAL_MS the
// dumper writes an aggregated report to tdrawprof.txt and resets all counters.
//
// Thread-safety: only safe to call from one thread (the GUI thread). All hot
// paths we instrument are GUI-thread-only, so no atomics are needed.

#pragma once

#include "config.h"
#include <cstdint>
#include <climits>

#ifndef TDRAW_PROFILING
#define TDRAW_PROFILING 0
#endif

#if TDRAW_PROFILING

namespace tdrawprof {

class Counter
{
public:
    explicit Counter(const char* name);

    void add(int64_t ticks)
    {
        ++m_count;
        m_totalTicks += ticks;
        if (ticks < m_minTicks) m_minTicks = ticks;
        if (ticks > m_maxTicks) m_maxTicks = ticks;
    }

    void reset()
    {
        m_count = 0;
        m_totalTicks = 0;
        m_minTicks = INT64_MAX;
        m_maxTicks = 0;
    }

    const char* name()       const { return m_name; }
    uint64_t    count()      const { return m_count; }
    int64_t     totalTicks() const { return m_totalTicks; }
    int64_t     minTicks()   const { return m_minTicks; }
    int64_t     maxTicks()   const { return m_maxTicks; }
    Counter*    next()       const { return m_next; }

    static Counter* head() { return s_head; }

private:
    const char* m_name;
    uint64_t    m_count;
    int64_t     m_totalTicks;
    int64_t     m_minTicks;
    int64_t     m_maxTicks;
    Counter*    m_next;

    static Counter* s_head;
};

class Scope
{
public:
    explicit Scope(Counter& c) : m_counter(c), m_start(now()) {}
    ~Scope() { m_counter.add(now() - m_start); }

    static int64_t now();

private:
    Counter& m_counter;
    int64_t  m_start;
};

void FrameTick();
void DumpIfDue();
void DumpNow();

} // namespace tdrawprof

#define TDRAW_PROF_CONCAT_(a, b) a##b
#define TDRAW_PROF_CONCAT(a, b)  TDRAW_PROF_CONCAT_(a, b)
#define PROFILE_SCOPE(scopeName)                                                \
    static ::tdrawprof::Counter TDRAW_PROF_CONCAT(_tdrawProfCtr_, __LINE__){scopeName}; \
    ::tdrawprof::Scope TDRAW_PROF_CONCAT(_tdrawProfScp_, __LINE__){TDRAW_PROF_CONCAT(_tdrawProfCtr_, __LINE__)}

#define PROFILE_FRAME()       ::tdrawprof::FrameTick()
#define PROFILE_DUMP_IF_DUE() ::tdrawprof::DumpIfDue()

#else // TDRAW_PROFILING

#define PROFILE_SCOPE(name)   ((void)0)
#define PROFILE_FRAME()       ((void)0)
#define PROFILE_DUMP_IF_DUE() ((void)0)

#endif // TDRAW_PROFILING
