cmake_minimum_required(VERSION 3.23)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_definitions(-DUNICODE -D_UNICODE)

project(install_helper)

set(SOURCES
    basic_command.cpp
    command_parser.cpp
    install_service_command.cpp
    logger.cpp
    windscribe_install_helper.cpp
    windscribe_install_helper.rc
    ../../../client/common/utils/servicecontrolmanager.cpp
)

add_compile_options("$<$<CONFIG:Release>:/Zi>")
add_link_options("$<$<CONFIG:Release>:/DEBUG>" "$<$<CONFIG:Release>:/OPT:REF>" "$<$<CONFIG:Release>:/OPT:ICF>")

add_executable(WindscribeInstallHelper ${SOURCES})
set_property(TARGET WindscribeInstallHelper PROPERTY LINK_FLAGS "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\"")

target_include_directories(WindscribeInstallHelper
    PRIVATE
    ../../../client/common/utils
)

install(TARGETS WindscribeInstallHelper
    RUNTIME DESTINATION .
)
