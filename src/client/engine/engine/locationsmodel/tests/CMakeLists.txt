if(IS_BUILD_TESTS)
    add_executable(customconfiglocationinfo.test
        customconfiglocationinfo.test.cpp
        customconfiglocationinfo.test.h
    )
    target_link_libraries(customconfiglocationinfo.test PRIVATE
        Qt6::Network Qt6::Test engine client-common spdlog::spdlog wsnet::wsnet ${OS_SPECIFIC_LIBRARIES})
    target_include_directories(customconfiglocationinfo.test PRIVATE
        ${PROJECT_DIRECTORY}/engine
        ${PROJECT_DIRECTORY}/client-common
    )
    set_target_properties(customconfiglocationinfo.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
    ws_add_test(customconfiglocationinfo)
endif(IS_BUILD_TESTS)
