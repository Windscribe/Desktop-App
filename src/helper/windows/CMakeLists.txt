option(DEFINE_RUN_AS_CONSOLE_APP_MACRO "Run the helper as a console app for debugging" OFF)
if(DEFINE_RUN_AS_CONSOLE_APP_MACRO)
    add_definitions(-DRUN_AS_CONSOLE_APP)
endif(DEFINE_RUN_AS_CONSOLE_APP_MACRO)

add_definitions(-DUNICODE -D_UNICODE -DWINDSCRIBE_SERVICE -DWIN32_LEAN_AND_MEAN)

find_package(Boost REQUIRED COMPONENTS serialization)
find_package(spdlog CONFIG REQUIRED)
find_path(WINREG_INCLUDE_DIRS "winreg/WinReg.hpp")

set(SOURCES
    active_processes.cpp
    adapters_info.cpp
    apps_ids.cpp
    clear_wifi_history/clear_wifi_history.cpp
    changeics/icsmanager.cpp
    close_tcp_connections.cpp
    dns_firewall.cpp
    dohdata.cpp
    executecmd.cpp
    firewallfilter.cpp
    firewallonboot.cpp
    fwpm_wrapper.cpp
    helper.cpp
    hostsedit.cpp
    ikev2ipsec.cpp
    ikev2route.cpp
    ipv6_firewall.cpp
    macaddressspoof.cpp
    main.cpp
    network_validation.cpp
    openvpncontroller.cpp
    pipe_for_process.cpp
    process_command.cpp
    reinstall_wan_ikev2.cpp
    remove_app_network_profiles.cpp
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
    win32service.cpp
    wireguard/phantom_wireguard_devices.cpp
    wireguard/wireguardcontroller.cpp
    wmi_utils.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/version.rc
    ../../client/client-common/utils/crashdump.cpp
    ../../client/client-common/utils/crashhandler.cpp
    ../../client/client-common/utils/executable_signature/executable_signature.cpp
    ../../client/client-common/utils/executable_signature/executable_signature_win.cpp
    ../../client/client-common/types/ipaddress.cpp
    ../../client/client-common/utils/servicecontrolmanager.cpp
)

if(CI_MODE)
    add_compile_options("$<$<CONFIG:Release>:/Zi>")
    add_link_options("$<$<CONFIG:Release>:/DEBUG>" "$<$<CONFIG:Release>:/OPT:REF>" "$<$<CONFIG:Release>:/OPT:ICF>")
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version.rc.in ${CMAKE_CURRENT_BINARY_DIR}/version.rc @ONLY)

add_executable(${WS_APP_IDENTIFIER}Service ${SOURCES})
set_property(TARGET ${WS_APP_IDENTIFIER}Service PROPERTY COMPILE_WARNING_AS_ERROR ON)

# Enable the process mitigations applied from the TLS callback in wsprocessmitigations.h.  This is
# a native, statically-linked (/MT) SYSTEM service with no Qt.  MicrosoftSignedOnly (SIGNATURE) and
# DisableExtensionPoints (EXTENSION_POINT) are opted out: the service does in-process networking via
# wsnet and uses ICS (netshell COM) and WMI, which can legitimately load third-party Winsock LSPs /
# DNS NSPs and in-proc COM handlers depending on the end user's machine.  The remaining policies
# (dynamic code, image-load, strict handle, redirection trust) are environment-independent.
target_compile_definitions(${WS_APP_IDENTIFIER}Service
                           PRIVATE
                           WS_ENABLE_PROCESS_MITIGATIONS
                           WS_DISABLE_SIGNATURE_POLICY
                           WS_DISABLE_EXTENSION_POINT_POLICY
)

target_link_libraries(${WS_APP_IDENTIFIER}Service
                      PRIVATE
                      Dnsapi
                      Iphlpapi
                      rpcrt4
                      Fwpuclnt
                      Ws2_32
                      Wtsapi32
                      Setupapi
                      wbemuuid
                      ntdll
                      Newdev
                      Shlwapi
                      wevtapi
                      wsnet::wsnet
                      Boost::serialization
                      spdlog::spdlog
)

target_include_directories(${WS_APP_IDENTIFIER}Service PRIVATE
                           ../../client/client-common
                           ${WINREG_INCLUDE_DIRS}
)

install(TARGETS ${WS_APP_IDENTIFIER}Service
    RUNTIME DESTINATION .
)

add_subdirectory(tests)
