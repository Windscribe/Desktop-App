if(IS_BUILD_TESTS)
    add_executable(connectionmanager.test
        connectionmanager.test.cpp
        connectionmanager.test.h
        fakes.h
    )
    target_link_libraries(connectionmanager.test PRIVATE
        Qt6::Network Qt6::Test engine client-common wsnet::wsnet spdlog::spdlog ${OS_SPECIFIC_LIBRARIES})
    if(WIN32)
        target_compile_definitions(connectionmanager.test PRIVATE WIN32_LEAN_AND_MEAN PIO_APC_ROUTINE_DEFINED)
    endif()
    target_include_directories(connectionmanager.test PRIVATE
        ${PROJECT_DIRECTORY}/engine
        ${PROJECT_DIRECTORY}/client-common
    )
    set_target_properties(connectionmanager.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
    ws_add_test(connectionmanager)
endif(IS_BUILD_TESTS)
