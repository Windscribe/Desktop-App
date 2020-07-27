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

INCLUDEPATH += $$COMMON_PATH

win32 {

QMAKE_CXXFLAGS_RELEASE -= -Zc:strictStrings
QMAKE_CFLAGS_RELEASE -= -Zc:strictStrings
QMAKE_CFLAGS -= -Zc:strictStrings
QMAKE_CXXFLAGS -= -Zc:strictStrings

# Windows 7 platform
DEFINES += "WINVER=0x0601"
DEFINES += "PIO_APC_ROUTINE_DEFINED"

SOURCES += engine/networkstatemanager/networkstatemanager_win.cpp \
           engine/connectionmanager/resetwindscribetap_win.cpp \
           engine/dnsinfo_win.cpp \
           engine/taputils/tapinstall_win.cpp \
           engine/helper/helper_win.cpp \
           engine/proxy/autodetectproxy_win.cpp \
           engine/firewall/firewallcontroller_win.cpp \
           utils/bfecontroller_win.cpp \
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
           $$COMMON_PATH/utils/winutils.cpp \
           $$COMMON_PATH/utils/executable_signature/executable_signature_win.cpp \
           engine/connectionmanager/finishactiveconnections.cpp


HEADERS += engine/networkstatemanager/networkstatemanager_win.h \
           engine/connectionmanager/resetwindscribetap_win.h \
           engine/dnsinfo_win.h \
           engine/taputils/tapinstall_win.h \
           engine/helper/helper_win.h \
           engine/proxy/autodetectproxy_win.h \
           engine/firewall/firewallcontroller_win.h \
           utils/bfecontroller_win.h \
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
           $$COMMON_PATH/utils/winutils.h \
           $$COMMON_PATH/utils/executable_signature/executable_signature_win.h \
           engine/connectionmanager/finishactiveconnections.h


LIBS += Ws2_32.lib Advapi32.lib Iphlpapi.lib \
    Wininet.lib User32.lib Sensapi.lib Dnsapi.lib \
    Ole32.lib Shlwapi.lib Version.lib Psapi.lib \
    rasapi32.lib Pdh.lib Shell32.lib netapi32.lib msi.lib

INCLUDEPATH += C:/libs/boost/include/boost-1_67
LIBS += -L"C:/libs/boost/lib"

INCLUDEPATH += "C:/libs/curl/include"
LIBS += -L"C:/libs/curl/lib" -llibcurl

#DEFINES += "CARES_STATICLIB"
INCLUDEPATH += "C:/libs/cares/dll_x32/include"
LIBS += -L"C:/libs/cares/dll_x32/lib" -lcares

INCLUDEPATH += "C:/libs/openssl/include"
LIBS += -L"C:/libs/openssl/lib" -llibeay32 -lssleay32

# protobuf libs
CONFIG(release, debug|release){
    INCLUDEPATH += c:/libs/protobuf_release/include
    LIBS += -Lc:/libs/protobuf_release/lib -llibprotobuf
}
CONFIG(debug, debug|release){
    INCLUDEPATH += c:/libs/protobuf_debug/include
    LIBS += -Lc:/libs/protobuf_debug/lib -llibprotobufd
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
INCLUDEPATH += $$HOMEDIR/LibsWindscribe/boost/include
LIBS += $$HOMEDIR/LibsWindscribe/boost/lib/libboost_system.a
LIBS += $$HOMEDIR/LibsWindscribe/boost/lib/libboost_serialization.a

INCLUDEPATH += $$HOMEDIR/LibsWindscribe/openssl/include
LIBS+=-L$$HOMEDIR/LibsWindscribe/openssl/lib -lssl -lcrypto
INCLUDEPATH += $$HOMEDIR/LibsWindscribe/curl/include
LIBS += -L$$HOMEDIR/LibsWindscribe/curl/lib/ -lcurl

#protobuf include and libs
INCLUDEPATH += $$(HOME)/LibsWindscribe/protobuf/include
LIBS += -L$$(HOME)/LibsWindscribe/protobuf/lib -lprotobuf

#c-ares library
# don't forget remove *.dylib files for static link
INCLUDEPATH += $$HOMEDIR/LibsWindscribe/cares/include
LIBS += -L$$HOMEDIR/LibsWindscribe/cares/lib -lcares


SOURCES += engine/networkstatemanager/networkstatemanager_mac.cpp \
           engine/firewall/firewallcontroller_mac.cpp \
           engine/ipv6controller_mac.cpp \
           engine/connectionmanager/networkextensionlog_mac.cpp \
           engine/ping/pinghost_icmp_mac.cpp \
           engine/networkdetectionmanager/networkdetectionmanager_mac.cpp \
           engine/helper/helper_mac.cpp \
           engine/dnsresolver/dnsutils_mac.cpp \
           engine/macaddresscontroller/macaddresscontroller_mac.cpp

HEADERS +=     $$COMMON_PATH/utils/macutils.h \
               engine/connectionmanager/sleepevents_mac.h \
               engine/networkstatemanager/reachabilityevents.h \
               engine/connectionmanager/restorednsmanager_mac.h \
               engine/networkstatemanager/networkstatemanager_mac.h \
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
copy_helper.commands = $(COPY_DIR) $$PWD/../../../client-desktop-installer/Mac/binaries/helper/com.windscribe.helper.macos $$OUT_PWD/WindscribeEngine.app/Contents/Library/LaunchServices
copy_profile.commands = $(COPY_DIR) $$PWD/../mac/provisioning_profile/embedded.provisionprofile $$OUT_PWD/WindscribeEngine.app/Contents
mkdir_helpers.commands = $(MKDIR) $$OUT_PWD/WindscribeEngine.app/Contents/Helpers
copy_openvpn.commands = cp $$HOMEDIR/LibsWindscribe/openvpn_2_4_8/sbin/openvpn $$OUT_PWD/WindscribeEngine.app/Contents/Helpers/windscribeopenvpn_2_4_8
copy_stunnel.commands = cp $$HOMEDIR/LibsWindscribe/stunnel/bin/stunnel $$OUT_PWD/WindscribeEngine.app/Contents/Helpers/windscribestunnel
copy_wstunnel.commands = cp $$PWD/../Mac/wstunnel/windscribewstunnel $$OUT_PWD/WindscribeEngine.app/Contents/Helpers/windscribewstunnel
copy_kext.commands = $(COPY_DIR) $$PWD/../Mac/Kext/Binary/WindscribeKext.kext $$OUT_PWD/WindscribeEngine.app/Contents/Helpers/WindscribeKext.kext


first.depends = $(first) copy_resources mkdir_launch_services copy_helper copy_profile mkdir_helpers copy_openvpn copy_stunnel copy_wstunnel copy_kext
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
QMAKE_EXTRA_TARGETS += first copy_resources mkdir_launch_services copy_helper copy_profile mkdir_helpers copy_openvpn copy_stunnel copy_wstunnel copy_kext

} # end macx


SOURCES += main.cpp \
    $$COMMON_PATH/ipc/generated_proto/types.pb.cc \
    $$COMMON_PATH/utils/utils.cpp \
    $$COMMON_PATH/utils/logger.cpp \
    $$COMMON_PATH/utils/extraconfig.cpp \
    $$COMMON_PATH/version/appversion.cpp \
    $$COMMON_PATH/utils/executable_signature/executable_signature.cpp \
    application/windowsnativeeventfilter.cpp \
    application/windscribeapplication.cpp \
    engine/ping/keepalivemanager.cpp \
    engine/serversmodel/serversmodel.cpp \
    engine/serversmodel/pingmanager.cpp \
    engine/packetsizecontroller.cpp \
    engine/enginesettings.cpp \
    utils/mergelog.cpp \
    engine/tempscripts_mac.cpp \
    utils/simplecrypt.cpp \
    engine/logincontroller/logincontroller.cpp \
    engine/logincontroller/getallconfigscontroller.cpp \
    engine/types/apiinfo.cpp \
    engine/proxy/proxysettings.cpp \
    engine/types/portmap.cpp \
    engine/types/connectionsettings.cpp \
    engine/serversmodel/nodesspeedratings.cpp \
    engine/getmyipcontroller.cpp \
    engine/firewall/uniqueiplist.cpp \
    engine/firewall/firewallexceptions.cpp \
    engine/firewall/firewallcontroller.cpp \
    engine/proxy/proxyservercontroller.cpp \
    engine/serversmodel/failedpinglogcontroller.cpp \
    engine/types/dnsresolutionsettings.cpp \
    engine/connectionmanager/makeovpnfile.cpp \
    engine/connectionmanager/stunnelmanager.cpp \
    engine/connectionmanager/testvpntunnel.cpp \
    engine/connectionmanager/testvpntunnelhelper.cpp \
    engine/connectionmanager/openvpnconnection.cpp \
    engine/connectionmanager/automanualconnectioncontroller.cpp \
    engine/connectionmanager/connectionmanager.cpp \
    engine/connectionmanager/availableport.cpp \
    engine/types/locationid.cpp \
    engine/serversmodel/nodeselectionalgorithm.cpp \
    engine/serversmodel/nodesspeedstore.cpp \
    engine/logincontroller/getapiaccessips.cpp \
    engine/helper/initializehelper.cpp \
    engine/refetchservercredentialshelper.cpp \
    engine/curlinitcontroller.cpp \
    localhttpserver/localhttpserver.cpp \
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
    engine/vpnshare/vpnsharesettings.cpp \
    engine/vpnshare/vpnsharecontroller.cpp \
    engine/vpnshare/connecteduserscounter.cpp \
    engine/vpnshare/httpproxyserver/httpproxyreply.cpp \
    engine/vpnshare/socketutils/detectlocalip.cpp \
    engine/connectstatecontroller/connectstatecontroller.cpp \
    engine/openvpnversioncontroller.cpp \
    engine/types/types.cpp \
    engine/serverapi/curlnetworkmanager.cpp \
    engine/serverapi/curlrequest.cpp \
    engine/serverapi/dnscache.cpp \
    engine/serverapi/serverapi.cpp \
    engine/serverapi/certmanager.cpp \
    utils/ipvalidation.cpp \
    engine/hardcodedsettings.cpp \
    engine/engine.cpp \
    engine/crossplatformobjectfactory.cpp \
    engine/types/loginsettings.cpp \
    engine/types/servercredentials.cpp \
    engine/emergencycontroller/emergencycontroller.cpp \
    engine/dnsresolver/areslibraryinit.cpp \
    engine/dnsresolver/dnsresolver.cpp \
    engine/types/protocoltype.cpp \
    engine/detectlanrange.cpp \
    engine/connectionmanager/ikev2connection_test.cpp \
    engine/types/serverlocation.cpp \
    engine/types/servernode.cpp \
    engine/serversmodel/pingnodescontroller.cpp \
    engine/serversmodel/pinglog.cpp \
    engine/serversmodel/testpingnodecontroller.cpp \
    engine/serversmodel/mutablelocationinfo.cpp \
    engine/serverlocationsapiwrapper.cpp \
    engine/helper/simple_xor_crypt.cpp \
    engine/tests/sessionandlocations_test.cpp \
    engine/sessionstatustimer.cpp \
    engine/serversmodel/pingtime.cpp \
    engine/connectionmanager/wstunnelmanager.cpp \
    engine/customovpnconfigs/customovpnconfigs.cpp \
    engine/connectionmanager/makeovpnfilefromcustom.cpp \
    engine/customovpnconfigs/parseovpnconfigline.cpp \
    engine/customovpnconfigs/customovpnauthcredentialsstorage.cpp \
    engine/ping/pinghost_tcp.cpp \
    engine/ping/pinghost.cpp \
    engine/serversmodel/resolvehostnamesforcustomovpn.cpp \
    engine/customovpnconfigs/customconfigsdirwatcher.cpp \
    engine/types/staticipslocation.cpp \
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
    engine/splittunnelingnetworkinfo/splittunnelingnetworkinfo.cpp

HEADERS  += engine/serversmodel/serversmodel.h \
    $$COMMON_PATH/ipc/generated_proto/types.pb.h \
    $$COMMON_PATH/utils/utils.h \
    $$COMMON_PATH/utils/logger.h \
    $$COMMON_PATH/utils/extraconfig.h \
    $$COMMON_PATH/version/appversion.h \
    $$COMMON_PATH/version/windscribe_version.h \
    $$COMMON_PATH/utils/executable_signature/executable_signature.h \
    application/windowsnativeeventfilter.h \
    application/windscribeapplication.h \
    engine/connectionmanager/makeovpnfile.h \
    engine/dnsresolver/dnsutils.h \
    engine/macaddresscontroller/imacaddresscontroller.h \
    engine/networkdetectionmanager/inetworkdetectionmanager.h \
    $$COMMON_PATH/utils/networktypes.h \
    engine/ping/keepalivemanager.h \
    engine/serversmodel/pingmanager.h \
    engine/packetsizecontroller.h \
    engine/enginesettings.h \
    utils/mergelog.h \
    engine/connectionmanager/stunnelmanager.h \
    engine/tempscripts_mac.h \
    engine/connectionmanager/iconnectionmanager.h \
    engine/networkstatemanager/inetworkstatemanager.h \
    utils/simplecrypt.h \
    engine/logincontroller/logincontroller.h \
    engine/logincontroller/getallconfigscontroller.h \
    engine/types/apiinfo.h \
    engine/proxy/proxysettings.h \
    engine/types/portmap.h \
    engine/types/connectionsettings.h \
    engine/getmyipcontroller.h \
    engine/firewall/uniqueiplist.h \
    engine/helper/ihelper.h \
    engine/firewall/firewallcontroller.h \
    engine/firewall/firewallexceptions.h \
    engine/proxy/proxyservercontroller.h \
    engine/serversmodel/failedpinglogcontroller.h \
    engine/connectionmanager/testvpntunnel.h \
    engine/types/dnsresolutionsettings.h \
    engine/connectionmanager/iconnection.h \
    engine/connectionmanager/testvpntunnelhelper.h \
    engine/connectionmanager/openvpnconnection.h \
    engine/connectionmanager/isleepevents.h \
    utils/boost_includes.h \
    engine/types/types.h \
    engine/connectionmanager/automanualconnectioncontroller.h \
    engine/connectionmanager/connectionmanager.h \
    engine/types/locationid.h \
    engine/serversmodel/iserversmodel.h \
    engine/serversmodel/nodeselectionalgorithm.h \
    engine/logincontroller/getapiaccessips.h \
    engine/helper/initializehelper.h \
    engine/refetchservercredentialshelper.h \
    engine/curlinitcontroller.h \
    localhttpserver/localhttpserver.h \
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
    engine/vpnshare/vpnsharesettings.h \
    engine/vpnshare/vpnsharecontroller.h \
    engine/vpnshare/connecteduserscounter.h \
    engine/vpnshare/httpproxyserver/httpproxyreply.h \
    engine/vpnshare/socketutils/detectlocalip.h \
    engine/connectstatecontroller/iconnectstatecontroller.h \
    engine/connectstatecontroller/connectstatecontroller.h \
    engine/openvpnversioncontroller.h \
    engine/serverapi/curlnetworkmanager.h \
    engine/serverapi/curlrequest.h \
    engine/serverapi/dnscache.h \
    engine/serverapi/serverapi.h \
    engine/serverapi/certmanager.h \
    utils/ipvalidation.h \
    engine/hardcodedsettings.h \
    engine/engine.h \
    engine/crossplatformobjectfactory.h \
    engine/types/loginsettings.h \
    engine/types/servercredentials.h \
    engine/emergencycontroller/emergencycontroller.h \
    engine/dnsresolver/areslibraryinit.h \
    engine/dnsresolver/dnsresolver.h \
    engine/types/protocoltype.h \
    engine/detectlanrange.h \
    engine/connectionmanager/ikev2connection_test.h \
    engine/types/serverlocation.h \
    engine/types/servernode.h \
    engine/serversmodel/pingnodescontroller.h \
    engine/serversmodel/pinglog.h \
    engine/serversmodel/testpingnodecontroller.h \
    engine/serversmodel/nodesspeedratings.h \
    engine/serversmodel/mutablelocationinfo.h \
    engine/serversmodel/nodesspeedstore.h \
    engine/serverlocationsapiwrapper.h \
    engine/helper/simple_xor_crypt.h \
    engine/tests/sessionandlocations_test.h \
    engine/sessionstatustimer.h \
    engine/serversmodel/pingtime.h \
    engine/connectionmanager/wstunnelmanager.h \
    engine/customovpnconfigs/customovpnconfigs.h \
    engine/connectionmanager/makeovpnfilefromcustom.h \
    engine/customovpnconfigs/parseovpnconfigline.h \
    engine/customovpnconfigs/customovpnauthcredentialsstorage.h \
    engine/ping/icmp_header.h \
    engine/ping/ipv4_header.h \
    engine/ping/pinghost_tcp.h \
    engine/ping/pinghost.h \
    engine/serversmodel/resolvehostnamesforcustomovpn.h \
    engine/customovpnconfigs/customconfigsdirwatcher.h \
    engine/types/staticipslocation.h \
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
    engine/splittunnelingnetworkinfo/splittunnelingnetworkinfo.h

RESOURCES += \
    engine.qrc
