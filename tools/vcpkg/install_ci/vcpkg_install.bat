@echo off
setlocal EnableDelayedExpansion

REM This script installs vcpkg to the home directory if that directory does not already exist.

if "%~1"=="" (
    echo The path parameter VCPKG_PATH is not set.
    echo "Usage: vcpkg_install.bat <VCPKG_PATH> [--configure-git]"
    EXIT /B 1
)

if "%~2"=="--configure-git" (
    echo Configure git config for vcpkg
    REM remove previous insteadof sections for the git@gitlab.int.windscribe.com address
    for /f delims^=^ eol^= %%i  in ('git config --list ^| findstr /i "insteadof=git@gitlab.int.windscribe.com"') do (
        for /f "tokens=1 delims==" %%a in ("%%i") do (
            set result=%%a
            git config --global --remove-section !result:~0,-10!
        )
    )
    git config --global url."https://gitlab-ci-token:%CI_JOB_TOKEN%@gitlab.int.windscribe.com/".insteadOf "git@gitlab.int.windscribe.com:"
)

set VCPKG_ROOT=%1

mkdir c:\vcpkg_cache

if exist %VCPKG_ROOT%\vcpkg.exe (
    echo vcpkg is installed
    %VCPKG_ROOT%\vcpkg version
) else (
    echo vcpkg is not installed, install it
    if exist %VCPKG_ROOT%\ rmdir /s/q %VCPKG_ROOT%
    mkdir %VCPKG_ROOT%
    PUSHD .
    cd %VCPKG_ROOT%
    git clone https://github.com/Microsoft/vcpkg.git .
    bootstrap-vcpkg.bat -disableMetrics
    POPD
)
