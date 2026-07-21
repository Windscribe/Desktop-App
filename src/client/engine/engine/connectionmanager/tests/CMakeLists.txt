if(IS_BUILD_TESTS)
    # Adds <name>.test built from <name>.test.cpp/.h plus any extra sources in ARGN.
    function(add_connectionmanager_test name)
        add_executable(${name}.test
            ${name}.test.cpp
            ${name}.test.h
            ${ARGN}
        )
        target_link_libraries(${name}.test PRIVATE
            Qt6::Network Qt6::Test engine client-common wsnet::wsnet spdlog::spdlog ${OS_SPECIFIC_LIBRARIES})
        if(WIN32)
            target_compile_definitions(${name}.test PRIVATE WIN32_LEAN_AND_MEAN PIO_APC_ROUTINE_DEFINED)
        endif()
        target_include_directories(${name}.test PRIVATE
            ${PROJECT_DIRECTORY}/engine
            ${PROJECT_DIRECTORY}/client-common
        )
        set_target_properties(${name}.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
        ws_add_test(${name})
    endfunction()

    add_connectionmanager_test(connectionmanager fakes.h)
    add_connectionmanager_test(emergencyconnsettingspolicy)
    add_connectionmanager_test(ikev2connection fakes.h)
    add_connectionmanager_test(openvpnconnection fakes.h)
    add_connectionmanager_test(wireguardconnection)
endif(IS_BUILD_TESTS)
