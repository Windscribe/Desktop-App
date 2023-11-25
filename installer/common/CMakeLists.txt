set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(WINDSCRIBE_TS_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_installer_ar.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_installer_cs.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_installer_de.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_installer_en.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_installer_es.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_installer_fa.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_installer_fr.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_installer_hi.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_installer_id.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_installer_it.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_installer_ja.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_installer_ko.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_installer_pl.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_installer_pt.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_installer_ru.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_installer_tr.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_installer_uk.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_installer_vi.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_installer_zh-CN.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/windscribe_installer_zh-TW.ts
)
qt6_create_translation(WINDSCRIBE_QM_FILES ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../windows/installer ${WINDSCRIBE_TS_FILES} OPTIONS -extensions cpp,h -locations none)

qt_add_resources(installer qm_files
    PREFIX
        "/translations"
    BASE
        ${CMAKE_CURRENT_BINARY_DIR}
    FILES
        ${WINDSCRIBE_QM_FILES}
)

target_sources(installer PRIVATE
    ../../client/common/utils/languagesutil.cpp
    alertwindow.cpp
    alertwindowcontents.cpp
    hoverbutton.cpp
    initialwindow.cpp
    installbutton.cpp
    languagecontroller.cpp
    mainwindow.cpp
    settingswindow.cpp
    spinner.cpp
    svgresources.cpp
    themecontroller.cpp
    togglebutton.cpp
)
