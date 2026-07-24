target_sources(engine PRIVATE
    classifyconnecterror.cpp
    classifyconnecterror.h
    connectionmanager.cpp
    connectionmanager.h
    connectrequest.h
    connectors/connectionfactory.cpp
    connectors/connectionfactory.h
    connectors/connectionplatformpolicy.cpp
    connectors/connectionplatformpolicy.h
    connectors/iconnection.h
    connectors/iconnectionfactory.h
    connectors/iconnectionplatformpolicy.h
    connectors/ikev2/ikev2connectionbase.cpp
    connectors/ikev2/ikev2connectionbase.h
    connectors/ikev2/ikev2sessionparams.h
    connectors/openvpn/availableport.cpp
    connectors/openvpn/availableport.h
    connectors/openvpn/makeovpnfile.cpp
    connectors/openvpn/makeovpnfile.h
    connectors/openvpn/openvpnconnection.cpp
    connectors/openvpn/openvpnconnection.h
    connectors/openvpn/openvpnsessionparams.h
    connectors/openvpn/stunnelmanager.cpp
    connectors/openvpn/stunnelmanager.h
    connectors/openvpn/wstunnelmanager.cpp
    connectors/openvpn/wstunnelmanager.h
    connectors/wireguard/wireguardconnectionbase.cpp
    connectors/wireguard/wireguardconnectionbase.h
    connectors/wireguard/wireguardsessionparams.h
    attemptstrategy/autoconnectionattemptstrategy.cpp
    attemptstrategy/autoconnectionattemptstrategy.h
    attemptstrategy/iconnectionattemptstrategy.cpp
    attemptstrategy/iconnectionattemptstrategy.h
    attemptstrategy/connectionattemptstrategyfactory.cpp
    attemptstrategy/connectionattemptstrategyfactory.h
    attemptstrategy/customconfigconnectionattemptstrategy.cpp
    attemptstrategy/customconfigconnectionattemptstrategy.h
    attemptstrategy/emergencyconnectionattemptstrategy.cpp
    attemptstrategy/emergencyconnectionattemptstrategy.h
    attemptstrategy/iconnectionattemptstrategyfactory.h
    attemptstrategy/manualconnectionattemptstrategy.cpp
    attemptstrategy/manualconnectionattemptstrategy.h
    isleepevents.h
)

if (WIN32)
    target_sources(engine PRIVATE
        connectors/ikev2/ikev2connection_win.cpp
        connectors/ikev2/ikev2connection_win.h
        connectors/wireguard/wireguardconnection_win.cpp
        connectors/wireguard/wireguardconnection_win.h
        connectors/wireguard/wireguardringlogger.cpp
        connectors/wireguard/wireguardringlogger.h
        sleepevents_win.cpp
        sleepevents_win.h
    )
elseif(APPLE)
    target_sources(engine PRIVATE
        connectors/ikev2/ikev2connection_mac.h
        connectors/ikev2/ikev2connection_mac.mm
        connectors/ikev2/networkextensionlog_mac.cpp
        connectors/ikev2/networkextensionlog_mac.h
        connectors/wireguard/wireguardconnection_posix.cpp
        connectors/wireguard/wireguardconnection_posix.h
        sleepevents_mac.h
        sleepevents_mac.mm
    )
elseif(UNIX)
    target_sources(engine PRIVATE
        connectors/ikev2/ikev2connection_linux.cpp
        connectors/ikev2/ikev2connection_linux.h
        connectors/wireguard/wireguardconnection_posix.cpp
        connectors/wireguard/wireguardconnection_posix.h
    )
endif()

add_subdirectory(tests)
