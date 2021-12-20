ECHO off
ECHO ===== Installing CMake =====

set CMAKE_TEMP="C:\cmake_temp"

rd /s /q %CMAKE_TEMP%

mkdir %CMAKE_TEMP%
curl.exe https://github.com/Kitware/CMake/releases/download/v3.12.4/cmake-3.12.4-win64-x64.msi -o %CMAKE_TEMP%\cmake-3.12.4-win64-x64.msi -k -L 
call %CMAKE_TEMP%\cmake-3.12.4-win64-x64.msi /qn /norestart

rd /s /q %CMAKE_TEMP%

