cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_definitions(-DUNICODE -D_UNICODE /wd4090 -D_CRT_SECURE_NO_WARNINGS)

project(installer)

set(SOURCES
    ../utils/applicationinfo.cpp
    ../utils/base64codec.cpp
    ../utils/directories_of_a_windows.cpp
    ../utils/directory.cpp
    ../utils/logger.cpp
    ../utils/path.cpp
    ../utils/paths_to_folders.cpp
    ../utils/process1.cpp
    ../utils/redirection.cpp
    ../utils/registry.cpp
    ../utils/services.cpp
    ../utils/utils.cpp
    ../utils/version_os.cpp
    ../utils/windscribepathcheck.cpp
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
    gui/application.cpp
    gui/closebutton.cpp
    gui/desktopshortcutcontrol.cpp
    gui/dpiscale.cpp
    gui/escbutton.cpp
    gui/eulabutton.cpp
    gui/factoryresetcontrol.cpp
    gui/fontresources.cpp
    gui/imageresources.cpp
    gui/installbutton.cpp
    gui/mainwindow.cpp
    gui/minimizebutton.cpp
    gui/pathcontrol.cpp
    gui/png/pngimage.cpp
    gui/settingsbutton.cpp
    gui/svg/svgimage.cpp
    gui/textitem.cpp
    installer/blocks/install_authhelper.cpp
    installer/iinstall_block.cpp
    installer/installer.cpp
    installer/blocks/files.cpp
    installer/blocks/fixadaptername.cpp
    installer/blocks/icons.cpp
    installer/blocks/install_splittunnel.cpp
    installer/blocks/install_tap.cpp
    installer/blocks/install_wintun.cpp
    installer/blocks/service.cpp
    installer/blocks/uninstallprev.cpp
    installer/blocks/uninstall_info.cpp
    installer/installer_base.cpp
    installer/settings.cpp
    installer/shellexecuteasuser.cpp
    resources/windscribe.7z
    installer.rc
    main.cpp
    ossupport.manifest
    version.rc
)

add_executable(installer WIN32 ${SOURCES})
set_property(TARGET installer PROPERTY VS_DPI_AWARE "PerMonitor")
set_property(TARGET installer PROPERTY COMPILE_FLAGS "/Gy /Oi /sdl")
set_property(TARGET installer PROPERTY LINK_FLAGS "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\" /DELAYLOAD:dwmapi.dll /DELAYLOAD:uxtheme.dll /DELAYLOAD:wldp.dll")

target_link_directories(installer
                        PRIVATE
                        ../../../build-libs/boost/lib
                        ../../../build-libs/cares/dll_x64/lib
                        ../../../build-libs/cares/static_x64/lib
)

target_link_libraries(installer
                      PRIVATE
                      Iphlpapi
                      version
                      rpcrt4
                      Fwpuclnt
                      Ws2_32
                      cares
                      delayimp
)

target_include_directories(installer PRIVATE
                           ../../../build-libs/boost/include
                           ../../../build-libs/cares/dll_x64/include
                           ../../../client/common/
)

