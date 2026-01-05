cmake_minimum_required(VERSION 3.23)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
project(bootstrap)

add_definitions(-DUNICODE -D_UNICODE /wd4090 -D_CRT_SECURE_NO_WARNINGS -D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)

set(SOURCES
    ../utils/archive.cpp
    installer.rc
    main.cpp
    ossupport.manifest
    version.rc
)

add_executable(bootstrap WIN32 ${SOURCES})

# Output file should still be named windscribe_installer.exe
set_target_properties(bootstrap PROPERTIES OUTPUT_NAME "windscribe_installer")

if(NOT DEFINED WINDSCRIBE_INSTALLER_NAME OR WINDSCRIBE_INSTALLER_NAME STREQUAL "")
    message(FATAL_ERROR "WINDSCRIBE_INSTALLER_NAME must be defined")
endif()
add_definitions(-DWINDSCRIBE_INSTALLER_NAME=L\"${WINDSCRIBE_INSTALLER_NAME}\")

if(VCPKG_TARGET_TRIPLET MATCHES "arm64")
    # JDRM - I could find no built-in way to detect arm64 compilation in the MS resource compiler.
    add_definitions(-DWINDSCRIBE_ARM64_TARGET)
    set_source_files_properties(installer.rc OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/resources/arm64/7zr.exe)
else()
    set_source_files_properties(installer.rc OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/resources/amd64/7zr.exe)
endif()

set_source_files_properties(installer.rc OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/resources/windscribeinstaller.7z)
set_property(TARGET bootstrap PROPERTY COMPILE_WARNING_AS_ERROR ON)
set_property(TARGET bootstrap PROPERTY VS_DPI_AWARE "PerMonitor")
set_property(TARGET bootstrap PROPERTY COMPILE_FLAGS "/Gy /Oi /sdl")
set_property(TARGET bootstrap PROPERTY LINK_FLAGS "/DELAYLOAD:dwmapi.dll /DELAYLOAD:uxtheme.dll /DELAYLOAD:wldp.dll")

target_link_libraries(bootstrap
    PRIVATE
    delayimp
)

target_include_directories(bootstrap
    PRIVATE
    ../../utils
    ../../../client/common/types
    ../../../client/common/utils
)

install(TARGETS bootstrap
    RUNTIME DESTINATION .
)
