QT += core gui widgets

TARGET = WindscribeLogViewer
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

win32 {
    RC_FILE = windscribelogviewer.rc
}

macx {
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.11
    ICON = resources/icons/icon.icns
    QMAKE_INFO_PLIST = info.plist
}

SOURCES += \
    logdata.cpp \
    logwatcher.cpp \
    main.cpp \
    mainwindow.cpp \
    texthighlighter.cpp

HEADERS += \
    common.h \
    logdata.h \
    logwatcher.h \
    mainwindow.h \
    texthighlighter.h

RESOURCES += \
    windscribelogviewer.qrc
