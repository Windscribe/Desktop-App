cmake_minimum_required(VERSION 3.23)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
project(bootstrap)

find_package(LibArchive REQUIRED)

add_definitions(-DUNICODE -D_UNICODE /wd4090)

set(SOURCES
    ../utils/payloadextractor.cpp
    ../utils/persistent_log.cpp
    ../utils/secure_dir.cpp
    installer.rc
    main.cpp
    ossupport.manifest
    version.rc
)

add_executable(bootstrap WIN32 ${SOURCES})

# Output file should still be named windscribe_installer.exe
set_target_properties(bootstrap PROPERTIES OUTPUT_NAME "${WS_PRODUCT_NAME_LOWER}_installer")

if(NOT DEFINED WINDSCRIBE_INSTALLER_NAME OR WINDSCRIBE_INSTALLER_NAME STREQUAL "")
    message(FATAL_ERROR "WINDSCRIBE_INSTALLER_NAME must be defined")
endif()
add_definitions(-DWINDSCRIBE_INSTALLER_NAME=L\"${WINDSCRIBE_INSTALLER_NAME}\")

set_source_files_properties(installer.rc OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/resources/windscribeinstaller.7z)
set_property(TARGET bootstrap PROPERTY COMPILE_WARNING_AS_ERROR ON)
set_property(TARGET bootstrap PROPERTY VS_DPI_AWARE "PerMonitor")
set_property(TARGET bootstrap PROPERTY COMPILE_FLAGS "/Gy /Oi")
set_property(TARGET bootstrap PROPERTY LINK_FLAGS "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\"")

# Enable the "Microsoft-signed DLLs only" + "prohibit dynamic code" process mitigations,
# applied from the TLS callback in wsprocessmitigations.h.  Safe here because the bootstrapper
# loads no non-Microsoft-signed DLL (statically or at runtime).
target_compile_definitions(bootstrap PRIVATE WS_ENABLE_PROCESS_MITIGATIONS)

target_link_libraries(bootstrap
    PRIVATE
    bcrypt
    LibArchive::LibArchive
)

target_include_directories(bootstrap
    PRIVATE
    ../../utils
    ../../../../client/client-common
    ../../../../client/client-common/types
    ../../../../client/client-common/utils
)

install(TARGETS bootstrap
    RUNTIME DESTINATION .
)
