#pragma once

#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

namespace boost {
namespace serialization {

template<class Archive>
void serialize(Archive &ar, CMD_ANSWER &a, const unsigned int version)
{
    UNUSED(version);
    ar & a.cmdId;
    ar & a.executed;
    ar & a.customInfoValue[0];
    ar & a.customInfoValue[1];
    ar & a.body;
    ar & a.exitCode;
}

template<class Archive>
void serialize(Archive &ar, CMD_START_OPENVPN &a, const unsigned int version)
{
    UNUSED(version);
    ar & a.config;
    ar & a.port;
    ar & a.httpProxy;
    ar & a.socksProxy;
    ar & a.httpPort;
    ar & a.socksPort;
    ar & a.dnsManager;
    ar & a.isCustomConfig;
}

template<class Archive>
void serialize(Archive &ar, CMD_GET_CMD_STATUS &a, const unsigned int version)
{
    UNUSED(version);
    ar & a.cmdId;
}

template<class Archive>
void serialize(Archive &ar, CMD_CLEAR_CMDS &a, const unsigned int version)
{
    UNUSED(a);
    UNUSED(ar);
    UNUSED(version);
}

template<class Archive>
void serialize(Archive & ar, CMD_SPLIT_TUNNELING_SETTINGS &g, const unsigned int version)
{
    UNUSED(version);
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
    UNUSED(version);
    ar & a.adapterName;
    ar & a.adapterIp;
    ar & a.gatewayIp;
    ar & a.dnsServers;
}

template<class Archive>
void serialize(Archive &ar, CMD_SEND_CONNECT_STATUS &a, const unsigned int version)
{
    UNUSED(version);
    ar & a.isConnected;
    ar & a.protocol;

    ar & a.defaultAdapter;
    ar & a.vpnAdapter;

    ar & a.connectedIp;
    ar & a.remoteIp;
}

template<class Archive>
void serialize(Archive &ar, CMD_CONFIGURE_WIREGUARD &a, const unsigned int version)
{
    UNUSED(version);
    ar & a.clientPrivateKey;
    ar & a.clientIpAddress;
    ar & a.clientDnsAddressList;
    ar & a.clientDnsScriptName;
    ar & a.peerPublicKey;
    ar & a.peerPresharedKey;
    ar & a.peerEndpoint;
    ar & a.allowedIps;
    ar & a.dnsManager;
    ar & a.listenPort;
}

template<class Archive>
void serialize(Archive &ar, CMD_START_CTRLD &a, const unsigned int version)
{
    UNUSED(version);
    ar & a.upstream1;
    ar & a.upstream2;
    ar & a.domains;
    ar & a.isCreateLog;
}

template<class Archive>
void serialize(Archive &ar, CMD_KILL_PROCESS &a, const unsigned int version)
{
    UNUSED(version);
    ar & a.processId;
}

template<class Archive>
void serialize(Archive &ar, CMD_INSTALLER_FILES_SET_PATH &a, const unsigned int version)
{
    UNUSED(version);
    ar & a.archivePath;
    ar & a.installPath;
}

template<class Archive>
void serialize(Archive &ar, CMD_APPLY_CUSTOM_DNS &a, const unsigned int version)
{
    UNUSED(version);
    ar & a.ipAddress;
    ar & a.networkService;
}

template<class Archive>
void serialize(Archive &ar, CMD_CHANGE_MTU &a, const unsigned int version)
{
    UNUSED(version);
    ar & a.mtu;
    ar & a.adapterName;
}

template<class Archive>
void serialize(Archive &ar, CMD_DELETE_ROUTE &a, const unsigned int version)
{
    UNUSED(version);
    ar & a.range;
    ar & a.mask;
    ar & a.gateway;
}

template<class Archive>
void serialize(Archive &ar, CMD_SET_IPV6_ENABLED &a, const unsigned int version)
{
    UNUSED(version);
    ar & a.enabled;
}

template<class Archive>
void serialize(Archive &ar, CMD_SET_DNS_LEAK_PROTECT_ENABLED &a, const unsigned int version)
{
    UNUSED(version);
    ar & a.enabled;
}

template<class Archive>
void serialize(Archive &ar, CMD_SET_DNS_SCRIPT_ENABLED &a, const unsigned int version)
{
    UNUSED(version);
    ar & a.enabled;
}

template<class Archive>
void serialize(Archive &ar, CMD_CHECK_FIREWALL_STATE &a, const unsigned int version)
{
    UNUSED(version);
    ar & a.tag;
}

template<class Archive>
void serialize(Archive &ar, CMD_SET_FIREWALL_RULES &a, const unsigned int version)
{
    UNUSED(version);
    ar & a.ipVersion;
    ar & a.table;
    ar & a.group;
    ar & a.rules;
}

template<class Archive>
void serialize(Archive &ar, CMD_CLEAR_FIREWALL_RULES &a, const unsigned int version)
{
    UNUSED(version);
    ar & a.isKeepPfEnabled;
}


template<class Archive>
void serialize(Archive &ar, CMD_GET_FIREWALL_RULES &a, const unsigned int version)
{
    UNUSED(version);
    ar & a.ipVersion;
    ar & a.table;
    ar & a.group;
}

template<class Archive>
void serialize(Archive &ar, CMD_INSTALLER_REMOVE_OLD_INSTALL &a, const unsigned int version)
{
    UNUSED(version);
    ar & a.path;
}

template<class Archive>
void serialize(Archive &ar, CMD_SET_FIREWALL_ON_BOOT &a, const unsigned int version)
{
    UNUSED(version);
    ar & a.enabled;
    ar & a.allowLanTraffic;
    ar & a.ipTable;
}

template<class Archive>
void serialize(Archive &ar, CMD_SET_MAC_SPOOFING_ON_BOOT &a, const unsigned int version)
{
    UNUSED(version);
    ar & a.enabled;
    ar & a.interface;
    ar & a.macAddress;
}

template<class Archive>
void serialize(Archive &ar, CMD_SET_MAC_ADDRESS &a, const unsigned int version)
{
    UNUSED(version);
    ar & a.interface;
    ar & a.network;
    ar & a.macAddress;
    ar & a.isWifi;
}

template<class Archive>
void serialize(Archive &ar, CMD_TASK_KILL &a, const unsigned int version)
{
    UNUSED(version);
    ar & a.target;
}

template<class Archive>
void serialize(Archive &ar, CMD_START_STUNNEL &a, const unsigned int version)
{
    UNUSED(version);
    ar & a.hostname;
    ar & a.port;
    ar & a.localPort;
    ar & a.extraPadding;
}

template<class Archive>
void serialize(Archive &ar, CMD_START_WSTUNNEL &a, const unsigned int version)
{
    UNUSED(version);
    ar & a.hostname;
    ar & a.port;
    ar & a.localPort;
}

template<class Archive>
void serialize(Archive &ar, CMD_INSTALLER_CREATE_CLI_SYMLINK_DIR &a, const unsigned int version)
{
    UNUSED(version);
    ar & a.uid;
}

template<class Archive>
void serialize(Archive &ar, CMD_GET_INTERFACE_SSID &a, const unsigned int version)
{
    UNUSED(version);
    ar & a.interface;
}

template<class Archive>
void serialize(Archive &ar, CMD_RESET_MAC_ADDRESSES &a, const unsigned int version)
{
    UNUSED(version);
    ar & a.ignoreNetwork;
}

}
}
