# ------------------------------------------------------------------------------
# vcpkg Integration
# Handles vcpkg toolchain setup
# ------------------------------------------------------------------------------

# Check for VCPKG_ROOT environment variable
if(NOT DEFINED ENV{VCPKG_ROOT})
    message(FATAL_ERROR "VCPKG_ROOT environment variable is not set. Please set it to your vcpkg installation directory.")
endif()

set(VCPKG_ROOT "$ENV{VCPKG_ROOT}" CACHE PATH "vcpkg root directory")
message(STATUS "Using vcpkg from: ${VCPKG_ROOT}")

# Set vcpkg toolchain file
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "vcpkg toolchain file")
endif()

# Verify toolchain file exists
if(NOT EXISTS "${CMAKE_TOOLCHAIN_FILE}")
    message(FATAL_ERROR "vcpkg toolchain file not found: ${CMAKE_TOOLCHAIN_FILE}")
endif()

# Set vcpkg overlay paths for custom triplets
set(VCPKG_OVERLAY_TRIPLETS "${CMAKE_CURRENT_SOURCE_DIR}/tools/vcpkg/triplets" CACHE PATH "vcpkg overlay triplets")

# Determine vcpkg triplet based on platform and architecture
if(NOT DEFINED VCPKG_TARGET_TRIPLET)
    if(WIN32)
        if(BUILD_ARM64)
            set(VCPKG_TARGET_TRIPLET "arm64-windows-static-release")
        elseif(CI_MODE)
            set(VCPKG_TARGET_TRIPLET "x64-windows-static-release")
        else()
            set(VCPKG_TARGET_TRIPLET "x64-windows-static")
        endif()
    elseif(APPLE)
        if(CI_MODE)
            # Universal binary on CI
            set(VCPKG_TARGET_TRIPLET "universal-osx")
        else()
            # Detect host architecture
            execute_process(
                COMMAND uname -m
                OUTPUT_VARIABLE MACOS_ARCH
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            if(MACOS_ARCH STREQUAL "arm64")
                set(VCPKG_TARGET_TRIPLET "arm64-osx")
            else()
                set(VCPKG_TARGET_TRIPLET "x64-osx")
            endif()
        endif()
    elseif(UNIX)
        # Linux - detect host architecture
        execute_process(
            COMMAND uname -m
            OUTPUT_VARIABLE LINUX_ARCH
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if(LINUX_ARCH MATCHES "aarch64|arm64")
            set(VCPKG_TARGET_TRIPLET "arm64-linux")
        else()
            set(VCPKG_TARGET_TRIPLET "x64-linux")
        endif()
    endif()

    set(VCPKG_TARGET_TRIPLET "${VCPKG_TARGET_TRIPLET}" CACHE STRING "vcpkg target triplet")
endif()

message(STATUS "vcpkg triplet: ${VCPKG_TARGET_TRIPLET}")

# ------------------------------------------------------------------------------
# Macro to install vcpkg dependencies
# Call this after project() to install dependencies
# ------------------------------------------------------------------------------

macro(install_vcpkg_dependencies)
    message(STATUS "Installing vcpkg dependencies for triplet: ${VCPKG_TARGET_TRIPLET}")

    set(VCPKG_INSTALL_CMD
        "${VCPKG_ROOT}/vcpkg" install
        "--x-install-root=${VCPKG_ROOT}/installed"
        "--x-manifest-root=${CMAKE_CURRENT_SOURCE_DIR}/tools/vcpkg"
        "--overlay-triplets=${CMAKE_CURRENT_SOURCE_DIR}/tools/vcpkg/triplets"
        "--triplet=${VCPKG_TARGET_TRIPLET}"
        "--clean-buildtrees-after-build"
        "--clean-packages-after-build"
    )

    # Add host triplet for Windows and Linux
    if(WIN32)
        if(CI_MODE)
            list(APPEND VCPKG_INSTALL_CMD "--host-triplet=x64-windows-static-release")
        else()
            list(APPEND VCPKG_INSTALL_CMD "--host-triplet=x64-windows-static")
        endif()
    elseif(UNIX AND NOT APPLE)
        list(APPEND VCPKG_INSTALL_CMD "--host-triplet=${VCPKG_TARGET_TRIPLET}")
    endif()

    execute_process(
        COMMAND ${VCPKG_INSTALL_CMD}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE VCPKG_INSTALL_RESULT
    )

    if(NOT VCPKG_INSTALL_RESULT EQUAL 0)
        message(FATAL_ERROR "vcpkg install failed with exit code ${VCPKG_INSTALL_RESULT}")
    endif()
endmacro()
