#ifndef HelperCommandsSerialize_h
#define HelperCommandsSerialize_h

namespace boost {
namespace serialization {

template<class Archive>
void serialize(Archive &ar, CMD_ANSWER &a, const unsigned int version)
{
    ar & a.cmdId;
    ar & a.executed;
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
void serialize(Archive &ar, CMD_SEND_CONNECT_STATUS &a, const unsigned int version)
{
    ar & a.isConnected;
    ar & a.protocol;
    
    ar & a.gatewayIp;
    ar & a.interfaceName;
    ar & a.interfaceIp;
    
    ar & a.connectedIp;
    
    ar & a.routeVpnGateway;
    ar & a.routeNetGateway;
    ar & a.remote_1;
    ar & a.ifconfigTunIp;
    
    ar & a.ikev2AdapterAddress;
    
    ar & a.vpnAdapterName;
    
    ar & a.dnsServers;
}

template<class Archive>
void serialize(Archive &ar, CMD_SET_KEXT_PATH &a, const unsigned int version)
{
    ar & a.kextPath;
}

}
}


#endif
