QT       += core gui network svg

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Windscribe
TEMPLATE = app

DEFINES += QT_MESSAGELOGCONTEXT

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

COMMON_PATH = $$PWD/../common
BUILD_LIBS_PATH = $$PWD/../build-libs

INCLUDEPATH += $$COMMON_PATH

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# build_all.py adds 'use_signature_check' to the CONFIG environment when invoked without the '--no-sign' flag.
contains(CONFIG, use_signature_check) {
    CONFIG(release, debug|release) {
        DEFINES += USE_SIGNATURE_CHECK
    }
}

win32 {
    # Windows 7 platform
    DEFINES += "WINVER=0x0601"
    DEFINES += "_WIN32_WINNT=0x0601"
    DEFINES += "PIO_APC_ROUTINE_DEFINED"

    LIBS += Ws2_32.lib Advapi32.lib Iphlpapi.lib \
    Wininet.lib User32.lib Sensapi.lib Dnsapi.lib \
    Ole32.lib Shlwapi.lib Version.lib Psapi.lib \
    rasapi32.lib Pdh.lib Shell32.lib netapi32.lib msi.lib

    CONFIG(release, debug|release){
        INCLUDEPATH += $$BUILD_LIBS_PATH/protobuf/release/include
        LIBS += -L$$BUILD_LIBS_PATH/protobuf/release/lib -llibprotobuf
    }
    CONFIG(debug, debug|release){
        INCLUDEPATH += $$BUILD_LIBS_PATH/protobuf/debug/include
        LIBS += -L$$BUILD_LIBS_PATH/protobuf/debug/lib -llibprotobufd
    }

    INCLUDEPATH += $$BUILD_LIBS_PATH/boost/include
    LIBS += -L"$$BUILD_LIBS_PATH/boost/lib"

    INCLUDEPATH += "$$BUILD_LIBS_PATH/curl/include"
    LIBS += -L"$$BUILD_LIBS_PATH/curl/lib" -llibcurl

    INCLUDEPATH += "$$BUILD_LIBS_PATH/cares/dll_x32/include"
    LIBS += -L"$$BUILD_LIBS_PATH/cares/dll_x32/lib" -lcares

    INCLUDEPATH += "$$BUILD_LIBS_PATH/openssl/include"
    LIBS += -L"$$BUILD_LIBS_PATH/openssl/lib" -llibeay32 -lssleay32


    RC_FILE = client.rc

    # Supress protobuf linker warnings
    QMAKE_LFLAGS += /IGNORE:4099

    QMAKE_CXXFLAGS_RELEASE -= -Zc:strictStrings
    QMAKE_CFLAGS_RELEASE -= -Zc:strictStrings
    QMAKE_CFLAGS -= -Zc:strictStrings
    QMAKE_CXXFLAGS -= -Zc:strictStrings


    # Generate debug information (symbol files) for Windows
    QMAKE_CXXFLAGS_RELEASE += /Zi
    QMAKE_CXXFLAGS += /Zi
    QMAKE_LFLAGS += /DEBUG

} # win32


macx {

LIBS += -framework Foundation
LIBS += -framework AppKit
LIBS += -framework CoreFoundation
LIBS += -framework CoreServices
LIBS += -framework Security
LIBS += -framework SystemConfiguration
LIBS += -framework ServiceManagement
LIBS += -framework ApplicationServices

INCLUDEPATH += $$BUILD_LIBS_PATH/protobuf/include
LIBS += -L$$BUILD_LIBS_PATH/protobuf/lib -lprotobuf

#remove unused parameter warnings
QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter

QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.11
ICON = windscribe.icns
QMAKE_INFO_PLIST = info.plist

#copy WindscribeLauncher.app to Windscribe.app/Contents/Library/LoginItems folder
makedir.commands = $(MKDIR) $$OUT_PWD/Windscribe.app/Contents/Library/LoginItems
copydata.commands = $(COPY_DIR) $$PWD/../../installer/mac/binaries/launcher/WindscribeLauncher.app $$OUT_PWD/Windscribe.app/Contents/Library/LoginItems

first.depends = $(first) makedir copydata
export(first.depends)
export(makedir.commands)
export(copydata.commands)
#export(makedir4.commands)
QMAKE_EXTRA_TARGETS += first makedir copydata #makedir4 copydata4

# only for release build
# comment CONFIG... line if need embedded engine and cli for debug purposes
CONFIG(release, debug|release) {

    #copy WindscribeEngine.app to Windscribe.app/Contents/Library folder
    copydata3.commands = $(COPY_DIR) $$PWD/../../installer/mac/binaries/WindscribeEngine.app $$OUT_PWD/Windscribe.app/Contents/Library
    first.depends += copydata3
    export(copydata3.commands)
    QMAKE_EXTRA_TARGETS += copydata3

    # package cli inside Windscribe.app/Contents/MacOS
    makedir4.commands = $(MKDIR) $$OUT_PWD/Windscribe.app/Contents/MacOS
    first.depends += makedir4
    export(makedir4.commands)
    QMAKE_EXTRA_TARGETS += makedir4
    copydata4.commands = $(COPY_FILE) $$PWD/../../installer/mac/binaries/windscribe-cli $$OUT_PWD/Windscribe.app/Contents/MacOS/windscribe-cli
    first.depends += copydata4
    export(copydata4.commands)
    QMAKE_EXTRA_TARGETS += copydata4

    osslicense.files = $$COMMON_PATH/licenses/open_source_licenses.txt
    osslicense.path  = Contents/Resources
    QMAKE_BUNDLE_DATA += osslicense
}

} # macx

linux {

#remove linux deprecated copy warnings
QMAKE_CXXFLAGS_WARN_ON += -Wno-deprecated-copy

INCLUDEPATH += $$BUILD_LIBS_PATH/protobuf/include
LIBS += -L$$BUILD_LIBS_PATH/protobuf/lib -lprotobuf

INCLUDEPATH += $$BUILD_LIBS_PATH/openssl/include
LIBS += -L$$BUILD_LIBS_PATH/openssl/lib -lssl -lcrypto

INCLUDEPATH += $$BUILD_LIBS_PATH/boost/include
LIBS += $$BUILD_LIBS_PATH/boost/lib/libboost_filesystem.a

} # linux


SOURCES += main.cpp

include(common.pri)
include(gui/gui.pri)
include(engine/engine.pri)

