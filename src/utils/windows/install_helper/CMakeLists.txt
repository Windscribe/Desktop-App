cmake_minimum_required(VERSION 3.23)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_definitions(-DUNICODE -D_UNICODE)

project(install_helper)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/install_helper.rc.in
    ${CMAKE_CURRENT_BINARY_DIR}/install_helper.rc
    @ONLY
)

set(SOURCES
    basic_command.cpp
    command_parser.cpp
    install_service_command.cpp
    logger.cpp
    install_helper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/install_helper.rc
    ../../../client/client-common/utils/servicecontrolmanager.cpp
)

if(CI_MODE)
    add_compile_options("$<$<CONFIG:Release>:/Zi>")
    add_link_options("$<$<CONFIG:Release>:/DEBUG>" "$<$<CONFIG:Release>:/OPT:REF>" "$<$<CONFIG:Release>:/OPT:ICF>")
endif()

add_executable(InstallHelper ${SOURCES})
set_property(TARGET InstallHelper PROPERTY LINK_FLAGS "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\"")

target_include_directories(InstallHelper
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ../../../client/client-common
    ../../../client/client-common/utils
)

install(TARGETS InstallHelper
    RUNTIME DESTINATION .
)
