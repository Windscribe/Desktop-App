cmake_minimum_required(VERSION 3.5)

set(PROJECT_SOURCES
        pingmanager_test_main.cpp
        pingmanager_test_widget.cpp
        pingmanager_test_widget.h
        pingmanager_test_widget.ui
	pingmanager_test.qrc
)

add_executable(pingmanager_visual_test ${PROJECT_SOURCES} )
target_link_libraries(pingmanager_visual_test PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Test engine common ${OS_SPECIFIC_LIBRARIES})

target_include_directories(pingmanager_visual_test PRIVATE
    ${PROJECT_DIRECTORY}/engine
    ${PROJECT_DIRECTORY}/common
    ${WINDSCRIBE_BUILD_LIBS_PATH}/curl/include
    ${WINDSCRIBE_BUILD_LIBS_PATH}/openssl/include
)

set_target_properties(pingmanager_visual_test PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

configure_file(../test_ips.txt "${CMAKE_BINARY_DIR}" COPYONLY)
