# ------------------------------------------------------------------------------
# Architecture and Platform Configuration
# Handles multi-architecture builds and platform-specific settings
# ------------------------------------------------------------------------------

# Product subdirectory under build-libs/ for dependency artifacts.
# Defaults to "windscribe"; override with -DWS_BUILD_LIBS_SUBDIR=<product> for whitelabel builds.
# Also set in each integration file (e.g. windscribe.cmake) for documentation.
if(NOT DEFINED WS_BUILD_LIBS_SUBDIR)
    set(WS_BUILD_LIBS_SUBDIR "windscribe" CACHE STRING "Product subdirectory under build-libs")
endif()

# Windows architecture handling
if(WIN32)
    if(BUILD_ARM64)
        set(PACKAGE_ARCH "arm64")
        set(WINDSCRIBE_BUILD_LIBS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build-libs-arm64/${WS_BUILD_LIBS_SUBDIR}" CACHE PATH "Custom build libraries path")
    else()
        set(PACKAGE_ARCH "amd64")
        set(WINDSCRIBE_BUILD_LIBS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build-libs/${WS_BUILD_LIBS_SUBDIR}" CACHE PATH "Custom build libraries path")
    endif()
endif()

# macOS architecture handling
if(APPLE)
    if(CI_MODE)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architectures" FORCE)
    elseif(NOT DEFINED CMAKE_OSX_ARCHITECTURES)
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
            set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "macOS architectures" FORCE)
        else()
            set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "macOS architectures" FORCE)
        endif()
    endif()
    set(WINDSCRIBE_BUILD_LIBS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build-libs/${WS_BUILD_LIBS_SUBDIR}" CACHE PATH "Custom build libraries path")
endif()

# Linux architecture handling
if(UNIX AND NOT APPLE)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
        set(PACKAGE_ARCH "arm64")
    else()
        set(PACKAGE_ARCH "amd64")
    endif()
    set(WINDSCRIBE_BUILD_LIBS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build-libs/${WS_BUILD_LIBS_SUBDIR}" CACHE PATH "Custom build libraries path")
endif()

message(STATUS "Build libraries path: ${WINDSCRIBE_BUILD_LIBS_PATH}")
