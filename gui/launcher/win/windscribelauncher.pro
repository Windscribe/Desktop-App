#-------------------------------------------------
#
# Project created by QtCreator 2016-11-18T18:16:12
#
#-------------------------------------------------

QT       -= core gui

greaterThan(QT_MAJOR_VERSION, 4): QT -= widgets

TARGET = WindscribeLauncher
TEMPLATE = app

# target Windows XP
DEFINES += "_WIN32_WINNT=0x0501"

#RC_ICONS = Windscribe.ico

SOURCES += main.cpp

LIBS += Shlwapi.lib
LIBS += -lAdvapi32
LIBS += -lUser32

RC_FILE = windscribelauncher.rc
