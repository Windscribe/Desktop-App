cmake_minimum_required(VERSION 3.23)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(X_VCPKG_APPLOCAL_DEPS_INSTALL ON)

set(CMAKE_OSX_DEPLOYMENT_TARGET "13" CACHE STRING "Minimum OS X deployment version")

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(${WS_APP_TARGET})

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS serialization)
find_package(spdlog CONFIG REQUIRED)

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/fetch_wsnet.cmake")

# build_all.py sets this option when invoked with the '--sign' flag. Disabled by default
option(DEFINE_USE_SIGNATURE_CHECK_MACRO "Add define USE_SIGNATURE_CHECK to project" OFF)
if(DEFINE_USE_SIGNATURE_CHECK_MACRO)
    add_definitions(-DUSE_SIGNATURE_CHECK)
endif(DEFINE_USE_SIGNATURE_CHECK_MACRO)

# if a build identifier is provided, add it to the project.
option(DEFINE_USE_BUILD_ID_MACRO "Add define BUILD_ID to project" "")
if(DEFINE_USE_BUILD_ID_MACRO)
    add_definitions(-DUSE_BUILD_ID="${DEFINE_USE_BUILD_ID_MACRO}")
endif(DEFINE_USE_BUILD_ID_MACRO)

option(DEFINE_QT_CREATOR_DEV_BUILD_MACRO "Add define QT_CREATOR_DEV_BUILD to project" OFF)
if(DEFINE_QT_CREATOR_DEV_BUILD_MACRO)
    add_definitions(-DQT_CREATOR_DEV_BUILD)
endif(DEFINE_QT_CREATOR_DEV_BUILD_MACRO)

# This is a workaround for https://bugreports.qt.io/browse/QTBUG-89754 / #499
# Without this flag, on Linux libOpenGL.so.0 will be linked instead of libGL,
# which causes the application to not start on some distributions where libopengl0 is not
# installed by default.
if("Widgets" IN_LIST WS_QT_COMPONENTS)
    set(OpenGL_GL_PREFERENCE LEGACY)
endif()

find_package(Qt6 REQUIRED COMPONENTS ${WS_QT_COMPONENTS})

set(PROJECT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

if (WIN32)
    set (OS_SPECIFIC_LIBRARIES
         psapi.lib
         iphlpapi.lib
         dnsapi.lib
         rasapi32.lib
         pdh.lib
         Crypt32.lib
         Version.lib
         # $<$<NOT:$<CONFIG:Debug>>:../libs/wssecure/wssecure>
    )

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/client.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/client.rc
        @ONLY
    )
    list(APPEND PROJECT_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/client.rc)
    if(CI_MODE)
        add_compile_options("$<$<CONFIG:Release>:/Zi>")
        add_link_options("$<$<CONFIG:Release>:/DEBUG>" "$<$<CONFIG:Release>:/OPT:REF>" "$<$<CONFIG:Release>:/OPT:ICF>")
    endif()

elseif (APPLE)
    set (OS_SPECIFIC_LIBRARIES
        "-framework AppKit"
        "-framework ApplicationServices"
        "-framework CoreFoundation"
        "-framework CoreLocation"
        "-framework CoreServices"
        "-framework CoreWLAN"
        "-framework Foundation"
        "-framework NetworkExtension"
        "-framework Security"
        "-framework ServiceManagement"
        "-framework SystemConfiguration"
        "-framework SystemExtensions"
    )

    # add Mac icon to Resources bundle subfolder
    set(MAC_APP_ICON ${CMAKE_CURRENT_SOURCE_DIR}/${WS_MAC_APP_ICON})
    get_filename_component(WS_MAC_ICON_FILE "${WS_MAC_APP_ICON}" NAME)
    set_source_files_properties(${MAC_APP_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    list(APPEND PROJECT_SOURCES ${MAC_APP_ICON})

    # configure and add additional files to Resources bundle subfolder
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/engine/mac/resources/dns.sh.in ${CMAKE_CURRENT_BINARY_DIR}/dns.sh @ONLY)
    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/dns.sh PROPERTIES MACOSX_PACKAGE_LOCATION "Resources" GENERATED TRUE)
    list(APPEND PROJECT_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/dns.sh)
endif()

qt_add_executable(${WS_APP_TARGET}
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
)

if (WIN32)
    if (NOT DEFINE_QT_CREATOR_DEV_BUILD_MACRO)
        foreach(_entry ${WS_BUNDLED_HELPERS})
            string(REPLACE "|" ";" _pair "${_entry}")
            list(GET _pair 0 _src)
            list(GET _pair 1 _dest)
            add_custom_command(TARGET ${WS_APP_TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                              "${_src}" "$<TARGET_FILE_DIR:${WS_APP_TARGET}>/${_dest}")
        endforeach()
    endif()
elseif (APPLE)
    #postbuild copy commands for Mac
    if (NOT DEFINE_QT_CREATOR_DEV_BUILD_MACRO)
        # Ensure launcher is built before client
        add_dependencies(${WS_APP_TARGET} ${WS_MAC_LAUNCHER_TARGET})
        add_dependencies(${WS_APP_TARGET} ${WS_MAC_SPLIT_TUNNEL_BUNDLE_ID})

        # Copy launcher bundle
        add_custom_command(TARGET ${WS_APP_TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory
                           $<TARGET_BUNDLE_DIR:${WS_MAC_LAUNCHER_TARGET}> $<TARGET_FILE_DIR:${WS_APP_TARGET}>/../Library/LoginItems/${WS_MAC_LAUNCHER_TARGET}.app)

        # Copy helper executable
        add_custom_command(TARGET ${WS_APP_TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory
                           $<TARGET_FILE_DIR:${WS_APP_TARGET}>/../Library/LaunchServices)
        add_custom_command(TARGET ${WS_APP_TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                           $<TARGET_FILE:${WS_MAC_HELPER_BUNDLE_ID}> $<TARGET_FILE_DIR:${WS_APP_TARGET}>/../Library/LaunchServices/)

        # Copy shared libraries to Frameworks
        foreach(_entry ${WS_SHARED_LIBS})
            string(REPLACE "|" ";" _pair "${_entry}")
            list(GET _pair 0 _src)
            list(GET _pair 1 _dest)
            add_custom_command(TARGET ${WS_APP_TARGET} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_src}" "$<TARGET_FILE_DIR:${WS_APP_TARGET}>/../Frameworks/${_dest}"
            )
        endforeach()

        # Fix RPATH on wsnet so it can find sibling libraries in Frameworks
        if("wsnet" IN_LIST WS_SHARED_LIB_NAMES)
            add_custom_command(TARGET ${WS_APP_TARGET} POST_BUILD
                COMMAND install_name_tool -add_rpath "@loader_path" $<TARGET_FILE_DIR:${WS_APP_TARGET}>/../Frameworks/libwsnet.dylib
            )
        endif()

        add_custom_command(TARGET ${WS_APP_TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${WS_APP_TARGET}>/../Helpers)

        # Copy bundled helpers to Helpers/
        foreach(_entry ${WS_BUNDLED_HELPERS})
            string(REPLACE "|" ";" _pair "${_entry}")
            list(GET _pair 0 _src)
            list(GET _pair 1 _dest)
            add_custom_command(TARGET ${WS_APP_TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                               "${_src}" "$<TARGET_FILE_DIR:${WS_APP_TARGET}>/../Helpers/${_dest}")
        endforeach()

        # Copy split tunnel extension
        add_custom_command(TARGET ${WS_APP_TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory
                           $<TARGET_BUNDLE_DIR:${WS_MAC_SPLIT_TUNNEL_BUNDLE_ID}> $<TARGET_FILE_DIR:${WS_APP_TARGET}>/../Library/SystemExtensions/${WS_MAC_SPLIT_TUNNEL_BUNDLE_ID}.systemextension)

        # Fix openssl dynamic link paths in helpers that link against openssl
        foreach(_helper ${WS_MAC_RPATH_BINARIES})
            add_custom_command(TARGET ${WS_APP_TARGET} POST_BUILD
                COMMAND bash -c [[OLD=$(otool -L ${0} | grep ${1} | uniq | sed s/.dylib.*/.dylib/ | awk '{$1=$1;print}'); install_name_tool -change ${OLD} @executable_path/../Frameworks/${1}.3.dylib ${0}]] $<TARGET_FILE_DIR:${WS_APP_TARGET}>/../Helpers/${WS_PRODUCT_NAME_LOWER}${_helper} libssl
                COMMAND bash -c [[OLD=$(otool -L ${0} | grep ${1} | uniq | sed s/.dylib.*/.dylib/ | awk '{$1=$1;print}'); install_name_tool -change ${OLD} @executable_path/../Frameworks/${1}.3.dylib ${0}]] $<TARGET_FILE_DIR:${WS_APP_TARGET}>/../Helpers/${WS_PRODUCT_NAME_LOWER}${_helper} libcrypto
                VERBATIM
            )
        endforeach()

        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../data/provisioning_profile/embedded.provisionprofile")
            add_custom_command(TARGET ${WS_APP_TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                               "${CMAKE_CURRENT_SOURCE_DIR}/../../data/provisioning_profile/embedded.provisionprofile" $<TARGET_FILE_DIR:${WS_APP_TARGET}>/../embedded.provisionprofile)
        endif()

        if(TARGET ${WS_CLI_TARGET})
            add_custom_command(TARGET ${WS_APP_TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                               $<TARGET_FILE:${WS_CLI_TARGET}> $<TARGET_FILE_DIR:${WS_APP_TARGET}>/${WS_CLI_EXECUTABLE_NAME})
        endif()
        add_custom_command(TARGET ${WS_APP_TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                           ${CMAKE_CURRENT_SOURCE_DIR}/client-common/licenses/open_source_licenses.txt $<TARGET_FILE_DIR:${WS_APP_TARGET}>/../Resources/open_source_licenses.txt)
    endif()
endif()

if (WIN32)
    target_link_options(${WS_APP_TARGET} PRIVATE "/IGNORE:4099")
elseif (APPLE)
    set_property(TARGET ${WS_APP_TARGET} APPEND_STRING PROPERTY COMPILE_FLAGS "-fobjc-arc")
endif()

target_link_libraries(${WS_APP_TARGET} PRIVATE ${FRONTEND_TYPE} frontend-common ${WS_CLIENT_EXTRA_LIBS} engine client-common wsnet::wsnet spdlog::spdlog Qt6::Network ${OS_SPECIFIC_LIBRARIES})

if (UNIX AND (NOT APPLE))
    qt_import_plugins(${WS_APP_TARGET} INCLUDE Qt6::QWaylandIntegrationPlugin Qt6::QXcbIntegrationPlugin)
endif()

target_include_directories(${WS_APP_TARGET} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/client-common
    ${CMAKE_CURRENT_SOURCE_DIR}/frontend/frontend-common
)

# Configure Info.plist with DEVELOPMENT_TEAM substitution
if(APPLE)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/info.plist
        ${CMAKE_CURRENT_BINARY_DIR}/info.plist.configured
        @ONLY
    )
    set(WINDSCRIBE_INFO_PLIST ${CMAKE_CURRENT_BINARY_DIR}/info.plist.configured)

    # Configure entitlements with DEVELOPMENT_TEAM substitution
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/engine/mac/engine.entitlements
        ${CMAKE_CURRENT_BINARY_DIR}/engine.entitlements.configured
        @ONLY
    )
    set(WINDSCRIBE_ENTITLEMENTS ${CMAKE_CURRENT_BINARY_DIR}/engine.entitlements.configured)
else()
    set(WINDSCRIBE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/info.plist)
endif()

set_target_properties(${WS_APP_TARGET} PROPERTIES
    WIN32_EXECUTABLE TRUE
    COMPILE_WARNING_AS_ERROR TRUE
)

if(APPLE)
    set_target_properties(${WS_APP_TARGET} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${WINDSCRIBE_INFO_PLIST}
        OUTPUT_NAME "${WS_PRODUCT_NAME}"
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "@executable_path/../Frameworks"
    )
endif()

add_subdirectory(engine)
add_subdirectory(client-common)
add_subdirectory(frontend)

qt_finalize_executable(${WS_APP_TARGET})

# ----- Install section -----
if (APPLE)
    install(TARGETS ${WS_APP_TARGET} wsnet
        RUNTIME DESTINATION .
        LIBRARY DESTINATION "${WS_PRODUCT_NAME}.app/Contents/Frameworks"
        BUNDLE DESTINATION .
    )
    install(FILES ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/libcrypto.3.dylib DESTINATION "${WS_PRODUCT_NAME}.app/Contents/Frameworks")
    install(FILES ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/libssl.3.dylib DESTINATION "${WS_PRODUCT_NAME}.app/Contents/Frameworks")
else()
    install(TARGETS ${WS_APP_TARGET}
        RUNTIME DESTINATION .
        LIBRARY DESTINATION "lib"
        BUNDLE DESTINATION .
    )
endif()

if(WIN32 OR (UNIX AND NOT APPLE))
    foreach(_entry ${WS_BUNDLED_HELPERS})
        string(REPLACE "|" ";" _pair "${_entry}")
        list(GET _pair 0 _src)
        list(GET _pair 1 _dest)
        install(FILES "${_src}"
                RENAME "${_dest}"
                DESTINATION .
                PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        )
    endforeach()
endif()

if(UNIX AND (NOT APPLE))
    install(IMPORTED_RUNTIME_ARTIFACTS wsnet DESTINATION lib)
    install(FILES ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/libcrypto.so.3 DESTINATION lib)
    install(FILES ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/libssl.so.3 DESTINATION lib)
endif()
