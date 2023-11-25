cmake_minimum_required(VERSION 3.23)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

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
)

add_library(ws_proxy_stub SHARED ${SOURCES})

target_link_libraries(ws_proxy_stub
                      PRIVATE
                      Ole32
                      Rpcrt4
)

if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "arm64")
    set(MIDL_ENV_TARGET arm64)
    add_definitions(-D_M_ARM64)
else()
    set(MIDL_ENV_TARGET x64)
endif()

set(MIDL_HEADER_FILE ${CMAKE_CURRENT_LIST_DIR}/ws_proxy_stub/ws_proxy_stub_h.h)
set(MIDL_IID_FILE ${CMAKE_CURRENT_LIST_DIR}/ws_proxy_stub/ws_proxy_stub_i.c)
set(MIDL_PROXY_FILE ${CMAKE_CURRENT_LIST_DIR}/ws_proxy_stub/ws_proxy_stub_p.c)
set(MIDL_DLLDATA_FILE ${CMAKE_CURRENT_LIST_DIR}/ws_proxy_stub/dlldata.c)

set(MIDL_OUTPUT
    ${MIDL_HEADER_FILE}
    ${MIDL_IID_FILE}
    ${MIDL_PROXY_FILE}
    ${MIDL_DLLDATA_FILE}
)

set(MIDL_FILE
    ${CMAKE_CURRENT_LIST_DIR}/ws_proxy_stub/ws_proxy_stub.idl
)

add_custom_command(
    OUTPUT ${MIDL_OUTPUT}
    COMMAND midl /env ${MIDL_ENV_TARGET} /h "${MIDL_HEADER_FILE}" /iid "${MIDL_IID_FILE}" /proxy "${MIDL_PROXY_FILE}" /dlldata "${MIDL_DLLDATA_FILE}" /W1 /char signed /target "NT60" /nologo ${MIDL_FILE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${MIDL_FILE}
    VERBATIM
)

target_sources(ws_proxy_stub PRIVATE ${MIDL_FILE} ${MIDL_OUTPUT})
