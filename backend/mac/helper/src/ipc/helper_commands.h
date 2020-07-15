#ifndef HelperCommands_h
#define HelperCommands_h

#include <string>
#include <vector>

#define HELPER_CMD_EXECUTE                      0
#define HELPER_CMD_EXECUTE_OPENVPN              1
#define HELPER_CMD_GET_CMD_STATUS               2
#define HELPER_CMD_CLEAR_CMDS                   3
#define HELPER_CMD_SET_KEYCHAIN_ITEM            4
#define HELPER_CMD_SPLIT_TUNNELING_SETTINGS     5
#define HELPER_CMD_SEND_CONNECT_STATUS          6
#define HELPER_CMD_SET_KEXT_PATH                7


struct CMD_EXECUTE
{
    std::string cmdline;
};

struct CMD_EXECUTE_OPENVPN
{
    std::string cmdline;
};

struct CMD_GET_CMD_STATUS
{
    unsigned long cmdId;
};

struct CMD_CLEAR_CMDS
{
};

struct CMD_SET_KEYCHAIN_ITEM
{
    std::string username;
    std::string password;
};

struct CMD_SPLIT_TUNNELING_SETTINGS
{
    bool isActive;
    bool isExclude;     // true -> SPLIT_TUNNELING_MODE_EXCLUDE, false -> SPLIT_TUNNELING_MODE_INCLUDE

    std::vector<std::string> files;
    std::vector<std::string> ips;
    std::vector<std::string> hosts;
};

struct CMD_SEND_DEFAULT_ROUTE
{
    std::string gatewayIp;
    std::string interfaceName;
    std::string interfaceIp;
};

enum CMD_PROTOCOL_TYPE { CMD_PROTOCOL_IKEV2 = 0, CMD_PROTOCOL_OPENVPN,  CMD_PROTOCOL_STUNNEL_OR_WSTUNNEL};

struct CMD_SEND_CONNECT_STATUS
{
    bool isConnected;
    CMD_PROTOCOL_TYPE protocol;

    std::string gatewayIp;
    std::string interfaceName;
    std::string interfaceIp;
    
    // need for stunnel/wstunnel routing
    std::string connectedIp;

    // need for openvpn routing
    std::string routeVpnGateway;
    std::string routeNetGateway;
    std::string remote_1;
    std::string ifconfigTunIp;
    
    // need for ikev2 routing
    std::string ikev2AdapterAddress;
    
    // common
    std::string vpnAdapterName;
    
    std::vector<std::string> dnsServers;
};

struct CMD_SET_KEXT_PATH
{
    std::string kextPath;
};

struct CMD_ANSWER
{
    unsigned long cmdId;        // unique ID of command
    int    executed;            // 0 - failed, 1 - executed, 2 - still executing (for openvpn)
    std::string body;
};


#endif
