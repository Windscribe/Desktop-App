cmake_minimum_required(VERSION 3.23)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "arm64")
    set(WINDSCRIBE_BUILD_LIBS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../build-libs-arm64)
else()
    set(WINDSCRIBE_BUILD_LIBS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../build-libs)
endif()

add_definitions(-DUNICODE -D_UNICODE)

project(wireguard_service)

set(SOURCES
    ServiceMain.cpp
    wireguard_service.rc
    ../../../client/common/utils/servicecontrolmanager.cpp
)

add_compile_options("$<$<CONFIG:Release>:/Zi>")
add_link_options("$<$<CONFIG:Release>:/DEBUG>" "$<$<CONFIG:Release>:/OPT:REF>" "$<$<CONFIG:Release>:/OPT:ICF>")

add_executable(WireguardService ${SOURCES})

target_include_directories(WireguardService PRIVATE
                           ../../../client/common
)

install(TARGETS WireguardService
    RUNTIME DESTINATION .
)
