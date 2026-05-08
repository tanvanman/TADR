#include "Profiler.h"

#if TDRAW_PROFILING

#include <windows.h>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <vector>
#include <algorithm>

namespace tdrawprof {

Counter* Counter::s_head = nullptr;

Counter::Counter(const char* name)
    : m_name(name)
    , m_count(0)
    , m_totalTicks(0)
    , m_minTicks(INT64_MAX)
    , m_maxTicks(0)
    , m_next(s_head)
{
    s_head = this;
}

static int64_t s_qpcFreq = 0;

static int64_t QpcFreq()
{
    if (s_qpcFreq == 0) {
        LARGE_INTEGER f;
        QueryPerformanceFrequency(&f);
        s_qpcFreq = f.QuadPart ? f.QuadPart : 1;
    }
    return s_qpcFreq;
}

int64_t Scope::now()
{
    LARGE_INTEGER c;
    QueryPerformanceCounter(&c);
    return c.QuadPart;
}

// 5 seconds: cheap dump cadence, plenty of frames per window for stable means.
static const int64_t DUMP_INTERVAL_MS = 5000;

static uint64_t s_frameCount       = 0;
static int64_t  s_windowStart      = 0;
static int64_t  s_dumpIntervalTks  = 0;
static HANDLE   s_logFile          = INVALID_HANDLE_VALUE;
static bool     s_logOpenAttempted = false;

static void EnsureLogOpen()
{
    if (s_logOpenAttempted) return;
    s_logOpenAttempted = true;
    s_logFile = CreateFileA("tdrawprof.txt",
                            GENERIC_WRITE,
                            FILE_SHARE_READ,
                            NULL,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);
}

// Append into a heap buffer so the dump becomes a single WriteFile call.
struct DumpBuf
{
    std::vector<char> data;

    void appendf(const char* fmt, ...)
    {
        char tmp[512];
        va_list ap;
        va_start(ap, fmt);
        int n = _vsnprintf_s(tmp, sizeof(tmp), _TRUNCATE, fmt, ap);
        va_end(ap);
        if (n <= 0) return;
        data.insert(data.end(), tmp, tmp + n);
    }
};

void FrameTick()
{
    if (s_windowStart == 0) {
        s_windowStart     = Scope::now();
        s_dumpIntervalTks = QpcFreq() * DUMP_INTERVAL_MS / 1000;
    }
    ++s_frameCount;
}

void DumpIfDue()
{
    if (s_windowStart == 0) return;
    int64_t now = Scope::now();
    if ((now - s_windowStart) < s_dumpIntervalTks) return;
    DumpNow();
}

void DumpNow()
{
    EnsureLogOpen();
    if (s_logFile == INVALID_HANDLE_VALUE) return;

    int64_t now          = Scope::now();
    int64_t freq         = QpcFreq();
    int64_t elapsedTicks = (s_windowStart != 0) ? (now - s_windowStart) : 0;
    double  windowMs     = (double)elapsedTicks * 1000.0 / (double)freq;

    std::vector<Counter*> all;
    for (Counter* c = Counter::head(); c; c = c->next()) {
        all.push_back(c);
    }
    std::sort(all.begin(), all.end(), [](Counter* a, Counter* b) {
        return a->totalTicks() > b->totalTicks();
    });

    DumpBuf buf;
    buf.data.reserve(4096);

    SYSTEMTIME st;
    GetLocalTime(&st);
    buf.appendf("=== profile dump %04d-%02d-%02d %02d:%02d:%02d  frames=%llu  window=%.0f ms ===\r\n",
                st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,
                (unsigned long long)s_frameCount, windowMs);
    buf.appendf("%-36s %8s %8s %10s %9s %9s %9s %8s\r\n",
                "scope", "count", "per_fr", "tot_ms", "mean_us", "min_us", "max_us", "%window");

    for (Counter* c : all) {
        if (c->count() == 0) continue;
        double totalMs  = (double)c->totalTicks() * 1000.0 / (double)freq;
        double meanUs   = (double)c->totalTicks() * 1.0e6 / ((double)c->count() * (double)freq);
        double minUs    = (double)c->minTicks()   * 1.0e6 / (double)freq;
        double maxUs    = (double)c->maxTicks()   * 1.0e6 / (double)freq;
        double pct      = windowMs > 0.0 ? totalMs * 100.0 / windowMs : 0.0;
        double perFrame = s_frameCount > 0 ? (double)c->count() / (double)s_frameCount : 0.0;
        buf.appendf("%-36s %8llu %8.2f %10.2f %9.1f %9.1f %9.1f %7.2f%%\r\n",
                    c->name(), (unsigned long long)c->count(), perFrame,
                    totalMs, meanUs, minUs, maxUs, pct);
    }
    buf.appendf("\r\n");

    if (!buf.data.empty()) {
        DWORD written = 0;
        // No FlushFileBuffers — lazy-flush keeps the render thread off disk.
        WriteFile(s_logFile, buf.data.data(), (DWORD)buf.data.size(), &written, NULL);
    }

    for (Counter* c : all) c->reset();
    s_frameCount  = 0;
    s_windowStart = now;
}

} // namespace tdrawprof

#endif // TDRAW_PROFILING
