cmake_minimum_required(VERSION 3.23)

project(gui)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network Svg Core5Compat)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network Svg Core5Compat)


set(PROJECT_SOURCES
    # --- sources and headers ---
    blockconnect.cpp
    blockconnect.h
    dpiscaleawarewidget.cpp
    dpiscaleawarewidget.h
    dpiscalemanager.cpp
    dpiscalemanager.h
    freetrafficnotificationcontroller.cpp
    freetrafficnotificationcontroller.h
    guitest.cpp
    guitest.h
    languagecontroller.cpp
    languagecontroller.h
    loginattemptscontroller.cpp
    loginattemptscontroller.h
    mainwindow.cpp
    mainwindow.h
    mainwindowcontroller.cpp
    mainwindowcontroller.h
    mainwindowstate.cpp
    mainwindowstate.h
    showingdialogstate.cpp
    showingdialogstate.h

    # --- resources ---
    jpg.qrc
    svg.qrc
    windscribe.qrc
)

add_library(gui STATIC ${PROJECT_SOURCES})

target_link_libraries(gui PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Svg Qt${QT_VERSION_MAJOR}::Core5Compat)
target_compile_definitions(gui PRIVATE CMAKE_LIBRARY_LIBRARY
                                  WINVER=0x0601
                                  _WIN32_WINNT=0x0601
                                  WIN32_LEAN_AND_MEAN
                                  PIO_APC_ROUTINE_DEFINED)


target_include_directories(gui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../engine
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common
    ${CMAKE_CURRENT_SOURCE_DIR}/../../build-libs/protobuf/release/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../build-libs/boost/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../build-libs/curl/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../build-libs/cares/dll_x64/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../build-libs/openssl/include
)

add_subdirectory(application)
add_subdirectory(backend)
add_subdirectory(bottominfowidget)
add_subdirectory(commongraphics)
add_subdirectory(commonwidgets)
add_subdirectory(connectwindow)
add_subdirectory(dialogs)
add_subdirectory(emergencyconnectwindow)
add_subdirectory(externalconfig)
add_subdirectory(graphicresources)
add_subdirectory(launchonstartup)
add_subdirectory(localipcserver)
add_subdirectory(locationswindow)
add_subdirectory(log)
add_subdirectory(loginwindow)
add_subdirectory(multipleaccountdetection)
add_subdirectory(newsfeedwindow)
add_subdirectory(overlaysconnectwindow)
add_subdirectory(preferenceswindow)
add_subdirectory(systemtray)
add_subdirectory(tooltips)
add_subdirectory(twofactorauth)
add_subdirectory(utils)

