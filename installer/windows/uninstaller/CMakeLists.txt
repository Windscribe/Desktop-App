cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_definitions(-DUNICODE -D_UNICODE)

project(uninstaller)

set(SOURCES
    arguments_parser.cpp
    authhelper.cpp
    copy_and_run.cpp
    main.cpp
    registry.cpp
    remove_directory.cpp
    uninstall.cpp
    ../../../client/common/utils/servicecontrolmanager.cpp
    ../utils/applicationinfo.cpp
    ../utils/logger.cpp
    ../utils/path.cpp
    ../utils/utils.cpp
    uninstall.rc
)

add_executable(uninstall WIN32 ${SOURCES})
set_property(TARGET uninstall PROPERTY LINK_FLAGS "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\" /DELAYLOAD:dwmapi.dll /DELAYLOAD:uxtheme.dll /DELAYLOAD:wldp.dll")


target_include_directories(uninstall
    PRIVATE
    ../../../client/common
    ../../../client/common/utils
)

target_link_libraries(uninstall
                      PRIVATE
                      delayimp
)
