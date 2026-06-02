if(IS_BUILD_TESTS)
    add_executable(locationsmodel.test
        locationsmodel.test.cpp
        locationsmodel.test.h
        locationsmodel.test.qrc
    )
    target_link_libraries(locationsmodel.test PRIVATE
        Qt6::Test frontend-common engine client-common spdlog::spdlog ${OS_SPECIFIC_LIBRARIES})
    target_include_directories(locationsmodel.test PRIVATE
        ${PROJECT_DIRECTORY}/frontend/frontend-common
        ${PROJECT_DIRECTORY}/client-common
    )
    set_target_properties(locationsmodel.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
    ws_add_test(locationsmodel)

    add_executable(networkingvalidation.test
        networkingvalidation.test.cpp
        networkingvalidation.test.h
    )
    target_link_libraries(networkingvalidation.test PRIVATE
        Qt6::Test Qt6::Network engine client-common spdlog::spdlog ${OS_SPECIFIC_LIBRARIES})
    target_include_directories(networkingvalidation.test PRIVATE
        ${PROJECT_DIRECTORY}/client-common
    )
    set_target_properties(networkingvalidation.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
    ws_add_test(networkingvalidation)

    add_executable(settingsvalidation.test
        settingsvalidation.test.cpp
        settingsvalidation.test.h
    )
    target_link_libraries(settingsvalidation.test PRIVATE
        Qt6::Test Qt6::Network engine client-common spdlog::spdlog ${OS_SPECIFIC_LIBRARIES})
    target_include_directories(settingsvalidation.test PRIVATE
        ${PROJECT_DIRECTORY}/client-common
    )
    set_target_properties(settingsvalidation.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
    ws_add_test(settingsvalidation)
endif(IS_BUILD_TESTS)
