win32 {

SOURCES += $$COMMON_PATH/utils/crashdump.cpp \
           $$COMMON_PATH/utils/crashhandler.cpp \
           $$COMMON_PATH/utils/winutils.cpp \
           $$COMMON_PATH/utils/widgetutils_win.cpp \
           $$COMMON_PATH/utils/executable_signature/executable_signature_win.cpp \
           $$COMMON_PATH/utils/servicecontrolmanager.cpp

HEADERS += $$COMMON_PATH/utils/crashdump.h \
           $$COMMON_PATH/utils/crashhandler.h \
           $$COMMON_PATH/utils/winutils.h \
           $$COMMON_PATH/utils/widgetutils_win.h \
           $$COMMON_PATH/utils/executable_signature/executable_signature_win.h \
           $$COMMON_PATH/utils/servicecontrolmanager.h

} # win32


macx {

SOURCES += $$COMMON_PATH/utils/network_utils/network_utils_mac.cpp

OBJECTIVE_SOURCES += $$COMMON_PATH/utils/macutils.mm \
                $$COMMON_PATH/exithandler_mac.mm \
                $$COMMON_PATH/utils/widgetutils_mac.mm \
                $$COMMON_PATH/utils/executable_signature/executable_signature_mac.mm

HEADERS += $$COMMON_PATH//utils/macutils.h \
           $$COMMON_PATH/utils/network_utils/network_utils_mac.h \
           $$COMMON_PATH/exithandler_mac.h \
           $$COMMON_PATH/utils/widgetutils_mac.h \
           $$COMMON_PATH/utils/executable_signature/executable_signature_mac.h

} # macx

linux {

SOURCES += \
    $$COMMON_PATH/utils/executable_signature/executablesignature_linux.cpp \
    $$COMMON_PATH/utils/linuxutils.cpp

HEADERS += \
    $$COMMON_PATH/utils/executable_signature/executablesignature_linux.h \
    $$COMMON_PATH/utils/linuxutils.h

} # linux


SOURCES += \
    $$COMMON_PATH/ipc/proto/types.pb.cc \
    $$COMMON_PATH/ipc/proto/apiinfo.pb.cc \
    $$COMMON_PATH/ipc/proto/cli.pb.cc \
    $$COMMON_PATH/types/locationid.cpp \
    $$COMMON_PATH/types/pingtime.cpp \
    $$COMMON_PATH/utils/clean_sensitive_info.cpp \
    $$COMMON_PATH/utils/extraconfig.cpp \
    $$COMMON_PATH/utils/languagesutil.cpp \
    $$COMMON_PATH/utils/logger.cpp \
    $$COMMON_PATH/utils/mergelog.cpp \
    $$COMMON_PATH/utils/utils.cpp \
    $$COMMON_PATH/utils/widgetutils.cpp \
    $$COMMON_PATH/utils/executable_signature/executable_signature.cpp \
    $$COMMON_PATH/utils/ipvalidation.cpp \
    $$COMMON_PATH/version/appversion.cpp \
    $$COMMON_PATH/utils/hardcodedsettings.cpp \
    $$COMMON_PATH/utils/simplecrypt.cpp \
    $$COMMON_PATH/ipc/commandfactory.cpp \
    $$COMMON_PATH/ipc/connection.cpp \
    $$COMMON_PATH/ipc/server.cpp \
    $$COMMON_PATH/ipc/proto/clientcommands.pb.cc \
    $$COMMON_PATH/ipc/proto/servercommands.pb.cc \

HEADERS += \
    $$COMMON_PATH/names.h \
    $$COMMON_PATH/ipc/proto/types.pb.h \
    $$COMMON_PATH/ipc/proto/apiinfo.pb.h \
    $$COMMON_PATH/ipc/proto/cli.pb.h \
    $$COMMON_PATH/types/locationid.h \
    $$COMMON_PATH/types/pingtime.h \
    $$COMMON_PATH/utils/clean_sensitive_info.h \
    $$COMMON_PATH/utils/extraconfig.h \
    $$COMMON_PATH/utils/languagesutil.h \
    $$COMMON_PATH/utils/logger.h \
    $$COMMON_PATH/utils/mergelog.h \
    $$COMMON_PATH/utils/multiline_message_logger.h \
    $$COMMON_PATH/utils/utils.h \
    $$COMMON_PATH/utils/protobuf_includes.h \
    $$COMMON_PATH/utils/widgetutils.h \
    $$COMMON_PATH/utils/executable_signature/executable_signature.h \
    $$COMMON_PATH/utils/ipvalidation.h \
    $$COMMON_PATH/version/appversion.h \
    $$COMMON_PATH/version/windscribe_version.h \
    $$COMMON_PATH/utils/hardcodedsettings.h \
    $$COMMON_PATH/utils/simplecrypt.h \
    $$COMMON_PATH/ipc/command.h \
    $$COMMON_PATH/ipc/commandfactory.h \
    $$COMMON_PATH/ipc/connection.h \
    $$COMMON_PATH/ipc/iconnection.h \
    $$COMMON_PATH/ipc/iserver.h \
    $$COMMON_PATH/ipc/protobufcommand.h \
    $$COMMON_PATH/ipc/server.h \
    $$COMMON_PATH/ipc/proto/clientcommands.pb.h \
    $$COMMON_PATH/ipc/proto/servercommands.pb.h
