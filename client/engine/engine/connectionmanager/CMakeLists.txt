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
endif (WIN32)

#todo linux and mac
