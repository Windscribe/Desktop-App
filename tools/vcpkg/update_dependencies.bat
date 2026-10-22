@echo off
setlocal

REM This script installs or updates all dependencies locally.
REM PROCESSOR_ARCHITECTURE follows the process, so an x64 cmd on Windows on Arm reports AMD64; Win32_Processor.Architecture 12 is ARM64.
REM The triplets match the ones a CMake Release build uses for target and host.
set "WS_CPU_ARCH="
for /f %%A in ('%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe -NoProfile -NonInteractive -Command "(Get-CimInstance -ClassName Win32_Processor | Select-Object -First 1 -ExpandProperty Architecture)"') do set "WS_CPU_ARCH=%%A"
if "%WS_CPU_ARCH%"=="12" (
    set "WS_TRIPLET=ws-arm64-windows-static-release"
) else (
    set "WS_TRIPLET=ws-x64-windows-static-release"
)
%VCPKG_ROOT%\vcpkg install --x-install-root=%VCPKG_ROOT%\installed-%WS_TRIPLET% --x-manifest-root=%~dp0 --triplet=%WS_TRIPLET% --host-triplet=%WS_TRIPLET% --clean-buildtrees-after-build --clean-packages-after-build
