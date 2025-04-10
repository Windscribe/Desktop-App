cmake_minimum_required(VERSION 3.23)

project(gui)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Svg LinguistTools)

set(WS_TS_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/ws_desktop_ar.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/ws_desktop_cs.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/ws_desktop_de.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/ws_desktop_en.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/ws_desktop_es.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/ws_desktop_fa.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/ws_desktop_fr.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/ws_desktop_hi.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/ws_desktop_id.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/ws_desktop_it.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/ws_desktop_ja.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/ws_desktop_ko.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/ws_desktop_pl.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/ws_desktop_pt.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/ws_desktop_ru.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/ws_desktop_tr.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/ws_desktop_uk.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/ws_desktop_vi.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/ws_desktop_zh-CN.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/ws_desktop_zh-TW.ts)

# Using the -extensions flag to skip processing the .ui files in the test projects.  Otherwise, the
# strings in said ui files will be added to the .ts output files.
qt6_create_translation(WS_QM_FILES ${CMAKE_CURRENT_SOURCE_DIR}/.. ${WS_TS_FILES} OPTIONS -extensions cpp,h -locations none)

set(PROJECT_SOURCES
    # --- sources and headers ---
    dpiscaleawarewidget.cpp
    dpiscaleawarewidget.h
    dpiscalemanager.cpp
    dpiscalemanager.h
    freetrafficnotificationcontroller.cpp
    freetrafficnotificationcontroller.h
    generalmessage.h
    generalmessagecontroller.cpp
    generalmessagecontroller.h
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
    windowsizemanager.cpp
    windowsizemanager.h

    # --- resources ---
    gif.qrc
    jpg.qrc
    svg.qrc
    windscribe.qrc
)

if (APPLE)
    list(APPEND PROJECT_SOURCES windscribe_mac.qrc)
endif (APPLE)

add_library(gui STATIC ${PROJECT_SOURCES})

qt_add_resources(gui "translations"
    PREFIX
        "/translations"
    BASE
        ${CMAKE_CURRENT_BINARY_DIR}
    FILES
        ${WS_QM_FILES}
)

target_link_libraries(gui PRIVATE Qt6::Core Qt6::Network Qt6::Widgets Qt6::Svg wsnet::wsnet spdlog::spdlog Boost::serialization)

if (WIN32)
    target_compile_definitions(gui PRIVATE CMAKE_LIBRARY_LIBRARY
                                  WINVER=0x0601
                                  _WIN32_WINNT=0x0601
                                  WIN32_LEAN_AND_MEAN
                                  PIO_APC_ROUTINE_DEFINED)
endif (WIN32)

if (UNIX)
    find_package(Qt6 REQUIRED COMPONENTS DBus)
    target_link_libraries(gui PRIVATE Qt6::DBus)
endif (UNIX)

target_include_directories(gui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../engine
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
    ${CMAKE_CURRENT_SOURCE_DIR}/../base
)

add_subdirectory(application)
add_subdirectory(bottominfowidget)
add_subdirectory(commongraphics)
add_subdirectory(commonwidgets)
add_subdirectory(connectwindow)
add_subdirectory(dialogs)
add_subdirectory(emergencyconnectwindow)
add_subdirectory(externalconfig)
add_subdirectory(generalmessage)
add_subdirectory(graphicresources)
add_subdirectory(locations)
add_subdirectory(locationswindow)
add_subdirectory(log)
add_subdirectory(loginwindow)
add_subdirectory(newsfeedwindow)
add_subdirectory(overlaysconnectwindow)
add_subdirectory(permissions)
add_subdirectory(preferenceswindow)
add_subdirectory(protocolwindow)
add_subdirectory(systemtray)
add_subdirectory(tooltips)
add_subdirectory(twofactorauth)
add_subdirectory(widgetutils)
add_subdirectory(tests)
add_subdirectory(utils)
