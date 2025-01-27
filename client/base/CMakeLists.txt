cmake_minimum_required(VERSION 3.23)

project(base)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (DEFINE_CLI_ONLY_MACRO)
    find_package(Qt6 REQUIRED COMPONENTS Core5Compat Network)
else()
    find_package(Qt6 REQUIRED COMPONENTS Core5Compat Network Widgets)
endif()

set(PROJECT_SOURCES
    blockconnect.cpp
    languagecontroller.cpp
)

add_library(base STATIC ${PROJECT_SOURCES})

target_link_libraries(base PRIVATE Qt6::Core Qt6::Network Qt6::Core5Compat wsnet::wsnet spdlog::spdlog Boost::serialization)
if (NOT DEFINE_CLI_ONLY_MACRO)
    target_link_libraries(base PRIVATE Qt6::Widgets)
endif()

if (WIN32)
    target_compile_definitions(base PRIVATE CMAKE_LIBRARY_LIBRARY
                                  WINVER=0x0601
                                  _WIN32_WINNT=0x0601
                                  WIN32_LEAN_AND_MEAN
                                  PIO_APC_ROUTINE_DEFINED)
endif (WIN32)

target_include_directories(base PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../base
    ${CMAKE_CURRENT_SOURCE_DIR}/../engine
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
)

add_subdirectory(backend)
add_subdirectory(launchonstartup)
add_subdirectory(localipcserver)
add_subdirectory(locations)
add_subdirectory(multipleaccountdetection)
add_subdirectory(utils)
