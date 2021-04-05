TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle

QT += core gui network svg
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
TARGET = TestGui

BUILD_LIBS_PATH = $$PWD/../../../build-libs

win32 {
    # enable when linking to MT-built gtest
    #QMAKE_CXXFLAGS_RELEASE -= -MD
    #QMAKE_CXXFLAGS_RELEASE += -MT

    QMAKE_CXXFLAGS_RELEASE += /Zi
    QMAKE_CXXFLAGS += /Zi
    QMAKE_LFLAGS += /DEBUG

}

INCLUDEPATH += $$BUILD_LIBS_PATH/gtest/include
LIBS += -L$$BUILD_LIBS_PATH/gtest/lib -lgmock -lgtest

SOURCES += \
        main.cpp \
        sometest.cpp
