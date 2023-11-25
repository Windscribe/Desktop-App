cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
project(bootstrap)

add_definitions(-DUNICODE -D_UNICODE /wd4090 -D_CRT_SECURE_NO_WARNINGS -D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)

set(SOURCES
    archive/archive.cpp
    archive/LZMA_SDK/C/7zAlloc.c
    archive/LZMA_SDK/C/7zArcIn.c
    archive/LZMA_SDK/C/7zBuf.c
    archive/LZMA_SDK/C/7zCrc.c
    archive/LZMA_SDK/C/7zCrcOpt.c
    archive/LZMA_SDK/C/7zDec.c
    archive/LZMA_SDK/C/7zFile.c
    archive/LZMA_SDK/C/7zStream.c
    archive/LZMA_SDK/C/Bcj2.c
    archive/LZMA_SDK/C/Bra.c
    archive/LZMA_SDK/C/Bra86.c
    archive/LZMA_SDK/C/BraIA64.c
    archive/LZMA_SDK/C/CpuArch.c
    archive/LZMA_SDK/C/Delta.c
    archive/LZMA_SDK/C/Lzma2Dec.c
    archive/LZMA_SDK/C/LzmaDec.c
    archive/LZMA_SDK/C/Ppmd7.c
    installer.rc
    main.cpp
    ossupport.manifest
    resources/windscribeinstaller.7z
    version.rc
)

add_executable(windscribe_installer WIN32 ${SOURCES})

option(WINDSCRIBE_INSTALLER_NAME "Name of internal installer exe" "")
if(WINDSCRIBE_INSTALLER_NAME)
    add_definitions(-DWINDSCRIBE_INSTALLER_NAME=L\"${WINDSCRIBE_INSTALLER_NAME}\")
endif(WINDSCRIBE_INSTALLER_NAME)

set_source_files_properties(installer.rc OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/resources/windscribeinstaller.7z)
set_property(TARGET windscribe_installer PROPERTY COMPILE_WARNING_AS_ERROR ON)
set_property(TARGET windscribe_installer PROPERTY VS_DPI_AWARE "PerMonitor")
set_property(TARGET windscribe_installer PROPERTY COMPILE_FLAGS "/Gy /Oi /sdl")

target_include_directories(windscribe_installer
    PRIVATE
    ../../utils
    ../../../client/common/utils
)
