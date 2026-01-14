#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <spdlog/spdlog.h>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/vector.hpp>

enum class HelperCommand {
    // Common
    getHelperVersion,
    setSplitTunnelingSettings,
    sendConnectStatus,
    changeMtu,
    executeOpenVPN,
    executeTaskKill,
    startWireGuard,
    stopWireGuard,
    configureWireGuard,
    getWireGuardStatus,
    setFirewallOnBoot,

    // Windows
    firewallOn,
    firewallOff,
    firewallActualState,
    setCustomDnsWhileConnected,
    executeSetMetric,
    executeWmicGetConfigManagerErrorCode,
    isIcsSupported,
    startIcs,
    changeIcs,
    stopIcs,
    enableBFE,
    resetAndStartRAS,
    setIPv6EnabledInFirewall,
    addHosts,
    removeHosts,
    closeAllTcpConnections,
    getProcessesList,
    whitelistPorts,
    deleteWhitelistPorts,
    enableDnsLeaksProtection,
    disableDnsLeaksProtection,
    reinstallWanIkev2,
    enableWanIkev2,
    setMacAddressRegistryValueSz,
    removeMacAddressRegistryProperty,
    resetNetworkAdapter,
    addIKEv2DefaultRoute,
    removeWindscribeNetworkProfiles,
    setIKEv2IPSecParameters,
    makeHostsFileWritable,
    createOpenVpnAdapter,
    removeOpenVpnAdapter,
    disableDohSettings,
    enableDohSettings,
    ssidFromInterfaceGUID,

    // Posix (Mac and Linux)
    startCtrld,
    stopCtrld,
    checkFirewallState,
    clearFirewallRules,
    setFirewallRules,
    getFirewallRules,
    startStunnel,
    startWstunnel,
    setMacAddress,

    // Mac
    setDnsScriptEnabled,
    enableMacSpoofingOnBoot,
    setDnsOfDynamicStoreEntry,
    setIpv6Enabled,
    deleteRoute,

    // Mac Installer
    removeOldInstall,
    setInstallerPaths,
    executeFilesStep,
    createCliSymlink,

    // Linux
    setDnsLeakProtectEnabled,
    resetMacAddresses
};


enum CmdKillTarget {
    kTargetWindscribe,
    kTargetOpenVpn,
    kTargetStunnel,
    kTargetWStunnel,
    kTargetWireGuard,
    kTargetCtrld
};

enum CmdIpVersion {
    kIpv4 = 4,
    kIpv6 = 6,
};

enum CmdDnsManager {
    kResolvConf,
    kSystemdResolved,
    kNetworkManager,
};

struct ADAPTER_GATEWAY_INFO
{
    std::string adapterName;
    std::string adapterIp;
    std::string gatewayIp;
    std::vector<std::string> dnsServers;
    unsigned long ifIndex;
};

enum CmdProtocolType {
    kCmdProtocolIkev2,
    kCmdProtocolOpenvpn,
    kCmdProtocolStunnelOrWstunnel,
    kCmdProtocolWireGuard,
};

enum WireGuardServiceState {
    kWgStateNone,       // WireGuard is not started.
    kWgStateError,      // WireGuard is in error state.
    kWgStateStarting,   // WireGuard is initializing/starting up.
    kWgStateListening,  // WireGuard is listening for UAPI commands, but not connected.
    kWgStateConnecting, // WireGuard is configured and awaits for a handshake.
    kWgStateActive,     // WireGuard is connected.
};

struct ConnectStatus {
    bool isConnected;
    CmdProtocolType protocol;

    ADAPTER_GATEWAY_INFO defaultAdapter;
    ADAPTER_GATEWAY_INFO vpnAdapter;

    // need for stunnel/wstunnel/openvpn routing
    std::string connectedIp;
    std::string remoteIp;

    // Windows only
    bool isTerminateSocket;
    bool isKeepLocalSocket;

};

namespace boost {
namespace serialization {
template<class Archive>
void serialize(Archive &ar, ADAPTER_GATEWAY_INFO &a, const unsigned int /*version*/)
{
    ar & a.adapterName;
    ar & a.adapterIp;
    ar & a.gatewayIp;
    ar & a.dnsServers;
    ar & a.ifIndex;
}
}
}

template<typename... Args>
std::string serializeResult(const Args&... args)
{
    std::ostringstream oss(std::ios::binary);
    boost::archive::binary_oarchive archive(oss, boost::archive::no_header);
    if constexpr (sizeof...(Args) > 0) {
        (archive << ... << args);
    }
    return oss.str();
}

template<typename... Args>
void deserializePars(const std::string &data, Args&... args)
{
    if (data.empty()) {
        spdlog::error("deserializePars error: data is empty");
        return;
    }

    try {
        std::istringstream iss(data, std::ios::binary);
        boost::archive::binary_iarchive archive(iss, boost::archive::no_header);
        if constexpr (sizeof...(Args) > 0) {
            (archive >> ... >> args);
        }
    } catch(const std::exception & ex) {
        spdlog::error("deserializePars exception: {}", ex.what());
    }
}


