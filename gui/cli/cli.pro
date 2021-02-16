QT += core gui network

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
INCLUDEPATH += $$COMMON_PATH

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

win32{
    CONFIG(release, debug|release){
        INCLUDEPATH += c:/libs/protobuf_release/include
        LIBS += -Lc:/libs/protobuf_release/lib -llibprotobuf
    }
    CONFIG(debug, debug|release){
        INCLUDEPATH += c:/libs/protobuf_debug/include
        LIBS += -Lc:/libs/protobuf_debug/lib -llibprotobufd
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


    # Supress protobuf linker warnings
    QMAKE_LFLAGS += /IGNORE:4099

    # Generate debug information (symbol files) for Windows
    QMAKE_CXXFLAGS_RELEASE += /Zi
    QMAKE_CXXFLAGS += /Zi
    QMAKE_LFLAGS += /DEBUG
}

macx {

    #remove unused parameter warnings
    QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter

    #LIBS += -framework Foundation
    LIBS += -framework AppKit
    LIBS += -framework CoreFoundation
    LIBS += -framework CoreServices
    LIBS += -framework Security

    INCLUDEPATH += $$(HOME)/LibsWindscribe/protobuf/include
    LIBS += -L$$(HOME)/LibsWindscribe/protobuf/lib -lprotobuf

    OBJECTIVE_SOURCES += \
            $$COMMON_PATH/utils/executable_signature/executable_signature_mac.mm \
            $$COMMON_PATH/utils/macutils.mm

    HEADERS += \
            $$COMMON_PATH/utils/executable_signature/executable_signature_mac.h \
            $$COMMON_PATH/utils/macutils.h
}

macx {
QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.11
}

SOURCES += \
        ../backend/backend.cpp \
        ../backend/connectstatehelper.cpp \
        ../backend/firewallstatehelper.cpp \
        ../backend/notificationscontroller.cpp \
        $$COMMON_PATH/ipc/commandfactory.cpp \
        $$COMMON_PATH/ipc/connection.cpp \
        $$COMMON_PATH/ipc/generated_proto/clientcommands.pb.cc \
        $$COMMON_PATH/ipc/generated_proto/servercommands.pb.cc \
        $$COMMON_PATH/ipc/generated_proto/types.pb.cc \
        $$COMMON_PATH/ipc/server.cpp \
        $$COMMON_PATH/ipc/tcpconnection.cpp \
        $$COMMON_PATH/ipc/tcpserver.cpp \
        ../backend/locationsmodel/alllocationsmodel.cpp \
        ../backend/locationsmodel/basiccitiesmodel.cpp \
        ../backend/locationsmodel/basiclocationsmodel.cpp \
        ../backend/locationsmodel/configuredcitiesmodel.cpp \
        ../backend/locationsmodel/favoritecitiesmodel.cpp \
        ../backend/locationsmodel/favoritelocationsstorage.cpp \
        ../backend/locationsmodel/locationsmodel.cpp \
        ../backend/locationsmodel/sortlocationsalgorithms.cpp \
        ../backend/locationsmodel/staticipscitiesmodel.cpp \
        ../backend/preferences/accountinfo.cpp \
        ../backend/preferences/detectlanrange.cpp \
        ../backend/preferences/guisettingsfromver1.cpp \
        ../backend/preferences/preferences.cpp \
        ../backend/preferences/preferenceshelper.cpp \
        $$COMMON_PATH/types/locationid.cpp \
        ../backend/types/pingtime.cpp \
        ../backend/types/types.cpp \
        ../backend/types/upgrademodetype.cpp \
        $$COMMON_PATH/utils/extraconfig.cpp \
        $$COMMON_PATH/utils/languagesutil.cpp \
        $$COMMON_PATH/utils/logger.cpp \
        $$COMMON_PATH/utils/utils.cpp \
        $$COMMON_PATH/version/appversion.cpp \
        $$COMMON_PATH/utils/executable_signature/executable_signature.cpp \
        $$COMMON_PATH/utils/clean_sensitive_info.cpp \
        ../backend/persistentstate.cpp \
        backendcommander.cpp \
        cliapplication.cpp \
        main.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    ../backend/backend.h \
    ../backend/connectstatehelper.h \
    ../backend/firewallstatehelper.h \
    ../backend/ibackend.h \
    ../backend/notificationscontroller.h \
    $$COMMON_PATH/ipc/command.h \
    $$COMMON_PATH/ipc/commandfactory.h \
    $$COMMON_PATH/ipc/connection.h \
    $$COMMON_PATH/ipc/generated_proto/clientcommands.pb.h \
    $$COMMON_PATH/ipc/generated_proto/servercommands.pb.h \
    $$COMMON_PATH/ipc/generated_proto/types.pb.h \
    $$COMMON_PATH/ipc/iconnection.h \
    $$COMMON_PATH/ipc/iserver.h \
    $$COMMON_PATH/ipc/protobufcommand.h \
    $$COMMON_PATH/ipc/server.h \
    $$COMMON_PATH/ipc/tcpconnection.h \
    $$COMMON_PATH/ipc/tcpserver.h \
    ../backend/locationsmodel/alllocationsmodel.h \
    ../backend/locationsmodel/basiccitiesmodel.h \
    ../backend/locationsmodel/basiclocationsmodel.h \
    ../backend/locationsmodel/configuredcitiesmodel.h \
    ../backend/locationsmodel/favoritecitiesmodel.h \
    ../backend/locationsmodel/favoritelocationsstorage.h \
    ../backend/locationsmodel/locationmodelitem.h \
    ../backend/locationsmodel/locationsmodel.h \
    ../backend/locationsmodel/sortlocationsalgorithms.h \
    ../backend/locationsmodel/staticipscitiesmodel.h \
    ../backend/preferences/accountinfo.h \
    ../backend/preferences/detectlanrange.h \
    ../backend/preferences/guisettingsfromver1.h \
    ../backend/preferences/preferences.h \
    ../backend/preferences/preferenceshelper.h \
    $$COMMON_PATH/types/locationid.h \
    ../backend/types/pingtime.h \
    ../backend/types/types.h \
    ../backend/types/upgrademodetype.h \
    $$COMMON_PATH/utils/extraconfig.h \
    $$COMMON_PATH/utils/languagesutil.h \
    $$COMMON_PATH/utils/logger.h \
    $$COMMON_PATH/utils/utils.h \
    $$COMMON_PATH/version/appversion.h \
    $$COMMON_PATH/version/windscribe_version.h \
    $$COMMON_PATH/utils/executable_signature/executable_signature.h \
    $$COMMON_PATH/utils/clean_sensitive_info.h \
    ../backend/persistentstate.h \
    backendcommander.h \
    cliapplication.h
