if(IS_BUILD_TESTS)
    find_package(spdlog CONFIG REQUIRED)

    add_executable(appuser.test appuser.test.cpp ../appuser.cpp)
    target_link_libraries(appuser.test PRIVATE spdlog::spdlog)
    set_target_properties(appuser.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
    ws_add_test(appuser)

    # Runs the real OpenDirectory seam read-only against built-in records, so a framework behaviour change
    # shows up here instead of as a helper that silently stops provisioning.
    add_executable(directory.test directory.test.cpp ../directory_mac.mm)
    target_link_libraries(directory.test PRIVATE spdlog::spdlog "-framework Foundation" "-framework OpenDirectory")
    set_target_properties(directory.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
    ws_add_test(directory)

    # The helper has no library form, so the test compiles the source under test directly.
    add_executable(staging.test
        staging.test.cpp
        ../installer/staging.cpp
    )
    target_link_libraries(staging.test PRIVATE spdlog::spdlog)
    set_target_properties(staging.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
    ws_add_test(staging)
endif(IS_BUILD_TESTS)
