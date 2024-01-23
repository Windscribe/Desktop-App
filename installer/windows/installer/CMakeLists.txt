cmake_minimum_required(VERSION 3.23)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
project(installer)

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
    ../utils/logger.cpp
    ../utils/path.cpp
    ../utils/utils.cpp
    ../utils/windscribepathcheck.cpp
    ../../../client/common/archive/archive.cpp
    ../../../client/common/archive/LZMA_SDK/C/7zAlloc.c
    ../../../client/common/archive/LZMA_SDK/C/7zArcIn.c
    ../../../client/common/archive/LZMA_SDK/C/7zBuf.c
    ../../../client/common/archive/LZMA_SDK/C/7zCrc.c
    ../../../client/common/archive/LZMA_SDK/C/7zCrcOpt.c
    ../../../client/common/archive/LZMA_SDK/C/7zDec.c
    ../../../client/common/archive/LZMA_SDK/C/7zFile.c
    ../../../client/common/archive/LZMA_SDK/C/7zStream.c
    ../../../client/common/archive/LZMA_SDK/C/Bcj2.c
    ../../../client/common/archive/LZMA_SDK/C/Bra.c
    ../../../client/common/archive/LZMA_SDK/C/Bra86.c
    ../../../client/common/archive/LZMA_SDK/C/BraIA64.c
    ../../../client/common/archive/LZMA_SDK/C/CpuArch.c
    ../../../client/common/archive/LZMA_SDK/C/Delta.c
    ../../../client/common/archive/LZMA_SDK/C/Lzma2Dec.c
    ../../../client/common/archive/LZMA_SDK/C/LzmaDec.c
    ../../../client/common/archive/LZMA_SDK/C/Ppmd7.c
    installer/installer.cpp
    installer/installer_shim.cpp
    installer/blocks/files.cpp
    installer/blocks/icons.cpp
    installer/blocks/install_authhelper.cpp
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
    Crypt32.lib
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Svg
)

target_include_directories(installer
    PRIVATE
    ../../../client/common
    ../../../client/common/utils
    ../../common
)
