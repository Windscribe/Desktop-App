if(IS_BUILD_TESTS)
    find_package(Boost CONFIG REQUIRED COMPONENTS serialization)
    find_package(skyr-url CONFIG REQUIRED)
    find_package(spdlog CONFIG REQUIRED)

    # Runs the real Utils::executeCommand against /bin/sh, so utils.cpp is compiled in together with the
    # validation code it calls (the helper has no library form).
    add_executable(helperutils.test
        helperutils.test.cpp
        ../utils.cpp
        ../../common/validation_posix.cpp
        ../../../client/client-common/types/ipaddress.cpp
    )
    target_include_directories(helperutils.test PRIVATE
        ../../../client/client-common
    )
    target_link_libraries(helperutils.test PRIVATE
        Boost::serialization
        skyr::skyr-url
        spdlog::spdlog
    )
    set_target_properties(helperutils.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
    ws_add_test(helperutils)
endif(IS_BUILD_TESTS)
