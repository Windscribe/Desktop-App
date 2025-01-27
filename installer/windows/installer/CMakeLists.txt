cmake_minimum_required(VERSION 3.23)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "arm64")
    set(VCPKG_TARGET_TRIPLET arm64-windows-static)
else()
    set(VCPKG_TARGET_TRIPLET x64-windows-static)
endif()

project(installer)

find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS
             Widgets
             Gui
             Svg
             LinguistTools)
qt_standard_project_setup(REQUIRES 6.5)

add_definitions(-DUNICODE -D_UNICODE /wd4090 -D_CRT_SECURE_NO_WARNINGS -D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)

set(SOURCES
    ../../../client/common/utils/languagesutil.cpp
    ../../../client/common/utils/servicecontrolmanager.cpp
    ../../../client/common/utils/wincryptutils.cpp
    ../utils/applicationinfo.cpp
    ../utils/path.cpp
    ../utils/utils.cpp
    ../utils/windscribepathcheck.cpp
    ../utils/archive.cpp
    installer/installer.cpp
    installer/installer_shim.cpp
    installer/installer_utils.cpp
    installer/blocks/files.cpp
    installer/blocks/icons.cpp
    installer/blocks/install_openvpn_dco.cpp
    installer/blocks/install_splittunnel.cpp
    installer/blocks/service.cpp
    installer/blocks/uninstallprev.cpp
    installer/blocks/uninstall_info.cpp
    installer/installer_base.cpp
    installer/settings.cpp
    installer/shellexecuteasuser.cpp
    installer.rc
    main.cpp
    ossupport.manifest
    resources/windscribe.7z
    version.rc
)

qt_add_resources(rc ../../common/installer_win.qrc)
qt_add_executable(installer WIN32 ${SOURCES} ${rc})
add_subdirectory(../../common ${CMAKE_CURRENT_BINARY_DIR}/common)

set_source_files_properties(installer.rc OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/resources/windscribe.7z)
set_property(TARGET installer PROPERTY COMPILE_WARNING_AS_ERROR ON)
set_property(TARGET installer PROPERTY VS_DPI_AWARE "PerMonitor")
set_property(TARGET installer PROPERTY COMPILE_FLAGS "/Gy /Oi /sdl")
set_property(TARGET installer PROPERTY LINK_FLAGS "/MANIFESTUAC:\"level='asInvoker' uiAccess='false'\" /DELAYLOAD:dwmapi.dll /DELAYLOAD:uxtheme.dll /DELAYLOAD:wldp.dll")

target_link_directories(installer PRIVATE)

target_link_libraries(installer
    PRIVATE
    Iphlpapi
    version
    rpcrt4
    Fwpuclnt
    Ws2_32
    delayimp
    Crypt32
    spdlog::spdlog_header_only
    fmt::fmt-header-only
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Svg
)

target_include_directories(installer
    PRIVATE
    ../../../client/common
    ../../../client/common/types
    ../../../client/common/utils
    ../../common
)

install(TARGETS installer
    RUNTIME DESTINATION .
)

