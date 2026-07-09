cmake_minimum_required(VERSION 3.23)

project(engine)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Network)

add_library(engine STATIC)

target_link_libraries(engine PRIVATE Qt6::Core Qt6::Network wsnet::wsnet OpenSSL::Crypto Boost::serialization spdlog::spdlog)

if(WIN32)
    target_compile_definitions(engine PRIVATE CMAKE_LIBRARY_LIBRARY WIN32_LEAN_AND_MEAN PIO_APC_ROUTINE_DEFINED)
endif()

target_include_directories(engine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../client-common
)

add_subdirectory(engine)
add_subdirectory(utils)
