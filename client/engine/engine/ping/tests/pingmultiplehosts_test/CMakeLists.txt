set(TEST_SOURCES
    pingmultiplehosts.test.cpp
    pingmultiplehosts.test.h
    resources.qrc
)

add_executable (pingmultiplehosts.test ${TEST_SOURCES})
target_link_libraries(pingmultiplehosts.test PRIVATE Qt6::Test Qt6::Network engine common ${OS_SPECIFIC_LIBRARIES})
target_include_directories(pingmultiplehosts.test PRIVATE
    ${PROJECT_DIRECTORY}/engine
    ${PROJECT_DIRECTORY}/common
    ${WINDSCRIBE_BUILD_LIBS_PATH}/curl/include
    ${WINDSCRIBE_BUILD_LIBS_PATH}/openssl/include
)
set_target_properties( pingmultiplehosts.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" )
configure_file(../test_ips.txt "${CMAKE_BINARY_DIR}" COPYONLY)
