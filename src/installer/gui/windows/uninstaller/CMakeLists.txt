cmake_minimum_required(VERSION 3.23)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_definitions(-DUNICODE -D_UNICODE)

project(uninstaller)

find_package(spdlog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

set(SOURCES
    arguments_parser.cpp
    copy_and_run.cpp
    main.cpp
    registry.cpp
    remove_directory.cpp
    uninstall.cpp
    ../../../../client/client-common/utils/servicecontrolmanager.cpp
    ../utils/applicationinfo.cpp
    ../utils/path.cpp
    ../utils/secure_dir.cpp
    ../utils/utils.cpp
    uninstall.rc
)

add_executable(uninstall WIN32 ${SOURCES})
set_property(TARGET uninstall PROPERTY LINK_FLAGS "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\"")

# Enable the process mitigations applied from the TLS callback in wsprocessmitigations.h (already
# included by main.cpp).  This elevated uninstaller deletes directories, so MicrosoftSignedOnly,
# image-load restrictions (anti DLL-planting) and redirection trust (anti symlink/junction
# redirection) are all apt.  MicrosoftSignedOnly is safe here because the uninstaller loads no
# non-Microsoft DLL.  The shell folder-path calls load only Microsoft-signed shell32.
# DisableExtensionPoints (EXTENSION_POINT) is opted out because it would block legacy assistive
# tech / IME for the confirmation dialogs.
target_compile_definitions(uninstall
                           PRIVATE
                           WS_ENABLE_PROCESS_MITIGATIONS
                           WS_DISABLE_EXTENSION_POINT_POLICY
)


target_include_directories(uninstall
    PRIVATE
    ../../../../client/client-common
    ../../../../client/client-common/types
    ../../../../client/client-common/utils
)

target_link_libraries(uninstall
                      PRIVATE
                      bcrypt
                      shlwapi
                      fmt::fmt-header-only
                      spdlog::spdlog_header_only
)

install(TARGETS uninstall
    RUNTIME DESTINATION .
)
