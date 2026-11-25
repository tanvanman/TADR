#include "ArenaCapacityFix.h"
#include "iddrawsurface.h"

#include <windows.h>
#include <cstring>

namespace ArenaCapacityFix {

namespace {

constexpr DWORD kSite = 0x0042D41E;
constexpr size_t kLen = 13;

// cdq; and edx,0xFFFFF; add eax,edx; sar eax,0x14; inc eax
constexpr BYTE kOriginal[kLen] = { 0x99, 0x81, 0xE2, 0xFF, 0xFF, 0x0F, 0x00, 0x03, 0xC2, 0xC1, 0xF8, 0x14, 0x40 };
// shr eax,0x14; inc eax; nop x9
constexpr BYTE kPatched[kLen] = { 0xC1, 0xE8, 0x14, 0x40, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };

bool g_patched = false;

bool WriteCode(const BYTE* bytes)
{
    void* at = reinterpret_cast<void*>(kSite);
    DWORD old;
    if (!VirtualProtect(at, kLen, PAGE_EXECUTE_READWRITE, &old))
        return false;
    memcpy(at, bytes, kLen);
    VirtualProtect(at, kLen, old, &old);
    FlushInstructionCache(GetCurrentProcess(), at, kLen);
    return memcmp(at, bytes, kLen) == 0;
}

}  // namespace

void Install()
{
    if (g_patched)
        return;
    if (memcmp(reinterpret_cast<const void*>(kSite), kOriginal, kLen) != 0)
    {
        IDDrawSurface::OutptFmtTxt("[ArenaCapacityFix] refused: the 13 bytes at 0x%08X are not the expected ones", (unsigned)kSite);
        return;
    }
    g_patched = WriteCode(kPatched);
    IDDrawSurface::OutptFmtTxt("[ArenaCapacityFix] self-check %s: unsigned RAM size at 0x%08X", g_patched ? "OK" : "FAILED", (unsigned)kSite);
}

void Shutdown()
{
    if (!g_patched)
        return;
    WriteCode(kOriginal);
    g_patched = false;
}

}  // namespace ArenaCapacityFix
