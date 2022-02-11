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
           $$PWD/engine/connectionmanager/wireguardconnection_win.cpp \
           $$PWD/engine/connectionmanager/wireguardringlogger.cpp

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
           $$PWD/engine/connectionmanager/wireguardconnection_win.h \
           $$PWD/engine/connectionmanager/wireguardringlogger.h

} #end win32

macx {

SOURCES += $$PWD/engine/firewall/firewallcontroller_mac.cpp \
           $$PWD/engine/ipv6controller_mac.cpp \
           $$PWD/engine/connectionmanager/networkextensionlog_mac.cpp \
           $$PWD/engine/ping/pinghost_icmp_mac.cpp \
           $$PWD/engine/networkdetectionmanager/networkdetectionmanager_mac.cpp \
           $$PWD/engine/helper/helper_posix.cpp \
           $$PWD/engine/helper/helper_mac.cpp \
           $$PWD/engine/dnsresolver/dnsutils_mac.cpp \
           $$PWD/engine/macaddresscontroller/macaddresscontroller_mac.cpp \
           $$PWD/engine/autoupdater/autoupdaterhelper_mac.cpp \
           $$PWD/engine/connectionmanager/wireguardconnection_posix.cpp

HEADERS +=     $$PWD/engine/connectionmanager/sleepevents_mac.h \
               $$PWD/engine/networkdetectionmanager/reachabilityevents.h \
               $$PWD/engine/connectionmanager/restorednsmanager_mac.h \
               $$PWD/engine/helper/helper_posix.h \
               $$PWD/engine/helper/helper_mac.h \
               $$PWD/engine/helper/installhelper_mac.h \
               $$PWD/engine/proxy/pmachelpers.h \
               $$PWD/engine/proxy/autodetectproxy_mac.h \
               $$PWD/engine/firewall/firewallcontroller_mac.h \
               $$PWD/engine/ipv6controller_mac.h \
               $$PWD/engine/connectionmanager/ikev2connection_mac.h \
               $$PWD/engine/connectionmanager/networkextensionlog_mac.h \
               $$PWD/engine/ping/pinghost_icmp_mac.h \
               $$PWD/engine/networkdetectionmanager/networkdetectionmanager_mac.h \
               $$PWD/engine/macaddresscontroller/macaddresscontroller_mac.h \
               $$PWD/engine/autoupdater/autoupdaterhelper_mac.h \
               $$PWD/engine/connectionmanager/wireguardconnection_posix.h

OBJECTIVE_HEADERS += \
               $$PWD/engine/networkdetectionmanager/reachability.h

OBJECTIVE_SOURCES += $$PWD/engine/connectionmanager/sleepevents_mac.mm \
                     $$PWD/engine/networkdetectionmanager/reachability.m \
                     $$PWD/engine/networkdetectionmanager/reachabilityevents.mm \
                     $$PWD/engine/connectionmanager/restorednsmanager_mac.mm \
                     $$PWD/engine/helper/installhelper_mac.mm \
                     $$PWD/engine/proxy/pmachelpers.mm \
                     $$PWD/engine/proxy/autodetectproxy_mac.mm \
                     $$PWD/engine/connectionmanager/ikev2connection_mac.mm

} # end macx


linux {

SOURCES += \
           $$PWD/utils/dnsscripts_linux.cpp \
           $$PWD/engine/ping/pinghost_icmp_mac.cpp \
           $$PWD/engine/dnsresolver/dnsutils_linux.cpp \
           $$PWD/engine/helper/helper_posix.cpp \
           $$PWD/engine/helper/helper_linux.cpp \
           $$PWD/engine/firewall/firewallcontroller_linux.cpp \
           $$PWD/engine/connectionmanager/ikev2connection_linux.cpp \
           $$PWD/engine/networkdetectionmanager/networkdetectionmanager_linux.cpp \
           $$PWD/engine/macaddresscontroller/macaddresscontroller_linux.cpp \
           $$PWD/engine/connectionmanager/wireguardconnection_posix.cpp

HEADERS += \
           $$PWD/utils/dnsscripts_linux.h \
           $$PWD/engine/ping/pinghost_icmp_mac.h \
           $$PWD/engine/helper/helper_posix.h \
           $$PWD/engine/helper/helper_linux.h \
           $$PWD/engine/firewall/firewallcontroller_linux.h \
           $$PWD/engine/connectionmanager/ikev2connection_linux.h \
           $$PWD/engine/networkdetectionmanager/networkdetectionmanager_linux.h \
           $$PWD/engine/macaddresscontroller/macaddresscontroller_linux.h \
           $$PWD/engine/connectionmanager/wireguardconnection_posix.h

} # linux




SOURCES += $$PWD/engine/apiinfo/apiinfo.cpp \
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
    $$PWD/engine/locationsmodel/enginelocationsmodel.cpp \
    $$PWD/engine/locationsmodel/apilocationsmodel.cpp \
    $$PWD/engine/locationsmodel/customconfiglocationsmodel.cpp \
    $$PWD/engine/locationsmodel/locationitem.cpp \
    $$PWD/engine/locationsmodel/pingipscontroller.cpp \
    $$PWD/engine/locationsmodel/pingstorage.cpp \
    $$PWD/engine/locationsmodel/bestlocation.cpp \
    $$PWD/engine/locationsmodel/baselocationinfo.cpp \
    $$PWD/engine/locationsmodel/mutablelocationinfo.cpp \
    $$PWD/engine/locationsmodel/customconfiglocationinfo.cpp \
    $$PWD/engine/locationsmodel/pinglog.cpp \
    $$PWD/engine/locationsmodel/failedpinglogcontroller.cpp \
    $$PWD/engine/locationsmodel/nodeselectionalgorithm.cpp \
    $$PWD/engine/packetsizecontroller.cpp \
    $$PWD/engine/enginesettings.cpp \
    $$PWD/engine/tempscripts_mac.cpp \
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
    $$PWD/engine/macaddresscontroller/imacaddresscontroller.cpp \
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
    $$PWD/clientconnectiondescr.cpp \
    $$PWD/engine/connectionmanager/finishactiveconnections.cpp \
    $$PWD/engine/networkaccessmanager/certmanager.cpp \
    $$PWD/engine/networkaccessmanager/curlinitcontroller.cpp \
    $$PWD/engine/networkaccessmanager/curlnetworkmanager2.cpp \
    $$PWD/engine/networkaccessmanager/curlreply.cpp \
    $$PWD/engine/networkaccessmanager/networkrequest.cpp \
    $$PWD/engine/networkaccessmanager/dnscache2.cpp \
    $$PWD/engine/networkaccessmanager/networkaccessmanager.cpp

HEADERS  +=  $$PWD/engine/locationsmodel/enginelocationsmodel.h \
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
    $$PWD/engine/locationsmodel/pinglog.h \
    $$PWD/engine/locationsmodel/failedpinglogcontroller.h \
    $$PWD/engine/locationsmodel/nodeselectionalgorithm.h \
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
    $$PWD/engine/ping/keepalivemanager.h \
    $$PWD/engine/packetsizecontroller.h \
    $$PWD/engine/enginesettings.h \
    $$PWD/engine/connectionmanager/stunnelmanager.h \
    $$PWD/engine/tempscripts_mac.h \
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
    $$PWD/utils/boost_includes.h \
    $$PWD/engine/types/types.h \
    $$PWD/engine/connectionmanager/connsettingspolicy/baseconnsettingspolicy.h \
    $$PWD/engine/connectionmanager/connsettingspolicy/autoconnsettingspolicy.h \
    $$PWD/engine/connectionmanager/connsettingspolicy/manualconnsettingspolicy.h \
    $$PWD/engine/connectionmanager/connsettingspolicy/customconfigconnsettingspolicy.h \
    $$PWD/engine/connectionmanager/connectionmanager.h \
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
    $$PWD/clientconnectiondescr.h \
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
