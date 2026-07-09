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
# CMAKE_OSX_ARCHITECTURES is set before project() in the root CMakeLists -- it must precede the wsnet
# FetchContent fetch so wsnet (a SHARED dylib) builds for the same archs as the app.  Only the
# build-libs path is set here.
if(APPLE)
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
