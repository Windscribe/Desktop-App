cmake_minimum_required(VERSION 3.23)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "arm64")
    set(WINDSCRIBE_BUILD_LIBS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../build-libs-arm64)
else()
    set(WINDSCRIBE_BUILD_LIBS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../build-libs)
endif()

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
                        ${WINDSCRIBE_BUILD_LIBS_PATH}/boost/lib
)
target_include_directories(WireguardService PRIVATE
                           ${WINDSCRIBE_BUILD_LIBS_PATH}/boost/include
                           ../../../client/common
)
