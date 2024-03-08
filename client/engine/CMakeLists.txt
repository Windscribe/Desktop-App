cmake_minimum_required(VERSION 3.23)

project(engine)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Core5Compat)

add_library(engine STATIC)

target_link_libraries(engine PRIVATE Qt6::Core Qt6::Network Qt6::Core5Compat wsnet::wsnet OpenSSL::Crypto Boost::serialization
)
target_compile_definitions(engine PRIVATE CMAKE_LIBRARY_LIBRARY
                                  WINVER=0x0601
                                  _WIN32_WINNT=0x0601
                                  WIN32_LEAN_AND_MEAN
                                  PIO_APC_ROUTINE_DEFINED)


target_include_directories(engine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
)

add_subdirectory(engine)
add_subdirectory(utils)


