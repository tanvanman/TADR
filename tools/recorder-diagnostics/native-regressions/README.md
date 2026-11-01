# Recorder/configuration regression probes

These optional developer probes compile actual pre-fix and corrected source
methods. They do not run TA and are not part of the DLL release package.

On Windows, with Python/Git and FPC 3.2.2 win32 (root contains bin/i386-win32 and
units/i386-win32):

```powershell
python tools/recorder-diagnostics/native-regressions/test_pascal_hardening.py --fpc 'C:\path\to\fpc-root'
python tools/recorder-diagnostics/native-regressions/test_startup_hardening.py --fpc 'C:\path\to\fpc-root'
python tools/recorder-diagnostics/native-regressions/test_cpp_hardening.py
```

The C++ probe defaults to VS2022 Community's vcvars32.bat; override --vcvars if
needed. All probes accept --source, --out and --baseline. The fixed old baseline
is dcff5dd so old-fault checks keep working after the fix is committed. Generated
outputs default to the ignored parent build/ directory.

FPC tests enable range/overflow checking in Delphi mode, compile the new registry
and clock units, and extract the actual packet/startup method bodies with small
game/COM doubles. They do not validate the complete Delphi 7 recorder or its ABI.
C++ tests compile actual INI/string/registry methods with fixture dependencies.
Old failing cases run in separate child processes; some deliberately exit with
an access violation/runtime-check error. That is an expected test result.
Registry fixtures use a process-private hive file, never actual HKCU/HKLM keys.

The full Delphi 7 recorder build, offline replay/recording, resource displays and
private multiplayer synchronization still need validation before release. The
release workflow packages committed recorder DLLs; it does not compile the
Pascal changes. No new recorder DLLs are supplied by these tests.
