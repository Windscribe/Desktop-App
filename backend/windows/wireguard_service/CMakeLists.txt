cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DUNICODE -D_UNICODE -DBOOST_AUTO_LINK_TAGGED)

project(wireguard_service)

set(SOURCES
    ServiceMain.cpp
    ../../../client/common/utils/servicecontrolmanager.cpp
)

add_compile_options("$<$<CONFIG:Release>:/Zi>")
add_link_options("$<$<CONFIG:Release>:/DEBUG>" "$<$<CONFIG:Release>:/OPT:REF>" "$<$<CONFIG:Release>:/OPT:ICF>")

add_executable(WireguardService ${SOURCES})

target_link_directories(WireguardService
                        PRIVATE
                        ../../../build-libs/boost/lib
)
target_include_directories(WireguardService PRIVATE
                           ../../../build-libs/boost/include
                           ../../../client/common
)
