find_package(Boost REQUIRED COMPONENTS serialization thread)
find_package(skyr-url CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

# build_all.py sets this option when invoked with the '--sign' flag. Disabled by default
option(DEFINE_USE_SIGNATURE_CHECK_MACRO "Add define USE_SIGNATURE_CHECK to project" OFF)
if(DEFINE_USE_SIGNATURE_CHECK_MACRO)
    ADD_DEFINITIONS(-DUSE_SIGNATURE_CHECK)
endif(DEFINE_USE_SIGNATURE_CHECK_MACRO)

set(SOURCES
    ../../client/common/utils/executable_signature/executable_signature.cpp
    ../../client/common/utils/executable_signature/executable_signature.h
    ../../client/common/utils/executable_signature/executable_signature_mac.mm
    ../../client/common/utils/executable_signature/executable_signature.h
    3rdparty/pstream.h
    execute_cmd.cpp
    execute_cmd.h
    files_manager.cpp
    files_manager.h
    firewallcontroller.cpp
    firewallcontroller.h
    firewallonboot.cpp
    firewallonboot.h
    helper.cpp
    installer/files.cpp
    installer/files.h
    installer/iinstall_block.h
    ipc/helper_security.h
    ipc/helper_security.mm
    ipv6/ipv6manager.cpp
    ipv6/ipv6manager.h
    macspoofingonboot.cpp
    macspoofingonboot.h
    macutils.h
    macutils.mm
    ovpn.cpp
    ovpn.h
    process_command.cpp
    process_command.h
    routes_manager/bound_route.cpp
    routes_manager/bound_route.h
    routes_manager/routes.cpp
    routes_manager/routes.h
    routes_manager/routes_manager.cpp
    routes_manager/routes_manager.h
    server.cpp
    server.h
    utils.cpp
    utils.h
)

# -Wno-shorten-64-to-32 to silence warnings in pstream.h
set(CMAKE_CXX_FLAGS "-Wno-shorten-64-to-32")

add_executable(com.windscribe.helper.macos ${SOURCES})
set_property(TARGET com.windscribe.helper.macos PROPERTY COMPILE_WARNING_AS_ERROR ON)

target_link_libraries(com.windscribe.helper.macos
                      PRIVATE
                      Boost::serialization
                      Boost::thread
                      skyr::skyr-url
                      spdlog::spdlog
                      "-framework Foundation"
                      "-framework Security"
                      "-framework SystemConfiguration"
                      "-sectcreate __TEXT __info_plist ${CMAKE_CURRENT_SOURCE_DIR}/helper-info.plist"
                      "-sectcreate __TEXT __launchd_plist ${CMAKE_CURRENT_SOURCE_DIR}/helper-launchd.plist"
)
set_target_properties(com.windscribe.helper.macos PROPERTIES
        XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++20"
        XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++"
        XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC "YES"
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Developer ID Application"
        XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Manual"
        XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${DEVELOPMENT_TEAM}"
        XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES"
        XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "12"
        XCODE_ATTRIBUTE_PROVISIONING_PROFILE ""
        XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER ""
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.windscribe.helper.macos"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_NAME "com.windscribe.helper.macos"
)


target_include_directories(com.windscribe.helper.macos PRIVATE
                           ../../client/common
)
