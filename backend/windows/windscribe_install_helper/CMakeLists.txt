cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DUNICODE -D_UNICODE)

project(install_helper)

set(SOURCES
    basic_command.cpp
    command_parser.cpp
    install_service_command.cpp
    install_tap_command.cpp
    logger.cpp
    stdafx.cpp
    uninstall_tap_command.cpp
    windscribe_install_helper.cpp
    windscribe_install_helper.rc
)

add_compile_options("$<$<CONFIG:Release>:/Zi>")
add_link_options("$<$<CONFIG:Release>:/DEBUG>" "$<$<CONFIG:Release>:/OPT:REF>" "$<$<CONFIG:Release>:/OPT:ICF>")

add_executable(WindscribeInstallHelper ${SOURCES})
