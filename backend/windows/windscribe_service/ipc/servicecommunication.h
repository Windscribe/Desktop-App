#pragma once

//#define AA_COMMAND_EXECUTE                                0
#define AA_COMMAND_FIREWALL_ON                              1
#define AA_COMMAND_FIREWALL_OFF                             2
#define AA_COMMAND_FIREWALL_CHANGE                          3 // deprecated
#define AA_COMMAND_FIREWALL_STATUS                          4
#define AA_COMMAND_FIREWALL_IPV6_ENABLE                     5
#define AA_COMMAND_FIREWALL_IPV6_DISABLE                    6
#define AA_COMMAND_REMOVE_WINDSCRIBE_FROM_HOSTS             7
#define AA_COMMAND_CHECK_UNBLOCKING_CMD_STATUS              8
#define AA_COMMAND_GET_UNBLOCKING_CMD_COUNT                 9
#define AA_COMMAND_CLEAR_UNBLOCKING_CMD                     10
#define AA_COMMAND_GET_HELPER_VERSION                       11
#define AA_COMMAND_OS_IPV6_STATE                            12 // deprecated
#define AA_COMMAND_OS_IPV6_ENABLE                           13 // deprecated
#define AA_COMMAND_OS_IPV6_DISABLE                          14 // deprecated
#define AA_COMMAND_ICS_IS_SUPPORTED                         15
#define AA_COMMAND_ICS_START                                16
#define AA_COMMAND_ICS_CHANGE                               17
#define AA_COMMAND_ICS_STOP                                 18
#define AA_COMMAND_ADD_HOSTS                                19
#define AA_COMMAND_REMOVE_HOSTS                             20
#define AA_COMMAND_DISABLE_DNS_TRAFFIC                      21
#define AA_COMMAND_ENABLE_DNS_TRAFFIC                       22
#define AA_COMMAND_REINSTALL_WAN_IKEV2                      23
#define AA_COMMAND_ENABLE_WAN_IKEV2                         24
#define AA_COMMAND_CLOSE_TCP_CONNECTIONS                    25
#define AA_COMMAND_ENUM_PROCESSES                           26
#define AA_COMMAND_TASK_KILL                                27
#define AA_COMMAND_SET_METRIC                               28
#define AA_COMMAND_WMIC_ENABLE                              29 // deprecated
#define AA_COMMAND_WMIC_GET_CONFIG_ERROR_CODE               30
#define AA_COMMAND_ENABLE_BFE                               31
#define AA_COMMAND_RUN_OPENVPN                              32
#define AA_COMMAND_WHITELIST_PORTS                          33
#define AA_COMMAND_DELETE_WHITELIST_PORTS                   34
#define AA_COMMAND_SET_MAC_ADDRESS_REGISTRY_VALUE_SZ        35
#define AA_COMMAND_REMOVE_MAC_ADDRESS_REGISTRY_PROPERTY     36
#define AA_COMMAND_RESET_NETWORK_ADAPTER                    37
#define AA_COMMAND_SPLIT_TUNNELING_SETTINGS                 38
#define AA_COMMAND_ADD_IKEV2_DEFAULT_ROUTE                  39
#define AA_COMMAND_REMOVE_WINDSCRIBE_NETWORK_PROFILES       40
#define AA_COMMAND_CHANGE_MTU                               41
#define AA_COMMAND_RESET_AND_START_RAS                      42
#define AA_COMMAND_SET_IKEV2_IPSEC_PARAMETERS               43
#define AA_COMMAND_START_WIREGUARD                          44
#define AA_COMMAND_STOP_WIREGUARD                           45
#define AA_COMMAND_GET_WIREGUARD_STATUS                     46
#define AA_COMMAND_CONNECT_STATUS                           47
#define AA_COMMAND_SUSPEND_UNBLOCKING_CMD                   48
#define AA_COMMAND_CONNECTED_DNS                            49
#define AA_COMMAND_MAKE_HOSTS_FILE_WRITABLE                 50
#define AA_COMMAND_CREATE_OPENVPN_ADAPTER                   51
#define AA_COMMAND_REMOVE_OPENVPN_ADAPTER                   52
#define AA_COMMAND_DISABLE_DOH_SETTINGS                     53
#define AA_COMMAND_ENABLE_DOH_SETTINGS                      54
#define AA_COMMAND_CONFIGURE_WIREGUARD                      55
#define AA_COMMAND_SSID_FROM_INTERFACE_GUID                 56
#define AA_COMMAND_SET_NETWORK_CATEGORY                     57

#include <string>
#include <vector>

enum CmdKillTarget {
    kTargetOpenVpn
};

struct CMD_FIREWALL_ON
{
    bool allowLanTraffic = false;
    bool isCustomConfig = false;
    std::wstring connectingIp;
    std::wstring ip;
};

struct CMD_ADD_HOSTS
{
    std::wstring hosts;
};

struct CMD_CHECK_UNBLOCKING_CMD_STATUS
{
    unsigned long cmdId;
};

struct CMD_ENABLE_FIREWALL_ON_BOOT
{
    bool bEnable;
};

struct CMD_CLEAR_UNBLOCKING_CMD
{
    unsigned long blockingCmdId;
};

struct CMD_SUSPEND_UNBLOCKING_CMD
{
    unsigned long blockingCmdId;
};

struct CMD_TASK_KILL
{
    CmdKillTarget target;
};

struct CMD_SET_METRIC
{
    std::wstring szInterfaceType;
    std::wstring szInterfaceName;
    std::wstring szMetricNumber;
};

struct CMD_WMIC_GET_CONFIG_ERROR_CODE
{
    std::wstring szAdapterName;
};

struct CMD_RUN_OPENVPN
{
    std::wstring    szConfig;
    unsigned int    portNumber = 0;
    std::wstring    szHttpProxy;           // empty string, if no http
    unsigned int    httpPortNumber = 0;
    std::wstring    szSocksProxy;          // empty string, if no socks
    unsigned int    socksPortNumber = 0;
    bool            isCustomConfig = false;
};

struct CMD_ICS_CHANGE
{
    std::wstring szAdapterName;
};

struct CMD_WHITELIST_PORTS
{
    std::wstring ports;
};

struct CMD_SET_MAC_ADDRESS_REGISTRY_VALUE_SZ
{
    std::wstring szInterfaceName;
    std::wstring szValue;
};

struct CMD_REMOVE_MAC_ADDRESS_REGISTRY_PROPERTY
{
    std::wstring szInterfaceName;
};

struct CMD_RESET_NETWORK_ADAPTER
{
    bool bringBackUp = false;
    std::wstring szInterfaceName;
};


struct CMD_SPLIT_TUNNELING_SETTINGS
{
    bool isActive;
    bool isExclude;     // true -> SPLIT_TUNNELING_MODE_EXCLUDE, false -> SPLIT_TUNNELING_MODE_INCLUDE
    bool isAllowLanTraffic;

    std::vector<std::wstring> files;
    std::vector<std::wstring> ips;
    std::vector<std::string> hosts;
};

struct ADAPTER_GATEWAY_INFO
{
    std::string adapterName;
    std::string adapterIp;
    std::string gatewayIp;
    std::vector<std::string> dnsServers;
    unsigned long ifIndex;
};

enum CMD_PROTOCOL_TYPE {
    CMD_PROTOCOL_IKEV2,
    CMD_PROTOCOL_OPENVPN,
    CMD_PROTOCOL_STUNNEL_OR_WSTUNNEL,
    CMD_PROTOCOL_WIREGUARD
};

// used to manage split tunneling
struct CMD_CONNECT_STATUS
{
    bool isConnected;

    bool isTerminateSocket;
    bool isKeepLocalSocket;

    CMD_PROTOCOL_TYPE protocol;

    ADAPTER_GATEWAY_INFO defaultAdapter;
    ADAPTER_GATEWAY_INFO vpnAdapter;

    // need for stunnel/wstunnel/openvpn routing
    std::string connectedIp;
    std::string remoteIp;
};

struct CMD_CONNECTED_DNS
{
    unsigned long ifIndex = 0;
    std::wstring szDnsIpAddress;
};

struct CMD_CLOSE_TCP_CONNECTIONS
{
    bool isKeepLocalSockets;
};

struct CMD_CHANGE_MTU
{
    int mtu = 0;
    bool storePersistent = false;
    std::wstring szAdapterName;
};

struct CMD_CONFIGURE_WIREGUARD
{
    std::wstring config;
};

struct CMD_DISABLE_DNS_TRAFFIC
{
    std::vector<std::wstring> excludedIps;
};

enum WireGuardServiceState
{
    WIREGUARD_STATE_NONE,       // WireGuard is not started.
    WIREGUARD_STATE_ERROR,      // WireGuard is in error state.
    WIREGUARD_STATE_STARTING,   // WireGuard is initializing/starting up.
    WIREGUARD_STATE_LISTENING,  // WireGuard is listening for UAPI commands, but not connected.
    WIREGUARD_STATE_CONNECTING, // WireGuard is configured and awaits for a handshake.
    WIREGUARD_STATE_ACTIVE,     // WireGuard is connected.
};

struct CMD_CREATE_OPENVPN_ADAPTER
{
    bool useDCODriver;
};

struct CMD_SSID_FROM_INTERFACE_GUID
{
    std::wstring interfaceGUID;
};

enum NetworkCategory {
    kNetworkCategoryPublic = 0,
    kNetworkCategoryPrivate,
    kNetworkCategoryDomainAuthenticated
};

struct CMD_SET_NETWORK_CATEGORY
{
    std::wstring networkName;
    NetworkCategory category;
};

struct MessagePacketResult
{
    __int64 id = 0;
    bool success = false;
    unsigned long exitCode = 0;
    unsigned long blockingCmdId = 0;    // id for check status of blocking cmd
    bool blockingCmdFinished = false;
    unsigned __int64 customInfoValue[3] = { 0, 0, 0 };
    std::string additionalString;
};
