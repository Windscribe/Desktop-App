# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2026, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.23)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)  # option() honors normal variables

# Set macOS deployment target and architecture before project()
set(CMAKE_OSX_DEPLOYMENT_TARGET "13" CACHE STRING "Minimum OS X deployment version")

# Detect and set macOS architecture to match host unless in CI mode (which builds universal)
if(APPLE AND NOT CI_MODE AND NOT DEFINED CMAKE_OSX_ARCHITECTURES)
    execute_process(
        COMMAND uname -m
        OUTPUT_VARIABLE MACOS_HOST_ARCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(CMAKE_OSX_ARCHITECTURES "${MACOS_HOST_ARCH}" CACHE STRING "macOS architecture")
endif()

# Enable vcpkg dependency installation
set(X_VCPKG_APPLOCAL_DEPS_INSTALL ON)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSVC runtime library (static linkage)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")


# ------------------------------------------------------------------------------
# Build Options (defaults match build_all.py behavior)
# ------------------------------------------------------------------------------

option(BUILD_APP "Build the main GUI application" ON)
option(BUILD_INSTALLER "Build the installer" ON)
option(BUILD_TESTS "Build test executables" OFF)
set(INTEGRATION_TYPE "gui" CACHE STRING "Integration type to build (e.g. gui, cli)")
option(SIGN_APP "Enable app code signing" OFF)
option(SIGN_INSTALLER "Enable installer code signing" OFF)
option(SIGN_BOOTSTRAP "Enable bootstrap code signing" OFF)
option(ENABLE_NOTARIZE "Enable macOS notarization" OFF)
option(CI_MODE "CI-specific behavior" OFF)
if(CI_MODE)
    set(DEFINE_USE_SIGNATURE_CHECK_MACRO ON CACHE BOOL "Add define USE_SIGNATURE_CHECK to project" FORCE)
endif()
option(BUILD_SYMBOLS "Collect PDB symbol files" OFF)
option(STATIC_ANALYSIS "Run clang-tidy static analysis" OFF)
option(UPDATE_TRANSLATIONS "Run lupdate to regenerate .ts translation files" OFF)

# Platform-specific build options (these use WIN32/APPLE which are set before project()
# when using a platform-native generator, e.g. Visual Studio on Windows)
if(WIN32)
    option(BUILD_BOOTSTRAP "Build the bootstrap (Windows only)" ON)
    option(BUILD_ARM64 "Build for ARM64 architecture" OFF)
elseif(UNIX AND NOT APPLE)
    # Linux package type options (default to DEB)
    option(BUILD_DEB "Build Debian package" ON)
    option(BUILD_RPM "Build RPM package" OFF)
    option(BUILD_RPM_OPENSUSE "Build OpenSUSE RPM package" OFF)
endif()

# Default behavior: if no operations specified, enable BUILD_APP, BUILD_INSTALLER, BUILD_BOOTSTRAP
if(NOT BUILD_APP AND NOT BUILD_INSTALLER AND NOT BUILD_BOOTSTRAP AND NOT SIGN_APP AND NOT SIGN_INSTALLER AND NOT SIGN_BOOTSTRAP)
    message(STATUS "No build operations specified, defaulting to BUILD_APP, BUILD_INSTALLER, BUILD_BOOTSTRAP")
    set(BUILD_APP ON CACHE BOOL "Build the main GUI application" FORCE)
    set(BUILD_INSTALLER ON CACHE BOOL "Build the installer" FORCE)
    if(WIN32)
        set(BUILD_BOOTSTRAP ON CACHE BOOL "Build the bootstrap (Windows only)" FORCE)
    endif()
endif()

# Debug/Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# Static Analysis Configuration
if(STATIC_ANALYSIS)
    set(CMAKE_CXX_CLANG_TIDY
        "clang-tidy"
        "--checks=clang-analyzer-*,-clang-analyzer-optin.cplusplus.VirtualCall,-clang-diagnostic-deprecated-declarations,-clang-analyzer-osx.cocoa.RetainCount"
        "--header-filter=.*/(src|libs)/.*"
        "--exclude-header-filter=.*vcpkg/.*"
        "--warnings-as-errors=*"
    )
endif()

# ------------------------------------------------------------------------------
# Project Configuration
# ------------------------------------------------------------------------------

# vcpkg integration (only needed for build operations, must be before project())
if(BUILD_APP OR BUILD_INSTALLER OR BUILD_BOOTSTRAP)
    include(cmake/vcpkg.cmake)
endif()

project(WS LANGUAGES CXX C)

include(cmake/architectures.cmake)

# Load integration configuration (after architectures.cmake so that
# WINDSCRIBE_BUILD_LIBS_PATH and PACKAGE_ARCH are available)
set(WS_INTEGRATION_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/cmake/integrations/${INTEGRATION_TYPE}.cmake")
if(NOT EXISTS "${WS_INTEGRATION_CONFIG}")
    message(FATAL_ERROR "Integration config not found: ${WS_INTEGRATION_CONFIG}")
endif()
include("${WS_INTEGRATION_CONFIG}")

set(PROJECT_VERSION "${WS_VERSION_MAJOR}.${WS_VERSION_MINOR}.${WS_VERSION_BUILD}" CACHE STRING "" FORCE)

include(cmake/utilities.cmake)

# Derive build suffix from build type (empty for stable releases)
if(WS_BUILD_TYPE STREQUAL "stable")
    set(WS_BUILD_SUFFIX "")
else()
    set(WS_BUILD_SUFFIX "${WS_BUILD_TYPE}")
endif()

# Expose branding to C++ code
add_compile_definitions(
    WS_APP_EXECUTABLE_NAME="${WS_APP_EXECUTABLE_NAME}"
    WS_CLI_EXECUTABLE_NAME="${WS_CLI_EXECUTABLE_NAME}"
    WS_APP_IDENTIFIER="${WS_APP_IDENTIFIER}"
    WS_COPYRIGHT="${WS_COPYRIGHT}"
    WS_EXTRA_CONFIG_NAME="${WS_EXTRA_CONFIG_NAME}"
    WS_LINUX_INSTALL_DIR="${WS_LINUX_INSTALL_DIR}"
    WS_LINUX_LOG_DIR="${WS_LINUX_LOG_DIR}"
    WS_LINUX_TMP_DIR="${WS_LINUX_TMP_DIR}"
    WS_MAC_APP_DIR="${WS_MAC_APP_DIR}"
    WS_MAC_GID="${WS_MAC_GID}"
    WS_MAC_GUI_BUNDLE_ID="${WS_MAC_GUI_BUNDLE_ID}"
    WS_MAC_HELPER_BUNDLE_ID="${WS_MAC_HELPER_BUNDLE_ID}"
    WS_MAC_HELPER_BUNDLE_ID_PATH="Contents/Library/LaunchServices/${WS_MAC_HELPER_BUNDLE_ID}"
    WS_MAC_IKEV2_KEYCHAIN_SERVICE="${WS_MAC_IKEV2_KEYCHAIN_SERVICE}"
    WS_MAC_INSTALLER_BUNDLE_ID="${WS_MAC_INSTALLER_BUNDLE_ID}"
    WS_MAC_INSTALLER_BUNDLE_NAME="${WS_MAC_INSTALLER_BUNDLE_NAME}"
    WS_MAC_INSTALLER_INNER_BINARY="Contents/MacOS/${WS_MAC_INSTALLER_TARGET}"
    WS_MAC_LAUNCHER_BUNDLE_ID="${WS_MAC_LAUNCHER_BUNDLE_ID}"
    WS_MAC_SPLIT_TUNNEL_BUNDLE_ID="${WS_MAC_SPLIT_TUNNEL_BUNDLE_ID}"
    WS_MAC_UID="${WS_MAC_UID}"
    WS_MAC_VPN_DESCRIPTION="${WS_MAC_VPN_DESCRIPTION}"
    WS_POSIX_CONFIG_DIR="${WS_POSIX_CONFIG_DIR}"
    WS_POSIX_RUN_DIR="${WS_POSIX_RUN_DIR}"
    WS_PRODUCT_NAME="${WS_PRODUCT_NAME}"
    WS_PRODUCT_NAME_LOWER="${WS_PRODUCT_NAME_LOWER}"
    WS_PRODUCT_NAME_UPPER="${WS_PRODUCT_NAME_UPPER}"
    WS_SETTINGS_APP="${WS_SETTINGS_APP}"
    WS_SETTINGS_ORG="${WS_SETTINGS_ORG}"
    WS_VERSION="${WS_VERSION_MAJOR}.${WS_VERSION_MINOR}.${WS_VERSION_BUILD}"
    WS_VERSION_BUILD=${WS_VERSION_BUILD}
    WS_VERSION_MAJOR=${WS_VERSION_MAJOR}
    WS_VERSION_MAJOR_MINOR="${WS_VERSION_MAJOR}.${WS_VERSION_MINOR}"
    WS_VERSION_MINOR=${WS_VERSION_MINOR}
    WS_WIN_CONFIG_SUBDIR="${WS_WIN_CONFIG_SUBDIR}"
    WS_WIN_IKEV2_CONNECTION_NAME="${WS_WIN_IKEV2_CONNECTION_NAME}"
)
if(DEFINED WS_COMPILE_DEFINITIONS)
    add_compile_definitions(${WS_COMPILE_DEFINITIONS})
endif()
if(WS_BUILD_TYPE STREQUAL "guinea_pig")
    add_compile_definitions(WINDSCRIBE_IS_GUINEA_PIG)
elseif(WS_BUILD_TYPE STREQUAL "beta")
    add_compile_definitions(WINDSCRIBE_IS_BETA)
endif()

if(NOT CMAKE_SYSTEM_NAME IN_LIST WS_SUPPORTED_PLATFORMS)
    message(FATAL_ERROR "Integration '${INTEGRATION_TYPE}' does not support ${CMAKE_SYSTEM_NAME}. Supported: ${WS_SUPPORTED_PLATFORMS}")
endif()

# ------------------------------------------------------------------------------
# Build Configuration Summary
# ------------------------------------------------------------------------------

message(STATUS "")
message(STATUS "==================================================")
message(STATUS "Windscribe Build Configuration Summary")
message(STATUS "==================================================")
message(STATUS "Version:              ${PROJECT_VERSION}")
message(STATUS "Build Variant:        ${WS_BUILD_TYPE}")
message(STATUS "Build Type:           ${CMAKE_BUILD_TYPE}")
message(STATUS "Platform:             ${CMAKE_SYSTEM_NAME}")
if(APPLE)
message(STATUS "Architecture:         ${CMAKE_OSX_ARCHITECTURES}")
else()
message(STATUS "Architecture:         ${CMAKE_SYSTEM_PROCESSOR}")
endif()
message(STATUS "CI Mode:              ${CI_MODE}")
message(STATUS "")
message(STATUS "Build Options:")
message(STATUS "  Integration:        ${INTEGRATION_TYPE}")
if(UNIX AND NOT APPLE)
message(STATUS "  Build DEB:          ${BUILD_DEB}")
message(STATUS "  Build RPM:          Fedora ${BUILD_RPM} OpenSUSE ${BUILD_RPM_OPENSUSE}")
endif()
message(STATUS "  Build Tests:        ${BUILD_TESTS}")
message(STATUS "  Static Analysis:    ${STATIC_ANALYSIS}")
message(STATUS "  Translations:       ${UPDATE_TRANSLATIONS}")
if(APPLE)
message(STATUS "  Notarization:       ${ENABLE_NOTARIZE}")
endif()
message(STATUS "==================================================")
message(STATUS "")

if(BUILD_APP OR BUILD_INSTALLER OR BUILD_BOOTSTRAP)
    install_vcpkg_dependencies()
endif()

# ------------------------------------------------------------------------------
# Custom Dependencies (WireGuard, WSTunnel, Wintun, OpenVPN DCO)
# ------------------------------------------------------------------------------

# Verify WINDSCRIBE_BUILD_LIBS_PATH is set (from architectures.cmake)
if(NOT DEFINED WINDSCRIBE_BUILD_LIBS_PATH)
    message(FATAL_ERROR "WINDSCRIBE_BUILD_LIBS_PATH is not defined")
endif()

# Check if custom dependencies directory exists
if(NOT EXISTS "${WINDSCRIBE_BUILD_LIBS_PATH}")
    message(WARNING "Dependencies directory not found: ${WINDSCRIBE_BUILD_LIBS_PATH}")
endif()

# ------------------------------------------------------------------------------
ws_resolve_bundled_helpers()
ws_resolve_shared_libs()

# ------------------------------------------------------------------------------
# Code Signing Configuration
# ------------------------------------------------------------------------------

include(cmake/signing.cmake)

# ------------------------------------------------------------------------------
# Build Components
# ------------------------------------------------------------------------------

if(BUILD_APP)
    if(BUILD_TESTS)
        set(IS_BUILD_TESTS ON)
    endif()

    # Additional components from integration config
    foreach(_subdir ${WS_ADDITIONAL_SUBDIRS})
        add_subdirectory(${_subdir})
    endforeach()

    add_subdirectory(src/helper)
    add_subdirectory(src/client)

    # Load wsnet from the separate repository
    include("${CMAKE_SOURCE_DIR}/cmake/fetch_wsnet.cmake")

    if(WIN32)
        add_custom_target(build-app ALL DEPENDS ${WS_WIN_APP_TARGETS})
    elseif(APPLE)
        add_custom_target(build-app ALL DEPENDS ${WS_MAC_APP_TARGETS})
    elseif(UNIX)
        add_custom_target(build-app ALL DEPENDS ${WS_LINUX_APP_TARGETS})
    endif()
endif()

# Installers
if(BUILD_INSTALLER)
    if(WIN32)
        add_subdirectory(src/installer/${INSTALLER_TYPE}/windows/installer)
    elseif(APPLE)
        add_subdirectory(src/installer/${INSTALLER_TYPE}/macos/installer)
    endif()
endif()

if(BUILD_BOOTSTRAP)
    if(WIN32)
        ws_resolve_output_name("${WS_WIN_OUTPUT_NAME}" _resolved_installer_name)
        set(WINDSCRIBE_INSTALLER_NAME "${_resolved_installer_name}.exe" CACHE STRING "Name of internal installer exe")
        add_subdirectory(src/installer/${INSTALLER_TYPE}/windows/bootstrap)
    endif()
endif()

# ------------------------------------------------------------------------------
# Packaging and Signing
# ------------------------------------------------------------------------------

set(BUILD_EXE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build-exe")
set(BUILD_TEMP_DIR "${CMAKE_BINARY_DIR}/temp")
set(BUILD_INSTALLER_FILES "${BUILD_TEMP_DIR}/InstallerFiles")
set(BUILD_SYMBOLS_DIR "${BUILD_TEMP_DIR}/SymbolFiles")
set(BUILD_BOOTSTRAP_FILES "${BUILD_TEMP_DIR}/BootstrapFiles")

file(MAKE_DIRECTORY "${BUILD_TEMP_DIR}")
file(MAKE_DIRECTORY "${BUILD_INSTALLER_FILES}")

# Include platform-specific packaging configuration
if(WIN32)
    include(cmake/packaging_windows.cmake)
elseif(APPLE)
    include(cmake/packaging_macos.cmake)
elseif(UNIX)
    include(cmake/packaging_linux.cmake)
endif()

