# ------------------------------------------------------------------------------
# Architecture and Platform Configuration
# Handles multi-architecture builds and platform-specific settings
# ------------------------------------------------------------------------------

# Windows architecture handling
if(WIN32)
    if(BUILD_ARM64)
        set(WINDSCRIBE_ARCH "arm64")
        set(WINDSCRIBE_BUILD_LIBS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build-libs-arm64" CACHE PATH "Custom build libraries path")
        message(STATUS "Building for ARM64 architecture")
    else()
        set(WINDSCRIBE_ARCH "x86_64")
        set(WINDSCRIBE_BUILD_LIBS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build-libs" CACHE PATH "Custom build libraries path")
        message(STATUS "Building for x86_64 architecture")
    endif()
endif()

# macOS architecture handling
if(APPLE)
    if(CI_MODE)
        # Universal binary on CI
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architectures" FORCE)
        message(STATUS "Building universal binary (arm64 + x86_64)")
    elseif(NOT DEFINED CMAKE_OSX_ARCHITECTURES)
        # Native architecture
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
            set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "macOS architectures" FORCE)
        else()
            set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "macOS architectures" FORCE)
        endif()
    endif()
    set(WINDSCRIBE_BUILD_LIBS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build-libs" CACHE PATH "Custom build libraries path")
endif()

# Linux architecture handling
if(UNIX AND NOT APPLE)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
        set(WINDSCRIBE_ARCH "aarch64")
    else()
        set(WINDSCRIBE_ARCH "x86_64")
    endif()
    set(WINDSCRIBE_BUILD_LIBS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build-libs" CACHE PATH "Custom build libraries path")
    message(STATUS "Building for ${WINDSCRIBE_ARCH} architecture")
endif()

# Export WINDSCRIBE_BUILD_LIBS_PATH for use in component CMakeLists.txt files
message(STATUS "Custom build libraries path: ${WINDSCRIBE_BUILD_LIBS_PATH}")
