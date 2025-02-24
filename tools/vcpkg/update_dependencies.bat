@echo off

REM This script installs or updates all dependencies locally
%VCPKG_ROOT%\vcpkg install --x-install-root=%VCPKG_ROOT%\installed --x-manifest-root=%~dp0 --triplet=x64-windows-static