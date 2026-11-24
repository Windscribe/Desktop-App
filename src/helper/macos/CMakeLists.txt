find_package(Boost REQUIRED COMPONENTS serialization thread)
find_package(skyr-url CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

set(SOURCES
    ../../client/client-common/types/ipaddress.cpp
    ../../client/client-common/types/ipaddress.h
    ../../client/client-common/utils/executable_signature/executable_signature.cpp
    ../../client/client-common/utils/executable_signature/executable_signature.h
    ../../client/client-common/utils/executable_signature/executable_signature_mac.mm
    ../../client/client-common/utils/executable_signature/executable_signature.h
    ../../client/client-common/utils/log/debug_trace.cpp
    ../../client/client-common/utils/log/debug_trace.h
    ../common/default_gateway_posix.cpp
    ../common/default_gateway_posix.h
    ../common/defaultroutemonitor_posix.cpp
    ../common/defaultroutemonitor_posix.h
    ../common/io_posix.cpp
    ../common/io_posix.h
    ../common/ovpn_early_exit_log.cpp
    ../common/ovpn_mgmt_posix.cpp
    ../common/ovpn_mgmt_posix.h
    ../common/ovpn_posix.cpp
    ../common/ovpn_posix.h
    ../common/process_command_posix.cpp
    ../common/process_command_posix.h
    ../common/process_kill_posix.cpp
    ../common/process_kill_posix.h
    ../common/spawn_posix.cpp
    ../common/spawn_posix.h
    ../common/validation_posix.cpp
    ../common/validation_posix.h
    ../common/wg_dns_routes.cpp
    ../common/wg_dns_routes.h
    ../common/wg_handshake_tracker.h
    ../common/wireguardcommunicator_posix.cpp
    ../common/wireguardcommunicator_posix.h
    appuser.cpp
    appuser.h
    clear_wifi_history/clear_wifi_history.mm
    directory_mac.h
    directory_mac.mm
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
    installer/staging.cpp
    installer/staging.h
    ipc/helper_security.h
    ipc/helper_security.mm
    macutils.h
    macutils.mm
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
    wireguard/wireguardadapter.cpp
    wireguard/wireguardadapter.h
    wireguard/wireguardcontroller.cpp
    wireguard/wireguardcontroller.h
)

# Configure helper-info.plist with DEVELOPMENT_TEAM substitution
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/helper-info.plist
    ${CMAKE_CURRENT_BINARY_DIR}/helper-info.plist.configured
    @ONLY
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/helper-launchd.plist
    ${CMAKE_CURRENT_BINARY_DIR}/helper-launchd.plist.configured
    @ONLY
)

add_executable(${WS_MAC_HELPER_BUNDLE_ID} ${SOURCES})
set_property(TARGET ${WS_MAC_HELPER_BUNDLE_ID} PROPERTY COMPILE_WARNING_AS_ERROR ON)
target_compile_definitions(${WS_MAC_HELPER_BUNDLE_ID} PRIVATE DEBUG_TRACE_SPDLOG)

target_link_libraries(${WS_MAC_HELPER_BUNDLE_ID}
                      PRIVATE
                      Boost::serialization
                      Boost::thread
                      skyr::skyr-url
                      spdlog::spdlog
                      "-framework Foundation"
                      "-framework OpenDirectory"
                      "-framework Security"
                      "-framework SystemConfiguration"
                      "-sectcreate __TEXT __info_plist ${CMAKE_CURRENT_BINARY_DIR}/helper-info.plist.configured"
                      "-sectcreate __TEXT __launchd_plist ${CMAKE_CURRENT_BINARY_DIR}/helper-launchd.plist.configured"
)

# Sign helper immediately after building (required for SMJobBless)
add_custom_command(TARGET ${WS_MAC_HELPER_BUNDLE_ID} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Signing helper..."
    COMMAND ${CODESIGN_EXECUTABLE}
            --force
            --options runtime
            --timestamp
            --sign "Developer ID Application"
            $<TARGET_FILE:${WS_MAC_HELPER_BUNDLE_ID}>
    COMMAND ${CODESIGN_EXECUTABLE} -v $<TARGET_FILE:${WS_MAC_HELPER_BUNDLE_ID}>
)

target_include_directories(${WS_MAC_HELPER_BUNDLE_ID} PRIVATE
                           ../../client/client-common
)
