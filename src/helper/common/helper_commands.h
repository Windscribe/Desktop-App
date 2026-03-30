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
    clearWifiHistoryData,

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
    setGaiIpv4PriorityEnabled,
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

struct AmneziawgConfig
{
    std::string title;
    int jc = 0;
    int jmin = 0;
    int jmax = 0;
    int s1 = 0;
    int s2 = 0;
    int s3 = 0;
    int s4 = 0;
    std::string h1;
    std::string h2;
    std::string h3;
    std::string h4;
    // I1, I2, I3, I4, I5 (in order).  Only I1 is mandatory, the others may not exist.
    std::vector<std::string> iValues;

    bool isValid() const
    {
        // We do not check the title, as a WG custom config with AmneziaWG params will not have one.
        return (jc > 0) || (jmin > 0) || (jmax > 0) || (s1 > 0) || (s2 > 0) || (s3 > 0) || (s4 > 0) ||
               (!h1.empty()) || (!h2.empty()) || (!h3.empty()) || (!h4.empty()) || (!iValues.empty());
    }
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

template<class Archive>
void serialize(Archive &ar, AmneziawgConfig &a, const unsigned int /*version*/)
{
    ar & a.title;
    ar & a.jc;
    ar & a.jmin;
    ar & a.jmax;
    ar & a.s1;
    ar & a.s2;
    ar & a.s3;
    ar & a.s4;
    ar & a.h1;
    ar & a.h2;
    ar & a.h3;
    ar & a.h4;
    ar & a.iValues;
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


