cmake_minimum_required(VERSION 3.23)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(X_VCPKG_APPLOCAL_DEPS_INSTALL ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "arm64")
    set(VCPKG_TARGET_TRIPLET arm64-windows-static)
    set(WINDSCRIBE_BUILD_LIBS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../build-libs-arm64)
else()
    set(VCPKG_TARGET_TRIPLET x64-windows-static)
    set(WINDSCRIBE_BUILD_LIBS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../build-libs)
endif()

# build_all.py sets this option when invoked with the '--sign' flag. Disabled by default
option(DEFINE_USE_SIGNATURE_CHECK_MACRO "Add define USE_SIGNATURE_CHECK to project" OFF)
if(DEFINE_USE_SIGNATURE_CHECK_MACRO)
    add_definitions(-DUSE_SIGNATURE_CHECK)
endif(DEFINE_USE_SIGNATURE_CHECK_MACRO)

add_definitions(-DUNICODE -D_UNICODE -DWINDSCRIBE_SERVICE -D_WIN32_WINNT=0x0601)

project(Service)

find_package(Boost REQUIRED COMPONENTS serialization)
find_package(spdlog CONFIG REQUIRED)
find_path(WINREG_INCLUDE_DIRS "winreg/WinReg.hpp")

add_subdirectory(../../../libs/wsnet wsnet)

set(SOURCES
    active_processes.cpp
    adapters_info.cpp
    apps_ids.cpp
    changeics/icsmanager.cpp
    close_tcp_connections.cpp
    dns_firewall.cpp
    executecmd.cpp
    firewallfilter.cpp
    fwpm_wrapper.cpp
    hostsedit.cpp
    ikev2ipsec.cpp
    ikev2route.cpp
    ioutils.cpp
    ipv6_firewall.cpp
    ip_address/ip4_address_and_mask.cpp
    ip_address/ip6_address_and_prefix.cpp
    network_category.cpp
    openvpncontroller.cpp
    ovpn.cpp
    pipe_for_process.cpp
    process_command.cpp
    registry.cpp
    reinstall_wan_ikev2.cpp
    remove_windscribe_network_profiles.cpp
    simple_xor_crypt.cpp
    split_tunneling/callout_filter.cpp
    split_tunneling/hostnames_manager/dns_resolver.cpp
    split_tunneling/hostnames_manager/hostnames_manager.cpp
    split_tunneling/hostnames_manager/ip_routes.cpp
    split_tunneling/ip_address_table.cpp
    split_tunneling/ip_forward_table.cpp
    split_tunneling/routes.cpp
    split_tunneling/routes_manager.cpp
    split_tunneling/split_tunneling.cpp
    split_tunneling/split_tunnel_service_manager.cpp
    utils.cpp
    windscribe_service.cpp
    wireguard/wireguardcontroller.cpp
    windscribe_service.rc
    ../../../client/common/utils/crashdump.cpp
    ../../../client/common/utils/crashhandler.cpp
    ../../../client/common/utils/executable_signature/executable_signature.cpp
    ../../../client/common/utils/executable_signature/executable_signature_win.cpp
    ../../../client/common/utils/servicecontrolmanager.cpp
)

add_compile_options("$<$<CONFIG:Release>:/Zi>")
add_link_options("$<$<CONFIG:Release>:/DEBUG>" "$<$<CONFIG:Release>:/OPT:REF>" "$<$<CONFIG:Release>:/OPT:ICF>")

add_executable(WindscribeService ${SOURCES})
set_property(TARGET WindscribeService PROPERTY COMPILE_WARNING_AS_ERROR ON)

target_link_libraries(WindscribeService
                      PRIVATE
                      Iphlpapi
                      version
                      rpcrt4
                      Fwpuclnt
                      Ws2_32
                      wsnet::wsnet
                      Boost::serialization
                      spdlog::spdlog
                      ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/oqs.lib
                      ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/oqsprovider.lib
)

target_include_directories(WindscribeService PRIVATE
                           ${WINDSCRIBE_BUILD_LIBS_PATH}/wintun/include
                           ../../../client/common
                           ${WINREG_INCLUDE_DIRS}
)

install(TARGETS WindscribeService
    RUNTIME DESTINATION .
)
