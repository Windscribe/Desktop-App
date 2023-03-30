TARGET = WireguardService
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

TEMPLATE = app

BUILD_LIBS_PATH = $$PWD/../../../build-libs

SOURCES += \
    ServiceMain.cpp \
    ../../../client/common/utils/servicecontrolmanager.cpp

#HEADERS +=

INCLUDEPATH += \
    ../../../client/common/utils \
    $$BUILD_LIBS_PATH/boost/include

LIBS += Advapi32.lib
LIBS += -L"$$BUILD_LIBS_PATH/boost/lib"

DEFINES += \
    BOOST_AUTO_LINK_TAGGED

# build_all.py adds 'use_signature_check' to the CONFIG environment when invoked with the '--sign' flag.
contains(CONFIG, use_signature_check) {
    CONFIG(release, debug|release) {
        DEFINES += USE_SIGNATURE_CHECK
    }
}

RC_FILE = wireguard_service.rc

QMAKE_CXXFLAGS_RELEASE -= -MD
QMAKE_CXXFLAGS_RELEASE += -MT
QMAKE_CXXFLAGS_DEBUG -= -MDd
QMAKE_CXXFLAGS_DEBUG += -MTd
