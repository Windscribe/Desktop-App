INCLUDEPATH += $$PWD

win32 {

SOURCES += $$PWD/engine/connectionmanager/adapterutils_win.cpp \
           $$PWD/engine/dnsinfo_win.cpp \
           $$PWD/engine/taputils/tapinstall_win.cpp \
           $$PWD/engine/helper/helper_win.cpp \
           $$PWD/engine/proxy/autodetectproxy_win.cpp \
           $$PWD/engine/firewall/firewallcontroller_win.cpp \
           $$PWD/utils/bfe_service_win.cpp \
           $$PWD/utils/ras_service_win.cpp \
           $$PWD/engine/dnsresolver/dnsutils_win.cpp \
           $$PWD/engine/adaptermetricscontroller_win.cpp \
           $$PWD/engine/connectionmanager/sleepevents_win.cpp \
           $$PWD/utils/installedantiviruses_win.cpp \
           $$PWD/engine/taputils/checkadapterenable.cpp \
           $$PWD/engine/vpnshare/wifisharing/wifisharing.cpp \
           $$PWD/engine/vpnshare/wifisharing/icsmanager.cpp \
           $$PWD/engine/vpnshare/wifisharing/wlanmanager.cpp \
           $$PWD/engine/helper/windscribeinstallhelper_win.cpp \
           $$PWD/engine/connectionmanager/ikev2connection_win.cpp \
           $$PWD/engine/measurementcpuusage.cpp \
           $$PWD/engine/ping/pinghost_icmp_win.cpp \
           $$PWD/engine/connectionmanager/ikev2connectiondisconnectlogic_win.cpp \
           $$PWD/engine/macaddresscontroller/macaddresscontroller_win.cpp \
           $$PWD/engine/networkdetectionmanager/networkdetectionmanager_win.cpp \
           $$PWD/engine/networkdetectionmanager/networkchangeworkerthread.cpp \
           $$COMMON_PATH/utils/crashdump.cpp \
           $$COMMON_PATH/utils/crashhandler.cpp \
           $$COMMON_PATH/utils/winutils.cpp \
           $$COMMON_PATH/utils/executable_signature/executable_signature_win.cpp


HEADERS += $$PWD/engine/connectionmanager/adapterutils_win.h \
           $$PWD/engine/dnsinfo_win.h \
           $$PWD/engine/taputils/tapinstall_win.h \
           $$PWD/engine/helper/helper_win.h \
           $$PWD/engine/proxy/autodetectproxy_win.h \
           $$PWD/engine/firewall/firewallcontroller_win.h \
           $$PWD/utils/bfe_service_win.h \
           $$PWD/utils/ras_service_win.h \
           $$PWD/engine/adaptermetricscontroller_win.h \
           $$PWD/engine/connectionmanager/sleepevents_win.h \
           $$PWD/utils/installedantiviruses_win.h \
           $$PWD/engine/taputils/checkadapterenable.h \
           $$PWD/engine/vpnshare/wifisharing/wifisharing.h \
           $$PWD/engine/vpnshare/wifisharing/icsmanager.h \
           $$PWD/engine/vpnshare/wifisharing/wlanmanager.h \
           $$PWD/engine/helper/windscribeinstallhelper_win.h \
           $$PWD/engine/connectionmanager/ikev2connection_win.h \
           $$PWD/engine/measurementcpuusage.h \
           $$PWD/engine/ping/pinghost_icmp_win.h \
           $$PWD/engine/connectionmanager/ikev2connectiondisconnectlogic_win.h \
           $$PWD/engine/macaddresscontroller/macaddresscontroller_win.h \
           $$PWD/engine/networkdetectionmanager/networkdetectionmanager_win.h \
           $$PWD/engine/networkdetectionmanager/networkchangeworkerthread.h \
           $$COMMON_PATH/utils/crashdump.h \
           $$COMMON_PATH/utils/crashhandler.h \
           $$COMMON_PATH/utils/winutils.h \
           $$COMMON_PATH/utils/executable_signature/executable_signature_win.h
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
QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter -Wno-deprecated-declarations

#boost include and libs
INCLUDEPATH += $$BUILD_LIBS_PATH/boost/include
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


SOURCES += engine/firewall/firewallcontroller_mac.cpp \
           engine/ipv6controller_mac.cpp \
           engine/connectionmanager/networkextensionlog_mac.cpp \
           engine/ping/pinghost_icmp_mac.cpp \
           engine/networkdetectionmanager/networkdetectionmanager_mac.cpp \
           engine/helper/helper_posix.cpp \
           engine/helper/helper_mac.cpp \
           engine/dnsresolver/dnsutils_mac.cpp \
           engine/macaddresscontroller/macaddresscontroller_mac.cpp \
           engine/autoupdater/autoupdaterhelper_mac.cpp

HEADERS +=     $$COMMON_PATH/utils/macutils.h \
               engine/connectionmanager/sleepevents_mac.h \
               engine/networkdetectionmanager/reachabilityevents.h \
               engine/connectionmanager/restorednsmanager_mac.h \
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
               $$COMMON_PATH/utils/executable_signature/executable_signature_mac.h \
               engine/autoupdater/autoupdaterhelper_mac.h

OBJECTIVE_HEADERS += \
               engine/networkdetectionmanager/reachability.h \
               $$COMMON_PATH/exithandler_mac.h

OBJECTIVE_SOURCES += $$COMMON_PATH/utils/macutils.mm \
                     engine/connectionmanager/sleepevents_mac.mm \
                     engine/networkdetectionmanager/reachability.m \
                     engine/networkdetectionmanager/reachabilityevents.mm \
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

# The referenced file doesn't exist... perhaps this is old debugging code?
#MY_ENTITLEMENTS.name = CODE_SIGN_ENTITLEMENTS
#MY_ENTITLEMENTS.value = $$PWD/mac/windscribe.entitlements
#QMAKE_MAC_XCODE_SETTINGS += MY_ENTITLEMENTS

#postbuild copy commands
copy_resources.commands = $(COPY_DIR) $$PWD/mac/resources $$OUT_PWD/WindscribeEngine.app/Contents
mkdir_launch_services.commands = $(MKDIR) $$OUT_PWD/WindscribeEngine.app/Contents/Library/LaunchServices
copy_helper.commands = $(COPY_DIR) $$PWD/../../installer/mac/binaries/helper/com.windscribe.helper.macos $$OUT_PWD/WindscribeEngine.app/Contents/Library/LaunchServices
mkdir_helpers.commands = $(MKDIR) $$OUT_PWD/WindscribeEngine.app/Contents/Helpers
copy_openvpn.commands = cp $$BUILD_LIBS_PATH/openvpn_2_5_4/openvpn $$OUT_PWD/WindscribeEngine.app/Contents/Helpers/windscribeopenvpn_2_5_4
copy_stunnel.commands = cp $$BUILD_LIBS_PATH/stunnel/stunnel $$OUT_PWD/WindscribeEngine.app/Contents/Helpers/windscribestunnel
copy_wstunnel.commands = cp $$PWD/../mac/wstunnel/windscribewstunnel $$OUT_PWD/WindscribeEngine.app/Contents/Helpers/windscribewstunnel
copy_kext.commands = $(COPY_DIR) $$PWD/../mac/kext/Binary/WindscribeKext.kext $$OUT_PWD/WindscribeEngine.app/Contents/Helpers/WindscribeKext.kext
copy_wireguard.commands = cp $$BUILD_LIBS_PATH/wireguard/windscribewireguard $$OUT_PWD/WindscribeEngine.app/Contents/Helpers/windscribewireguard

exists( $$PWD/../mac/provisioning_profile/embedded.provisionprofile ) {
    copy_profile.commands = $(COPY_DIR) $$PWD/../mac/provisioning_profile/embedded.provisionprofile $$OUT_PWD/WindscribeEngine.app/Contents
}

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
LIBS += $$BUILD_LIBS_PATH/boost/lib/libboost_serialization.a
LIBS += $$BUILD_LIBS_PATH/boost/lib/libboost_filesystem.a

INCLUDEPATH += $$BUILD_LIBS_PATH/openssl/include
LIBS +=-L$$BUILD_LIBS_PATH/openssl/lib -lssl -lcrypto
INCLUDEPATH += $$BUILD_LIBS_PATH/curl/include
LIBS += -L$$BUILD_LIBS_PATH/curl/lib/ -lcurl

#protobuf include and libs
INCLUDEPATH += $$BUILD_LIBS_PATH/protobuf/include
LIBS += -L$$BUILD_LIBS_PATH/protobuf/lib -lprotobuf

#c-ares library
INCLUDEPATH += $$BUILD_LIBS_PATH/cares/include
LIBS += -L$$BUILD_LIBS_PATH/cares/lib -lcares

SOURCES += \
           $$COMMON_PATH/utils/executable_signature/executablesignature_linux.cpp \
           $$COMMON_PATH/utils/linuxutils.cpp \
           utils/dnsscripts_linux.cpp \
           engine/ping/pinghost_icmp_mac.cpp \
           engine/dnsresolver/dnsutils_linux.cpp \
           engine/helper/helper_posix.cpp \
           engine/helper/helper_linux.cpp \
           engine/firewall/firewallcontroller_linux.cpp \
           engine/connectionmanager/ikev2connection_linux.cpp \
           engine/networkdetectionmanager/networkdetectionmanager_linux.cpp \
           engine/macaddresscontroller/macaddresscontroller_linux.cpp

HEADERS += \
           $$COMMON_PATH/utils/executable_signature/executablesignature_linux.h \
           $$COMMON_PATH/utils/linuxutils.h \
           utils/dnsscripts_linux.h \
           engine/ping/pinghost_icmp_mac.h \
           engine/helper/helper_posix.h \
           engine/helper/helper_linux.h \
           engine/firewall/firewallcontroller_linux.h \
           engine/connectionmanager/ikev2connection_linux.h \
           engine/networkdetectionmanager/networkdetectionmanager_linux.h \
           engine/macaddresscontroller/macaddresscontroller_linux.h

} # linux




SOURCES += $$COMMON_PATH/ipc/generated_proto/types.pb.cc \
    $$COMMON_PATH/ipc/generated_proto/apiinfo.pb.cc \
    $$COMMON_PATH/utils/clean_sensitive_info.cpp \
    $$COMMON_PATH/utils/utils.cpp \
    $$COMMON_PATH/utils/logger.cpp \
    $$COMMON_PATH/utils/mergelog.cpp \
    $$COMMON_PATH/utils/extraconfig.cpp \
    $$COMMON_PATH/utils/ipvalidation.cpp \
    $$COMMON_PATH/version/appversion.cpp \
    $$COMMON_PATH/utils/executable_signature/executable_signature.cpp \
    $$PWD/engine/apiinfo/apiinfo.cpp \
    $$PWD/engine/apiinfo/checkupdate.cpp \
    $$PWD/engine/apiinfo/sessionstatus.cpp \
    $$PWD/engine/apiinfo/location.cpp \
    $$PWD/engine/apiinfo/group.cpp \
    $$PWD/engine/apiinfo/node.cpp \
    $$PWD/engine/apiinfo/notification.cpp \
    $$PWD/engine/apiinfo/portmap.cpp \
    $$PWD/engine/apiinfo/staticips.cpp \
    $$PWD/engine/apiinfo/servercredentials.cpp \
    $$PWD/engine/autoupdater/downloadhelper.cpp \
    $$PWD/engine/ping/keepalivemanager.cpp \
    $$PWD/engine/locationsmodel/locationsmodel.cpp \
    $$PWD/engine/locationsmodel/apilocationsmodel.cpp \
    $$PWD/engine/locationsmodel/customconfiglocationsmodel.cpp \
    $$PWD/engine/locationsmodel/locationitem.cpp \
    $$PWD/engine/locationsmodel/pingipscontroller.cpp \
    $$PWD/engine/locationsmodel/pingstorage.cpp \
    $$PWD/engine/locationsmodel/bestlocation.cpp \
    $$PWD/engine/locationsmodel/baselocationinfo.cpp \
    $$PWD/engine/locationsmodel/mutablelocationinfo.cpp \
    $$PWD/engine/locationsmodel/customconfiglocationinfo.cpp \
    $$COMMON_PATH/types/pingtime.cpp \
    $$PWD/engine/locationsmodel/pinglog.cpp \
    $$PWD/engine/locationsmodel/failedpinglogcontroller.cpp \
    $$PWD/engine/locationsmodel/nodeselectionalgorithm.cpp \
    $$PWD/engine/packetsizecontroller.cpp \
    $$PWD/engine/enginesettings.cpp \
    $$PWD/engine/tempscripts_mac.cpp \
    $$COMMON_PATH/utils/simplecrypt.cpp \
    $$PWD/engine/logincontroller/logincontroller.cpp \
    $$PWD/engine/logincontroller/getallconfigscontroller.cpp \
    $$PWD/engine/proxy/proxysettings.cpp \
    $$PWD/engine/types/connectionsettings.cpp \
    $$PWD/engine/getmyipcontroller.cpp \
    $$PWD/engine/firewall/uniqueiplist.cpp \
    $$PWD/engine/firewall/firewallexceptions.cpp \
    $$PWD/engine/firewall/firewallcontroller.cpp \
    $$PWD/engine/proxy/proxyservercontroller.cpp \
    $$PWD/engine/types/dnsresolutionsettings.cpp \
    $$PWD/engine/connectionmanager/makeovpnfile.cpp \
    $$PWD/engine/connectionmanager/adaptergatewayinfo.cpp \
    $$PWD/engine/connectionmanager/stunnelmanager.cpp \
    $$PWD/engine/connectionmanager/testvpntunnel.cpp \
    $$PWD/engine/connectionmanager/openvpnconnection.cpp \
    $$PWD/engine/connectionmanager/connsettingspolicy/autoconnsettingspolicy.cpp \
    $$PWD/engine/connectionmanager/connsettingspolicy/manualconnsettingspolicy.cpp \
    $$PWD/engine/connectionmanager/connsettingspolicy/customconfigconnsettingspolicy.cpp \
    $$PWD/engine/connectionmanager/connectionmanager.cpp \
    $$PWD/engine/connectionmanager/availableport.cpp \
    $$PWD/engine/connectionmanager/wireguardconnection.cpp \
    $$PWD/engine/macaddresscontroller/imacaddresscontroller.cpp \
    $$COMMON_PATH/types/locationid.cpp \
    $$PWD/engine/logincontroller/getapiaccessips.cpp \
    $$PWD/engine/helper/initializehelper.cpp \
    $$PWD/engine/refetchservercredentialshelper.cpp \
    $$PWD/engine/vpnshare/httpproxyserver/httpproxyserver.cpp \
    $$PWD/engine/vpnshare/httpproxyserver/httpproxyconnectionmanager.cpp \
    $$PWD/engine/vpnshare/httpproxyserver/httpproxyconnection.cpp \
    $$PWD/engine/vpnshare/httpproxyserver/httpproxyrequestparser.cpp \
    $$PWD/engine/vpnshare/httpproxyserver/httpproxyrequest.cpp \
    $$PWD/engine/vpnshare/httpproxyserver/httpproxywebanswer.cpp \
    $$PWD/engine/vpnshare/httpproxyserver/httpproxywebanswerparser.cpp \
    $$PWD/engine/vpnshare/socksproxyserver/socksproxyserver.cpp \
    $$PWD/engine/vpnshare/socksproxyserver/socksproxyconnectionmanager.cpp \
    $$PWD/engine/vpnshare/socksproxyserver/socksproxyconnection.cpp \
    $$PWD/engine/vpnshare/socksproxyserver/socksproxyreadexactly.cpp \
    $$PWD/engine/vpnshare/socksproxyserver/socksproxyidentreqparser.cpp \
    $$PWD/engine/vpnshare/socketutils/socketwriteall.cpp \
    $$PWD/engine/vpnshare/socksproxyserver/socksproxycommandparser.cpp \
    $$PWD/engine/vpnshare/vpnsharecontroller.cpp \
    $$PWD/engine/vpnshare/connecteduserscounter.cpp \
    $$PWD/engine/vpnshare/httpproxyserver/httpproxyreply.cpp \
    $$PWD/engine/connectstatecontroller/connectstatecontroller.cpp \
    $$PWD/engine/openvpnversioncontroller.cpp \
    $$PWD/engine/types/types.cpp \
    $$PWD/engine/serverapi/curlnetworkmanager.cpp \
    $$PWD/engine/serverapi/curlrequest.cpp \
    $$PWD/engine/serverapi/dnscache.cpp \
    $$PWD/engine/serverapi/serverapi.cpp \
    $$COMMON_PATH/utils/hardcodedsettings.cpp \
    $$PWD/engine/engine.cpp \
    $$PWD/engine/crossplatformobjectfactory.cpp \
    $$PWD/engine/types/loginsettings.cpp \
    $$PWD/engine/emergencycontroller/emergencycontroller.cpp \
    $$PWD/engine/dnsresolver/areslibraryinit.cpp \
    $$PWD/engine/dnsresolver/dnsrequest.cpp \
    $$PWD/engine/dnsresolver/dnsserversconfiguration.cpp \
    $$PWD/engine/dnsresolver/dnsresolver.cpp \
    $$PWD/engine/types/protocoltype.cpp \
    $$PWD/engine/tests/sessionandlocations_test.cpp \
    $$PWD/engine/sessionstatustimer.cpp \
    $$PWD/engine/connectionmanager/wstunnelmanager.cpp \
    $$PWD/engine/customconfigs/customconfigs.cpp \
    $$PWD/engine/customconfigs/customconfigtype.cpp \
    $$PWD/engine/customconfigs/ovpncustomconfig.cpp \
    $$PWD/engine/customconfigs/wireguardcustomconfig.cpp \
    $$PWD/engine/connectionmanager/makeovpnfilefromcustom.cpp \
    $$PWD/engine/customconfigs/parseovpnconfigline.cpp \
    $$PWD/engine/customconfigs/customovpnauthcredentialsstorage.cpp \
    $$PWD/engine/ping/pinghost_tcp.cpp \
    $$PWD/engine/ping/pinghost.cpp \
    $$PWD/engine/customconfigs/customconfigsdirwatcher.cpp \
    $$PWD/engine/types/wireguardconfig.cpp \
    $$PWD/engine/getdeviceid.cpp \
    $$PWD/engineserver.cpp \
    $$COMMON_PATH/ipc/commandfactory.cpp \
    $$COMMON_PATH/ipc/connection.cpp \
    $$COMMON_PATH/ipc/server.cpp \
    $$COMMON_PATH/ipc/generated_proto/clientcommands.pb.cc \
    $$COMMON_PATH/ipc/generated_proto/servercommands.pb.cc \
    $$PWD/clientconnectiondescr.cpp \
    $$COMMON_PATH/ipc/tcpconnection.cpp \
    $$COMMON_PATH/ipc/tcpserver.cpp \
    $$PWD/engine/connectionmanager/finishactiveconnections.cpp \
    $$PWD/engine/networkaccessmanager/certmanager.cpp \
    $$PWD/engine/networkaccessmanager/curlinitcontroller.cpp \
    $$PWD/engine/networkaccessmanager/curlnetworkmanager2.cpp \
    $$PWD/engine/networkaccessmanager/curlreply.cpp \
    $$PWD/engine/networkaccessmanager/networkrequest.cpp \
    $$PWD/engine/networkaccessmanager/dnscache2.cpp \
    $$PWD/engine/networkaccessmanager/networkaccessmanager.cpp

HEADERS  +=  $$PWD/engine/locationsmodel/locationsmodel.h \
    $$PWD/engine/apiinfo/checkupdate.h \
    $$PWD/engine/locationsmodel/apilocationsmodel.h \
    $$PWD/engine/locationsmodel/customconfiglocationsmodel.h \
    $$PWD/engine/locationsmodel/locationitem.h \
    $$PWD/engine/locationsmodel/pingipscontroller.h \
    $$PWD/engine/locationsmodel/pingstorage.h \
    $$PWD/engine/locationsmodel/bestlocation.h \
    $$PWD/engine/locationsmodel/locationnode.h \
    $$PWD/engine/locationsmodel/baselocationinfo.h \
    $$PWD/engine/locationsmodel/mutablelocationinfo.h \
    $$PWD/engine/locationsmodel/customconfiglocationinfo.h \
    $$COMMON_PATH/types/pingtime.h \
    $$PWD/engine/locationsmodel/pinglog.h \
    $$PWD/engine/locationsmodel/failedpinglogcontroller.h \
    $$PWD/engine/locationsmodel/nodeselectionalgorithm.h \
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
    $$COMMON_PATH/names.h \
    $$PWD/engine/apiinfo/apiinfo.h \
    $$PWD/engine/apiinfo/sessionstatus.h \
    $$PWD/engine/apiinfo/location.h \
    $$PWD/engine/apiinfo/group.h \
    $$PWD/engine/apiinfo/node.h \
    $$PWD/engine/apiinfo/notification.h \
    $$PWD/engine/apiinfo/portmap.h \
    $$PWD/engine/apiinfo/staticips.h \
    $$PWD/engine/apiinfo/servercredentials.h \
    $$PWD/engine/connectionmanager/adaptergatewayinfo.h \
    $$PWD/engine/connectionmanager/makeovpnfile.h \
    $$PWD/engine/autoupdater/downloadhelper.h \
    $$PWD/engine/macaddresscontroller/imacaddresscontroller.h \
    $$PWD/engine/networkdetectionmanager/inetworkdetectionmanager.h \
    $$COMMON_PATH/utils/networktypes.h \
    $$PWD/engine/ping/keepalivemanager.h \
    $$PWD/engine/packetsizecontroller.h \
    $$PWD/engine/enginesettings.h \
    $$PWD/engine/connectionmanager/stunnelmanager.h \
    $$PWD/engine/tempscripts_mac.h \
    $$COMMON_PATH/utils/simplecrypt.h \
    $$PWD/engine/logincontroller/logincontroller.h \
    $$PWD/engine/logincontroller/getallconfigscontroller.h \
    $$PWD/engine/proxy/proxysettings.h \
    $$PWD/engine/types/connectionsettings.h \
    $$PWD/engine/getmyipcontroller.h \
    $$PWD/engine/firewall/uniqueiplist.h \
    $$PWD/engine/helper/ihelper.h \
    $$PWD/engine/firewall/firewallcontroller.h \
    $$PWD/engine/firewall/firewallexceptions.h \
    $$PWD/engine/proxy/proxyservercontroller.h \
    $$PWD/engine/connectionmanager/testvpntunnel.h \
    $$PWD/engine/types/dnsresolutionsettings.h \
    $$PWD/engine/connectionmanager/iconnection.h \
    $$PWD/engine/connectionmanager/openvpnconnection.h \
    $$PWD/engine/connectionmanager/isleepevents.h \
    $$PWD/engine/connectionmanager/wireguardconnection.h \
    $$PWD/utils/boost_includes.h \
    $$PWD/engine/types/types.h \
    $$PWD/engine/connectionmanager/connsettingspolicy/baseconnsettingspolicy.h \
    $$PWD/engine/connectionmanager/connsettingspolicy/autoconnsettingspolicy.h \
    $$PWD/engine/connectionmanager/connsettingspolicy/manualconnsettingspolicy.h \
    $$PWD/engine/connectionmanager/connsettingspolicy/customconfigconnsettingspolicy.h \
    $$PWD/engine/connectionmanager/connectionmanager.h \
    $$COMMON_PATH/types/locationid.h \
    $$PWD/engine/logincontroller/getapiaccessips.h \
    $$PWD/engine/helper/initializehelper.h \
    $$PWD/engine/refetchservercredentialshelper.h \
    $$PWD/engine/connectionmanager/availableport.h \
    $$PWD/engine/vpnshare/httpproxyserver/httpproxyserver.h \
    $$PWD/engine/vpnshare/httpproxyserver/httpproxyconnectionmanager.h \
    $$PWD/engine/vpnshare/httpproxyserver/httpproxyconnection.h \
    $$PWD/engine/vpnshare/httpproxyserver/httpproxyrequestparser.h \
    $$PWD/engine/vpnshare/httpproxyserver/httpproxyrequest.h \
    $$PWD/engine/vpnshare/httpproxyserver/httpproxyheader.h \
    $$PWD/engine/vpnshare/httpproxyserver/httpproxywebanswer.h \
    $$PWD/engine/vpnshare/httpproxyserver/httpproxywebanswerparser.h \
    $$PWD/engine/vpnshare/socksproxyserver/socksproxyserver.h \
    $$PWD/engine/vpnshare/socksproxyserver/socksproxyconnectionmanager.h \
    $$PWD/engine/vpnshare/socksproxyserver/socksproxyconnection.h \
    $$PWD/engine/vpnshare/socksproxyserver/socksstructs.h \
    $$PWD/engine/vpnshare/socksproxyserver/socksproxyreadexactly.h \
    $$PWD/engine/vpnshare/socksproxyserver/socksproxyidentreqparser.h \
    $$PWD/engine/vpnshare/socketutils/socketwriteall.h \
    $$PWD/engine/vpnshare/socksproxyserver/socksproxycommandparser.h \
    $$PWD/engine/vpnshare/vpnsharecontroller.h \
    $$PWD/engine/vpnshare/connecteduserscounter.h \
    $$PWD/engine/vpnshare/httpproxyserver/httpproxyreply.h \
    $$PWD/engine/connectstatecontroller/iconnectstatecontroller.h \
    $$PWD/engine/connectstatecontroller/connectstatecontroller.h \
    $$PWD/engine/openvpnversioncontroller.h \
    $$PWD/engine/serverapi/curlnetworkmanager.h \
    $$PWD/engine/serverapi/curlrequest.h \
    $$PWD/engine/serverapi/dnscache.h \
    $$PWD/engine/serverapi/serverapi.h \
    $$COMMON_PATH/utils/hardcodedsettings.h \
    $$PWD/engine/engine.h \
    $$PWD/engine/crossplatformobjectfactory.h \
    $$PWD/engine/types/loginsettings.h \
    $$PWD/engine/emergencycontroller/emergencycontroller.h \
    $$PWD/engine/dnsresolver/areslibraryinit.h \
    $$PWD/engine/dnsresolver/dnsutils.h \
    $$PWD/engine/dnsresolver/dnsrequest.h \
    $$PWD/engine/dnsresolver/dnsserversconfiguration.h \
    $$PWD/engine/dnsresolver/dnsresolver.h \
    $$PWD/engine/types/protocoltype.h \
    $$PWD/engine/tests/sessionandlocations_test.h \
    $$PWD/engine/sessionstatustimer.h \
    $$PWD/engine/connectionmanager/wstunnelmanager.h \
    $$PWD/engine/customconfigs/icustomconfig.h \
    $$PWD/engine/customconfigs/customconfigtype.h \
    $$PWD/engine/customconfigs/ovpncustomconfig.h \
    $$PWD/engine/customconfigs/wireguardcustomconfig.h \
    $$PWD/engine/customconfigs/customconfigs.h \
    $$PWD/engine/connectionmanager/makeovpnfilefromcustom.h \
    $$PWD/engine/customconfigs/parseovpnconfigline.h \
    $$PWD/engine/customconfigs/customovpnauthcredentialsstorage.h \
    $$PWD/engine/ping/icmp_header.h \
    $$PWD/engine/ping/ipv4_header.h \
    $$PWD/engine/ping/pinghost_tcp.h \
    $$PWD/engine/ping/pinghost.h \
    $$PWD/engine/customconfigs/customconfigsdirwatcher.h \
    $$PWD/engine/types/wireguardconfig.h \
    $$PWD/engine/getdeviceid.h \
    $$PWD/engineserver.h \
    $$COMMON_PATH/ipc/command.h \
    $$COMMON_PATH/ipc/commandfactory.h \
    $$COMMON_PATH/ipc/connection.h \
    $$COMMON_PATH/ipc/iconnection.h \
    $$COMMON_PATH/ipc/iserver.h \
    $$COMMON_PATH/ipc/protobufcommand.h \
    $$COMMON_PATH/ipc/server.h \
    $$COMMON_PATH/ipc/generated_proto/clientcommands.pb.h \
    $$COMMON_PATH/ipc/generated_proto/servercommands.pb.h \
    $$PWD/clientconnectiondescr.h \
    $$COMMON_PATH/ipc/tcpconnection.h \
    $$COMMON_PATH/ipc/tcpserver.h \
    $$PWD/engine/connectionmanager/finishactiveconnections.h \
    $$PWD/engine/networkaccessmanager/certmanager.h \
    $$PWD/engine/networkaccessmanager/curlinitcontroller.h \
    $$PWD/engine/networkaccessmanager/curlnetworkmanager2.h \
    $$PWD/engine/networkaccessmanager/curlreply.h \
    $$PWD/engine/networkaccessmanager/networkrequest.h \
    $$PWD/engine/networkaccessmanager/dnscache2.h \
    $$PWD/engine/networkaccessmanager/networkaccessmanager.h

RESOURCES += \
    $$PWD/engine.qrc
