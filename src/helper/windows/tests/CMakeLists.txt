if(IS_BUILD_TESTS)
    add_executable(phantomwireguarddevices.test
        phantomwireguarddevices.test.cpp
        ../wireguard/phantom_wireguard_devices.cpp
    )
    target_link_libraries(phantomwireguarddevices.test PRIVATE
        Cfgmgr32
        Setupapi
        spdlog::spdlog
    )
    target_include_directories(phantomwireguarddevices.test PRIVATE
        ..
        ../../../client/client-common
        ${WINREG_INCLUDE_DIRS}
    )
    set_target_properties(phantomwireguarddevices.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
    ws_add_test(phantomwireguarddevices)
    # The test creates real devnodes, so it self-skips without elevation instead of failing.
    set_tests_properties(phantomwireguarddevices PROPERTIES SKIP_RETURN_CODE 77)

    # NUL-smuggling regression tests for the address validators. Compiles network_validation.cpp
    # directly rather than linking the helper, which has no library form.
    add_executable(networkvalidation.test
        networkvalidation.test.cpp
        ../network_validation.cpp
    )
    target_link_libraries(networkvalidation.test PRIVATE
        Ws2_32
        Dnsapi
        Ole32
        Ntdll
    )
    target_include_directories(networkvalidation.test PRIVATE
        ..
    )
    set_target_properties(networkvalidation.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
    ws_add_test(networkvalidation)

    add_executable(ovpnmsgparse.test
        ovpnmsgparse.test.cpp
        ../openvpn/ovpn_msg_parse.cpp
    )
    target_link_libraries(ovpnmsgparse.test PRIVATE
        Ws2_32
    )
    target_include_directories(ovpnmsgparse.test PRIVATE
        ..
        "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include"
    )
    set_target_properties(ovpnmsgparse.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
    ws_add_test(ovpnmsgparse)

    add_executable(wireguardconfigwriter.test
        wireguardconfigwriter.test.cpp
        ../wireguard/wireguardconfigwriter.cpp
        ../network_validation.cpp
    )
    target_link_libraries(wireguardconfigwriter.test PRIVATE
        Ws2_32
        Dnsapi
        Ole32
        Ntdll
        Boost::serialization
        spdlog::spdlog
    )
    target_include_directories(wireguardconfigwriter.test PRIVATE
        ..
        ../../../client/client-common
    )
    set_target_properties(wireguardconfigwriter.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
    ws_add_test(wireguardconfigwriter)

    # Runs against the real Base Filtering Engine in a dynamic session, so it self-skips without
    # elevation instead of failing.
    add_executable(wfputils.test
        wfputils.test.cpp
        ../wfp_utils.cpp
        ../apps_ids.cpp
        ../../../client/client-common/types/ipaddress.cpp
        ../../../client/client-common/utils/log/debug_trace.cpp
    )
    target_link_libraries(wfputils.test PRIVATE
        Fwpuclnt
        Rpcrt4
        Ws2_32
        spdlog::spdlog
    )
    target_include_directories(wfputils.test PRIVATE
        ..
        ../../../client/client-common
    )
    set_target_properties(wfputils.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
    ws_add_test(wfputils)
    set_tests_properties(wfputils PROPERTIES SKIP_RETURN_CODE 77)

    add_executable(firewallincremental.test
        firewallincremental.test.cpp
    )
    target_include_directories(firewallincremental.test PRIVATE
        ..
    )
    set_target_properties(firewallincremental.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
    ws_add_test(firewallincremental)
endif(IS_BUILD_TESTS)
