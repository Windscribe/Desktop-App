set(TEST_SOURCES
    dnscache.test.cpp
)

add_executable (dnscache.test ${TEST_SOURCES})
target_link_libraries(dnscache.test PRIVATE Qt6::Test Qt6::Network engine common ${OS_SPECIFIC_LIBRARIES})
target_include_directories(dnscache.test PRIVATE
    ${PROJECT_DIRECTORY}/engine
    ${PROJECT_DIRECTORY}/common
)
set_target_properties( dnscache.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" )

