cmake_minimum_required(VERSION 3.23)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
project(bootstrap)

find_package(7zip CONFIG REQUIRED)

add_definitions(-DUNICODE -D_UNICODE /wd4090 -D_CRT_SECURE_NO_WARNINGS -D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)

set(SOURCES
    ../../../client/common/archive/archive.cpp
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
set_property(TARGET windscribe_installer PROPERTY LINK_FLAGS "/DELAYLOAD:dwmapi.dll /DELAYLOAD:uxtheme.dll /DELAYLOAD:wldp.dll")

target_link_libraries(windscribe_installer
    PRIVATE
    delayimp
    7zip::7zip
)

target_include_directories(windscribe_installer
    PRIVATE
    ../../utils
    ../../../client/common/types
    ../../../client/common/utils
)

install(TARGETS windscribe_installer
    RUNTIME DESTINATION .
)
