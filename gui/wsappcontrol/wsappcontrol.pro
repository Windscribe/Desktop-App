QT -= core
QT -= gui

CONFIG += c++11

TARGET = wsappcontrol
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

win32 {
DEFINES += "Q_OS_WIN"

INCLUDEPATH += C:/libs/boost/include/boost-1_67
LIBS += -L"C:/libs/boost/lib"

INCLUDEPATH += "C:/libs/openssl/include"
LIBS += -L"C:/libs/openssl/lib" -llibeay32 -lssleay32

RC_FILE = wsappcontrol.rc
}

macx {
QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.11
HOMEDIR = $$(HOME)

#boost include and libs
INCLUDEPATH += $$HOMEDIR/LibsWindscribe/boost/include
LIBS += $$HOMEDIR/LibsWindscribe/boost/lib/libboost_system.a

#openssl include
INCLUDEPATH += $$HOMEDIR/LibsWindscribe/openssl/include
LIBS+=-L$$HOMEDIR/LibsWindscribe/openssl/lib -lssl -lcrypto
}

SOURCES += localhttpsserver.cpp \
    certstorage.cpp \
    httpconnection.cpp \
    httpconnectionmanager.cpp \
    httpreply.cpp \
    httprequest.cpp \
    httprequesthandler.cpp \
    httprequestparser.cpp \
    httprequestratelimitcontroller.cpp \
    server.cpp \
    simple_crypt.cpp

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

HEADERS += \
    certstorage.h \
    httpconnection.h \
    httpconnectionmanager.h \
    httpheader.h \
    httpreply.h \
    httprequest.h \
    httprequesthandler.h \
    httprequestparser.h \
    httprequestratelimitcontroller.h \
    server.h \
    simple_crypt.h \
    types.h
