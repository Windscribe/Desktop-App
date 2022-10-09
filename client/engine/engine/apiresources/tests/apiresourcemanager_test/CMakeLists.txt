set(TEST_SOURCES
    apiresourcemanager.test.cpp
    apiresourcemanager.test.h
    resources.qrc
)

add_executable (apiresourcemanager.test ${TEST_SOURCES})
target_link_libraries(apiresourcemanager.test PRIVATE Qt6::Test Qt6::Network engine common ${OS_SPECIFIC_LIBRARIES})
target_include_directories(apiresourcemanager.test PRIVATE
    ${PROJECT_DIRECTORY}/engine
    ${PROJECT_DIRECTORY}/common
    ${WINDSCRIBE_BUILD_LIBS_PATH}/curl/include
    ${WINDSCRIBE_BUILD_LIBS_PATH}/openssl/include
)
set_target_properties( apiresourcemanager.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" )

