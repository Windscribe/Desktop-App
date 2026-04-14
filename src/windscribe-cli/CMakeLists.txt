cmake_minimum_required(VERSION 3.23)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_OSX_DEPLOYMENT_TARGET "13" CACHE STRING "Minimum OS X deployment version")

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

PROJECT(cli)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 COMPONENTS Core Network LinguistTools)
find_package(OpenSSL REQUIRED)
find_package(spdlog CONFIG REQUIRED)

include("${CMAKE_SOURCE_DIR}/cmake/fetch_wsnet.cmake")

# build_all.py sets this option when invoked with the '--sign' flag. Disabled by default
option(DEFINE_USE_SIGNATURE_CHECK_MACRO "Add define USE_SIGNATURE_CHECK to project" OFF)
if(DEFINE_USE_SIGNATURE_CHECK_MACRO)
    add_definitions(-DUSE_SIGNATURE_CHECK)
endif(DEFINE_USE_SIGNATURE_CHECK_MACRO)

set(SOURCES
    backendcommander.cpp
    backendcommander.h
    cliarguments.cpp
    cliarguments.h
    languagecontroller.cpp
    main.cpp
    strings.cpp
    utils.cpp
    ../client/client-common/utils/ipvalidation.cpp
    ../client/client-common/ipc/commandfactory.cpp
    ../client/client-common/ipc/connection.cpp
    ../client/client-common/ipc/server.cpp
    ../client/client-common/types/locationid.cpp
    ../client/client-common/types/protocol.cpp
    ../client/client-common/utils/extraconfig.cpp
    ../client/client-common/utils/languagesutil.cpp
    ../client/client-common/utils/log/clean_sensitive_info.cpp
    ../client/client-common/utils/log/categories.cpp
    ../client/client-common/utils/log/logger.cpp
    ../client/client-common/utils/log/paths.cpp
    ../client/client-common/utils/utils.cpp
    ../client/client-common/utils/hardcodedsettings.cpp
    ../client/client-common/utils/simplecrypt.cpp
    ../client/client-common/version/appversion.cpp
    ../client/client-common/utils/executable_signature/executable_signature.cpp
)

if(WIN32)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
    set(OS_SPECIFIC_LIBS
        user32
        Advapi32
        Iphlpapi
        Version)
    list(APPEND SOURCES
         ../client/client-common/utils/executable_signature/executable_signature_win.cpp
         ../client/client-common/utils/servicecontrolmanager.cpp
         ../client/client-common/utils/winutils.cpp
         cli.rc)
    if(CI_MODE)
        add_compile_options("$<$<CONFIG:Release>:/Zi>")
        add_link_options("$<$<CONFIG:Release>:/DEBUG>" "$<$<CONFIG:Release>:/OPT:REF>" "$<$<CONFIG:Release>:/OPT:ICF>")
    endif()
elseif(APPLE)
    set(CMAKE_MACOSX_RPATH 1)
    set(CMAKE_BUILD_RPATH "@executable_path/../Frameworks")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    set(OS_SPECIFIC_LIBS
        "-framework AppKit"
        "-framework CoreFoundation"
        "-framework CoreLocation"
        "-framework CoreServices"
        "-framework CoreWLAN"
        "-framework Security"
        "-framework SystemConfiguration"
        )
    list(APPEND SOURCES
         ../client/client-common/utils/network_utils/network_utils_mac.mm
         ../client/client-common/utils/executable_signature/executable_signature_mac.mm
         ../client/client-common/utils/macutils.mm)
else()
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    set(OS_SPECIFIC_LIBS
        OpenSSL::Crypto
    )
    list(APPEND SOURCES
         ../client/client-common/utils/executable_signature/executablesignature_linux.cpp
         ../client/client-common/utils/linuxutils.cpp)
endif()

add_executable(${WS_CLI_TARGET} ${SOURCES})

set(TS_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_cli_ar.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_cli_be.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_cli_cs.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_cli_de.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_cli_el.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_cli_en.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_cli_es.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_cli_fa.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_cli_fr.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_cli_hi.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_cli_id.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_cli_it.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_cli_ja.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_cli_ko.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_cli_pl.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_cli_pt.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_cli_ru.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_cli_sk.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_cli_tr.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_cli_uk.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_cli_vi.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_cli_zh-CN.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_cli_zh-TW.ts
)
if (UPDATE_TRANSLATIONS AND NOT DEFINE_QT_CREATOR_DEV_BUILD_MACRO)
    qt6_create_translation(QM_FILES ${CMAKE_CURRENT_SOURCE_DIR} ${TS_FILES} OPTIONS -extensions cpp,h -locations none)
endif()

qt_add_resources(${WS_CLI_TARGET} qm_files
    PREFIX
        "/translations"
    BASE
        ${CMAKE_CURRENT_BINARY_DIR}
    FILES
        ${QM_FILES}
)

if(APPLE)
    set_property(TARGET ${WS_CLI_TARGET} APPEND_STRING PROPERTY COMPILE_FLAGS "-fobjc-arc")
endif()

target_link_libraries(${WS_CLI_TARGET}
                      PRIVATE
                      ${OS_SPECIFIC_LIBS}
                      spdlog::spdlog
                      Qt6::Core
                      Qt6::Network
)

target_include_directories(${WS_CLI_TARGET} PRIVATE
                           ../client/client-common
                           $<TARGET_PROPERTY:wsnet::wsnet,INTERFACE_INCLUDE_DIRECTORIES>
)

install(TARGETS ${WS_CLI_TARGET}
    RUNTIME DESTINATION .
)
