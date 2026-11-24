if(IS_BUILD_TESTS)
    add_executable(routeeventfilter.test
        routeeventfilter.test.cpp
        routeeventfilter.test.h
        ../routeeventfilter_linux.cpp
    )
    target_link_libraries(routeeventfilter.test PRIVATE Qt6::Test)
    target_include_directories(routeeventfilter.test PRIVATE ..)
    set_target_properties(routeeventfilter.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
    ws_add_test(routeeventfilter)
endif(IS_BUILD_TESTS)
