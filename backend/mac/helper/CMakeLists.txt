cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(com.windscribe.helper.macos)

# build_all.py sets this option when invoked with the '--sign' flag. Disabled by default
option(DEFINE_USE_SIGNATURE_CHECK_MACRO "Add define USE_SIGNATURE_CHECK to project" OFF)
if(DEFINE_USE_SIGNATURE_CHECK_MACRO)
    ADD_DEFINITIONS(-DUSE_SIGNATURE_CHECK)
endif(DEFINE_USE_SIGNATURE_CHECK_MACRO)

ADD_DEFINITIONS(-DBOOST_NO_CXX98_FUNCTION_BASE)

set(SOURCES
    ../../../client/common/utils/executable_signature/executable_signature.cpp
    ../../../client/common/utils/executable_signature/executable_signature.h
    ../../../client/common/utils/executable_signature/executable_signature_mac.mm
    ../../../client/common/utils/executable_signature/executable_signature.h
    3rdparty/pstream.h
    execute_cmd.cpp
    execute_cmd.h
    files_manager.cpp
    files_manager.h
    firewallcontroller.cpp
    firewallcontroller.h
    firewallonboot.cpp
    firewallonboot.h
    helper.cpp
    installer/archive/LZMA_SDK/C/7z.h
    installer/archive/LZMA_SDK/C/7zAlloc.c
    installer/archive/LZMA_SDK/C/7zAlloc.h
    installer/archive/LZMA_SDK/C/7zArcIn.c
    installer/archive/LZMA_SDK/C/7zBuf.c
    installer/archive/LZMA_SDK/C/7zBuf.h
    installer/archive/LZMA_SDK/C/7zCrc.c
    installer/archive/LZMA_SDK/C/7zCrc.h
    installer/archive/LZMA_SDK/C/7zCrcOpt.c
    installer/archive/LZMA_SDK/C/7zDec.c
    installer/archive/LZMA_SDK/C/7zFile.c
    installer/archive/LZMA_SDK/C/7zFile.h
    installer/archive/LZMA_SDK/C/7zStream.c
    installer/archive/LZMA_SDK/C/7zTypes.h
    installer/archive/LZMA_SDK/C/7zVersion.h
    installer/archive/LZMA_SDK/C/Bcj2.c
    installer/archive/LZMA_SDK/C/Bcj2.h
    installer/archive/LZMA_SDK/C/Bra.c
    installer/archive/LZMA_SDK/C/Bra.h
    installer/archive/LZMA_SDK/C/Bra86.c
    installer/archive/LZMA_SDK/C/BraIA64.c
    installer/archive/LZMA_SDK/C/Compiler.h
    installer/archive/LZMA_SDK/C/CpuArch.c
    installer/archive/LZMA_SDK/C/CpuArch.h
    installer/archive/LZMA_SDK/C/Delta.c
    installer/archive/LZMA_SDK/C/Delta.h
    installer/archive/LZMA_SDK/C/Lzma2Dec.c
    installer/archive/LZMA_SDK/C/Lzma2Dec.h
    installer/archive/LZMA_SDK/C/LzmaDec.c
    installer/archive/LZMA_SDK/C/LzmaDec.h
    installer/archive/LZMA_SDK/C/Ppmd.h
    installer/archive/LZMA_SDK/C/Ppmd7.c
    installer/archive/LZMA_SDK/C/Ppmd7.h
    installer/archive/LZMA_SDK/C/Precomp.h
    installer/archive/archive.cpp
    installer/archive/archive.h
    installer/files.cpp
    installer/files.h
    installer/iinstall_block.h
    ip_hostnames/ip_hostnames_manager.cpp
    ip_hostnames/ip_hostnames_manager.h
    ip_hostnames/ip_routes.cpp
    ip_hostnames/ip_routes.h
    ipc/helper_security.h
    ipc/helper_security.mm
    ipv6/ipv6manager.cpp
    ipv6/ipv6manager.h
    logger.cpp
    logger.h
    macspoofingonboot.cpp
    macspoofingonboot.h
    macutils.h
    macutils.mm
    ovpn.cpp
    ovpn.h
    process_command.cpp
    process_command.h
    routes_manager/bound_route.cpp
    routes_manager/bound_route.h
    routes_manager/routes.cpp
    routes_manager/routes.h
    routes_manager/routes_manager.cpp
    routes_manager/routes_manager.h
    server.cpp
    server.h
    split_tunneling/split_tunneling.cpp
    split_tunneling/split_tunneling.h
    utils.cpp
    utils.h
    wireguard/defaultroutemonitor.cpp
    wireguard/defaultroutemonitor.h
    wireguard/wireguardadapter.cpp
    wireguard/wireguardadapter.h
    wireguard/wireguardcommunicator.cpp
    wireguard/wireguardcommunicator.h
    wireguard/wireguardcontroller.cpp
    wireguard/wireguardcontroller.h
)

set(CMAKE_CXX_FLAGS "-fvisibility=hidden")

add_executable(com.windscribe.helper.macos ${SOURCES})
set_property(TARGET com.windscribe.helper.macos PROPERTY COMPILE_WARNING_AS_ERROR ON)

target_link_directories(com.windscribe.helper.macos
                        PRIVATE
                        ../../../build-libs/boost/lib
)
target_link_libraries(com.windscribe.helper.macos
                      PRIVATE
                      libboost_serialization.a
                      libboost_thread.a
                      "-framework Foundation"
                      "-framework Security"
                      "-framework SystemConfiguration"
                      "-sectcreate __TEXT __info_plist ${CMAKE_CURRENT_SOURCE_DIR}/helper-info.plist"
                      "-sectcreate __TEXT __launchd_plist ${CMAKE_CURRENT_SOURCE_DIR}/helper-launchd.plist"
)
set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++17")
set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
set(CMAKE_XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC "YES")
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Developer ID Application")
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Manual")
set(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ${DEVELOPMENT_TEAM})
set(CMAKE_XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES")
set(CMAKE_XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "11")
set(CMAKE_XCODE_ATTRIBUTE_PROVISIONING_PROFILE "")
set(CMAKE_XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "")
set(CMAKE_XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.windscribe.helper.macos")
set(CMAKE_XCODE_ATTRIBUTE_PRODUCT_BUNDLE_NAME "com.windscribe.helper.macos")

target_include_directories(com.windscribe.helper.macos PRIVATE
                           ../../../build-libs/boost/include
                           ../../../client/common
)
