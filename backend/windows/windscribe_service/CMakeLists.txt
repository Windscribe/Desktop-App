cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "arm64")
    set(WINDSCRIBE_BUILD_LIBS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../build-libs-arm64)
else()
    set(WINDSCRIBE_BUILD_LIBS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../build-libs)
endif()

# build_all.py sets this option when invoked with the '--sign' flag. Disabled by default
option(DEFINE_USE_SIGNATURE_CHECK_MACRO "Add define USE_SIGNATURE_CHECK to project" OFF)
if(DEFINE_USE_SIGNATURE_CHECK_MACRO)
    add_definitions(-DUSE_SIGNATURE_CHECK)
endif(DEFINE_USE_SIGNATURE_CHECK_MACRO)

add_definitions(-DUNICODE -D_UNICODE -DWINDSCRIBE_SERVICE)

project(Service)

set(SOURCES
    adapters_info.cpp
    apps_ids.cpp
    cleardns_on_tap.cpp
    close_tcp_connections.cpp
    date_time_helper.cpp
    dns_firewall.cpp
    executecmd.cpp
    firewallfilter.cpp
    fwpm_wrapper.cpp
    get_active_processes.cpp
    hostsedit.cpp
    icsmanager.cpp
    ikev2ipsec.cpp
    ikev2route.cpp
    ioutils.cpp
    ipv6_firewall.cpp
    ip_address/ip4_address_and_mask.cpp
    ip_address/ip6_address_and_prefix.cpp
    logger.cpp
    ovpn.cpp
    pipe_for_process.cpp
    registry.cpp
    reinstall_tun_drivers.cpp
    reinstall_wan_ikev2.cpp
    remove_windscribe_network_profiles.cpp
    simple_xor_crypt.cpp
    split_tunneling/callout_filter.cpp
    split_tunneling/hostnames_manager/ares_library_init.cpp
    split_tunneling/hostnames_manager/dns_resolver.cpp
    split_tunneling/hostnames_manager/hostnames_manager.cpp
    split_tunneling/hostnames_manager/ip_routes.cpp
    split_tunneling/ip_address_table.cpp
    split_tunneling/ip_forward_table.cpp
    split_tunneling/routes.cpp
    split_tunneling/routes_manager.cpp
    split_tunneling/split_tunneling.cpp
    split_tunneling/split_tunnel_service_manager.cpp
    sys_ipv6_controller.cpp
    utils.cpp
    windscribe_service.cpp
    wireguard/wireguardcontroller.cpp
    ../../../client/common/utils/crashdump.cpp
    ../../../client/common/utils/crashhandler.cpp
    ../../../client/common/utils/executable_signature/executable_signature.cpp
    ../../../client/common/utils/executable_signature/executable_signature_win.cpp
    ../../../client/common/utils/servicecontrolmanager.cpp
    windscribe_service.rc
)

add_compile_options("$<$<CONFIG:Release>:/Zi>")
add_link_options("$<$<CONFIG:Release>:/DEBUG>" "$<$<CONFIG:Release>:/OPT:REF>" "$<$<CONFIG:Release>:/OPT:ICF>")

add_executable(WindscribeService ${SOURCES})

target_link_directories(WindscribeService
                        PRIVATE
                        ${WINDSCRIBE_BUILD_LIBS_PATH}/boost/lib
                        ${WINDSCRIBE_BUILD_LIBS_PATH}/cares//lib
)

target_link_libraries(WindscribeService
                      PRIVATE
                      Iphlpapi
                      version
                      rpcrt4
                      Fwpuclnt
                      Ws2_32
                      cares
)

target_include_directories(WindscribeService PRIVATE
                           ${WINDSCRIBE_BUILD_LIBS_PATH}/boost/include
                           ${WINDSCRIBE_BUILD_LIBS_PATH}/cares/include
                           ../../../client/common/
)
