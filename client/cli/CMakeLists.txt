cmake_minimum_required(VERSION 3.23)

project(cli)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Network)

set(PROJECT_SOURCES
    mainservice.cpp
)

add_library(cli STATIC ${PROJECT_SOURCES})

target_link_libraries(cli PRIVATE Qt6::Core Qt6::Network wsnet::wsnet spdlog::spdlog Boost::serialization)

target_include_directories(cli PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../engine
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
    ${CMAKE_CURRENT_SOURCE_DIR}/../base
)

add_subdirectory(application)
