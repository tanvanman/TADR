@echo off
setlocal
if "%~1"=="" (
    echo Usage: run_map_preview_test.cmd path-to-private-retail-TotalA.exe
    exit /b 2
)
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
if not exist "%~dp0bin" mkdir "%~dp0bin"
cl /nologo /EHsc /std:c++17 /W4 /O2 /Fe:"%~dp0bin\map_preview_copy_test.exe" /Fo:"%~dp0bin\map_preview_copy_test.obj" "%~dp0map_preview_copy_test.cpp" /link advapi32.lib
if errorlevel 1 exit /b 1
"%~dp0bin\map_preview_copy_test.exe" "%~1"
exit /b %errorlevel%
