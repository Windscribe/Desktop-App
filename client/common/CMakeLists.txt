cmake_minimum_required(VERSION 3.23)

project(common)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Network)

set(PROJECT_SOURCES
    names.h
    version/appversion.cpp
    version/appversion.h
    version/windscribe_version.h
)

add_library(common STATIC ${PROJECT_SOURCES})

target_link_libraries(common PRIVATE Qt6::Core Qt6::Network OpenSSL::Crypto wsnet::wsnet spdlog::spdlog)
target_compile_definitions(common PRIVATE CMAKE_LIBRARY_LIBRARY
                                  WINVER=0x0601
                                  _WIN32_WINNT=0x0601
                                  WIN32_LEAN_AND_MEAN
                                  PIO_APC_ROUTINE_DEFINED)

add_subdirectory(api_responses)
add_subdirectory(ipc)
add_subdirectory(types)
add_subdirectory(utils)

