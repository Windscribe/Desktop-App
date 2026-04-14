cmake_minimum_required(VERSION 3.23)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_definitions(-DUNICODE -D_UNICODE)

project(wireguard_service)

find_package(spdlog CONFIG REQUIRED)


configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/wireguard_service.rc.in
    ${CMAKE_CURRENT_BINARY_DIR}/wireguard_service.rc
    @ONLY
)

set(SOURCES
    ServiceMain.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/wireguard_service.rc
    ../../../client/client-common/utils/servicecontrolmanager.cpp
)

if(CI_MODE)
    add_compile_options("$<$<CONFIG:Release>:/Zi>")
    add_link_options("$<$<CONFIG:Release>:/DEBUG>" "$<$<CONFIG:Release>:/OPT:REF>" "$<$<CONFIG:Release>:/OPT:ICF>")
endif()

add_executable(WireguardService ${SOURCES})

target_include_directories(WireguardService PRIVATE
                           ../../../client/client-common
)

target_link_libraries(WireguardService PRIVATE
                      spdlog::spdlog
)

install(TARGETS WireguardService
    RUNTIME DESTINATION .
)
