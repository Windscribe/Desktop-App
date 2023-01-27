QT += core network

CONFIG += console
CONFIG -= app_bundle

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

DEFINES += WINDSCRIBE_CLI
TARGET = windscribe-cli

COMMON_PATH = $$PWD/../../common
BUILD_LIBS_PATH = $$PWD/../../build-libs
INCLUDEPATH += $$COMMON_PATH

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

win32{
    CONFIG(release, debug|release){
        INCLUDEPATH += $$BUILD_LIBS_PATH/protobuf/release/include
        LIBS += -L$$BUILD_LIBS_PATH/protobuf/release/lib -llibprotobuf
    }
    CONFIG(debug, debug|release){
        INCLUDEPATH += $$BUILD_LIBS_PATH/protobuf/debug/include
        LIBS += -L$$BUILD_LIBS_PATH/protobuf/debug/lib -llibprotobufd
    }

    LIBS += -luser32
    LIBS += -lAdvapi32
    LIBS += -lIphlpapi

    SOURCES += \
            $$COMMON_PATH/utils/executable_signature/executable_signature_win.cpp \
            $$COMMON_PATH/utils/winutils.cpp

    HEADERS += \
            $$COMMON_PATH/utils/executable_signature/executable_signature_win.cpp \
            $$COMMON_PATH/utils/winutils.cpp

    RC_FILE = cli.rc

    # Supress protobuf linker warnings
    QMAKE_LFLAGS += /IGNORE:4099

    # Generate debug information (symbol files) for Windows
    QMAKE_CXXFLAGS_RELEASE += /Zi
    QMAKE_CXXFLAGS += /Zi
    QMAKE_LFLAGS += /DEBUG
}

macx {
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.11

    #remove unused parameter warnings
    QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter

    #LIBS += -framework Foundation
    LIBS += -framework AppKit
    LIBS += -framework CoreFoundation
    LIBS += -framework CoreServices
    LIBS += -framework Security
    LIBS += -framework SystemConfiguration

    INCLUDEPATH += $$BUILD_LIBS_PATH/protobuf/include
    LIBS += -L$$BUILD_LIBS_PATH/protobuf/lib -lprotobuf

    SOURCES += \
        $$COMMON_PATH/utils/network_utils/network_utils_mac.cpp

    OBJECTIVE_SOURCES += \
            $$COMMON_PATH/utils/executable_signature/executable_signature_mac.mm \
            $$COMMON_PATH/utils/macutils.mm

    HEADERS += \
            $$COMMON_PATH/utils/executable_signature/executable_signature_mac.h \
            $$COMMON_PATH/utils/macutils.h \
            $$COMMON_PATH/utils/network_utils/network_utils_mac.h
}

linux {
    INCLUDEPATH += $$BUILD_LIBS_PATH/openssl/include
    LIBS += -L$$BUILD_LIBS_PATH/openssl/lib -lssl -lcrypto

    INCLUDEPATH += $$BUILD_LIBS_PATH/protobuf/include
    LIBS += -L$$BUILD_LIBS_PATH/protobuf/lib -lprotobuf

    SOURCES += \
        $$COMMON_PATH/utils/linuxutils.cpp

    HEADERS += \
        $$COMMON_PATH/utils/linuxutils.h

}

SOURCES += \
        ../../common/utils/ipvalidation.cpp \
        $$COMMON_PATH/ipc/commandfactory.cpp \
        $$COMMON_PATH/ipc/connection.cpp \
        $$COMMON_PATH/ipc/proto/clientcommands.pb.cc \
        $$COMMON_PATH/ipc/proto/servercommands.pb.cc \
        $$COMMON_PATH/ipc/proto/cli.pb.cc \
        $$COMMON_PATH/ipc/proto/types.pb.cc \
        $$COMMON_PATH/ipc/server.cpp \
        $$COMMON_PATH/types/locationid.cpp \
        $$COMMON_PATH/utils/extraconfig.cpp \
        $$COMMON_PATH/utils/languagesutil.cpp \
        $$COMMON_PATH/utils/logger.cpp \
        $$COMMON_PATH/utils/utils.cpp \
        $$COMMON_PATH/utils/hardcodedsettings.cpp \
        $$COMMON_PATH/utils/simplecrypt.cpp \
        $$COMMON_PATH/version/appversion.cpp \
        $$COMMON_PATH/utils/executable_signature/executable_signature.cpp \
        $$COMMON_PATH/utils/clean_sensitive_info.cpp \
        backendcommander.cpp \
        cliapplication.cpp \
        main.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    ../../common/utils/ipvalidation.h \
    $$COMMON_PATH/ipc/command.h \
    $$COMMON_PATH/ipc/commandfactory.h \
    $$COMMON_PATH/ipc/connection.h \
    $$COMMON_PATH/ipc/proto/clientcommands.pb.h \
    $$COMMON_PATH/ipc/proto/servercommands.pb.h \
    $$COMMON_PATH/ipc/proto/cli.pb.h \
    $$COMMON_PATH/ipc/proto/types.pb.h \
    $$COMMON_PATH/ipc/iconnection.h \
    $$COMMON_PATH/ipc/iserver.h \
    $$COMMON_PATH/ipc/protobufcommand.h \
    $$COMMON_PATH/ipc/server.h \
    $$COMMON_PATH/types/locationid.h \
    $$COMMON_PATH/utils/extraconfig.h \
    $$COMMON_PATH/utils/languagesutil.h \
    $$COMMON_PATH/utils/logger.h \
    $$COMMON_PATH/utils/utils.h \
    $$COMMON_PATH/utils/hardcodedsettings.h \
    $$COMMON_PATH/utils/simplecrypt.h \
    $$COMMON_PATH/version/appversion.h \
    $$COMMON_PATH/version/windscribe_version.h \
    $$COMMON_PATH/utils/executable_signature/executable_signature.h \
    $$COMMON_PATH/utils/clean_sensitive_info.h \
    backendcommander.h \
    cliapplication.h

RESOURCES += windscribe.qrc
