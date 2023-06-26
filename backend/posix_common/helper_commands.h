#ifndef HelperCommands_h
#define HelperCommands_h

#include <string>
#include <vector>
#include <unistd.h>

#define HELPER_CMD_EXECUTE                           0 // deprecated
#define HELPER_CMD_START_OPENVPN                     1
#define HELPER_CMD_GET_CMD_STATUS                    2
#define HELPER_CMD_CLEAR_CMDS                        3
#define HELPER_CMD_SET_KEYCHAIN_ITEM                 4
#define HELPER_CMD_SPLIT_TUNNELING_SETTINGS          5
#define HELPER_CMD_SEND_CONNECT_STATUS               6
#define HELPER_CMD_SET_KEXT_PATH                     7 // deprecated
#define HELPER_CMD_START_WIREGUARD                   8
#define HELPER_CMD_STOP_WIREGUARD                    9
#define HELPER_CMD_CONFIGURE_WIREGUARD               10
#define HELPER_CMD_GET_WIREGUARD_STATUS              11
#define HELPER_CMD_KILL_PROCESS                      12 // deprecated
#define HELPER_CMD_INSTALLER_SET_PATH                13
#define HELPER_CMD_INSTALLER_EXECUTE_COPY_FILE       14
#define HELPER_CMD_INSTALLER_REMOVE_OLD_INSTALL      15
#define HELPER_CMD_APPLY_CUSTOM_DNS                  16
#define HELPER_CMD_CHANGE_MTU                        17
#define HELPER_CMD_DELETE_ROUTE                      18
#define HELPER_CMD_SET_IPV6_ENABLED                  19
#define HELPER_CMD_SET_DNS_LEAK_PROTECT_ENABLED      20 // this script disables DNS queries on non-VPN interfaces
#define HELPER_CMD_SET_DNS_SCRIPT_ENABLED            21 // this script sets/unsets the VPN DNS as the system default
#define HELPER_CMD_CHECK_FOR_WIREGUARD_KERNEL_MODULE 22
#define HELPER_CMD_CLEAR_FIREWALL_RULES              23
#define HELPER_CMD_CHECK_FIREWALL_STATE              24
#define HELPER_CMD_SET_FIREWALL_RULES                25
#define HELPER_CMD_GET_FIREWALL_RULES                26
#define HELPER_CMD_DELETE_OLD_HELPER                 27
#define HELPER_CMD_SET_FIREWALL_ON_BOOT              28
#define HELPER_CMD_SET_MAC_SPOOFING_ON_BOOT          29
#define HELPER_CMD_SET_MAC_ADDRESS                   30
#define HELPER_CMD_TASK_KILL                         31
#define HELPER_CMD_START_CTRLD                       32
#define HELPER_CMD_START_STUNNEL                     33
#define HELPER_CMD_CONFIGURE_STUNNEL                 34
#define HELPER_CMD_START_WSTUNNEL                    35

// enums

enum CmdProtocolType {
    kCmdProtocolIkev2,
    kCmdProtocolOpenvpn,
    kCmdProtocolStunnelOrWstunnel,
    kCmdProtocolWireGuard,
};

enum CmdWireGuardServiceState {
    kWgStateNone,       // WireGuard is not started.
    kWgStateError,      // WireGuard is in error state.
    kWgStateStarting,   // WireGuard is initializing/starting up.
    kWgStateListening,  // WireGuard is listening for UAPI commands, but not connected.
    kWgStateConnecting, // WireGuard is configured and awaits for a handshake.
    kWgStateActive,     // WireGuard is connected.
};

enum CmdIpVersion {
    kIpv4 = 4,
    kIpv6 = 6,
};

enum CmdKillTarget {
    kTargetWindscribe,
    kTargetOpenVpn,
    kTargetStunnel,
    kTargetWStunnel,
    kTargetWireGuard,
    kTargetCtrld
};

enum CmdDnsManager {
    kResolvConf,
    kSystemdResolved,
    kNetworkManager,
};

// answer struct, common to all commands
struct CMD_ANSWER {
    unsigned long cmdId;        // unique ID of command
    int    executed;            // 0 - failed, 1 - executed, 2 - still executing (for openvpn)
    unsigned long long customInfoValue[2];
    std::string body;
    int exitCode;

    CMD_ANSWER() : cmdId(0), executed(0), customInfoValue(), body(), exitCode(-1) {}
};

// command structs

struct CMD_START_OPENVPN {
    std::string exePath;
    std::string executable;
    std::string config;
    std::string arguments;
    CmdDnsManager dnsManager; // Linux only
    bool isCustomConfig;
};

struct CMD_GET_CMD_STATUS {
    unsigned long cmdId;
};

struct CMD_CLEAR_CMDS {
};

struct CMD_SET_KEYCHAIN_ITEM {
    std::string username;
    std::string password;
};

struct CMD_SPLIT_TUNNELING_SETTINGS {
    bool isActive;
    bool isExclude;     // true -> SPLIT_TUNNELING_MODE_EXCLUDE, false -> SPLIT_TUNNELING_MODE_INCLUDE
    bool isAllowLanTraffic;

    std::vector<std::string> files;
    std::vector<std::string> ips;
    std::vector<std::string> hosts;
};

struct CMD_SEND_DEFAULT_ROUTE {
    std::string gatewayIp;
    std::string interfaceName;
    std::string interfaceIp;
};

struct ADAPTER_GATEWAY_INFO {
    std::string adapterName;
    std::string adapterIp;
    std::string gatewayIp;
    std::vector<std::string> dnsServers;
};

struct CMD_SEND_CONNECT_STATUS {
    bool isConnected;
    CmdProtocolType protocol;

    ADAPTER_GATEWAY_INFO defaultAdapter;
    ADAPTER_GATEWAY_INFO vpnAdapter;

    // need for stunnel/wstunnel/openvpn routing
    std::string connectedIp;
    std::string remoteIp;
};

struct CMD_START_WIREGUARD {
    std::string exePath;
    std::string executable;
    std::string deviceName;
};

struct CMD_CONFIGURE_WIREGUARD {
    std::string clientPrivateKey;
    std::string clientIpAddress;
    std::string clientDnsAddressList;
    std::string clientDnsScriptName;
    std::string peerPublicKey;
    std::string peerPresharedKey;
    std::string peerEndpoint;
    std::string allowedIps;
    CmdDnsManager dnsManager; // Linux only
};

struct CMD_START_CTRLD {
    std::string exePath;
    std::string executable;
    std::string parameters;
};

struct CMD_KILL_PROCESS {
    pid_t processId;
};

// installer commands
struct CMD_INSTALLER_FILES_SET_PATH {
    std::wstring archivePath;
    std::wstring installPath;
    uid_t userId;
    gid_t groupId;
};

struct CMD_APPLY_CUSTOM_DNS {
    std::string ipAddress;
    std::string networkService;
};

struct CMD_CHANGE_MTU {
    int mtu;
    std::string adapterName;
};

struct CMD_DELETE_ROUTE {
    std::string range;
    int mask;
    std::string gateway;
};

struct CMD_SET_IPV6_ENABLED {
    bool enabled;
};

struct CMD_SET_DNS_LEAK_PROTECT_ENABLED {
    bool enabled;
};

struct CMD_SET_DNS_SCRIPT_ENABLED {
    bool enabled;
};

struct CMD_CHECK_FIREWALL_STATE {
    std::string tag;
};

struct CMD_SET_FIREWALL_RULES {
    CmdIpVersion ipVersion;
    std::string table;
    std::string group;
    std::string rules;
};

struct CMD_CLEAR_FIREWALL_RULES {
    bool isKeekPfEnabled;
};

struct CMD_GET_FIREWALL_RULES {
    CmdIpVersion ipVersion;
    std::string table;
    std::string group;
};

struct CMD_INSTALLER_REMOVE_OLD_INSTALL {
    std::string path;
};

struct CMD_SET_FIREWALL_ON_BOOT {
    bool enabled;
    std::string ipTable;
};

struct CMD_SET_MAC_SPOOFING_ON_BOOT {
    bool enabled;
    std::string interface;
    std::string macAddress;
    bool robustMethod;
};

struct CMD_SET_MAC_ADDRESS {
    std::string interface;
    std::string macAddress;
    bool robustMethod;
};

struct CMD_TASK_KILL {
    CmdKillTarget target;
};

struct CMD_START_STUNNEL {
    std::string exePath;
    std::string executable;
};

struct CMD_CONFIGURE_STUNNEL {
    std::string hostname;
    int port;
    int localPort;
};

struct CMD_START_WSTUNNEL {
    std::string exePath;
    std::string executable;
    std::string hostname;
    int port;
    int localPort;
    bool isUdp;
};

#endif
