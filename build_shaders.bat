@echo off
REM Compile HLSL pixel shader to a compiled shader object using fxc.
REM Requires Windows SDK fxc.exe on PATH (or full path to fxc).

set "SRC=shaders\simple_lit.hlsl"
set "OUT=shaders\simple_lit.cso"

echo Compiling %SRC% -> %OUT% using ps_5_0 entrypoint main

n
fxc /T ps_5_0 /E main /Fo "%OUT%" "%SRC%"
if ERRORLEVEL 1 (
    echo.
    echo fxc failed. Ensure Visual Studio Windows SDK is installed and fxc.exe is on PATH.
    echo You can find fxc under "C:\Program Files (x86)\Windows Kits\10\bin\<version>\x86\fxc.exe" or similar.
    pause
) else (
    echo Compiled successfully.
)
