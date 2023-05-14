cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

PROJECT(cli)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 COMPONENTS Core Network Core5Compat)

if(WIN32 AND "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "arm64")
    set(WINDSCRIBE_BUILD_LIBS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../build-libs-arm64)
else()
    set(WINDSCRIBE_BUILD_LIBS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../build-libs)
endif()

set(SOURCES
    backendcommander.cpp
    backendcommander.h
    cliarguments.cpp
    cliarguments.h
    main.cpp
    ../../client/common/utils/ipvalidation.cpp
    ../../client/common/ipc/commandfactory.cpp
    ../../client/common/ipc/connection.cpp
    ../../client/common/ipc/clicommands.cpp
    ../../client/common/ipc/server.cpp
    ../../client/common/types/locationid.cpp
    ../../client/common/utils/extraconfig.cpp
    ../../client/common/utils/languagesutil.cpp
    ../../client/common/utils/logger.cpp
    ../../client/common/utils/utils.cpp
    ../../client/common/utils/dga_library.cpp
    ../../client/common/utils/hardcodedsettings.cpp
    ../../client/common/utils/simplecrypt.cpp
    ../../client/common/version/appversion.cpp
    ../../client/common/utils/executable_signature/executable_signature.cpp
    ../../client/common/utils/clean_sensitive_info.cpp
)

if(WIN32)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
    set(OS_SPECIFIC_LIBS
        user32
        Advapi32
        Iphlpapi
        Version)
    list(APPEND SOURCES
         ../../client/common/utils/executable_signature/executable_signature_win.cpp
         ../../client/common/utils/winutils.cpp
         cli.rc)
    add_compile_options("$<$<CONFIG:Release>:/Zi>")
    add_link_options("$<$<CONFIG:Release>:/DEBUG>" "$<$<CONFIG:Release>:/OPT:REF>" "$<$<CONFIG:Release>:/OPT:ICF>")
elseif(APPLE)
    set(CMAKE_MACOSX_RPATH 1)
    set(CMAKE_BUILD_RPATH "@executable_path/../Frameworks")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    set(OS_SPECIFIC_LIBS
        "-framework AppKit"
        "-framework CoreFoundation"
        "-framework CoreServices"
        "-framework Security"
        "-framework SystemConfiguration")
    list(APPEND SOURCES
         ../../client/common/utils/network_utils/network_utils_mac.cpp
         ../../client/common/utils/executable_signature/executable_signature_mac.mm
         ../../client/common/utils/macutils.mm)
else()
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    set(OS_SPECIFIC_LIBS
        ssl
        crypto)
    list(APPEND SOURCES
         ../../client/common/utils/linuxutils.cpp)
endif()

add_executable(windscribe-cli ${SOURCES})

if(APPLE)
    set_property(TARGET windscribe-cli APPEND_STRING PROPERTY COMPILE_FLAGS "-fobjc-arc")
endif()

target_include_directories(windscribe-cli
                           PRIVATE
                           ${WINDSCRIBE_BUILD_LIBS_PATH}/openssl_ech_draft/include
)

target_link_libraries(windscribe-cli
                      PRIVATE
                      ${OS_SPECIFIC_LIBS}
                      Qt6::Core
                      Qt6::Network
                      Qt6::Core5Compat
)

target_link_directories(windscribe-cli
                        PRIVATE
                        ${WINDSCRIBE_BUILD_LIBS_PATH}/openssl_ech_draft/lib
                        ${WINDSCRIBE_BUILD_LIBS_PATH}/openssl_ech_draft/lib64)

target_include_directories(windscribe-cli PRIVATE
                           ../../client/common
)
