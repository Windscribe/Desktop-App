set(TEST_SOURCES
    dnsrequest.test.cpp
)

add_executable (dnsrequest.test ${TEST_SOURCES})
target_link_libraries(dnsrequest.test PRIVATE Qt6::Test Qt6::Network engine common ${OS_SPECIFIC_LIBRARIES})
target_include_directories(dnsrequest.test PRIVATE
    ${PROJECT_DIRECTORY}/engine
    ${PROJECT_DIRECTORY}/common
)
set_target_properties( dnsrequest.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" )
configure_file(test_domains.txt "${CMAKE_BINARY_DIR}" COPYONLY)

