cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_definitions(-DUNICODE -D_UNICODE)

project(uninstaller)

set(SOURCES
    ../utils/applicationinfo.cpp
    ../utils/authhelper.cpp
    ../utils/directories_of_a_windows.cpp
    ../utils/directory.cpp
    ../utils/logger.cpp
    ../utils/path.cpp
    ../utils/paths_to_folders.cpp
    ../utils/process1.cpp
    ../utils/redirection.cpp
    ../utils/registry.cpp
    ../utils/remove_directory.cpp
    ../utils/services.cpp
    ../utils/utils.cpp
    ../utils/version_os.cpp
    arguments_parser.cpp
    copy_and_run.cpp
    main.cpp
    uninstall.cpp
    uninstall.rc
)

add_executable(uninstall WIN32 ${SOURCES})
set_property(TARGET uninstall PROPERTY LINK_FLAGS "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\" /DELAYLOAD:dwmapi.dll /DELAYLOAD:uxtheme.dll /DELAYLOAD:wldp.dll")

target_include_directories(uninstall PRIVATE
                           ../installer/client/common
)
target_link_libraries(uninstall
                      PRIVATE
                      delayimp
)