TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle

QT += core gui network svg
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
TARGET = TestGui

win32 {
    # enable when linking to MT-built gtest
    #QMAKE_CXXFLAGS_RELEASE -= -MD
    #QMAKE_CXXFLAGS_RELEASE += -MT

    QMAKE_CXXFLAGS_RELEASE += /Zi
    QMAKE_CXXFLAGS += /Zi
    QMAKE_LFLAGS += /DEBUG

    INCLUDEPATH += c:/libs/gtest/include
    LIBS += -Lc:/libs/gtest/lib -lgmock -lgtest
}

SOURCES += \
        main.cpp \
        sometest.cpp
