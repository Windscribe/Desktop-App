cmake_minimum_required(VERSION 3.23)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(installer)

find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS
             Widgets
             Gui
             Svg
             LinguistTools)
qt_standard_project_setup(REQUIRES 6.5)

add_definitions(-DUNICODE -D_UNICODE /wd4090)

set(SOURCES
    ../../../../client/client-common/utils/languagesutil.cpp
    ../../../../client/client-common/utils/servicecontrolmanager.cpp
    ../../../../client/client-common/utils/wincryptutils.cpp
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
    installer/blocks/install_tap_windows6.cpp
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
    version.rc
)

qt_add_resources(rc ../../installer-gui-common/installer_win.qrc)
qt_add_executable(installer WIN32 ${SOURCES} ${rc})
add_subdirectory(../../installer-gui-common ${CMAKE_CURRENT_BINARY_DIR}/common)

set_source_files_properties(installer.rc OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/resources/windscribe.7z)
add_dependencies(installer prep-installer-windows)
set_property(TARGET installer PROPERTY COMPILE_WARNING_AS_ERROR ON)
set_property(TARGET installer PROPERTY VS_DPI_AWARE "PerMonitor")
set_property(TARGET installer PROPERTY COMPILE_FLAGS "/Gy /Oi")
set_property(TARGET installer PROPERTY LINK_FLAGS "/MANIFESTUAC:\"level='asInvoker' uiAccess='false'\"")

# Minimal process-mitigation profile from the TLS callback in wsprocessmitigations.h (included by
# main.cpp).  This is an elevated (bootstrap-inherited token) Qt GUI app that writes to Program
# Files, so image-load restrictions (anti DLL-planting) and redirection trust are apt.  Only those
# two are enabled: SIGNATURE is off (GPU driver DLLs are vendor-signed, not Microsoft), DYNAMIC_CODE
# off (graphics paths may use dynamic code), EXTENSION_POINT off (assistive tech / IME), and
# STRICT_HANDLE_CHECK off (large framework, not worth the crash risk on a GUI app).
target_compile_definitions(installer
                           PRIVATE
                           WS_ENABLE_PROCESS_MITIGATIONS
                           WS_DISABLE_SIGNATURE_POLICY
                           WS_DISABLE_DYNAMIC_CODE_POLICY
                           WS_DISABLE_EXTENSION_POINT_POLICY
                           WS_DISABLE_STRICT_HANDLE_CHECK_POLICY
)

target_link_directories(installer PRIVATE)

target_link_libraries(installer
    PRIVATE
    Iphlpapi
    version
    rpcrt4
    Fwpuclnt
    Ws2_32
    Crypt32
    spdlog::spdlog_header_only
    fmt::fmt-header-only
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Svg
)

target_include_directories(installer
    PRIVATE
    ../../../../client/client-common
    ../../../../client/client-common/types
    ../../../../client/client-common/utils
    ../../installer-gui-common
)

install(TARGETS installer
    RUNTIME DESTINATION .
)

