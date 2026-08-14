if(IS_BUILD_TESTS)
    find_package(spdlog CONFIG REQUIRED)
    if(NOT WIN32)
        find_package(Boost CONFIG REQUIRED COMPONENTS serialization)
        find_package(skyr-url CONFIG REQUIRED)
    endif()

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
