project(helper)

find_package(Boost CONFIG REQUIRED COMPONENTS serialization thread filesystem)
find_package(OpenSSL REQUIRED)
find_package(skyr-url CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

# libnftables (system library): the helper manages the firewall via libnftables instead of shelling
# out. Discovered with plain CMake find_* (the project intentionally does not use pkg-config). Build
# hosts need the dev headers: libnftables-dev (Debian/Ubuntu), nftables-devel (Fedora/openSUSE),
# nftables (Arch; headers ship in the main package). The runtime library is declared as a package
# dependency in the Linux installers.
find_library(NFTABLES_LIB nftables REQUIRED)
find_path(NFTABLES_INCLUDE_DIR nftables/libnftables.h REQUIRED)

set(SOURCES
    ../../client/client-common/types/ipaddress.cpp
    ../common/io_posix.cpp
    ../common/ovpn_mgmt_posix.cpp
    ../common/spawn_posix.cpp
    ../common/validation_posix.cpp
    clear_wifi_history/clear_wifi_history.cpp
    dnsleakprotect.cpp
    firewallcontroller.cpp
    firewallonboot.cpp
    installer_verifier.cpp
    main.cpp
    nftables/nftablescontroller.cpp
    ovpn.cpp
    process_command.cpp
    server.cpp
    utils.cpp
    routes_manager/bound_route.cpp
    routes_manager/routes.cpp
    routes_manager/routes_manager.cpp
    split_tunneling/cgroups.cpp
    split_tunneling/process_monitor.cpp
    split_tunneling/split_tunneling.cpp
    split_tunneling/hostnames_manager/dns_resolver.cpp
    split_tunneling/hostnames_manager/hostnames_manager.cpp
    split_tunneling/hostnames_manager/ip_routes.cpp
    wireguard/defaultroutemonitor.cpp
    wireguard/wireguardadapter.cpp
    wireguard/userspace/wireguardgocommunicator.cpp
    wireguard/kernelmodule/kernelmodulecommunicator.cpp
    wireguard/kernelmodule/wireguard.c
    wireguard/wireguardcontroller.cpp
)

add_executable(helper ${SOURCES})
set_property(TARGET helper PROPERTY COMPILE_WARNING_AS_ERROR ON)

target_link_libraries(helper
                      PRIVATE
                      pthread
                      wsnet::wsnet
                      Boost::serialization
                      Boost::thread
                      Boost::filesystem
                      OpenSSL::Crypto
                      skyr::skyr-url
                      spdlog::spdlog
                      ${NFTABLES_LIB}
)

target_include_directories(helper PRIVATE
                           ../../posix_common
                           ../../client/client-common
                           ${NFTABLES_INCLUDE_DIR}
)

install(TARGETS helper
    RUNTIME DESTINATION .
)

