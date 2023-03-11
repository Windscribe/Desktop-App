cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(authhelper_win)

add_definitions(-DUNICODE -D_UNICODE -DREGISTER_PROXY_DLL)

set(SOURCES
    ws_com/components.cpp
    ws_com/dllmain.cpp
    ws_com/exports.cpp
    ws_com/ws_com.def
    ws_com/ws_com.rc
)

add_library(ws_com SHARED ${SOURCES})

set(SOURCES
    ws_com_server/ws_com_server.cpp
)

add_executable(ws_com_server ${SOURCES})

target_link_libraries(ws_com_server
                      PRIVATE
                      ws_com
)

set(SOURCES
    ws_proxy_stub/dlldata.c
    ws_proxy_stub/dllmain.cpp
    ws_proxy_stub/ws_proxy_stub.def
    ws_proxy_stub/ws_proxy_stub.idl
    ws_proxy_stub/ws_proxy_stub_i.c
    ws_proxy_stub/ws_proxy_stub_p.c
)

add_library(ws_proxy_stub SHARED ${SOURCES})

target_link_libraries(ws_proxy_stub
                      PRIVATE
                      Ole32
                      Rpcrt4
)

