cmake_minimum_required(VERSION 3.23)

project(client-common)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Network)

set(PROJECT_SOURCES
    version/appversion.cpp
    version/appversion.h
)

add_library(client-common STATIC ${PROJECT_SOURCES})

target_link_libraries(client-common PRIVATE Qt6::Core Qt6::Network OpenSSL::Crypto spdlog::spdlog wsnet::wsnet)

if(WIN32)
    target_compile_definitions(client-common PRIVATE CMAKE_LIBRARY_LIBRARY WIN32_LEAN_AND_MEAN PIO_APC_ROUTINE_DEFINED)
endif()

add_subdirectory(api_responses)
add_subdirectory(ipc)
add_subdirectory(tests)
add_subdirectory(types)
add_subdirectory(utils)
