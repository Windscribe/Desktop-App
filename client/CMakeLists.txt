cmake_minimum_required(VERSION 3.20)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.14" CACHE STRING "Minimum OS X deployment version")

project(Windscribe)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# This is a workaround for https://bugreports.qt.io/browse/QTBUG-89754 / #499
# Without this flag, on Linux libOpenGL.so.0 will be linked instead of libGL,
# which causes Windscribe to not start on some distributions where libopengl0 is not
# installed by default.
set(OpenGL_GL_PREFERENCE LEGACY)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Test LinguistTools)

# build_all.py sets this option when invoked with the '--sign' flag. Disabled by default
option(DEFINE_USE_SIGNATURE_CHECK_MACRO "Add define USE_SIGNATURE_CHECK to project" OFF)
if(DEFINE_USE_SIGNATURE_CHECK_MACRO)
    add_definitions(-DUSE_SIGNATURE_CHECK)
endif(DEFINE_USE_SIGNATURE_CHECK_MACRO)

#set(IS_BUILD_TESTS "1")

# if a build identifier is provided, add it to the project.
option(DEFINE_USE_BUILD_ID_MACRO "Add define BUILD_ID to project" "")
if(DEFINE_USE_BUILD_ID_MACRO)
    add_definitions(-DUSE_BUILD_ID="${DEFINE_USE_BUILD_ID_MACRO}")
endif(DEFINE_USE_BUILD_ID_MACRO)

if(WIN32 AND "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "arm64")
    set(WINDSCRIBE_BUILD_LIBS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../build-libs-arm64)
else()
    set(WINDSCRIBE_BUILD_LIBS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../build-libs)
endif()

set(PROJECT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

set(PROJECT_SOURCES
        main.cpp
)

if (WIN32)
    link_directories(${WINDSCRIBE_BUILD_LIBS_PATH}/boost/lib
                     ${WINDSCRIBE_BUILD_LIBS_PATH}/curl/lib
                     ${WINDSCRIBE_BUILD_LIBS_PATH}/openssl_ech_draft/lib
                     ${WINDSCRIBE_BUILD_LIBS_PATH}/cares/lib
    )

    set (OS_SPECIFIC_LIBRARIES psapi.lib iphlpapi.lib dnsapi.lib rasapi32.lib pdh.lib Crypt32.lib Version.lib
        libcurl_imp libssl libcrypto cares
    )
    list(APPEND PROJECT_SOURCES client.rc)
    # Generate debug information (symbol files) for Windows (release build only)
    add_compile_options("$<$<CONFIG:Release>:/Zi>")
    add_link_options("$<$<CONFIG:Release>:/DEBUG>" "$<$<CONFIG:Release>:/OPT:REF>" "$<$<CONFIG:Release>:/OPT:ICF>")

elseif(APPLE)
    link_directories(${WINDSCRIBE_BUILD_LIBS_PATH}/boost/lib
                     ${WINDSCRIBE_BUILD_LIBS_PATH}/curl/lib
                     ${WINDSCRIBE_BUILD_LIBS_PATH}/cares/lib
                     ${WINDSCRIBE_BUILD_LIBS_PATH}/openssl_ech_draft/lib
    )
    set (OS_SPECIFIC_LIBRARIES
        ssl crypto cares curl libboost_serialization.a
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

elseif(UNIX)
    link_directories(${WINDSCRIBE_BUILD_LIBS_PATH}/boost/lib
                     ${WINDSCRIBE_BUILD_LIBS_PATH}/curl/lib
                     ${WINDSCRIBE_BUILD_LIBS_PATH}/cares/lib
                     ${WINDSCRIBE_BUILD_LIBS_PATH}/openssl_ech_draft/lib
                     ${WINDSCRIBE_BUILD_LIBS_PATH}/openssl_ech_draft/lib64
     )
     set (OS_SPECIFIC_LIBRARIES ssl crypto cares curl libboost_serialization.a libboost_filesystem.a)
endif()

qt_add_executable(Windscribe
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
)


if (APPLE)
    #postbuild copy commands for Mac
    add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_CURRENT_SOURCE_DIR}/../build/gui/launcher/mac/$<CONFIG>/ $<TARGET_FILE_DIR:Windscribe>/../Library/LoginItems)
    add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_CURRENT_SOURCE_DIR}/../build/backend/mac/helper/$<CONFIG>/ $<TARGET_FILE_DIR:Windscribe>/../Library/LaunchServices)

    add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:Windscribe>/../Helpers)
    add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                       ${WINDSCRIBE_BUILD_LIBS_PATH}/openvpn/openvpn $<TARGET_FILE_DIR:Windscribe>/../Helpers/windscribeopenvpn)
    add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                       ${WINDSCRIBE_BUILD_LIBS_PATH}/wireguard/windscribewireguard $<TARGET_FILE_DIR:Windscribe>/../Helpers/windscribewireguard)
    add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                      ${WINDSCRIBE_BUILD_LIBS_PATH}/ctrld/ctrld $<TARGET_FILE_DIR:Windscribe>/../Helpers/windscribectrld)

    add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                      ${WINDSCRIBE_BUILD_LIBS_PATH}/wstunnel/windscribewstunnel $<TARGET_FILE_DIR:Windscribe>/../Helpers/windscribewstunnel)

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../backend/mac/provisioning_profile/embedded.provisionprofile")
        add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                           "${CMAKE_CURRENT_SOURCE_DIR}/../backend/mac/provisioning_profile/embedded.provisionprofile" $<TARGET_FILE_DIR:Windscribe>/../embedded.provisionprofile)
    endif()

    if(EXISTS "${WINDSCRIBE_BUILD_LIBS_PATH}/dga/libdga.dylib")
        add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                           "${WINDSCRIBE_BUILD_LIBS_PATH}/dga/libdga.dylib" $<TARGET_FILE_DIR:Windscribe>/../Helpers/libdga.dylib)
    endif()

    add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                       ${CMAKE_CURRENT_SOURCE_DIR}/../build/gui/cli/windscribe-cli $<TARGET_FILE_DIR:Windscribe>/windscribe-cli)
    add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                       ${CMAKE_CURRENT_SOURCE_DIR}/common/licenses/open_source_licenses.txt $<TARGET_FILE_DIR:Windscribe>/../Resources/open_source_licenses.txt)
endif (APPLE)

if (WIN32)
    target_link_options(Windscribe PRIVATE "/IGNORE:4099")
elseif (APPLE)
    set_property(TARGET Windscribe APPEND_STRING PROPERTY COMPILE_FLAGS "-fobjc-arc")
endif()

target_link_libraries(Windscribe PRIVATE Qt6::Widgets Qt6::Network gui engine common ${OS_SPECIFIC_LIBRARIES})

target_include_directories(Windscribe PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/gui
    ${CMAKE_CURRENT_SOURCE_DIR}/common
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

if(DEFINED IS_BUILD_TESTS)
    enable_testing ()
    add_test (NAME locationsmodel.test COMMAND locationsmodel.test)
    add_test (NAME dnsrequest.test COMMAND dnsrequest.test)
    add_test (NAME dnscache.test COMMAND dnscache.test)
    add_test (NAME curlnetworkmanager.test COMMAND curlnetworkmanager.test)
    add_test (NAME networkaccessmanager.test COMMAND networkaccessmanager.test)
endif (DEFINED IS_BUILD_TESTS)

