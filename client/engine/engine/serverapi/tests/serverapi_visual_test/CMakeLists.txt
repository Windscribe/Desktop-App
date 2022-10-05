cmake_minimum_required(VERSION 3.5)

set(PROJECT_SOURCES
        main.cpp
        widget.cpp
        widget.h
        widget.ui
	resources.qrc
)

add_executable(serverapi_visual_test ${PROJECT_SOURCES} )
target_link_libraries(serverapi_visual_test PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Test engine common ${OS_SPECIFIC_LIBRARIES})

target_include_directories(serverapi_visual_test PRIVATE
    ${PROJECT_DIRECTORY}/engine
    ${PROJECT_DIRECTORY}/common
    ${WINDSCRIBE_BUILD_LIBS_PATH}/curl/include
    ${WINDSCRIBE_BUILD_LIBS_PATH}/openssl/include
)

set_target_properties(serverapi_visual_test PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

