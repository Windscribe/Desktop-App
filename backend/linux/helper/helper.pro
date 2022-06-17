QT -= gui core

CONFIG += c++11 console
CONFIG -= app_bundle

BUILD_LIBS_PATH = $$PWD/../../../build-libs

#boost include and libs
INCLUDEPATH += $$BUILD_LIBS_PATH/boost/include \
               $$BUILD_LIBS_PATH/openssl/include

LIBS += $$BUILD_LIBS_PATH/boost/lib/libboost_serialization.a
LIBS += $$BUILD_LIBS_PATH/boost/lib/libboost_thread.a
LIBS += $$BUILD_LIBS_PATH/boost/lib/libboost_filesystem.a
LIBS += -L$$BUILD_LIBS_PATH/openssl/lib -lssl -lcrypto

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

DEFINES += BOOST_BIND_GLOBAL_PLACEHOLDERS

# build_all.py adds 'use_signature_check' to the CONFIG environment when invoked without the '--no-sign' flag.
contains(CONFIG, use_signature_check) {
    CONFIG(release, debug|release) {
        DEFINES += USE_SIGNATURE_CHECK
    }
}

SOURCES += \
        ../../../common/utils/executable_signature/executable_signature.cpp \
        ../../../common/utils/executable_signature/executablesignature_linux.cpp \
        execute_cmd.cpp \
        ipc/helper_security.cpp \
        logger.cpp \
        main.cpp \
        server.cpp \
        utils.cpp \
        routes_manager/routes.cpp \
        routes_manager/routes_manager.cpp \
        wireguard/defaultroutemonitor.cpp \
        wireguard/wireguardadapter.cpp \
        wireguard/userspace/wireguardgocommunicator.cpp \
        wireguard/kernelmodule/kernelmodulecommunicator.cpp \
        wireguard/kernelmodule/wireguard.c \
        wireguard/wireguardcontroller.cpp

HEADERS += \
    ../../../common/utils/executable_signature/executable_signature.h \
    ../../../common/utils/executable_signature/executablesignature_linux.h \
    ../../posix_common/helper_commands.h \
    ../../posix_common/helper_commands_serialize.h \
    3rdparty/pstream.h \
    execute_cmd.h \
    ipc/helper_security.h \
    logger.h \
    server.h \
    utils.h \
    routes_manager/routes.h \
    routes_manager/routes_manager.h \
    wireguard/defaultroutemonitor.h \
    wireguard/wireguardadapter.h \
    wireguard/iwireguardcommunicator.h \
    wireguard/userspace/wireguardgocommunicator.h \
    wireguard/kernelmodule/kernelmodulecommunicator.h \
    wireguard/kernelmodule/wireguard.h \
    wireguard/wireguardcontroller.h
