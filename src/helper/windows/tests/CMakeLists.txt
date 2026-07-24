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
endif(IS_BUILD_TESTS)
