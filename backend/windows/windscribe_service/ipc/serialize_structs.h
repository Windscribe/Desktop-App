#pragma once

namespace boost {
namespace serialization {

template<class Archive>
void serialize(Archive & ar, CMD_RUN_OPENVPN & g, const unsigned int version)
{
    UNREFERENCED_PARAMETER(version);
    ar & g.szConfig;
    ar & g.portNumber;
    ar & g.szHttpProxy;
    ar & g.httpPortNumber;
    ar & g.szSocksProxy;
    ar & g.socksPortNumber;
    ar & g.isCustomConfig;
}

template<class Archive>
void serialize(Archive & ar, CMD_TASK_KILL & g, const unsigned int version)
{
    UNREFERENCED_PARAMETER(version);
    ar & g.target;
}

template<class Archive>
void serialize(Archive & ar, CMD_SET_METRIC & g, const unsigned int version)
{
    UNREFERENCED_PARAMETER(version);
    ar & g.szInterfaceType;
    ar & g.szInterfaceName;
    ar & g.szMetricNumber;
}

template<class Archive>
void serialize(Archive & ar, CMD_WMIC_GET_CONFIG_ERROR_CODE & g, const unsigned int version)
{
    UNREFERENCED_PARAMETER(version);
    ar & g.szAdapterName;
}

template<class Archive>
void serialize(Archive & ar, CMD_ICS_CHANGE & g, const unsigned int version)
{
    UNREFERENCED_PARAMETER(version);
    ar & g.szAdapterName;
}

template<class Archive>
void serialize(Archive & ar, CMD_ADD_HOSTS & g, const unsigned int version)
{
    UNREFERENCED_PARAMETER(version);
    ar & g.hosts;
}

template<class Archive>
void serialize(Archive & ar, CMD_WHITELIST_PORTS & g, const unsigned int version)
{
    UNREFERENCED_PARAMETER(version);
    ar & g.ports;
}

template<class Archive>
void serialize(Archive & ar, CMD_CHECK_UNBLOCKING_CMD_STATUS & g, const unsigned int version)
{
    UNREFERENCED_PARAMETER(version);
    ar & g.cmdId;
}

template<class Archive>
void serialize(Archive & ar, CMD_CLEAR_UNBLOCKING_CMD & g, const unsigned int version)
{
    UNREFERENCED_PARAMETER(version);
    ar & g.blockingCmdId;
}

template<class Archive>
void serialize(Archive & ar, CMD_SUSPEND_UNBLOCKING_CMD & g, const unsigned int version)
{
    UNREFERENCED_PARAMETER(version);
    ar & g.blockingCmdId;
}

template<class Archive>
void serialize(Archive & ar, CMD_SET_MAC_ADDRESS_REGISTRY_VALUE_SZ & g, const unsigned int version)
{
    UNREFERENCED_PARAMETER(version);
    ar & g.szInterfaceName;
    ar & g.szValue;
}

template<class Archive>
void serialize(Archive & ar, CMD_REMOVE_MAC_ADDRESS_REGISTRY_PROPERTY & g, const unsigned int version)
{
    UNREFERENCED_PARAMETER(version);
    ar & g.szInterfaceName;
}

template<class Archive>
void serialize(Archive & ar, CMD_RESET_NETWORK_ADAPTER & g, const unsigned int version)
{
    UNREFERENCED_PARAMETER(version);
    ar & g.bringBackUp;
    ar & g.szInterfaceName;
}


template<class Archive>
void serialize(Archive & ar, CMD_SPLIT_TUNNELING_SETTINGS & g, const unsigned int version)
{
    UNREFERENCED_PARAMETER(version);
    ar & g.isActive;
    ar & g.isExclude;
    ar & g.isAllowLanTraffic;
    ar & g.files;
    ar & g.ips;
    ar & g.hosts;
}

template<class Archive>
void serialize(Archive &ar, ADAPTER_GATEWAY_INFO &a, const unsigned int version)
{
    UNREFERENCED_PARAMETER(version);
    ar & a.adapterName;
    ar & a.adapterIp;
    ar & a.gatewayIp;
    ar & a.dnsServers;
    ar & a.ifIndex;
}

template<class Archive>
void serialize(Archive & ar, CMD_CONNECT_STATUS & a, const unsigned int version)
{
    UNREFERENCED_PARAMETER(version);
    ar & a.isConnected;
    ar & a.isTerminateSocket;
    ar & a.isKeepLocalSocket;
    ar & a.protocol;

    ar & a.defaultAdapter;
    ar & a.vpnAdapter;

    ar & a.connectedIp;
    ar & a.remoteIp;
}

template<class Archive>
void serialize(Archive & ar, CMD_CONNECTED_DNS & g, const unsigned int version)
{
        UNREFERENCED_PARAMETER(version);
        ar & g.ifIndex;
        ar & g.szDnsIpAddress;
}

template<class Archive>
void serialize(Archive & ar, CMD_CLOSE_TCP_CONNECTIONS & g, const unsigned int version)
{
    UNREFERENCED_PARAMETER(version);
    ar & g.isKeepLocalSockets;
}

template<class Archive>
void serialize(Archive & ar, CMD_FIREWALL_ON & g, const unsigned int version)
{
    UNREFERENCED_PARAMETER(version);
    ar & g.allowLanTraffic;
    ar & g.isCustomConfig;
    ar & g.connectingIp;
    ar & g.ip;
}

template<class Archive>
void serialize(Archive & ar, CMD_CHANGE_MTU & g, const unsigned int version)
{
    UNREFERENCED_PARAMETER(version);
    ar & g.mtu;
    ar & g.storePersistent;
    ar & g.szAdapterName;
}

template<class Archive>
void serialize(Archive & ar, CMD_CONFIGURE_WIREGUARD & g, const unsigned int version)
{
    UNREFERENCED_PARAMETER(version);
    ar & g.config;
}

template<class Archive>
void serialize(Archive & ar, MessagePacketResult & g, const unsigned int version)
{
    UNREFERENCED_PARAMETER(version);
    ar & g.id;
    ar & g.success;
    ar & g.exitCode;
    ar & g.blockingCmdId;
    ar & g.blockingCmdFinished;
    ar & g.customInfoValue;
    ar & g.additionalString;
}

template<class Archive>
void serialize(Archive& ar, CMD_DISABLE_DNS_TRAFFIC& g, const unsigned int version)
{
    UNREFERENCED_PARAMETER(version);
    ar& g.excludedIps;
}

template<class Archive>
void serialize(Archive & ar, CMD_CREATE_OPENVPN_ADAPTER & g, const unsigned int version)
{
    UNREFERENCED_PARAMETER(version);
    ar & g.useDCODriver;
}

template<class Archive>
void serialize(Archive & ar, CMD_SSID_FROM_INTERFACE_GUID & g, const unsigned int version)
{
    UNREFERENCED_PARAMETER(version);
    ar & g.interfaceGUID;
}

template<class Archive>
void serialize(Archive & ar, CMD_SET_NETWORK_CATEGORY & g, const unsigned int version)
{
    UNREFERENCED_PARAMETER(version);
    ar & g.networkName;
    ar & g.category;
}

} // namespace serialization
} // namespace boost
