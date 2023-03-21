cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(helper)

set(SOURCES
    ../../../client/common/utils/executable_signature/executable_signature.cpp
    ../../../client/common/utils/executable_signature/executablesignature_linux.cpp
    execute_cmd.cpp
    ipc/helper_security.cpp
    logger.cpp
    main.cpp
    ovpn.cpp
    server.cpp
    utils.cpp
    routes_manager/routes.cpp
    routes_manager/routes_manager.cpp
    wireguard/defaultroutemonitor.cpp
    wireguard/wireguardadapter.cpp
    wireguard/userspace/wireguardgocommunicator.cpp
    wireguard/kernelmodule/kernelmodulecommunicator.cpp
    wireguard/kernelmodule/wireguard.c
    wireguard/wireguardcontroller.cpp
)

add_executable(helper ${SOURCES})
target_link_directories(helper
                        PRIVATE
                        ../../../build-libs/boost/lib
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
)

target_include_directories(helper PRIVATE
                           ../../../build-libs/boost/include
                           ../../../build-libs/openssl_ech_draft/include
                           ../../../client/common
)
