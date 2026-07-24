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

    # The address validators live in validation_posix.cpp (POSIX-only: pulls in <net/if.h>), so this
    # target is built on macOS/Linux only. It compiles the validator sources directly rather than
    # linking the helper, which has no library form.
    if(NOT WIN32)
        find_package(Boost CONFIG REQUIRED COMPONENTS serialization)
        find_package(skyr-url CONFIG REQUIRED)

        add_executable(validation.test
            validation.test.cpp
            ../validation_posix.cpp
            ../../../client/client-common/types/ipaddress.cpp
        )
        target_include_directories(validation.test PRIVATE
            ..
            ../../../client/client-common
        )
        target_link_libraries(validation.test PRIVATE
            Boost::serialization
            skyr::skyr-url
            spdlog::spdlog
        )
        set_target_properties(validation.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
        ws_add_test(validation)
    endif()
endif(IS_BUILD_TESTS)
