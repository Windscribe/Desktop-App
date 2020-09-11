#ifndef SERIALIZE_STRUCTS_H
#define SERIALIZE_STRUCTS_H

namespace boost {
namespace serialization {

template<class Archive>
void serialize(Archive & ar, CMD_RUN_OPENVPN & g, const unsigned int version)
{
    UNREFERENCED_PARAMETER(version);
    ar & g.szOpenVpnExecutable;
    ar & g.szConfigPath;
    ar & g.portNumber;
    ar & g.szHttpProxy;
    ar & g.httpPortNumber;
    ar & g.szSocksProxy;
    ar & g.socksPortNumber;
}

template<class Archive>
void serialize(Archive & ar, CMD_TASK_KILL & g, const unsigned int version)
{
    UNREFERENCED_PARAMETER(version);
    ar & g.szExecutableName;
}

template<class Archive>
void serialize(Archive & ar, CMD_RESET_TAP & g, const unsigned int version)
{
    UNREFERENCED_PARAMETER(version);
    ar & g.szTapName;
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
void serialize(Archive & ar, CMD_WMIC_ENABLE & g, const unsigned int version)
{
    UNREFERENCED_PARAMETER(version);
    ar & g.szAdapterName;
}

template<class Archive>
void serialize(Archive & ar, CMD_WMIC_GET_CONFIG_ERROR_CODE & g, const unsigned int version)
{
    UNREFERENCED_PARAMETER(version);
    ar & g.szAdapterName;
}

template<class Archive>
void serialize(Archive & ar, CMD_UPDATE_ICS & g, const unsigned int version)
{
    UNREFERENCED_PARAMETER(version);
    ar & g.cmd;
    ar & g.szConfigPath;
    ar & g.szPublicGuid;
    ar & g.szPrivateGuid;
    ar & g.szEventName;
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
    ar & g.isKeepLocalSockets;
    ar & g.files;
    ar & g.ips;
	ar & g.hosts;
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
void serialize(Archive & ar, CMD_RUN_UPDATE_INSTALLER & g, const unsigned int version)
{
	UNREFERENCED_PARAMETER(version);
	ar & g.szUpdateInstallerLocation;
}

} // namespace serialization
} // namespace boost


#endif // SERIALIZE_STRUCTS_H

