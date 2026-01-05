cmake_minimum_required(VERSION 3.23)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(X_VCPKG_APPLOCAL_DEPS_INSTALL ON)

set(CMAKE_OSX_DEPLOYMENT_TARGET "12" CACHE STRING "Minimum OS X deployment version")

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(Windscribe)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS serialization)
find_package(spdlog CONFIG REQUIRED)

if(NOT TARGET wsnet::wsnet)
    add_subdirectory(../../libs/wsnet ${CMAKE_CURRENT_BINARY_DIR}/wsnet)
endif()

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

if(UNIX AND (NOT APPLE))
    option(DEFINE_CLI_ONLY_MACRO "Build GUI-less client" OFF)
    # if build requested a headless client, set the necessary definitions
    if(DEFINE_CLI_ONLY_MACRO)
        add_definitions(-DCLI_ONLY)
        find_package(Qt6 REQUIRED COMPONENTS Network LinguistTools Test)
    endif(DEFINE_CLI_ONLY_MACRO)
endif()

if(NOT DEFINE_CLI_ONLY_MACRO)
    # This is a workaround for https://bugreports.qt.io/browse/QTBUG-89754 / #499
    # Without this flag, on Linux libOpenGL.so.0 will be linked instead of libGL,
    # which causes Windscribe to not start on some distributions where libopengl0 is not
    # installed by default.

    set(OpenGL_GL_PREFERENCE LEGACY)
    find_package(Qt6 REQUIRED COMPONENTS Widgets Network LinguistTools Test)
endif()

set(PROJECT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

if(DEFINE_CLI_ONLY_MACRO)
    set(PROJECT_SOURCES main_cli.cpp)
    set(PROJECT_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/cli)
else()
    set(PROJECT_SOURCES main_gui.cpp)
    set(PROJECT_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/gui)
endif()

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

    list(APPEND PROJECT_SOURCES client.rc)
    # Generate debug information (symbol files) for Windows (release build only)
    add_compile_options("$<$<CONFIG:Release>:/Zi>")
    add_link_options("$<$<CONFIG:Release>:/DEBUG>" "$<$<CONFIG:Release>:/OPT:REF>" "$<$<CONFIG:Release>:/OPT:ICF>")

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
    set(MAC_APP_ICON ${CMAKE_CURRENT_SOURCE_DIR}/gui/resources/icons/Mac/windscribe.icns)
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

if (WIN32)
    #postbuild copy commands for Windows
    add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                      ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/openvpn/openvpn.exe $<TARGET_FILE_DIR:Windscribe>/windscribeopenvpn.exe)

    add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                                    ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/ctrld/ctrld.exe $<TARGET_FILE_DIR:Windscribe>/windscribectrld.exe)

elseif (APPLE)
    #postbuild copy commands for Mac
    set(WINDSCRIBE_BUILD_LIBS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../build-libs)

    # Copy launcher bundle
    add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory
                       $<TARGET_BUNDLE_DIR:WindscribeLauncher> $<TARGET_FILE_DIR:Windscribe>/../Library/LoginItems)

    # Copy helper executable
    add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory
                       $<TARGET_FILE_DIR:Windscribe>/../Library/LaunchServices)
    add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                       $<TARGET_FILE:com.windscribe.helper.macos> $<TARGET_FILE_DIR:Windscribe>/../Library/LaunchServices/)

    # Copy openssl, wsnet, and post-quantum libs to Frameworks
    add_custom_command(TARGET Windscribe POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Copying dependency libraries to Frameworks..."
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/libcrypto.3.dylib
                $<TARGET_FILE_DIR:Windscribe>/../Frameworks/libcrypto.3.dylib
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/libssl.3.dylib
                $<TARGET_FILE_DIR:Windscribe>/../Frameworks/libssl.3.dylib
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:wsnet::wsnet>
                $<TARGET_FILE_DIR:Windscribe>/../Frameworks/
        COMMAND ${CMAKE_COMMAND} -E echo "Fixing RPATH on wsnet..."
        COMMAND install_name_tool -add_rpath "@loader_path" $<TARGET_FILE_DIR:Windscribe>/../Frameworks/libwsnet.dylib
    )

    add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:Windscribe>/../Helpers)

    add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                       ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/openvpn/sbin/openvpn $<TARGET_FILE_DIR:Windscribe>/../Helpers/windscribeopenvpn)

    # Copy split tunnel extension
    add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory
                       $<TARGET_BUNDLE_DIR:com.windscribe.client.splittunnelextension> $<TARGET_FILE_DIR:Windscribe>/../Library/SystemExtensions/com.windscribe.client.splittunnelextension.systemextension)

    # apply install_name_tool commands for the windscribeopenvpn binary, fix for the openssl libs
    add_custom_command(TARGET Windscribe POST_BUILD
        COMMAND bash -c [[OLD=$(otool -L ${0} | grep ${1} | uniq | sed s/.dylib.*/.dylib/ | awk '{$1=$1;print}'); install_name_tool -change ${OLD} @executable_path/../Frameworks/${1}.3.dylib ${0}]] $<TARGET_FILE_DIR:Windscribe>/../Helpers/windscribeopenvpn libssl
        COMMAND bash -c [[OLD=$(otool -L ${0} | grep ${1} | uniq | sed s/.dylib.*/.dylib/ | awk '{$1=$1;print}'); install_name_tool -change ${OLD} @executable_path/../Frameworks/${1}.3.dylib ${0}]] $<TARGET_FILE_DIR:Windscribe>/../Helpers/windscribeopenvpn libcrypto
        VERBATIM
    )

    add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                       ${WINDSCRIBE_BUILD_LIBS_PATH}/wireguard/windscribewireguard $<TARGET_FILE_DIR:Windscribe>/../Helpers/windscribewireguard)

    add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                      ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/ctrld/ctrld $<TARGET_FILE_DIR:Windscribe>/../Helpers/windscribectrld)

    add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                      ${WINDSCRIBE_BUILD_LIBS_PATH}/wstunnel/windscribewstunnel $<TARGET_FILE_DIR:Windscribe>/../Helpers/windscribewstunnel)

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../data/provisioning_profile/embedded.provisionprofile")
        add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                           "${CMAKE_CURRENT_SOURCE_DIR}/../../data/provisioning_profile/embedded.provisionprofile" $<TARGET_FILE_DIR:Windscribe>/../embedded.provisionprofile)
    endif()

    add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                       ${CMAKE_CURRENT_SOURCE_DIR}/../../build/src/windscribe-cli/windscribe-cli $<TARGET_FILE_DIR:Windscribe>/windscribe-cli)
    add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                       ${CMAKE_CURRENT_SOURCE_DIR}/common/licenses/open_source_licenses.txt $<TARGET_FILE_DIR:Windscribe>/../Resources/open_source_licenses.txt)
endif()

if (WIN32)
    target_link_options(Windscribe PRIVATE "/IGNORE:4099")
elseif (APPLE)
    set_property(TARGET Windscribe APPEND_STRING PROPERTY COMPILE_FLAGS "-fobjc-arc")
endif()

if(UNIX AND (NOT APPLE) AND DEFINE_CLI_ONLY_MACRO)
    set(CLIENT_LIBS cli base)
else()
    set(CLIENT_LIBS gui base Qt6::Widgets)
endif()

target_link_libraries(Windscribe PRIVATE ${CLIENT_LIBS} engine common wsnet::wsnet spdlog::spdlog Qt6::Network ${OS_SPECIFIC_LIBRARIES})

if (UNIX AND (NOT APPLE))
    qt_import_plugins(Windscribe INCLUDE Qt6::QWaylandIntegrationPlugin Qt6::QXcbIntegrationPlugin)
endif()

target_include_directories(Windscribe PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/base
    ${PROJECT_INCLUDE_DIRS}
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
        ${CMAKE_CURRENT_SOURCE_DIR}/engine/mac/windscribe_engine.entitlements
        ${CMAKE_CURRENT_BINARY_DIR}/windscribe_engine.entitlements.configured
        @ONLY
    )
    set(WINDSCRIBE_ENTITLEMENTS ${CMAKE_CURRENT_BINARY_DIR}/windscribe_engine.entitlements.configured)
else()
    set(WINDSCRIBE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/info.plist)
endif()

set_target_properties(Windscribe PROPERTIES
    WIN32_EXECUTABLE TRUE
    COMPILE_WARNING_AS_ERROR TRUE
)

if(APPLE)
    set_target_properties(Windscribe PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${WINDSCRIBE_INFO_PLIST}
        XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME YES
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "@executable_path/../Frameworks"
    )
endif()

add_subdirectory(engine)
add_subdirectory(common)
add_subdirectory(base)
if(UNIX AND (NOT APPLE) AND DEFINE_CLI_ONLY_MACRO)
    add_subdirectory(cli)
else()
    add_subdirectory(gui)
endif()

qt_finalize_executable(Windscribe)

# ----- Install section -----
if (APPLE)
    install(TARGETS Windscribe wsnet
        RUNTIME DESTINATION .
        LIBRARY DESTINATION "Windscribe.app/Contents/Frameworks"
        BUNDLE DESTINATION .
    )
    install(FILES ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/libcrypto.3.dylib DESTINATION Windscribe.app/Contents/Frameworks)
    install(FILES ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/libssl.3.dylib DESTINATION Windscribe.app/Contents/Frameworks)
else()
    install(TARGETS Windscribe
        RUNTIME DESTINATION .
        LIBRARY DESTINATION "lib"
        BUNDLE DESTINATION .
    )
endif()

if (WIN32)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/windscribeopenvpn.exe DESTINATION .)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/windscribectrld.exe DESTINATION .)
endif()

if(UNIX AND (NOT APPLE))
    install(IMPORTED_RUNTIME_ARTIFACTS wsnet DESTINATION lib)
    install(FILES ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/libcrypto.so.3 DESTINATION lib)
    install(FILES ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/libssl.so.3 DESTINATION lib)

    # copy openvpn
    install(FILES ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/openvpn/sbin/openvpn
            RENAME windscribeopenvpn
            DESTINATION .
            PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    )
    # copy ctrld
    install(FILES ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/ctrld/ctrld
            RENAME windscribectrld
            DESTINATION .
            PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    )
endif()
