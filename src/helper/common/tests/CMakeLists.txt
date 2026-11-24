if(IS_BUILD_TESTS)
    find_package(spdlog CONFIG REQUIRED)
    if(NOT WIN32)
        find_package(Boost CONFIG REQUIRED COMPONENTS serialization)
        find_package(skyr-url CONFIG REQUIRED)
    endif()

    add_executable(ipaddressrange.test
        ipaddressrange.test.cpp
        ../../../client/client-common/types/ipaddress.cpp
    )
    target_include_directories(ipaddressrange.test PRIVATE ../../../client/client-common)
    if(WIN32)
        target_link_libraries(ipaddressrange.test PRIVATE Ws2_32)
    endif()
    set_target_properties(ipaddressrange.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
    ws_add_test(ipaddressrange)

    add_executable(ovpn_early_exit_log.test
        ovpn_early_exit_log.test.cpp
        ../ovpn_early_exit_log.cpp
    )
    set_target_properties(ovpn_early_exit_log.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
    ws_add_test(ovpn_early_exit_log)

    add_executable(ovpndirectivewhitelist.test
        ovpndirectivewhitelist.test.cpp
    )
    # The filter validates dhcp-option DNS values through the platform's existing address validator,
    # so the test compiles that validator's source directly (the helper has no library form): the
    # POSIX validation_posix.cpp on macOS/Linux, network_validation.cpp on Windows.
    if(WIN32)
        target_sources(ovpndirectivewhitelist.test PRIVATE ../../windows/network_validation.cpp)
        target_include_directories(ovpndirectivewhitelist.test PRIVATE ../../windows)
        target_link_libraries(ovpndirectivewhitelist.test PRIVATE Ws2_32 Dnsapi Ole32 Ntdll spdlog::spdlog)
    else()
        target_sources(ovpndirectivewhitelist.test PRIVATE
            ../validation_posix.cpp
            ../../../client/client-common/types/ipaddress.cpp
        )
        target_include_directories(ovpndirectivewhitelist.test PRIVATE
            ..
            ../../../client/client-common
        )
        target_link_libraries(ovpndirectivewhitelist.test PRIVATE
            Boost::serialization
            skyr::skyr-url
            spdlog::spdlog
        )
    endif()
    set_target_properties(ovpndirectivewhitelist.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
    ws_add_test(ovpndirectivewhitelist)

    if(NOT WIN32)
        add_executable(spawn.test spawn.test.cpp ../spawn_posix.cpp ../io_posix.cpp)
        target_link_libraries(spawn.test PRIVATE spdlog::spdlog)
        set_target_properties(spawn.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
        ws_add_test(spawn)

        add_executable(run_command.test run_command.test.cpp ../spawn_posix.cpp ../io_posix.cpp)
        target_link_libraries(run_command.test PRIVATE spdlog::spdlog)
        set_target_properties(run_command.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
        ws_add_test(run_command)

        add_executable(process_kill.test
            process_kill.test.cpp
            ../process_kill_posix.cpp
            ../../../client/client-common/utils/log/debug_trace.cpp
        )
        target_include_directories(process_kill.test PRIVATE ../../../client/client-common)
        target_compile_definitions(process_kill.test PRIVATE DEBUG_TRACE_SPDLOG)
        target_link_libraries(process_kill.test PRIVATE spdlog::spdlog)
        set_target_properties(process_kill.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
        ws_add_test(process_kill)

        add_executable(default_gateway.test
            default_gateway.test.cpp
            ../default_gateway_posix.cpp
            ../../../client/client-common/types/ipaddress.cpp
        )
        target_include_directories(default_gateway.test PRIVATE ../../../client/client-common)
        set_target_properties(default_gateway.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
        ws_add_test(default_gateway)

        add_executable(wg_dns_routes.test
            wg_dns_routes.test.cpp
            ../wg_dns_routes.cpp
            ../../../client/client-common/types/ipaddress.cpp
        )
        target_include_directories(wg_dns_routes.test PRIVATE ../../../client/client-common)
        set_target_properties(wg_dns_routes.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
        ws_add_test(wg_dns_routes)

        if(NOT APPLE)
            add_executable(netlink_route.test netlink_route.test.cpp ../netlink_route_linux.cpp)
            set_target_properties(netlink_route.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
            ws_add_test(netlink_route)
        endif()

        add_executable(validation.test
            validation.test.cpp
            ../validation_posix.cpp
            ../../../client/client-common/types/ipaddress.cpp
        )
        target_include_directories(validation.test PRIVATE
            ..
            ../../../client/client-common
        )
        target_link_libraries(validation.test PRIVATE
            Boost::serialization
            skyr::skyr-url
            spdlog::spdlog
        )
        set_target_properties(validation.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
        ws_add_test(validation)
    endif()
endif(IS_BUILD_TESTS)
