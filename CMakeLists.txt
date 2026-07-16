# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2026, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.23)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)  # option() honors normal variables

# Set macOS deployment target and architecture before project()
set(CMAKE_OSX_DEPLOYMENT_TARGET "13" CACHE STRING "Minimum OS X deployment version")

# Set the macOS architecture(s) BEFORE project() so the value is in effect when wsnet's targets are
# created during the FetchContent fetch below.  wsnet is a SHARED lib (libwsnet.dylib) on macOS, and a
# target's architectures are captured from CMAKE_OSX_ARCHITECTURES at creation time; if it is still
# unset when wsnet is configured, wsnet builds for the host arch only and the universal CI link fails
# with "found architecture 'arm64', required architecture 'x86_64'".  CI builds universal; a local
# build matches the host CPU.  This is the single source of truth for the arch -- architectures.cmake
# only sets the build-libs path.
if(APPLE)
    if(CI_MODE)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architectures" FORCE)
    elseif(NOT DEFINED CMAKE_OSX_ARCHITECTURES)
        execute_process(
            COMMAND uname -m
            OUTPUT_VARIABLE MACOS_HOST_ARCH
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        set(CMAKE_OSX_ARCHITECTURES "${MACOS_HOST_ARCH}" CACHE STRING "macOS architectures")
    endif()
endif()

# Enable vcpkg dependency installation
set(X_VCPKG_APPLOCAL_DEPS_INSTALL ON)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSVC runtime library (static linkage)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Windows target OS floor: pin to Windows 10 2004 (build 19041 / NTDDI_WIN10_VB) so every
# Windows target builds against the same minimum-supported-OS API surface. This provides a
# compile-time guard against accidentally using APIs newer than the floor, and makes the
# effective target independent of whichever Windows SDK happens to be installed. Applied
# globally here rather than per-target so the installer, services and utilities share it too.
# TODO: remove the X_WARNING defines and fix the code to use non-deprecated APIs.
if(WIN32)
    add_compile_definitions(
        WINVER=0x0A00
        _WIN32_WINNT=0x0A00
        NTDDI_VERSION=0x0A000008
        _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
        _CRT_SECURE_NO_WARNINGS
        _WINSOCK_DEPRECATED_NO_WARNINGS
    )
endif()


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
option(DEV_MODE "Developer build: disable signature verification and relax security checks" OFF)
if(DEV_MODE AND (CI_MODE OR SIGN_APP OR SIGN_INSTALLER OR SIGN_BOOTSTRAP OR ENABLE_NOTARIZE))
    message(FATAL_ERROR "DEV_MODE disables signature verification and must never be combined with CI_MODE, signing, or notarization.")
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

# Install the vcpkg dependencies right after project(), BEFORE the wsnet fetch below.  wsnet's
# CMakeLists runs find_package(c-ares/CURL/OpenSSL/...) at configure time, which only resolves once
# these packages exist in ${VCPKG_ROOT}/installed.  On a clean CI machine that directory is empty
# until this macro runs; locally it is already populated from prior builds, which is why fetching
# wsnet ahead of the install only fails on CI.  The macro depends solely on state established before
# project() (VCPKG_ROOT, VCPKG_TARGET_TRIPLET, CMAKE_BUILD_TYPE, the manifest root), so hoisting it
# here is safe.
if(BUILD_APP OR BUILD_INSTALLER OR BUILD_BOOTSTRAP)
    install_vcpkg_dependencies()
endif()

# Fetch wsnet here, in a pristine directory scope, BEFORE any project-wide compile/link options
# exist: the Objective-C ARC option and the Windows hardening flags below, the AUTOMOC/AUTOUIC/
# AUTORCC defaults set per-target, and the WS_* branding defines added after the integration config.
# FetchContent pulls wsnet in via add_subdirectory, which snapshots this scope's options at this
# point, so wsnet builds with only its own settings rather than inheriting ours.
# The include(fetch_wsnet.cmake) calls in src/windscribe-cli, src/client and src/helper then become
# no-ops via their `if(NOT TARGET wsnet::wsnet)` guard, kept as a fallback for building those subdirs
# standalone.
if(BUILD_APP)
    include("${CMAKE_SOURCE_DIR}/cmake/fetch_wsnet.cmake")
endif()

# Enable Objective-C ARC for every Objective-C / Objective-C++ source in one place, rather than per
# target.  The OBJC/OBJCXX languages must be enabled so .m/.mm are classified as Objective-C(++) and the
# compile-language genex applies -fobjc-arc to them only, never to plain C/C++ sources.
if(APPLE)
    enable_language(OBJC)
    enable_language(OBJCXX)
    set(CMAKE_OBJCXX_STANDARD ${CMAKE_CXX_STANDARD})
    set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
    add_compile_options("$<$<COMPILE_LANGUAGE:OBJC,OBJCXX>:-fobjc-arc>")
endif()

# Windows binary hardening: compile- and link-time exploit mitigations for every target in this
# tree (client, CLI, helper service, install/wireguard helpers, installer, uninstaller, bootstrap).
if(WIN32)
    add_compile_options(
        "$<$<COMPILE_LANGUAGE:C,CXX>:/guard:cf>"    # Control Flow Guard
        "$<$<COMPILE_LANGUAGE:C,CXX>:/sdl>"         # additional security checks (superset of /GS)
        "$<$<COMPILE_LANGUAGE:C,CXX>:/Qspectre>"    # Spectre v1 mitigation
        "$<$<COMPILE_LANGUAGE:C,CXX>:/ZH:SHA_256>"  # SHA-256 source hashing in debug info (BinSkim BA2004)
    )
    add_link_options(
        /guard:cf
        /DYNAMICBASE                # ASLR (linker default; asserted so it cannot regress silently)
        /NXCOMPAT                   # DEP
        /HIGHENTROPYVA              # 64-bit ASLR
        /DEPENDENTLOADFLAG:0x800    # resolve imports/delay-loads from System32 only (anti DLL-planting)
    )
    if(NOT BUILD_ARM64)
        # CET shadow stack compatibility (x64-only flag).  /guard:ehcont is deliberately absent:
        # unlike /guard:cf, the linker refuses to mix ehcont and non-ehcont objects, so it cannot
        # be enabled until the vcpkg static libs (Qt, OpenSSL, ...) are also compiled with it in
        # the ws-vcpkg-registry triplets.  Without EHCONT metadata CET still protects returns;
        # only the opt-in strict-mode exception validation is unavailable.
        add_link_options(/CETCOMPAT)
    endif()
endif()

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
    WS_LINUX_RUN_DIR="${WS_LINUX_RUN_DIR}"
    WS_LINUX_HELPER_SOCKET="${WS_LINUX_RUN_DIR}/helper.sock"
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
    WS_OVPN_MGMT_SOCKET="${WS_OVPN_MGMT_SOCKET}"
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
if(DEV_MODE)
    add_compile_definitions(WINDSCRIBE_DEV_MODE)
else()
    add_compile_definitions(USE_SIGNATURE_CHECK)
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
message(STATUS "Dev Mode:             ${DEV_MODE}")
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
        add_compile_definitions(WINDSCRIBE_BUILD_TESTS)
        enable_testing()
    endif()

    # Additional components from integration config
    foreach(_subdir ${WS_ADDITIONAL_SUBDIRS})
        add_subdirectory(${_subdir})
    endforeach()

    add_subdirectory(src/helper)
    add_subdirectory(src/client)

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

