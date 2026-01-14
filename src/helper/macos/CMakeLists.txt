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
    wireguard/defaultroutemonitor.cpp
    wireguard/defaultroutemonitor.h
    wireguard/wireguardadapter.cpp
    wireguard/wireguardadapter.h
    wireguard/wireguardcommunicator.cpp
    wireguard/wireguardcommunicator.h
    wireguard/wireguardcontroller.cpp
    wireguard/wireguardcontroller.h
)

# -Wno-shorten-64-to-32 to silence warnings in pstream.h
set(CMAKE_CXX_FLAGS "-Wno-shorten-64-to-32")

# Configure helper-info.plist with DEVELOPMENT_TEAM substitution
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/helper-info.plist
    ${CMAKE_CURRENT_BINARY_DIR}/helper-info.plist.configured
    @ONLY
)

add_executable(com.windscribe.helper.macos ${SOURCES})
set_property(TARGET com.windscribe.helper.macos PROPERTY COMPILE_WARNING_AS_ERROR ON)

target_compile_options(com.windscribe.helper.macos PRIVATE "-fobjc-arc")

target_link_libraries(com.windscribe.helper.macos
                      PRIVATE
                      Boost::serialization
                      Boost::thread
                      skyr::skyr-url
                      spdlog::spdlog
                      "-framework Foundation"
                      "-framework Security"
                      "-framework SystemConfiguration"
                      "-sectcreate __TEXT __info_plist ${CMAKE_CURRENT_BINARY_DIR}/helper-info.plist.configured"
                      "-sectcreate __TEXT __launchd_plist ${CMAKE_CURRENT_SOURCE_DIR}/helper-launchd.plist"
)

# Sign helper immediately after building (required for SMJobBless)
add_custom_command(TARGET com.windscribe.helper.macos POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Signing helper..."
    COMMAND ${CODESIGN_EXECUTABLE}
            --force
            --options runtime
            --timestamp
            --sign "Developer ID Application"
            $<TARGET_FILE:com.windscribe.helper.macos>
    COMMAND ${CODESIGN_EXECUTABLE} -v $<TARGET_FILE:com.windscribe.helper.macos>
)

target_include_directories(com.windscribe.helper.macos PRIVATE
                           ../../client/common
)
