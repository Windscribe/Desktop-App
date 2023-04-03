target_sources(engine PRIVATE
    adaptergatewayinfo.cpp
    adaptergatewayinfo.h
    availableport.cpp
    availableport.h
    connectionmanager.cpp
    connectionmanager.h
    connsettingspolicy/autoconnsettingspolicy.cpp
    connsettingspolicy/autoconnsettingspolicy.h
    connsettingspolicy/baseconnsettingspolicy.h
    connsettingspolicy/customconfigconnsettingspolicy.cpp
    connsettingspolicy/customconfigconnsettingspolicy.h
    connsettingspolicy/manualconnsettingspolicy.cpp
    connsettingspolicy/manualconnsettingspolicy.h
    ctrldmanager.cpp
    ctrldmanager.h
    finishactiveconnections.cpp
    finishactiveconnections.h
    iconnection.h
    isleepevents.h
    makeovpnfile.cpp
    makeovpnfile.h
    makeovpnfilefromcustom.cpp
    makeovpnfilefromcustom.h
    openvpnconnection.cpp
    openvpnconnection.h
    stunnelmanager.cpp
    stunnelmanager.h
    testvpntunnel.cpp
    testvpntunnel.h
    wstunnelmanager.cpp
    wstunnelmanager.h
)

if (WIN32)
    target_sources(engine PRIVATE
        adapterutils_win.cpp
        adapterutils_win.h
        ikev2connection_win.cpp
        ikev2connection_win.h
        ikev2connectiondisconnectlogic_win.cpp
        ikev2connectiondisconnectlogic_win.h
        sleepevents_win.cpp
        sleepevents_win.h
        wireguardconnection_win.cpp
        wireguardconnection_win.h
        wireguardringlogger.cpp
        wireguardringlogger.h
    )
elseif(APPLE)
    target_sources(engine PRIVATE
        ikev2connection_mac.h
        ikev2connection_mac.mm
        networkextensionlog_mac.cpp
        networkextensionlog_mac.h
        restorednsmanager_mac.h
        restorednsmanager_mac.mm
        sleepevents_mac.h
        sleepevents_mac.mm
        wireguardconnection_posix.cpp
        wireguardconnection_posix.h
    )
elseif(UNIX)
    target_sources(engine PRIVATE
        ikev2connection_linux.cpp
        ikev2connection_linux.h
        wireguardconnection_posix.cpp
        wireguardconnection_posix.h
    )
endif()

