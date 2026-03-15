import ctypes, sys

k32 = ctypes.windll.kernel32

# Find TotalA.exe PID via snapshot
TH32CS_SNAPPROCESS = 0x2
PROCESSENTRY32 = None

import ctypes.wintypes as wt

class PROCESSENTRY32(ctypes.Structure):
    _fields_ = [
        ("dwSize", wt.DWORD),
        ("cntUsage", wt.DWORD),
        ("th32ProcessID", wt.DWORD),
        ("th32DefaultHeapID", ctypes.POINTER(ctypes.c_ulong)),
        ("th32ModuleID", wt.DWORD),
        ("cntThreads", wt.DWORD),
        ("th32ParentProcessID", wt.DWORD),
        ("pcPriClassBase", ctypes.c_long),
        ("dwFlags", wt.DWORD),
        ("szExeFile", ctypes.c_char * 260),
    ]

snap = k32.CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)
entry = PROCESSENTRY32()
entry.dwSize = ctypes.sizeof(PROCESSENTRY32)

pid = None
if k32.Process32First(snap, ctypes.byref(entry)):
    while True:
        if entry.szExeFile.lower() == b'totala.exe':
            pid = entry.th32ProcessID
            break
        if not k32.Process32Next(snap, ctypes.byref(entry)):
            break
k32.CloseHandle(snap)

if pid:
    h = k32.OpenProcess(1, False, pid)
    k32.TerminateProcess(h, 1)
    k32.CloseHandle(h)
    print(f"Killed TotalA.exe (pid {pid})")
else:
    print("TotalA.exe not found")
