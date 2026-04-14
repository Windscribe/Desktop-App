# Usage: cmake -DINTEGRATION=gui [-DNO_SUFFIX=1] -P cmake/print_version.cmake
if(NOT DEFINED INTEGRATION)
    set(INTEGRATION "gui")
endif()

include(${CMAKE_CURRENT_LIST_DIR}/integrations/${INTEGRATION}.cmake)

set(_VERSION "${WS_VERSION_MAJOR}.${WS_VERSION_MINOR}.${WS_VERSION_BUILD}")
if(NOT NO_SUFFIX)
    if(WS_BUILD_TYPE STREQUAL "beta")
        set(_VERSION "${_VERSION}_beta")
    elseif(WS_BUILD_TYPE STREQUAL "guinea_pig")
        set(_VERSION "${_VERSION}_guinea_pig")
    endif()
endif()

message("${_VERSION}")
