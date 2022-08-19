cmake_minimum_required(VERSION 3.5)

set(PROJECT_SOURCES
        main.cpp
        widget.cpp
        widget.h
        widget.ui
        locationsdata.test.qrc
)

add_executable(locationsmodel_visual_test ${PROJECT_SOURCES} )
target_link_libraries(locationsmodel_visual_test PRIVATE Qt${QT_VERSION_MAJOR}::Widgets  Qt${QT_VERSION_MAJOR}::Test common gui engine ${OS_SPECIFIC_LIBRARIES})

target_include_directories(locationsmodel_visual_test PRIVATE
    ${PROJECT_DIRECTORY}/gui
    ${PROJECT_DIRECTORY}/../common
)

set_target_properties(locationsmodel_visual_test PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

