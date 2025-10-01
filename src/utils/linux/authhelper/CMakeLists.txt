cmake_minimum_required(VERSION 3.23)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(authhelper_linux)

set(SOURCES
    main.cpp
)

add_executable(windscribe-authhelper ${SOURCES})

install(TARGETS windscribe-authhelper
    RUNTIME DESTINATION .
)
