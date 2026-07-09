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
endif(IS_BUILD_TESTS)
