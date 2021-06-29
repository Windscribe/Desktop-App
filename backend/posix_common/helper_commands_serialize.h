#ifndef HelperCommandsSerialize_h
#define HelperCommandsSerialize_h

namespace boost {
namespace serialization {

template<class Archive>
void serialize(Archive &ar, CMD_ANSWER &a, const unsigned int version)
{
    ar & a.cmdId;
    ar & a.executed;
    ar & a.customInfoValue[0];
    ar & a.customInfoValue[1];
    ar & a.body;
}

template<class Archive>
void serialize(Archive &ar, CMD_EXECUTE &a, const unsigned int version)
{
    ar & a.cmdline;
}

template<class Archive>
void serialize(Archive &ar, CMD_EXECUTE_OPENVPN &a, const unsigned int version)
{
    ar & a.cmdline;
}

template<class Archive>
void serialize(Archive &ar, CMD_GET_CMD_STATUS &a, const unsigned int version)
{
    ar & a.cmdId;
}

template<class Archive>
void serialize(Archive &ar, CMD_CLEAR_CMDS &a, const unsigned int version)
{
}

template<class Archive>
void serialize(Archive &ar, CMD_SET_KEYCHAIN_ITEM &a, const unsigned int version)
{
    ar & a.username;
    ar & a.password;
}

template<class Archive>
void serialize(Archive & ar, CMD_SPLIT_TUNNELING_SETTINGS &g, const unsigned int version)
{
    ar & g.isActive;
    ar & g.isExclude;
    ar & g.files;
    ar & g.ips;
    ar & g.hosts;
}

template<class Archive>
void serialize(Archive &ar, ADAPTER_GATEWAY_INFO &a, const unsigned int version)
{
    ar & a.adapterName;
    ar & a.adapterIp;
    ar & a.gatewayIp;
    ar & a.dnsServers;
}

template<class Archive>
void serialize(Archive &ar, CMD_SEND_CONNECT_STATUS &a, const unsigned int version)
{
    ar & a.isConnected;
    ar & a.protocol;
    
    ar & a.defaultAdapter;
    ar & a.vpnAdapter;
    
    ar & a.connectedIp;
    ar & a.remoteIp;
}

template<class Archive>
void serialize(Archive &ar, CMD_SET_KEXT_PATH &a, const unsigned int version)
{
    ar & a.kextPath;
}

template<class Archive>
void serialize(Archive &ar, CMD_START_WIREGUARD &a, const unsigned int version)
{
    ar & a.exePath;
    ar & a.deviceName;
}

template<class Archive>
void serialize(Archive &ar, CMD_CONFIGURE_WIREGUARD &a, const unsigned int version)
{
    ar & a.clientPrivateKey;
    ar & a.clientIpAddress;
    ar & a.clientDnsAddressList;
    ar & a.clientDnsScriptName;
    ar & a.peerPublicKey;
    ar & a.peerPresharedKey;
    ar & a.peerEndpoint;
    ar & a.allowedIps;
}

template<class Archive>
void serialize(Archive &ar, CMD_KILL_PROCESS &a, const unsigned int version)
{
    ar & a.processId;
}


template<class Archive>
void serialize(Archive &ar, CMD_INSTALLER_FILES_SET_PATH &a, const unsigned int version)
{
    ar & a.archivePath;
    ar & a.installPath;
    ar & a.userId;
    ar & a.groupId;
}

template<class Archive>
void serialize(Archive &ar, CMD_APPLY_CUSTOM_DNS &a, const unsigned int version)
{
    ar & a.ipAddress;
    ar & a.networkService;
}

}
}


#endif
