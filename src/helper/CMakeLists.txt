cmake_minimum_required(VERSION 3.23)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(X_VCPKG_APPLOCAL_DEPS_INSTALL ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(helper)

if(NOT TARGET wsnet::wsnet)
    add_subdirectory(../../libs/wsnet ${CMAKE_CURRENT_BINARY_DIR}/wsnet)
endif()

if (WIN32)
   add_subdirectory(windows)
elseif(APPLE)
   add_subdirectory(macos)
elseif(UNIX)
   add_subdirectory(linux)
endif()


