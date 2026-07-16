if(IS_BUILD_TESTS)
    find_package(spdlog CONFIG REQUIRED)

    add_executable(ovpndirectivewhitelist.test
        ovpndirectivewhitelist.test.cpp
    )
    target_link_libraries(ovpndirectivewhitelist.test PRIVATE
        spdlog::spdlog
    )
    set_target_properties(ovpndirectivewhitelist.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
    ws_add_test(ovpndirectivewhitelist)
endif(IS_BUILD_TESTS)
