#!/usr/bin/env python3
"""Fail the build if a PE image imports an API that does not exist on Windows 7.

TA is a 1997 game and a meaningful slice of the player base runs it on Windows 7
rigs kept around for old-game compatibility.  Newer MSVC toolsets happily emit
static imports for Win8+/Win10+ APIs (most notoriously
GetSystemTimePreciseAsFileTime, which the STL inlines into every caller of
std::chrono::system_clock::now()).  Such a DLL does not merely misbehave on
Windows 7 - it refuses to load at all, with

    the procedure entry point GetSystemTimePreciseAsFileTime could not be
    located in the dynamic link library KERNEL32.dll

Because the CI runner's toolset drifts independently of this repo, this check
runs on the built artifact rather than trusting the compiler settings.

Usage:  python tools/check_win7_imports.py Public/tdraw.dll [more.dll ...]
"""

import struct
import sys

# Imported functions that do not exist on Windows 7 SP1.
POST_WIN7_FUNCTIONS = {
    # --- Windows 8 ---
    "GetSystemTimePreciseAsFileTime",
    "WaitOnAddress",
    "WakeByAddressSingle",
    "WakeByAddressAll",
    "CreateFile2",
    "CopyFile2",
    "GetOverlappedResultEx",
    "SetProcessMitigationPolicy",
    "GetProcessMitigationPolicy",
    "GetCurrentPackageFullName",
    "GetCurrentPackageFamilyName",
    "AppPolicyGetProcessTerminationMethod",
    # --- Windows 8.1 ---
    "DiscardVirtualMemory",
    "OfferVirtualMemory",
    "ReclaimVirtualMemory",
    "GetSystemTimePreciseAsFileTimeStub",
    # --- Windows 10 ---
    "SetThreadDescription",
    "GetThreadDescription",
    "IsWow64Process2",
    "GetSystemTimeAdjustmentPrecise",
    "GetMachineTypeAttributes",
    # --- Windows 11 ---
    "GetTempPath2A",
    "GetTempPath2W",
}

# Importing from these modules at all implies Win8+ (or a UCRT redist that
# Windows 7 does not ship with).
POST_WIN7_MODULE_PREFIXES = ("api-ms-win-", "ext-ms-win-")
POST_WIN7_MODULES = {"ucrtbase.dll", "kernelbase.dll"}


def _read_imports(path):
    """Return {module_name: [function_name, ...]} for a PE32/PE32+ image."""
    data = open(path, "rb").read()
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    if data[pe : pe + 4] != b"PE\0\0":
        raise ValueError("%s is not a PE image" % path)

    section_count = struct.unpack_from("<H", data, pe + 6)[0]
    opt_header_size = struct.unpack_from("<H", data, pe + 20)[0]
    opt = pe + 24
    magic = struct.unpack_from("<H", data, opt)[0]
    # The import directory is data directory #1, which sits after the 96/112
    # byte fixed part of the optional header (PE32 vs PE32+).
    data_dirs = opt + (96 if magic == 0x10B else 112)
    import_rva = struct.unpack_from("<I", data, data_dirs + 8)[0]
    if import_rva == 0:
        return {}

    sections = []
    section_table = opt + opt_header_size
    for i in range(section_count):
        off = section_table + i * 40
        vsize, vaddr, rsize, raddr = struct.unpack_from("<IIII", data, off + 8)
        sections.append((vaddr, max(vsize, rsize), raddr))

    def to_offset(rva):
        for vaddr, size, raddr in sections:
            if vaddr <= rva < vaddr + size:
                return raddr + (rva - vaddr)
        raise ValueError("RVA 0x%x is outside every section" % rva)

    def to_string(offset):
        return data[offset : data.index(b"\0", offset)].decode("ascii", "replace")

    imports = {}
    descriptor = to_offset(import_rva)
    while True:
        lookup_rva, _, _, name_rva, address_rva = struct.unpack_from(
            "<IIIII", data, descriptor
        )
        if name_rva == 0:
            break
        module = to_string(to_offset(name_rva))
        functions = []
        # The lookup table is stripped in some images; fall back to the IAT.
        thunk = to_offset(lookup_rva or address_rva)
        entry_size = 4 if magic == 0x10B else 8
        entry_fmt = "<I" if entry_size == 4 else "<Q"
        ordinal_flag = 1 << (entry_size * 8 - 1)
        while True:
            entry = struct.unpack_from(entry_fmt, data, thunk)[0]
            if entry == 0:
                break
            if entry & ordinal_flag:
                functions.append("#%d" % (entry & 0xFFFF))
            else:
                # Hint/Name table entry: 2 byte hint, then the name.
                functions.append(to_string(to_offset(entry) + 2))
            thunk += entry_size
        imports[module] = functions
        descriptor += 20
    return imports


def check(path):
    """Print and return the Win7-incompatible imports of one image."""
    problems = []
    for module, functions in _read_imports(path).items():
        lowered = module.lower()
        if lowered.startswith(POST_WIN7_MODULE_PREFIXES) or lowered in POST_WIN7_MODULES:
            problems.append("%s (whole module is not present on Windows 7)" % module)
        for function in functions:
            if function in POST_WIN7_FUNCTIONS:
                problems.append("%s!%s" % (module, function))

    if problems:
        print("FAIL %s imports %d API(s) missing on Windows 7:" % (path, len(problems)))
        for problem in problems:
            print("       %s" % problem)
    else:
        print("ok   %s (no post-Windows 7 imports)" % path)
    return problems


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 2
    failed = False
    for path in argv[1:]:
        if check(path):
            failed = True
    if failed:
        print()
        print("This binary will not load on Windows 7.  The usual cause is a newer")
        print("MSVC toolset: std::chrono::system_clock::now() inlines")
        print("GetSystemTimePreciseAsFileTime.  Use std::chrono::steady_clock, or")
        print("pin the toolset, rather than dropping the Windows 7 target.")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
