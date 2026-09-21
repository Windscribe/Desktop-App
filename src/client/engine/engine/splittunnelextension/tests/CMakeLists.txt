if(IS_BUILD_TESTS)
    add_executable(splittunnelextensionmanager.test
        splittunnelextensionmanager.test.mm
        splittunnelextensionmanager.test.h)
    target_link_libraries(splittunnelextensionmanager.test PRIVATE
        Qt6::Network Qt6::Test engine client-common wsnet::wsnet spdlog::spdlog ${OS_SPECIFIC_LIBRARIES})
    target_include_directories(splittunnelextensionmanager.test PRIVATE
        ${PROJECT_DIRECTORY}/engine ${PROJECT_DIRECTORY}/engine/engine ${PROJECT_DIRECTORY}/client-common)
    set_target_properties(splittunnelextensionmanager.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
    ws_add_test(splittunnelextensionmanager)
endif()
