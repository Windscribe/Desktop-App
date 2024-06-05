cmake_minimum_required(VERSION 3.23)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(X_VCPKG_APPLOCAL_DEPS_INSTALL ON)

set(CMAKE_OSX_DEPLOYMENT_TARGET "11" CACHE STRING "Minimum OS X deployment version")

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

find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS serialization)
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

set(PROJECT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

set(PROJECT_SOURCES
        main.cpp
)

if (WIN32)
    set (OS_SPECIFIC_LIBRARIES psapi.lib iphlpapi.lib dnsapi.lib rasapi32.lib pdh.lib Crypt32.lib Version.lib)
    list(APPEND PROJECT_SOURCES client.rc)
    # Generate debug information (symbol files) for Windows (release build only)
    add_compile_options("$<$<CONFIG:Release>:/Zi>")
    add_link_options("$<$<CONFIG:Release>:/DEBUG>" "$<$<CONFIG:Release>:/OPT:REF>" "$<$<CONFIG:Release>:/OPT:ICF>")

elseif (APPLE)
    set (OS_SPECIFIC_LIBRARIES
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
    set(WINDSCRIBE_BUILD_LIBS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../build-libs)

    add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_CURRENT_SOURCE_DIR}/../build/gui/launcher/mac/$<CONFIG>/ $<TARGET_FILE_DIR:Windscribe>/../Library/LoginItems)
    add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_CURRENT_SOURCE_DIR}/../build/backend/mac/helper/$<CONFIG>/ $<TARGET_FILE_DIR:Windscribe>/../Library/LaunchServices)

    add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:Windscribe>/../Helpers)

    add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                       ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/openvpn/sbin/openvpn $<TARGET_FILE_DIR:Windscribe>/../Helpers/windscribeopenvpn)

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

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../backend/mac/provisioning_profile/embedded.provisionprofile")
        add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                           "${CMAKE_CURRENT_SOURCE_DIR}/../backend/mac/provisioning_profile/embedded.provisionprofile" $<TARGET_FILE_DIR:Windscribe>/../embedded.provisionprofile)
    endif()

    add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                       ${CMAKE_CURRENT_SOURCE_DIR}/../build/gui/cli/windscribe-cli $<TARGET_FILE_DIR:Windscribe>/windscribe-cli)
    add_custom_command(TARGET Windscribe POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                       ${CMAKE_CURRENT_SOURCE_DIR}/common/licenses/open_source_licenses.txt $<TARGET_FILE_DIR:Windscribe>/../Resources/open_source_licenses.txt)
endif()

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
    COMPILE_WARNING_AS_ERROR TRUE
)

add_subdirectory(../libs/wsnet wsnet)

add_subdirectory(engine)
add_subdirectory(gui)
add_subdirectory(common)

qt_finalize_executable(Windscribe)

if(DEFINED IS_BUILD_TESTS)
    # Disabled for now, revisit later
    # enable_testing ()
    # add_test (NAME locationsmodel.test COMMAND locationsmodel.test)
endif (DEFINED IS_BUILD_TESTS)

# ----- Install section -----
install(TARGETS Windscribe wsnet
    RUNTIME DESTINATION .
    LIBRARY DESTINATION lib
    BUNDLE DESTINATION .
)

if (WIN32)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/windscribeopenvpn.exe DESTINATION .)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/windscribectrld.exe DESTINATION .)
endif()

if(UNIX AND (NOT APPLE))
    install(IMPORTED_RUNTIME_ARTIFACTS wsnet DESTINATION lib)
    # for some reason the installation via the command IMPORTED_RUNTIME_ARTIFACTS does not work for openssl
    # so for now we just copy the openssl libs
    install(FILES ${_OPENSSL_LIBDIR}/libcrypto.so.3 DESTINATION lib)
    install(FILES ${_OPENSSL_LIBDIR}/libssl.so.3 DESTINATION lib)

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
