# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2025, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.23)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)  # option() honors normal variables

# Set macOS deployment target and architecture before project()
set(CMAKE_OSX_DEPLOYMENT_TARGET "12" CACHE STRING "Minimum OS X deployment version")

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

# Read version from header file
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/src/client/common/version/windscribe_version.h"
     VERSION_MAJOR_LINE REGEX "^#define WINDSCRIBE_MAJOR_VERSION")
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/src/client/common/version/windscribe_version.h"
     VERSION_MINOR_LINE REGEX "^#define WINDSCRIBE_MINOR_VERSION")
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/src/client/common/version/windscribe_version.h"
     VERSION_BUILD_LINE REGEX "^#define WINDSCRIBE_BUILD_VERSION")

string(REGEX REPLACE "^#define WINDSCRIBE_MAJOR_VERSION ([0-9]+).*" "\\1"
       WINDSCRIBE_VERSION_MAJOR "${VERSION_MAJOR_LINE}")
string(REGEX REPLACE "^#define WINDSCRIBE_MINOR_VERSION ([0-9]+).*" "\\1"
       WINDSCRIBE_VERSION_MINOR "${VERSION_MINOR_LINE}")
string(REGEX REPLACE "^#define WINDSCRIBE_BUILD_VERSION ([0-9]+).*" "\\1"
       WINDSCRIBE_VERSION_BUILD "${VERSION_BUILD_LINE}")

# Detect build type (guinea pig, beta, or stable)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/client/common/version/windscribe_version.h" VERSION_FILE_CONTENTS)
if(VERSION_FILE_CONTENTS MATCHES "^[^/]*#define WINDSCRIBE_IS_GUINEA_PIG" OR
   VERSION_FILE_CONTENTS MATCHES "\n[^/]*#define WINDSCRIBE_IS_GUINEA_PIG")
    set(WINDSCRIBE_BUILD_TYPE "guinea_pig")
    set(WINDSCRIBE_BUILD_SUFFIX "_guinea_pig")
elseif(VERSION_FILE_CONTENTS MATCHES "^[^/]*#define WINDSCRIBE_IS_BETA" OR
       VERSION_FILE_CONTENTS MATCHES "\n[^/]*#define WINDSCRIBE_IS_BETA")
    set(WINDSCRIBE_BUILD_TYPE "beta")
    set(WINDSCRIBE_BUILD_SUFFIX "_beta")
else()
    set(WINDSCRIBE_BUILD_TYPE "stable")
    set(WINDSCRIBE_BUILD_SUFFIX "")
endif()

# ------------------------------------------------------------------------------
# Build Options (defaults match build_all.py behavior)
# ------------------------------------------------------------------------------

option(BUILD_APP "Build the main GUI application" ON)
option(BUILD_INSTALLER "Build the installer" ON)
option(BUILD_CLI_ONLY "Build CLI-only version (Linux)" OFF)
option(BUILD_TESTS "Build test executables" OFF)
option(SIGN_APP "Enable app code signing" OFF)
option(SIGN_INSTALLER "Enable installer code signing" OFF)
option(SIGN_BOOTSTRAP "Enable bootstrap code signing" OFF)
option(ENABLE_NOTARIZE "Enable macOS notarization" OFF)
option(CI_MODE "CI-specific behavior" OFF)
option(STATIC_ANALYSIS "Run clang-tidy static analysis" OFF)

# Platform-specific build options
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

include(cmake/architectures.cmake)

# Static Analysis Configuration
if(STATIC_ANALYSIS)
    set(CMAKE_CXX_CLANG_TIDY
        "clang-tidy"
        "--checks=-clang-analyzer-optin.cplusplus.VirtualCall,-clang-diagnostic-deprecated-declarations,-clang-analyzer-osx.cocoa.RetainCount"
        "--allow-no-checks"
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

project(Windscribe
    VERSION ${WINDSCRIBE_VERSION_MAJOR}.${WINDSCRIBE_VERSION_MINOR}.${WINDSCRIBE_VERSION_BUILD}
    DESCRIPTION "Windscribe VPN Client"
    LANGUAGES CXX C
)

# ------------------------------------------------------------------------------
# Build Configuration Summary
# ------------------------------------------------------------------------------

message(STATUS "")
message(STATUS "==================================================")
message(STATUS "Windscribe Build Configuration Summary")
message(STATUS "==================================================")
message(STATUS "Version:              ${PROJECT_VERSION}")
message(STATUS "Build Variant:        ${WINDSCRIBE_BUILD_TYPE}")
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
if(UNIX AND NOT APPLE)
message(STATUS "  CLI Only:           ${BUILD_CLI_ONLY}")
message(STATUS "  Build DEB:          ${BUILD_DEB}")
message(STATUS "  Build RPM:          Fedora ${BUILD_RPM} OpenSUSE ${BUILD_RPM_OPENSUSE}")
endif()
message(STATUS "  Build Tests:        ${BUILD_TESTS}")
message(STATUS "  Static Analysis:    ${STATIC_ANALYSIS}")
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

    # Utility programs
    if(WIN32)
        add_subdirectory(src/utils/windows/windscribe_install_helper)
        add_subdirectory(src/utils/windows/wireguard_service)
        add_subdirectory(src/installer/windows/uninstaller)
    elseif(APPLE)
        add_subdirectory(src/utils/macos/launcher)
        add_subdirectory(src/splittunneling/macos)
    elseif(UNIX)
        add_subdirectory(src/utils/linux/authhelper)
    endif()

    if(BUILD_CLI_ONLY)
        set(DEFINE_CLI_ONLY_MACRO ON CACHE BOOL "Build GUI-less client" FORCE)
    endif()

    # Main GUI application
    add_subdirectory(src/windscribe-cli)
    add_subdirectory(libs/wsnet)
    add_subdirectory(src/helper)
    add_subdirectory(src/client)

    if(WIN32)
        add_custom_target(build-app ALL
            DEPENDS Windscribe windscribe-cli WindscribeService WindscribeInstallHelper WireguardService uninstall
        )
    elseif(APPLE)
        add_custom_target(build-app ALL
            DEPENDS Windscribe windscribe-cli com.windscribe.helper.macos WindscribeLauncher com.windscribe.client.splittunnelextension
        )
    elseif(UNIX)
        add_custom_target(build-app ALL
            DEPENDS Windscribe windscribe-cli helper windscribe-authhelper
        )
    endif()
endif()

# Installers
if(BUILD_INSTALLER)
    if(WIN32)
        add_subdirectory(src/installer/windows/installer)
    elseif(APPLE)
        add_subdirectory(src/installer/mac/installer)
    endif()
endif()

if(BUILD_BOOTSTRAP)
    if(WIN32)
        if(BUILD_ARM64)
            set(WINDSCRIBE_ARCH_SUFFIX "_arm64")
        else()
            set(WINDSCRIBE_ARCH_SUFFIX "_amd64")
        endif()
        set(WINDSCRIBE_INSTALLER_NAME "Windscribe_${PROJECT_VERSION}${WINDSCRIBE_BUILD_SUFFIX}${WINDSCRIBE_ARCH_SUFFIX}.exe" CACHE STRING "Name of internal installer exe")
        add_subdirectory(src/installer/windows/bootstrap)
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

