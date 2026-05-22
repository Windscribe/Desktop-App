if(IS_BUILD_TESTS)
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
endif(IS_BUILD_TESTS)
