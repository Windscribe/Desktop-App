#ifndef SERVICECOMMUNICATION
#define SERVICECOMMUNICATION

//#define AA_COMMAND_EXECUTE                                0
#define AA_COMMAND_FIREWALL_ON                              1
#define AA_COMMAND_FIREWALL_OFF                             2
#define AA_COMMAND_FIREWALL_CHANGE                          3
#define AA_COMMAND_FIREWALL_STATUS                          4
#define AA_COMMAND_FIREWALL_IPV6_ENABLE                     5
#define AA_COMMAND_FIREWALL_IPV6_DISABLE                    6
#define AA_COMMAND_REMOVE_WINDSCRIBE_FROM_HOSTS             7
#define AA_COMMAND_CHECK_UNBLOCKING_CMD_STATUS              8
#define AA_COMMAND_GET_UNBLOCKING_CMD_COUNT                 9
#define AA_COMMAND_CLEAR_UNBLOCKING_CMD                     10
#define AA_COMMAND_GET_HELPER_VERSION                       11
#define AA_COMMAND_OS_IPV6_STATE                            12
#define AA_COMMAND_OS_IPV6_ENABLE                           13
#define AA_COMMAND_OS_IPV6_DISABLE                          14
#define AA_COMMAND_IS_SUPPORTED_ICS                         15
#define AA_COMMAND_ADD_HOSTS                                16
#define AA_COMMAND_REMOVE_HOSTS                             17
#define AA_COMMAND_DISABLE_DNS_TRAFFIC                      18
#define AA_COMMAND_ENABLE_DNS_TRAFFIC                       19
#define AA_COMMAND_REINSTALL_WAN_IKEV2                      20
#define AA_COMMAND_ENABLE_WAN_IKEV2                         21
#define AA_COMMAND_CLOSE_TCP_CONNECTIONS                    22
#define AA_COMMAND_ENUM_PROCESSES                           23
#define AA_COMMAND_TASK_KILL                                24
#define AA_COMMAND_RESET_TAP                                25
#define AA_COMMAND_SET_METRIC                               26
#define AA_COMMAND_WMIC_ENABLE                              27
#define AA_COMMAND_WMIC_GET_CONFIG_ERROR_CODE               28
#define AA_COMMAND_CLEAR_DNS_ON_TAP                         29
#define AA_COMMAND_ENABLE_BFE                               30
#define AA_COMMAND_RUN_OPENVPN                              31
#define AA_COMMAND_UPDATE_ICS                               32
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
#define AA_COMMAND_CONFIGURE_WIREGUARD                      46
#define AA_COMMAND_GET_WIREGUARD_STATUS                     47
#define AA_COMMAND_RUN_UPDATE_INSTALLER                     48
#define AA_COMMAND_CONNECT_STATUS                           49
#define AA_COMMAND_SUSPEND_UNBLOCKING_CMD                   50


#define ENCRYPT_KEY "4WabPvORMXAEsgjdVU0C9MmcwOVHyjAiEBIn0dX5"

struct CMD_FIREWALL_ON
{
	bool allowLanTraffic = false;
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
    std::wstring szExecutableName;
};

struct CMD_RESET_TAP
{
    std::wstring szTapName;
};

struct CMD_SET_METRIC
{
    std::wstring szInterfaceType;
    std::wstring szInterfaceName;
    std::wstring szMetricNumber;
};

struct CMD_WMIC_ENABLE
{
    std::wstring szAdapterName;
};

struct CMD_WMIC_GET_CONFIG_ERROR_CODE
{
    std::wstring szAdapterName;
};

struct CMD_RUN_OPENVPN
{
    std::wstring    szOpenVpnExecutable;
    std::wstring    szConfigPath;
    unsigned int    portNumber = 0;
    std::wstring    szHttpProxy;           // empty string, if no http
    unsigned int    httpPortNumber = 0;
    std::wstring    szSocksProxy;          // empty string, if no socks
    unsigned int    socksPortNumber = 0;
};

struct CMD_UPDATE_ICS
{
    int             cmd = 0;   // 0 - save, 1 - restore, 2 - change
    std::wstring         szConfigPath;
    std::wstring         szPublicGuid;
    std::wstring         szPrivateGuid;
    std::wstring         szEventName;
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
    bool isKeepLocalSockets;

    std::vector<std::wstring> files;
    std::vector<std::wstring> ips;
    std::vector<std::string> hosts;
};

// used to manage split tunneling
struct CMD_CONNECT_STATUS
{
	bool isConnected;
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

struct CMD_START_WIREGUARD
{
    std::wstring    szExecutable;
    std::wstring    szDeviceName;
};

struct CMD_CONFIGURE_WIREGUARD
{
    std::string     clientPrivateKey;
    std::string     clientIpAddress;
    std::string     clientDnsAddressList;
    std::string     peerPublicKey;
    std::string     peerPresharedKey;
    std::string     peerEndpoint;
    std::string     allowedIps;
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

struct CMD_RUN_UPDATE_INSTALLER
{
    std::wstring szUpdateInstallerLocation;
};

struct MessagePacketResult
{
    __int64 id;
    bool success;
    DWORD exitCode;
    unsigned long blockingCmdId;    // id for check status of blocking cmd
    bool blockingCmdFinished;
    UINT64 customInfoValue[2];
    DWORD sizeOfAdditionalData;
    void *szAdditionalData;

    MessagePacketResult() : id(0), success(false), exitCode(0), blockingCmdId(0),
                            blockingCmdFinished(false), customInfoValue(), sizeOfAdditionalData(0),
                            szAdditionalData(nullptr)
    {
    }

    void clear()
    {
        if (szAdditionalData)
        {
            delete[] szAdditionalData;
            sizeOfAdditionalData = 0;
            szAdditionalData = NULL;
        }
    }
};

// This is a helper class to be serialized to/from a pipe. Unlike MessagePacketResult, it is
// architecture-independent, and has the same sizeof and alignment on both 32-bit and 64-bit OS.
struct MessagePacketResultSerialization
{
    INT64 id;
    UINT32 exitCode;
    UINT16 success;
    UINT16 blockingCmdFinished;
    UINT32 blockingCmdId;
    UINT64 customInfoValue[2];
    UINT32 sizeOfAdditionalData;

    MessagePacketResultSerialization() : id(0), exitCode(0), success(0), blockingCmdFinished(0),
        blockingCmdId(0), customInfoValue(), sizeOfAdditionalData(0) {}
    explicit MessagePacketResultSerialization(const MessagePacketResult &mpr)
        : id(mpr.id), exitCode(mpr.exitCode), success(mpr.success ? 1 : 0),
          blockingCmdFinished(mpr.blockingCmdFinished ? 1 : 0), blockingCmdId(mpr.blockingCmdId),
          customInfoValue{ mpr.customInfoValue[0], mpr.customInfoValue[1] },
          sizeOfAdditionalData(mpr.sizeOfAdditionalData) {}

    char *data() { return reinterpret_cast<char*>(this); }
    const char *const_data() const { return reinterpret_cast<const char*>(this); }
    MessagePacketResult result() const {
        MessagePacketResult mpr;
        mpr.id = id;
        mpr.exitCode = exitCode;
        mpr.success = !!success;
        mpr.blockingCmdFinished = !!blockingCmdFinished;
        mpr.blockingCmdId = blockingCmdId;
        mpr.customInfoValue[0] = customInfoValue[0];
        mpr.customInfoValue[1] = customInfoValue[1];
        mpr.sizeOfAdditionalData = sizeOfAdditionalData;
        return mpr;
    }
};

#endif // SERVICECOMMUNICATION

