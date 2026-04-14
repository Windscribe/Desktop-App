cmake_minimum_required(VERSION 3.23)

project(frontend-common)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS ${WS_QT_COMPONENTS})

set(PROJECT_SOURCES
    blockconnect.cpp
    languagecontroller.cpp
)

add_library(frontend-common STATIC ${PROJECT_SOURCES})

target_link_libraries(frontend-common PRIVATE Qt6::Core Qt6::Network ${WS_CLIENT_EXTRA_LIBS} wsnet::wsnet spdlog::spdlog Boost::serialization)

if (WIN32)
    target_compile_definitions(frontend-common PRIVATE CMAKE_LIBRARY_LIBRARY
                                  WINVER=0x0601
                                  _WIN32_WINNT=0x0601
                                  WIN32_LEAN_AND_MEAN
                                  PIO_APC_ROUTINE_DEFINED)
endif (WIN32)

target_include_directories(frontend-common PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../engine
    ${CMAKE_CURRENT_SOURCE_DIR}/../../client-common
)

add_subdirectory(backend)
add_subdirectory(launchonstartup)
add_subdirectory(localipcserver)
add_subdirectory(locations)
add_subdirectory(multipleaccountdetection)
add_subdirectory(utils)
