#-------------------------------------------------
#
# Project created by QtCreator 2015-10-31T22:26:50
#
#-------------------------------------------------

QT       += core network
QT       -= gui

CONFIG += app_bundle

TARGET = WindscribeEngine
TEMPLATE = app

DEFINES += QT_MESSAGELOGCONTEXT

COMMON_PATH = $$PWD/../../common
BUILD_LIBS_PATH = $$PWD/../../build-libs

INCLUDEPATH += $$COMMON_PATH


win32 {

QMAKE_CXXFLAGS_RELEASE -= -Zc:strictStrings
QMAKE_CFLAGS_RELEASE -= -Zc:strictStrings
QMAKE_CFLAGS -= -Zc:strictStrings
QMAKE_CXXFLAGS -= -Zc:strictStrings

# Supress protobuf linker warnings
QMAKE_LFLAGS += /IGNORE:4099

# Generate debug information (symbol files) for Windows
QMAKE_CXXFLAGS_RELEASE += /Zi
QMAKE_CXXFLAGS += /Zi
QMAKE_LFLAGS += /DEBUG

# Windows 7 platform
DEFINES += "WINVER=0x0601"
DEFINES += "_WIN32_WINNT=0x0601"
DEFINES += "PIO_APC_ROUTINE_DEFINED"

SOURCES += engine/networkstatemanager/networkstatemanager_win.cpp \
           engine/connectionmanager/adapterutils_win.cpp \
           engine/dnsinfo_win.cpp \
           engine/taputils/tapinstall_win.cpp \
           engine/helper/helper_win.cpp \
           engine/proxy/autodetectproxy_win.cpp \
           engine/firewall/firewallcontroller_win.cpp \
           utils/bfe_service_win.cpp \
           utils/ras_service_win.cpp \
           engine/dnsresolver/dnsutils_win.cpp \
           engine/adaptermetricscontroller_win.cpp \
           engine/connectionmanager/sleepevents_win.cpp \
           utils/installedantiviruses_win.cpp \
           engine/taputils/checkadapterenable.cpp \
           engine/vpnshare/wifisharing/wifisharing.cpp \
           engine/vpnshare/wifisharing/icsmanager.cpp \
           engine/vpnshare/wifisharing/wlanmanager.cpp \
           engine/helper/windscribeinstallhelper_win.cpp \
           engine/connectionmanager/ikev2connection_win.cpp \
           engine/measurementcpuusage.cpp \
           engine/ping/pinghost_icmp_win.cpp \
           engine/connectionmanager/ikev2connectiondisconnectlogic_win.cpp \
           engine/macaddresscontroller/macaddresscontroller_win.cpp \
           engine/networkdetectionmanager/networkdetectionmanager_win.cpp \
           engine/networkdetectionmanager/networkchangeworkerthread.cpp \
           $$COMMON_PATH/utils/crashdump.cpp \
           $$COMMON_PATH/utils/crashhandler.cpp \
           $$COMMON_PATH/utils/winutils.cpp \
           $$COMMON_PATH/utils/executable_signature/executable_signature_win.cpp


HEADERS += engine/networkstatemanager/networkstatemanager_win.h \
           engine/connectionmanager/adapterutils_win.h \
           engine/dnsinfo_win.h \
           engine/taputils/tapinstall_win.h \
           engine/helper/helper_win.h \
           engine/proxy/autodetectproxy_win.h \
           engine/firewall/firewallcontroller_win.h \
           utils/bfe_service_win.h \
           utils/ras_service_win.h \
           engine/adaptermetricscontroller_win.h \
           engine/connectionmanager/sleepevents_win.h \
           utils/installedantiviruses_win.h \
           engine/taputils/checkadapterenable.h \
           engine/vpnshare/wifisharing/wifisharing.h \
           engine/vpnshare/wifisharing/icsmanager.h \
           engine/vpnshare/wifisharing/wlanmanager.h \
           engine/helper/windscribeinstallhelper_win.h \
           engine/connectionmanager/ikev2connection_win.h \
           engine/measurementcpuusage.h \
           engine/ping/pinghost_icmp_win.h \
           engine/connectionmanager/ikev2connectiondisconnectlogic_win.h \
           engine/macaddresscontroller/macaddresscontroller_win.h \
           engine/networkdetectionmanager/networkdetectionmanager_win.h \
           engine/networkdetectionmanager/networkchangeworkerthread.h \
           $$COMMON_PATH/utils/crashdump.h \
           $$COMMON_PATH/utils/crashhandler.h \
           $$COMMON_PATH/utils/winutils.h \
           $$COMMON_PATH/utils/executable_signature/executable_signature_win.h


LIBS += Ws2_32.lib Advapi32.lib Iphlpapi.lib \
    Wininet.lib User32.lib Sensapi.lib Dnsapi.lib \
    Ole32.lib Shlwapi.lib Version.lib Psapi.lib \
    rasapi32.lib Pdh.lib Shell32.lib netapi32.lib msi.lib

INCLUDEPATH += $$BUILD_LIBS_PATH/boost/include/boost-1_69
LIBS += -L"$$BUILD_LIBS_PATH/boost/lib"

INCLUDEPATH += "$$BUILD_LIBS_PATH/curl/include"
LIBS += -L"$$BUILD_LIBS_PATH/curl/lib" -llibcurl

#DEFINES += "CARES_STATICLIB"
INCLUDEPATH += "$$BUILD_LIBS_PATH/cares/dll_x32/include"
LIBS += -L"$$BUILD_LIBS_PATH/cares/dll_x32/lib" -lcares

INCLUDEPATH += "$$BUILD_LIBS_PATH/openssl/include"
LIBS += -L"$$BUILD_LIBS_PATH/openssl/lib" -llibeay32 -lssleay32

# protobuf libs
CONFIG(release, debug|release){
    INCLUDEPATH += $$BUILD_LIBS_PATH/protobuf/release/include
    LIBS += -L$$BUILD_LIBS_PATH/protobuf/release/lib -llibprotobuf
}
CONFIG(debug, debug|release){
    INCLUDEPATH += $$BUILD_LIBS_PATH/protobuf/debug/include
    LIBS += -L$$BUILD_LIBS_PATH/protobuf/debug/lib -llibprotobufd
}

RC_FILE = engine.rc
} #end win32

macx {

HOMEDIR = $$(HOME)

LIBS += -framework Foundation
LIBS += -framework CoreFoundation
LIBS += -framework CoreServices
LIBS += -framework Security
LIBS += -framework SystemConfiguration
LIBS += -framework AppKit
LIBS += -framework ServiceManagement
LIBS += -framework NetworkExtension

#LIBS += -L"/Users/admin/Documents/TestLib/TestLib" -ltestlib

#QMAKE_OBJECTIVE_CFLAGS += -fobjc-arc

#remove unused parameter warnings
QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter

#boost include and libs
INCLUDEPATH += $$BUILD_LIBS_PATH/boost/include
LIBS += $$BUILD_LIBS_PATH/boost/lib/libboost_system.a
LIBS += $$BUILD_LIBS_PATH/boost/lib/libboost_serialization.a

INCLUDEPATH += $$BUILD_LIBS_PATH/openssl/include
LIBS+=-L$$BUILD_LIBS_PATH/openssl/lib -lssl -lcrypto
INCLUDEPATH += $$BUILD_LIBS_PATH/curl/include
LIBS += -L$$BUILD_LIBS_PATH/curl/lib/ -lcurl

#protobuf include and libs
INCLUDEPATH += $$BUILD_LIBS_PATH/protobuf/include
LIBS += -L$$BUILD_LIBS_PATH/protobuf/lib -lprotobuf

#c-ares library
# don't forget remove *.dylib files for static link
INCLUDEPATH += $$BUILD_LIBS_PATH/cares/include
LIBS += -L$$BUILD_LIBS_PATH/cares/lib -lcares


SOURCES += engine/networkstatemanager/networkstatemanager_mac.cpp \
           engine/firewall/firewallcontroller_mac.cpp \
           engine/ipv6controller_mac.cpp \
           engine/connectionmanager/networkextensionlog_mac.cpp \
           engine/ping/pinghost_icmp_mac.cpp \
           engine/networkdetectionmanager/networkdetectionmanager_mac.cpp \
           engine/helper/helper_posix.cpp \
           engine/helper/helper_mac.cpp \
           engine/dnsresolver/dnsutils_mac.cpp \
           engine/macaddresscontroller/macaddresscontroller_mac.cpp

HEADERS +=     $$COMMON_PATH/utils/macutils.h \
               engine/connectionmanager/sleepevents_mac.h \
               engine/networkstatemanager/reachabilityevents.h \
               engine/connectionmanager/restorednsmanager_mac.h \
               engine/networkstatemanager/networkstatemanager_mac.h \
               engine/helper/helper_posix.h \
               engine/helper/helper_mac.h \
               engine/helper/installhelper_mac.h \
               engine/proxy/pmachelpers.h \
               engine/proxy/autodetectproxy_mac.h \
               engine/firewall/firewallcontroller_mac.h \
               engine/ipv6controller_mac.h \
               engine/connectionmanager/ikev2connection_mac.h \
               engine/connectionmanager/networkextensionlog_mac.h \
               engine/ping/pinghost_icmp_mac.h \
               engine/networkdetectionmanager/networkdetectionmanager_mac.h \
               engine/macaddresscontroller/macaddresscontroller_mac.h \
               $$COMMON_PATH/utils/executable_signature/executable_signature_mac.h

OBJECTIVE_HEADERS += \
               engine/networkstatemanager/reachability.h \
               $$COMMON_PATH/exithandler_mac.h

OBJECTIVE_SOURCES += $$COMMON_PATH/utils/macutils.mm \
                     engine/connectionmanager/sleepevents_mac.mm \
                     engine/networkstatemanager/reachability.m \
                     engine/networkstatemanager/reachabilityevents.mm \
                     engine/connectionmanager/restorednsmanager_mac.mm \
                     engine/helper/installhelper_mac.mm \
                     engine/proxy/pmachelpers.mm \
                     engine/proxy/autodetectproxy_mac.mm \
                     engine/connectionmanager/ikev2connection_mac.mm \
                     $$COMMON_PATH/exithandler_mac.mm \
                     $$COMMON_PATH/utils/executable_signature/executable_signature_mac.mm

QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.11

ICON = mac/windscribe.icns
QMAKE_INFO_PLIST = mac/info.plist

#QMAKE_LFLAGS += -sectcreate __TEXT __info_plist $$shell_quote($$PWD/Mac/Info.plist)

MY_ENTITLEMENTS.name = CODE_SIGN_ENTITLEMENTS
MY_ENTITLEMENTS.value = $$PWD/mac/windscribe.entitlements
QMAKE_MAC_XCODE_SETTINGS += MY_ENTITLEMENTS

#postbuild copy commands
copy_resources.commands = $(COPY_DIR) $$PWD/mac/resources $$OUT_PWD/WindscribeEngine.app/Contents
mkdir_launch_services.commands = $(MKDIR) $$OUT_PWD/WindscribeEngine.app/Contents/Library/LaunchServices
copy_helper.commands = $(COPY_DIR) $$PWD/../../installer/mac/binaries/helper/com.windscribe.helper.macos $$OUT_PWD/WindscribeEngine.app/Contents/Library/LaunchServices
copy_profile.commands = $(COPY_DIR) $$PWD/../mac/provisioning_profile/embedded.provisionprofile $$OUT_PWD/WindscribeEngine.app/Contents
mkdir_helpers.commands = $(MKDIR) $$OUT_PWD/WindscribeEngine.app/Contents/Helpers
copy_openvpn.commands = cp $$BUILD_LIBS_PATH/openvpn_2_5_0/openvpn $$OUT_PWD/WindscribeEngine.app/Contents/Helpers/windscribeopenvpn_2_5_0
copy_stunnel.commands = cp $$BUILD_LIBS_PATH/stunnel/stunnel $$OUT_PWD/WindscribeEngine.app/Contents/Helpers/windscribestunnel
copy_wstunnel.commands = cp $$PWD/../mac/wstunnel/windscribewstunnel $$OUT_PWD/WindscribeEngine.app/Contents/Helpers/windscribewstunnel
copy_kext.commands = $(COPY_DIR) $$PWD/../mac/kext/Binary/WindscribeKext.kext $$OUT_PWD/WindscribeEngine.app/Contents/Helpers/WindscribeKext.kext
copy_wireguard.commands = cp $$BUILD_LIBS_PATH/wireguard/windscribewireguard $$OUT_PWD/WindscribeEngine.app/Contents/Helpers/windscribewireguard


first.depends = $(first) copy_resources mkdir_launch_services copy_helper copy_profile mkdir_helpers copy_openvpn copy_stunnel copy_wstunnel copy_wireguard copy_kext
export(first.depends)
export(copy_resources.commands)
export(mkdir_launch_services.commands)
export(copy_helper.commands)
export(copy_profile.commands)
export(mkdir_helpers.commands)
export(copy_openvpn.commands)
export(copy_stunnel.commands)
export(copy_wstunnel.commands)
export(copy_kext.commands)
export(copy_wireguard.commands)
QMAKE_EXTRA_TARGETS += first copy_resources mkdir_launch_services copy_helper copy_profile mkdir_helpers copy_openvpn copy_stunnel copy_wstunnel copy_wireguard copy_kext

} # end macx


linux {

#remove linux deprecated copy warnings
QMAKE_CXXFLAGS_WARN_ON += -Wno-deprecated-copy


#boost include and libs
INCLUDEPATH += $$BUILD_LIBS_PATH/boost/include
LIBS += $$BUILD_LIBS_PATH/boost/lib/libboost_system.a
LIBS += $$BUILD_LIBS_PATH/boost/lib/libboost_serialization.a

INCLUDEPATH += $$BUILD_LIBS_PATH/openssl/include
LIBS+=-L$$BUILD_LIBS_PATH/openssl/lib -lssl -lcrypto
INCLUDEPATH += $$BUILD_LIBS_PATH/curl/include
LIBS += -L$$BUILD_LIBS_PATH/curl/lib/ -lcurl

#protobuf include and libs
INCLUDEPATH += $$BUILD_LIBS_PATH/protobuf/include
LIBS += -L$$BUILD_LIBS_PATH/protobuf/lib -lprotobuf

#c-ares library
INCLUDEPATH += $$BUILD_LIBS_PATH/cares/include
LIBS += -L$$BUILD_LIBS_PATH/cares/lib -lcares

SOURCES += \
           $$COMMON_PATH/utils/linuxutils.cpp \
           utils/dnsscripts_linux.cpp \
           engine/ping/pinghost_icmp_mac.cpp \
           engine/dnsresolver/dnsutils_linux.cpp \
           engine/helper/helper_posix.cpp \
           engine/helper/helper_linux.cpp \
           engine/networkstatemanager/networkstatemanager_linux.cpp \
           engine/firewall/firewallcontroller_linux.cpp \
           engine/connectionmanager/ikev2connection_linux.cpp \
           engine/networkdetectionmanager/networkdetectionmanager_linux.cpp \
           engine/macaddresscontroller/macaddresscontroller_linux.cpp

HEADERS += \
           $$COMMON_PATH/utils/linuxutils.h \
           utils/dnsscripts_linux.h \
           engine/ping/pinghost_icmp_mac.h \
           engine/helper/helper_posix.h \
           engine/helper/helper_linux.h \
           engine/networkstatemanager/networkstatemanager_linux.h \
           engine/firewall/firewallcontroller_linux.h \
           engine/connectionmanager/ikev2connection_linux.h \
           engine/networkdetectionmanager/networkdetectionmanager_linux.h \
           engine/macaddresscontroller/macaddresscontroller_linux.h
} # unix




SOURCES += main.cpp \
    $$COMMON_PATH/ipc/generated_proto/types.pb.cc \
    $$COMMON_PATH/ipc/generated_proto/apiinfo.pb.cc \
    $$COMMON_PATH/utils/clean_sensitive_info.cpp \
    $$COMMON_PATH/utils/utils.cpp \
    $$COMMON_PATH/utils/logger.cpp \
    $$COMMON_PATH/utils/mergelog.cpp \
    $$COMMON_PATH/utils/extraconfig.cpp \
    $$COMMON_PATH/utils/ipvalidation.cpp \
    $$COMMON_PATH/version/appversion.cpp \
    $$COMMON_PATH/utils/executable_signature/executable_signature.cpp \
    application/windowsnativeeventfilter.cpp \
    application/windscribeapplication.cpp \
    engine/apiinfo/apiinfo.cpp \
    engine/apiinfo/sessionstatus.cpp \
    engine/apiinfo/location.cpp \
    engine/apiinfo/group.cpp \
    engine/apiinfo/node.cpp \
    engine/apiinfo/notification.cpp \
    engine/apiinfo/portmap.cpp \
    engine/apiinfo/staticips.cpp \
    engine/apiinfo/servercredentials.cpp \
    engine/autoupdater/downloadhelper.cpp \
    engine/ping/keepalivemanager.cpp \
    engine/locationsmodel/locationsmodel.cpp \
    engine/locationsmodel/apilocationsmodel.cpp \
    engine/locationsmodel/customconfiglocationsmodel.cpp \
    engine/locationsmodel/locationitem.cpp \
    engine/locationsmodel/pingipscontroller.cpp \
    engine/locationsmodel/pingstorage.cpp \
    engine/locationsmodel/bestlocation.cpp \
    engine/locationsmodel/baselocationinfo.cpp \
    engine/locationsmodel/mutablelocationinfo.cpp \
    engine/locationsmodel/customconfiglocationinfo.cpp \
    engine/locationsmodel/pingtime.cpp \
    engine/locationsmodel/pinglog.cpp \
    engine/locationsmodel/failedpinglogcontroller.cpp \
    engine/locationsmodel/nodeselectionalgorithm.cpp \
    engine/packetsizecontroller.cpp \
    engine/enginesettings.cpp \
    engine/autoupdater/autoupdaterhelper_mac.cpp \
    engine/tempscripts_mac.cpp \
    utils/simplecrypt.cpp \
    engine/logincontroller/logincontroller.cpp \
    engine/logincontroller/getallconfigscontroller.cpp \
    engine/proxy/proxysettings.cpp \
    engine/types/connectionsettings.cpp \
    engine/getmyipcontroller.cpp \
    engine/firewall/uniqueiplist.cpp \
    engine/firewall/firewallexceptions.cpp \
    engine/firewall/firewallcontroller.cpp \
    engine/proxy/proxyservercontroller.cpp \
    engine/types/dnsresolutionsettings.cpp \
    engine/connectionmanager/makeovpnfile.cpp \
    engine/connectionmanager/adaptergatewayinfo.cpp \
    engine/connectionmanager/stunnelmanager.cpp \
    engine/connectionmanager/testvpntunnel.cpp \
    engine/connectionmanager/openvpnconnection.cpp \
    engine/connectionmanager/connsettingspolicy/autoconnsettingspolicy.cpp \
    engine/connectionmanager/connsettingspolicy/manualconnsettingspolicy.cpp \
    engine/connectionmanager/connsettingspolicy/customconfigconnsettingspolicy.cpp \
    engine/connectionmanager/connectionmanager.cpp \
    engine/connectionmanager/availableport.cpp \
    engine/connectionmanager/wireguardconnection.cpp \
    $$COMMON_PATH/types/locationid.cpp \
    engine/logincontroller/getapiaccessips.cpp \
    engine/helper/initializehelper.cpp \
    engine/refetchservercredentialshelper.cpp \
    engine/vpnshare/httpproxyserver/httpproxyserver.cpp \
    engine/vpnshare/httpproxyserver/httpproxyconnectionmanager.cpp \
    engine/vpnshare/httpproxyserver/httpproxyconnection.cpp \
    engine/vpnshare/httpproxyserver/httpproxyrequestparser.cpp \
    engine/vpnshare/httpproxyserver/httpproxyrequest.cpp \
    engine/vpnshare/httpproxyserver/httpproxywebanswer.cpp \
    engine/vpnshare/httpproxyserver/httpproxywebanswerparser.cpp \
    engine/vpnshare/socksproxyserver/socksproxyserver.cpp \
    engine/vpnshare/socksproxyserver/socksproxyconnectionmanager.cpp \
    engine/vpnshare/socksproxyserver/socksproxyconnection.cpp \
    engine/vpnshare/socksproxyserver/socksproxyreadexactly.cpp \
    engine/vpnshare/socksproxyserver/socksproxyidentreqparser.cpp \
    engine/vpnshare/socketutils/socketwriteall.cpp \
    engine/vpnshare/socksproxyserver/socksproxycommandparser.cpp \
    engine/vpnshare/vpnsharecontroller.cpp \
    engine/vpnshare/connecteduserscounter.cpp \
    engine/vpnshare/httpproxyserver/httpproxyreply.cpp \
    engine/connectstatecontroller/connectstatecontroller.cpp \
    engine/openvpnversioncontroller.cpp \
    engine/types/types.cpp \
    engine/serverapi/curlnetworkmanager.cpp \
    engine/serverapi/curlrequest.cpp \
    engine/serverapi/dnscache.cpp \
    engine/serverapi/serverapi.cpp \
    engine/hardcodedsettings.cpp \
    engine/engine.cpp \
    engine/crossplatformobjectfactory.cpp \
    engine/types/loginsettings.cpp \
    engine/emergencycontroller/emergencycontroller.cpp \
    engine/dnsresolver/areslibraryinit.cpp \
    engine/dnsresolver/dnsrequest.cpp \
    engine/dnsresolver/dnsserversconfiguration.cpp \
    engine/dnsresolver/dnsresolver.cpp \
    engine/types/protocoltype.cpp \
    engine/helper/simple_xor_crypt.cpp \
    engine/tests/sessionandlocations_test.cpp \
    engine/sessionstatustimer.cpp \
    engine/connectionmanager/wstunnelmanager.cpp \
    engine/customconfigs/customconfigs.cpp \
    engine/customconfigs/customconfigtype.cpp \
    engine/customconfigs/ovpncustomconfig.cpp \
    engine/customconfigs/wireguardcustomconfig.cpp \
    engine/connectionmanager/makeovpnfilefromcustom.cpp \
    engine/customconfigs/parseovpnconfigline.cpp \
    engine/customconfigs/customovpnauthcredentialsstorage.cpp \
    engine/ping/pinghost_tcp.cpp \
    engine/ping/pinghost.cpp \
    engine/customconfigs/customconfigsdirwatcher.cpp \
    engine/types/wireguardconfig.cpp \
    engine/getdeviceid.cpp \
    qconsolelistener.cpp \
    engineserver.cpp \
    $$COMMON_PATH/ipc/commandfactory.cpp \
    $$COMMON_PATH/ipc/connection.cpp \
    $$COMMON_PATH/ipc/server.cpp \
    $$COMMON_PATH/ipc/generated_proto/clientcommands.pb.cc \
    $$COMMON_PATH/ipc/generated_proto/servercommands.pb.cc \
    clientconnectiondescr.cpp \
    $$COMMON_PATH/ipc/tcpconnection.cpp \
    $$COMMON_PATH/ipc/tcpserver.cpp \
    engine/connectionmanager/finishactiveconnections.cpp \
    engine/networkaccessmanager/certmanager.cpp \
    engine/networkaccessmanager/curlinitcontroller.cpp \
    engine/networkaccessmanager/curlnetworkmanager2.cpp \
    engine/networkaccessmanager/curlreply.cpp \
    engine/networkaccessmanager/networkrequest.cpp \
    engine/networkaccessmanager/dnscache2.cpp \
    engine/networkaccessmanager/networkaccessmanager.cpp

HEADERS  +=  engine/locationsmodel/locationsmodel.h \
    engine/locationsmodel/apilocationsmodel.h \
    engine/locationsmodel/customconfiglocationsmodel.h \
    engine/locationsmodel/locationitem.h \
    engine/locationsmodel/pingipscontroller.h \
    engine/locationsmodel/pingstorage.h \
    engine/locationsmodel/bestlocation.h \
    engine/locationsmodel/locationnode.h \
    engine/locationsmodel/baselocationinfo.h \
    engine/locationsmodel/mutablelocationinfo.h \
    engine/locationsmodel/customconfiglocationinfo.h \
    engine/locationsmodel/pingtime.h \
    engine/locationsmodel/pinglog.h \
    engine/locationsmodel/failedpinglogcontroller.h \
    engine/locationsmodel/nodeselectionalgorithm.h \
    $$COMMON_PATH/ipc/generated_proto/types.pb.h \
    $$COMMON_PATH/ipc/generated_proto/apiinfo.pb.h \
    $$COMMON_PATH/utils/clean_sensitive_info.h \
    $$COMMON_PATH/utils/utils.h \
    $$COMMON_PATH/utils/protobuf_includes.h \
    $$COMMON_PATH/utils/logger.h \
    $$COMMON_PATH/utils/mergelog.h \
    $$COMMON_PATH/utils/multiline_message_logger.h \
    $$COMMON_PATH/utils/extraconfig.h \
    $$COMMON_PATH/version/appversion.h \
    $$COMMON_PATH/version/windscribe_version.h \
    $$COMMON_PATH/utils/executable_signature/executable_signature.h \
    $$COMMON_PATH/utils/ipvalidation.h \
    ../../common/names.h \
    application/windowsnativeeventfilter.h \
    application/windscribeapplication.h \
    engine/apiinfo/apiinfo.h \
    engine/apiinfo/sessionstatus.h \
    engine/apiinfo/location.h \
    engine/apiinfo/group.h \
    engine/apiinfo/node.h \
    engine/apiinfo/notification.h \
    engine/apiinfo/portmap.h \
    engine/apiinfo/staticips.h \
    engine/apiinfo/servercredentials.h \
    engine/connectionmanager/adaptergatewayinfo.h \
    engine/connectionmanager/makeovpnfile.h \
    engine/autoupdater/downloadhelper.h \
    engine/macaddresscontroller/imacaddresscontroller.h \
    engine/networkdetectionmanager/inetworkdetectionmanager.h \
    $$COMMON_PATH/utils/networktypes.h \
    engine/ping/keepalivemanager.h \
    engine/packetsizecontroller.h \
    engine/enginesettings.h \
    engine/autoupdater/autoupdaterhelper_mac.h \
    engine/connectionmanager/stunnelmanager.h \
    engine/tempscripts_mac.h \
    engine/networkstatemanager/inetworkstatemanager.h \
    utils/simplecrypt.h \
    engine/logincontroller/logincontroller.h \
    engine/logincontroller/getallconfigscontroller.h \
    engine/proxy/proxysettings.h \
    engine/types/connectionsettings.h \
    engine/getmyipcontroller.h \
    engine/firewall/uniqueiplist.h \
    engine/helper/ihelper.h \
    engine/firewall/firewallcontroller.h \
    engine/firewall/firewallexceptions.h \
    engine/proxy/proxyservercontroller.h \
    engine/connectionmanager/testvpntunnel.h \
    engine/types/dnsresolutionsettings.h \
    engine/connectionmanager/iconnection.h \
    engine/connectionmanager/openvpnconnection.h \
    engine/connectionmanager/isleepevents.h \
    engine/connectionmanager/wireguardconnection.h \
    utils/boost_includes.h \
    engine/types/types.h \
    engine/connectionmanager/connsettingspolicy/baseconnsettingspolicy.h \
    engine/connectionmanager/connsettingspolicy/autoconnsettingspolicy.h \
    engine/connectionmanager/connsettingspolicy/manualconnsettingspolicy.h \
    engine/connectionmanager/connsettingspolicy/customconfigconnsettingspolicy.h \
    engine/connectionmanager/connectionmanager.h \
    $$COMMON_PATH/types/locationid.h \
    engine/logincontroller/getapiaccessips.h \
    engine/helper/initializehelper.h \
    engine/refetchservercredentialshelper.h \
    engine/connectionmanager/availableport.h \
    engine/vpnshare/httpproxyserver/httpproxyserver.h \
    engine/vpnshare/httpproxyserver/httpproxyconnectionmanager.h \
    engine/vpnshare/httpproxyserver/httpproxyconnection.h \
    engine/vpnshare/httpproxyserver/httpproxyrequestparser.h \
    engine/vpnshare/httpproxyserver/httpproxyrequest.h \
    engine/vpnshare/httpproxyserver/httpproxyheader.h \
    engine/vpnshare/httpproxyserver/httpproxywebanswer.h \
    engine/vpnshare/httpproxyserver/httpproxywebanswerparser.h \
    engine/vpnshare/socksproxyserver/socksproxyserver.h \
    engine/vpnshare/socksproxyserver/socksproxyconnectionmanager.h \
    engine/vpnshare/socksproxyserver/socksproxyconnection.h \
    engine/vpnshare/socksproxyserver/socksstructs.h \
    engine/vpnshare/socksproxyserver/socksproxyreadexactly.h \
    engine/vpnshare/socksproxyserver/socksproxyidentreqparser.h \
    engine/vpnshare/socketutils/socketwriteall.h \
    engine/vpnshare/socksproxyserver/socksproxycommandparser.h \
    engine/vpnshare/vpnsharecontroller.h \
    engine/vpnshare/connecteduserscounter.h \
    engine/vpnshare/httpproxyserver/httpproxyreply.h \
    engine/connectstatecontroller/iconnectstatecontroller.h \
    engine/connectstatecontroller/connectstatecontroller.h \
    engine/openvpnversioncontroller.h \
    engine/serverapi/curlnetworkmanager.h \
    engine/serverapi/curlrequest.h \
    engine/serverapi/dnscache.h \
    engine/serverapi/serverapi.h \
    engine/hardcodedsettings.h \
    engine/engine.h \
    engine/crossplatformobjectfactory.h \
    engine/types/loginsettings.h \
    engine/emergencycontroller/emergencycontroller.h \
    engine/dnsresolver/areslibraryinit.h \
    engine/dnsresolver/dnsutils.h \
    engine/dnsresolver/dnsrequest.h \
    engine/dnsresolver/dnsserversconfiguration.h \
    engine/dnsresolver/dnsresolver.h \
    engine/types/protocoltype.h \
    engine/helper/simple_xor_crypt.h \
    engine/tests/sessionandlocations_test.h \
    engine/sessionstatustimer.h \
    engine/connectionmanager/wstunnelmanager.h \
    engine/customconfigs/icustomconfig.h \
    engine/customconfigs/customconfigtype.h \
    engine/customconfigs/ovpncustomconfig.h \
    engine/customconfigs/wireguardcustomconfig.h \
    engine/customconfigs/customconfigs.h \
    engine/connectionmanager/makeovpnfilefromcustom.h \
    engine/customconfigs/parseovpnconfigline.h \
    engine/customconfigs/customovpnauthcredentialsstorage.h \
    engine/ping/icmp_header.h \
    engine/ping/ipv4_header.h \
    engine/ping/pinghost_tcp.h \
    engine/ping/pinghost.h \
    engine/customconfigs/customconfigsdirwatcher.h \
    engine/types/wireguardconfig.h \
    engine/getdeviceid.h \
    qconsolelistener.h \
    engineserver.h \
    $$COMMON_PATH/ipc/command.h \
    $$COMMON_PATH/ipc/commandfactory.h \
    $$COMMON_PATH/ipc/connection.h \
    $$COMMON_PATH/ipc/iconnection.h \
    $$COMMON_PATH/ipc/iserver.h \
    $$COMMON_PATH/ipc/protobufcommand.h \
    $$COMMON_PATH/ipc/server.h \
    $$COMMON_PATH/ipc/generated_proto/clientcommands.pb.h \
    $$COMMON_PATH/ipc/generated_proto/servercommands.pb.h \
    clientconnectiondescr.h \
    $$COMMON_PATH/ipc/tcpconnection.h \
    $$COMMON_PATH/ipc/tcpserver.h \
    engine/connectionmanager/finishactiveconnections.h \
    engine/networkaccessmanager/certmanager.h \
    engine/networkaccessmanager/curlinitcontroller.h \
    engine/networkaccessmanager/curlnetworkmanager2.h \
    engine/networkaccessmanager/curlreply.h \
    engine/networkaccessmanager/networkrequest.h \
    engine/networkaccessmanager/dnscache2.h \
    engine/networkaccessmanager/networkaccessmanager.h

RESOURCES += \
    engine.qrc
