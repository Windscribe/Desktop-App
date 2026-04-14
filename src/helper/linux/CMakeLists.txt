# build_all.py sets this option when invoked with the '--sign' flag. Disabled by default
option(DEFINE_USE_SIGNATURE_CHECK_MACRO "Add define USE_SIGNATURE_CHECK to project" OFF)
if(DEFINE_USE_SIGNATURE_CHECK_MACRO)
    add_definitions(-DUSE_SIGNATURE_CHECK)
endif(DEFINE_USE_SIGNATURE_CHECK_MACRO)

project(helper)

find_package(Boost CONFIG REQUIRED COMPONENTS serialization thread filesystem)
find_package(OpenSSL REQUIRED)
find_package(skyr-url CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

set(SOURCES
    clear_wifi_history/clear_wifi_history.cpp
    execute_cmd.cpp
    firewallcontroller.cpp
    firewallonboot.cpp
    main.cpp
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
)

target_include_directories(helper PRIVATE
                           ../../posix_common
                           ../../client/client-common
)

install(TARGETS helper
    RUNTIME DESTINATION .
)

