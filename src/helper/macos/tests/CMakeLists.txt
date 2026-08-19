if(IS_BUILD_TESTS)
    find_package(spdlog CONFIG REQUIRED)

    # The helper has no library form, so the test compiles the source under test directly.
    add_executable(staging.test
        staging.test.cpp
        ../installer/staging.cpp
    )
    target_link_libraries(staging.test PRIVATE spdlog::spdlog)
    set_target_properties(staging.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
    ws_add_test(staging)
endif(IS_BUILD_TESTS)
