cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_definitions(-DUNICODE -D_UNICODE)

if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "arm64")
    add_definitions(-D_M_ARM64)
endif()

project(wssecure LANGUAGES CXX)

add_library(wssecure SHARED
  dllmain.cpp
  wssecure.h
  wssecure.cpp
  version.rc
)

target_compile_definitions(wssecure PRIVATE WSSECURE_LIBRARY)

set_target_properties(wssecure PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)

install(TARGETS wssecure
    RUNTIME DESTINATION .
)
