cmake_minimum_required(VERSION 3.23)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# build_all.py sets this option when invoked with the '--sign' flag. Disabled by default
option(DEFINE_USE_SIGNATURE_CHECK_MACRO "Add define USE_SIGNATURE_CHECK to project" OFF)
if(DEFINE_USE_SIGNATURE_CHECK_MACRO)
    add_definitions(-DUSE_SIGNATURE_CHECK)
endif(DEFINE_USE_SIGNATURE_CHECK_MACRO)

project(helper)

set(SOURCES
    ../../../client/common/utils/executable_signature/executable_signature.cpp
    ../../../client/common/utils/executable_signature/executablesignature_linux.cpp
    execute_cmd.cpp
    firewallcontroller.cpp
    ipc/helper_security.cpp
    logger.cpp
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
    split_tunneling/hostnames_manager/ares_library_init.cpp
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
target_link_directories(helper
                        PRIVATE
                        ../../../build-libs/boost/lib
                        ../../../build-libs/cares/lib
                        ../../../build-libs/openssl_ech_draft/lib
                        ../../../build-libs/openssl_ech_draft/lib64
)
target_link_libraries(helper
                      PRIVATE
                      libboost_serialization.a
                      libboost_thread.a
                      libboost_filesystem.a
                      ssl
                      crypto
                      pthread
                      cares
)

target_include_directories(helper PRIVATE
                           ../../posix_common
                           ../../../build-libs/boost/include
                           ../../../build-libs/cares/include
                           ../../../build-libs/openssl_ech_draft/include
                           ../../../client/common
)
