cmake_minimum_required(VERSION 3.23)

project(Windscribe)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
        main.cpp
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../common/utils/hardcodedsecrets.ini")
    list(APPEND PROJECT_SOURCES secrets.qrc)
endif()

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../build-libs/boost/lib
                 ${CMAKE_CURRENT_SOURCE_DIR}/../build-libs/protobuf/debug/lib
                 ${CMAKE_CURRENT_SOURCE_DIR}/../build-libs/protobuf/release/lib
                 ${CMAKE_CURRENT_SOURCE_DIR}/../build-libs/curl/lib
                 ${CMAKE_CURRENT_SOURCE_DIR}/../build-libs/cares/dll_x64/lib
                 ${CMAKE_CURRENT_SOURCE_DIR}/../build-libs/openssl/lib
)

if (WIN32)
    set (WINDOWS_API_LIBRARIES psapi.lib iphlpapi.lib dnsapi.lib rasapi32.lib pdh.lib)
    list(APPEND PROJECT_SOURCES client.rc)
    # Generate debug information (symbol files) for Windows (release build only)
    add_compile_options("$<$<CONFIG:Release>:/Zi>")
    add_link_options("$<$<CONFIG:Release>:/DEBUG>" "$<$<CONFIG:Release>:/OPT:REF>" "$<$<CONFIG:Release>:/OPT:ICF>")
endif (WIN32)

qt_add_executable(Windscribe
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
)

if (WIN32)
    target_link_options(Windscribe PRIVATE "/IGNORE:4099")
endif (WIN32)

target_link_libraries(Windscribe PRIVATE Qt6::Widgets engine gui common
    ${WINDOWS_API_LIBRARIES}
    libcurl cares libeay32 ssleay32
    debug libprotobufd
    optimized libprotobuf
)

target_include_directories(Windscribe PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
    ${CMAKE_CURRENT_SOURCE_DIR}/../build-libs/protobuf/release/include
)


#set_target_properties(test_cmake PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
#    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
#    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
#    MACOSX_BUNDLE TRUE
#    WIN32_EXECUTABLE TRUE
#)

#install(TARGETS test_cmake
#    BUNDLE DESTINATION .
#    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

add_subdirectory(engine)
add_subdirectory(gui)
add_subdirectory(common)

qt_finalize_executable(Windscribe)


