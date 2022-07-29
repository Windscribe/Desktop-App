cmake_minimum_required(VERSION 3.23)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.14" CACHE STRING "Minimum OS X deployment version")

project(Windscribe)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

# build_all.py sets this option when invoked with the '--sign' flag. Disabled by default
OPTION(DEFINE_USE_SIGNATURE_CHECK_MACRO "Add define USE_SIGNATURE_CHECK to project" OFF)
IF(DEFINE_USE_SIGNATURE_CHECK_MACRO)
    ADD_DEFINITIONS(-DUSE_SIGNATURE_CHECK)
ENDIF(DEFINE_USE_SIGNATURE_CHECK_MACRO)

set(WINDSCRIBE_BUILD_LIBS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../build-libs)

set(PROJECT_SOURCES
        main.cpp
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../common/utils/hardcodedsecrets.ini")
    list(APPEND PROJECT_SOURCES secrets.qrc)
endif()


if (WIN32)
    link_directories(${WINDSCRIBE_BUILD_LIBS_PATH}/boost/lib
                     ${WINDSCRIBE_BUILD_LIBS_PATH}/protobuf/debug/lib
                     ${WINDSCRIBE_BUILD_LIBS_PATH}/protobuf/release/lib
                     ${WINDSCRIBE_BUILD_LIBS_PATH}/curl/lib
                     ${WINDSCRIBE_BUILD_LIBS_PATH}/cares/dll_x64/lib
                     ${WINDSCRIBE_BUILD_LIBS_PATH}/openssl/lib
    )

    set (OS_SPECIFIC_LIBRARIES psapi.lib iphlpapi.lib dnsapi.lib rasapi32.lib pdh.lib
        libcurl cares libeay32 ssleay32
        debug libprotobufd
        optimized libprotobuf
    )
    list(APPEND PROJECT_SOURCES client.rc)
    # Generate debug information (symbol files) for Windows (release build only)
    add_compile_options("$<$<CONFIG:Release>:/Zi>")
    add_link_options("$<$<CONFIG:Release>:/DEBUG>" "$<$<CONFIG:Release>:/OPT:REF>" "$<$<CONFIG:Release>:/OPT:ICF>")

elseif(APPLE)
    link_directories(${WINDSCRIBE_BUILD_LIBS_PATH}/boost/lib
                     ${WINDSCRIBE_BUILD_LIBS_PATH}/protobuf/lib
                     ${WINDSCRIBE_BUILD_LIBS_PATH}/curl/lib
                     ${WINDSCRIBE_BUILD_LIBS_PATH}/cares/lib
                     ${WINDSCRIBE_BUILD_LIBS_PATH}/openssl/lib
    )
    set (OS_SPECIFIC_LIBRARIES
        ssl crypto cares curl protobuf libboost_serialization.a
        "-framework Foundation"
        "-framework AppKit"
        "-framework CoreFoundation"
        "-framework CoreServices"
        "-framework Security"
        "-framework SystemConfiguration"
        "-framework ServiceManagement"
        "-framework ApplicationServices"
        "-framework NetworkExtension"
    )

    # add Mac icon to Resources bundle subfolder
    set(MAC_APP_ICON ${CMAKE_CURRENT_SOURCE_DIR}/windscribe.icns)
    set_source_files_properties(${MAC_APP_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    list(APPEND PROJECT_SOURCES ${MAC_APP_ICON})

    # add additional files to Resources bundle subfolder
    file(GLOB MAC_RESOURCES_FILES ${CMAKE_CURRENT_SOURCE_DIR}/engine/mac/resources/*.*)
    set_source_files_properties(${MAC_RESOURCES_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    list(APPEND PROJECT_SOURCES ${MAC_RESOURCES_FILES})
endif()

qt_add_executable(Windscribe
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
)


if (APPLE)
    #postbuild copy commands for Mac
    add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_CURRENT_SOURCE_DIR}/../installer/mac/binaries/launcher/ $<TARGET_FILE_DIR:Windscribe>/../Library/LoginItems/)
    add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_CURRENT_SOURCE_DIR}/../installer/mac/binaries/helper/ $<TARGET_FILE_DIR:Windscribe>/../Library/LaunchServices/)

    add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:Windscribe>/../Helpers)
    add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                       ${WINDSCRIBE_BUILD_LIBS_PATH}/openvpn_2_5_4/openvpn $<TARGET_FILE_DIR:Windscribe>/../Helpers/windscribeopenvpn_2_5_4)
    add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                       ${WINDSCRIBE_BUILD_LIBS_PATH}/stunnel/stunnel $<TARGET_FILE_DIR:Windscribe>/../Helpers/windscribestunnel)
    add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                       ${WINDSCRIBE_BUILD_LIBS_PATH}/wireguard/windscribewireguard $<TARGET_FILE_DIR:Windscribe>/../Helpers/windscribewireguard)

    add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                       ${CMAKE_CURRENT_SOURCE_DIR}/../backend/mac/wstunnel/windscribewstunnel $<TARGET_FILE_DIR:Windscribe>/../Helpers/windscribewstunnel)

    add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_CURRENT_SOURCE_DIR}/../backend/mac/kext/Binary/ $<TARGET_FILE_DIR:Windscribe>/../Helpers/)

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../backend/mac/provisioning_profile/embedded.provisionprofile")
        add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                           "${CMAKE_CURRENT_SOURCE_DIR}/../backend/mac/provisioning_profile/embedded.provisionprofile" $<TARGET_FILE_DIR:Windscribe>/../embedded.provisionprofile)
    endif()

    add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                       ${CMAKE_CURRENT_SOURCE_DIR}/../installer/mac/binaries/windscribe-cli $<TARGET_FILE_DIR:Windscribe>/windscribe-cli)
    add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                       ${CMAKE_CURRENT_SOURCE_DIR}/../common/licenses/open_source_licenses.txt $<TARGET_FILE_DIR:Windscribe>/../Resources/open_source_licenses.txt)
endif (APPLE)

if (WIN32)
    target_link_options(Windscribe PRIVATE "/IGNORE:4099")
endif (WIN32)

target_link_libraries(Windscribe PRIVATE Qt6::Widgets Qt6::Network engine gui common ${OS_SPECIFIC_LIBRARIES})

target_include_directories(Windscribe PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/gui
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
    ${WINDSCRIBE_BUILD_LIBS_PATH}/protobuf/release/include
    ${WINDSCRIBE_BUILD_LIBS_PATH}/protobuf/include
)

set_target_properties(Windscribe PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/info.plist
    WIN32_EXECUTABLE TRUE
)

add_subdirectory(engine)
add_subdirectory(gui)
add_subdirectory(common)

qt_finalize_executable(Windscribe)

