if(IS_BUILD_TESTS)
    find_package(Boost CONFIG REQUIRED COMPONENTS serialization)
    find_package(skyr-url CONFIG REQUIRED)
    find_package(spdlog CONFIG REQUIRED)

    # Runs the real Utils::executeCommand, so utils.cpp is compiled in together with the
    # validation and process-start code it calls (the helper has no library form).
    add_executable(helperutils.test
        helperutils.test.cpp
        ../utils.cpp
        ../../common/io_posix.cpp
        ../../common/spawn_posix.cpp
        ../../common/validation_posix.cpp
        ../../../client/client-common/types/ipaddress.cpp
    )
    target_include_directories(helperutils.test PRIVATE
        ../../../client/client-common
    )
    target_link_libraries(helperutils.test PRIVATE
        Boost::serialization
        skyr::skyr-url
        spdlog::spdlog
    )
    set_target_properties(helperutils.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
    ws_add_test(helperutils)

    find_package(RapidJSON CONFIG REQUIRED)
    add_executable(mainroutetable.test
        mainroutetable.test.cpp
        ../split_tunneling/hostnames_manager/main_route_table.cpp
        ../utils.cpp
        ../../common/io_posix.cpp
        ../../common/spawn_posix.cpp
        ../../common/validation_posix.cpp
        ../../../client/client-common/types/ipaddress.cpp
    )
    target_include_directories(mainroutetable.test PRIVATE
        ../../../client/client-common
    )
    target_link_libraries(mainroutetable.test PRIVATE
        Boost::serialization
        skyr::skyr-url
        spdlog::spdlog
        rapidjson
    )
    set_target_properties(mainroutetable.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
    ws_add_test(mainroutetable)

    add_executable(defaultroutemonitor.test
        defaultroutemonitor.test.cpp
        ../../common/default_gateway_posix.cpp
        ../../common/defaultroutemonitor_posix.cpp
        ../utils.cpp
        ../../common/io_posix.cpp
        ../../common/spawn_posix.cpp
        ../../common/validation_posix.cpp
        ../../../client/client-common/types/ipaddress.cpp
    )
    target_include_directories(defaultroutemonitor.test PRIVATE
        ../../../client/client-common
    )
    target_link_libraries(defaultroutemonitor.test PRIVATE
        pthread
        Boost::serialization
        skyr::skyr-url
        spdlog::spdlog
    )
    set_target_properties(defaultroutemonitor.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
    ws_add_test(defaultroutemonitor)

    add_executable(nftablescontroller.test
        nftablescontroller.test.cpp
        ../nftables/nftablescontroller.cpp
        ../../../client/client-common/utils/log/debug_trace.cpp
    )
    target_compile_definitions(nftablescontroller.test PRIVATE DEBUG_TRACE_SPDLOG)
    target_include_directories(nftablescontroller.test PRIVATE
        ../../../client/client-common
        ${NFTABLES_INCLUDE_DIR}
    )
    target_link_libraries(nftablescontroller.test PRIVATE
        spdlog::spdlog
        ${NFTABLES_LIB}
    )
    set_target_properties(nftablescontroller.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
    ws_add_test(nftablescontroller)
    set_tests_properties(nftablescontroller PROPERTIES SKIP_RETURN_CODE 77)

    # Reads the integration's NetworkManager drop-in from the installer tree; skips if it ships none.
    add_executable(tunnelinterfacenames.test tunnelinterfacenames.test.cpp)
    target_compile_definitions(tunnelinterfacenames.test PRIVATE
        WS_NM_DROP_IN_PATH="${CMAKE_SOURCE_DIR}/src/installer/${WS_PRODUCT_NAME_LOWER}/linux/usr/lib/NetworkManager/conf.d/90-${WS_PRODUCT_NAME_LOWER}.conf"
    )
    set_target_properties(tunnelinterfacenames.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
    ws_add_test(tunnelinterfacenames)
    set_tests_properties(tunnelinterfacenames PROPERTIES SKIP_RETURN_CODE 77)
endif(IS_BUILD_TESTS)
