set(TEST_SOURCES
    networkaccessmanager.test.cpp
    networkaccessmanager.test.h
    resources.qrc
)

add_executable (networkaccessmanager.test ${TEST_SOURCES})
target_link_libraries(networkaccessmanager.test PRIVATE Qt6::Test Qt6::Network engine common ${OS_SPECIFIC_LIBRARIES})
target_include_directories(networkaccessmanager.test PRIVATE
    ${PROJECT_DIRECTORY}/engine
    ${PROJECT_DIRECTORY}/common
    ${WINDSCRIBE_BUILD_LIBS_PATH}/curl/include
    ${WINDSCRIBE_BUILD_LIBS_PATH}/openssl/include
)
set_target_properties( networkaccessmanager.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" )

