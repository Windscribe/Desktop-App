QT -= gui core

CONFIG += c++11 console
CONFIG -= app_bundle

BUILD_LIBS_PATH = $$PWD/../../../build-libs

#boost include and libs
INCLUDEPATH += $$BUILD_LIBS_PATH/boost/include
LIBS += $$BUILD_LIBS_PATH/boost/lib/libboost_serialization.a
LIBS += $$BUILD_LIBS_PATH/boost/lib/libboost_thread.a

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

DEFINES += BOOST_BIND_GLOBAL_PLACEHOLDERS

SOURCES += \
        execute_cmd.cpp \
        logger.cpp \
        main.cpp \
        server.cpp \
        utils.cpp \
        wireguard/wireguardadapter.cpp \
        wireguard/wireguardcommunicator.cpp \
        wireguard/wireguardcontroller.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    ../../posix_common/helper_commands.h \
    ../../posix_common/helper_commands_serialize.h \
    3rdparty/pstream.h \
    execute_cmd.h \
    logger.h \
    server.h \
    utils.h \
    wireguard/wireguardadapter.h \
    wireguard/wireguardcommunicator.h \
    wireguard/wireguardcontroller.h
