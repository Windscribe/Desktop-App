if(IS_BUILD_TESTS)
    add_executable(ovpncredentialinliner.test
        ovpncredentialinliner.test.cpp
        ovpncredentialinliner.test.h
    )
    target_link_libraries(ovpncredentialinliner.test PRIVATE
        Qt6::Test engine client-common spdlog::spdlog ${OS_SPECIFIC_LIBRARIES})
    target_include_directories(ovpncredentialinliner.test PRIVATE
        ${PROJECT_DIRECTORY}/engine
        ${PROJECT_DIRECTORY}/client-common
    )
    set_target_properties(ovpncredentialinliner.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
    ws_add_test(ovpncredentialinliner)

    add_executable(wireguardcustomconfig.test
        wireguardcustomconfig.test.cpp
        wireguardcustomconfig.test.h
    )
    target_link_libraries(wireguardcustomconfig.test PRIVATE
        Qt6::Test engine client-common spdlog::spdlog ${OS_SPECIFIC_LIBRARIES})
    target_include_directories(wireguardcustomconfig.test PRIVATE
        ${PROJECT_DIRECTORY}/engine
        ${PROJECT_DIRECTORY}/client-common
    )
    set_target_properties(wireguardcustomconfig.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
    ws_add_test(wireguardcustomconfig)
endif(IS_BUILD_TESTS)
