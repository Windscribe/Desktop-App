QT += core gui network svg core5compat

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

# build_all.py adds 'use_signature_check' to the CONFIG environment when invoked with the '--sign' flag.
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
LIBS += -framework NetworkExtension


INCLUDEPATH += $$BUILD_LIBS_PATH/protobuf/include
LIBS += -L$$BUILD_LIBS_PATH/protobuf/lib -lprotobuf

#boost include and libs
INCLUDEPATH += $$BUILD_LIBS_PATH/boost/include
LIBS += $$BUILD_LIBS_PATH/boost/lib/libboost_serialization.a

INCLUDEPATH += $$BUILD_LIBS_PATH/openssl/include
LIBS+=-L$$BUILD_LIBS_PATH/openssl/lib -lssl -lcrypto
INCLUDEPATH += $$BUILD_LIBS_PATH/curl/include
LIBS += -L$$BUILD_LIBS_PATH/curl/lib/ -lcurl

#c-ares library
# don't forget remove *.dylib files for static link
INCLUDEPATH += $$BUILD_LIBS_PATH/cares/include
LIBS += -L$$BUILD_LIBS_PATH/cares/lib -lcares

#remove unused and deprecated parameter warnings
QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter -Wno-deprecated-declarations -Wno-range-loop-construct -Wno-deprecated-copy

QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.14
ICON = windscribe.icns
QMAKE_INFO_PLIST = info.plist

#postbuild copy commands
make_login_items.commands = $(MKDIR) $$OUT_PWD/Windscribe.app/Contents/Library/LoginItems
copy_launcher.commands = $(COPY_DIR) $$PWD/../installer/mac/binaries/launcher/WindscribeLauncher.app $$OUT_PWD/Windscribe.app/Contents/Library/LoginItems
copy_resources.commands = $(COPY_DIR) $$PWD/engine/mac/resources $$OUT_PWD/Windscribe.app/Contents
mkdir_launch_services.commands = $(MKDIR) $$OUT_PWD/Windscribe.app/Contents/Library/LaunchServices
copy_helper.commands = $(COPY_DIR) $$PWD/../installer/mac/binaries/helper/com.windscribe.helper.macos $$OUT_PWD/Windscribe.app/Contents/Library/LaunchServices

mkdir_helpers.commands = $(MKDIR) $$OUT_PWD/Windscribe.app/Contents/Helpers
copy_openvpn.commands = cp $$BUILD_LIBS_PATH/openvpn_2_5_4/openvpn $$OUT_PWD/Windscribe.app/Contents/Helpers/windscribeopenvpn_2_5_4
copy_stunnel.commands = cp $$BUILD_LIBS_PATH/stunnel/stunnel $$OUT_PWD/Windscribe.app/Contents/Helpers/windscribestunnel
copy_wstunnel.commands = cp $$PWD/../backend/mac/wstunnel/windscribewstunnel $$OUT_PWD/Windscribe.app/Contents/Helpers/windscribewstunnel
copy_kext.commands = $(COPY_DIR) $$PWD/../backend/mac/kext/Binary/WindscribeKext.kext $$OUT_PWD/Windscribe.app/Contents/Helpers/WindscribeKext.kext
copy_wireguard.commands = cp $$BUILD_LIBS_PATH/wireguard/windscribewireguard $$OUT_PWD/Windscribe.app/Contents/Helpers/windscribewireguard

exists( $$PWD/../backend/mac/provisioning_profile/embedded.provisionprofile ) {
    copy_profile.commands = $(COPY_DIR) $$PWD/../backend/mac/provisioning_profile/embedded.provisionprofile $$OUT_PWD/Windscribe.app/Contents
}

first.depends = $(first) make_login_items copy_launcher copy_resources mkdir_launch_services copy_helper \
                         mkdir_helpers copy_openvpn copy_stunnel copy_wstunnel copy_kext copy_wireguard copy_profile
export(first.depends)
export(make_login_items.commands)
export(copy_launcher.commands)
export(copy_resources.commands)
export(mkdir_launch_services.commands)
export(copy_helper.commands)
export(mkdir_helpers.commands)
export(copy_openvpn.commands)
export(copy_stunnel.commands)
export(copy_wstunnel.commands)
export(copy_kext.commands)
export(copy_wireguard.commands)
export(copy_profile.commands)
QMAKE_EXTRA_TARGETS += first make_login_items copy_launcher copy_resources mkdir_launch_services copy_helper \
                        mkdir_helpers copy_openvpn copy_stunnel copy_wstunnel copy_kext copy_wireguard copy_profile

# only for release build
# comment CONFIG... line if need embedded engine and cli for debug purposes
CONFIG(release, debug|release) {

    # package cli inside Windscribe.app/Contents/MacOS
    mkdir_cli.commands = $(MKDIR) $$OUT_PWD/Windscribe.app/Contents/MacOS
    first.depends += mkdir_cli
    export(mkdir_cli.commands)
    QMAKE_EXTRA_TARGETS += mkdir_cli
    copy_cli.commands = $(COPY_FILE) $$PWD/../installer/mac/binaries/windscribe-cli $$OUT_PWD/Windscribe.app/Contents/MacOS/windscribe-cli
    first.depends += copy_cli
    export(copy_cli.commands)
    QMAKE_EXTRA_TARGETS += copy_cli

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

INCLUDEPATH += $$BUILD_LIBS_PATH/curl/include
LIBS += -L$$BUILD_LIBS_PATH/curl/lib/ -lcurl

INCLUDEPATH += $$BUILD_LIBS_PATH/cares/include
LIBS += -L$$BUILD_LIBS_PATH/cares/lib -lcares

INCLUDEPATH += $$BUILD_LIBS_PATH/boost/include
LIBS += $$BUILD_LIBS_PATH/boost/lib/libboost_filesystem.a
LIBS += $$BUILD_LIBS_PATH/boost/lib/libboost_serialization.a

} # linux


SOURCES += main.cpp

include(common.pri)
include(gui/gui.pri)
include(engine/engine.pri)

exists($$COMMON_PATH/utils/hardcodedsecrets.ini) {
    RESOURCES += secrets.qrc
}

