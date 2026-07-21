#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>
#include <sstream>
#include <spdlog/spdlog.h>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/split_free.hpp>
#include <boost/serialization/vector.hpp>

#include "types/ipaddress.h"

// Upper bound on a single IPC frame's payload, used by the helper-side frame
// parsers to reject malformed/oversized lengths before any allocation. Set well
// above the largest legitimate command (executeOpenVPN with an embedded custom
// config containing full PEM cert chains is the practical maximum, ~100KB) but
// small enough that a malicious local client cannot use a single frame to
// exhaust memory in the privileged helper process.
inline constexpr std::size_t kMaxHelperFrameSize = 4 * 1024 * 1024;

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
    isWanIkev2AdapterDisabled,
    isIcsSupported,
    startIcs,
    changeIcs,
    stopIcs,
    queryBFEStatus,
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
    removeAppNetworkProfiles,
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
    setIpv6Enabled,  // deprecated: no-op since the macOS IPv6 rewrite (per-interface pf rules);
                     // slot kept so older clients don't get "unknown command id" from a newer helper.
    deleteRoute,

    // Mac Installer
    removeOldInstall,
    setInstallerPaths,
    executeFilesStep,
    createCliSymlink,

    // Linux
    setDnsLeakProtectEnabled,
    setGaiIpv4PriorityEnabled,
    resetMacAddresses,
    setOpenVpnDcoMode,
    installerStageAndVerify,
    installerCleanupStaged
};


enum CmdKillTarget {
    kTargetApp,
    kTargetOpenVpn,
    kTargetStunnel,
    kTargetWStunnel,
    kTargetWireGuard,
    kTargetCtrld
};

// Selects which firewall rules getFirewallRules returns. Replaces the previous free-form
// pf table/anchor strings so a caller cannot steer the helper into querying arbitrary tables.
enum CmdFirewallRulesQuery {
    kFirewallAllRules,
    kFirewallIpsTable,
};

enum CmdDnsManager {
    kResolvConf,
    kSystemdResolved,
    kNetworkManager,
};

struct ADAPTER_GATEWAY_INFO
{
    std::string adapterName;
    // Interface name for the IPv6 default route. May differ from adapterName on a
    // multi-homed host (v6 default on a different NIC than v4); empty means "same as
    // adapterName". Consumers that need a v6 `dev` should fall back to adapterName
    // when this is empty.
    std::string adapterNameV6;
    types::IpAddress adapterIp;
    types::IpAddress adapterIpV6;
    types::IpAddress gatewayIp;
    types::IpAddress gatewayIpV6;
    std::vector<types::IpAddress> dnsServers;
    unsigned long ifIndex = 0;
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
    types::IpAddress connectedIp;
    types::IpAddress remoteIp;

    // Windows only
    bool isTerminateSocket;
    bool isKeepLocalSocket;

    // Extra DNS IPs to permit through DNS-leak protection (ctrld listen IP plus its plain-DNS :53
    // upstreams). These are NOT set as system resolvers; they only bypass the :53 leak block so
    // ctrld can reach them. Consumed on macOS/Linux; Windows uses setCustomDnsWhileConnected instead.
    std::vector<std::string> dnsWhitelistIps;
};

// Intent-based firewall description. The privileged helper is the sole author of the actual
// iptables/pf ruleset; the client only sends this validated intent and the helper builds the
// rules from a fixed template. This deliberately replaces the previous "client sends a raw
// iptables-restore/pfctl blob" design so a local caller that reaches the helper socket cannot
// install arbitrary root firewall rules.
struct FirewallConfig
{
    std::string connectingIp;               // VPN server IP (IPv4); helper appends /32
    std::vector<std::string> allowedIps;    // API/server IPs (IPv4); helper appends /32
    bool allowLanTraffic = false;
    bool isCustomConfig = false;
    std::string vpnInterfaceName;           // VPN adapter name (e.g. wgN/utunN), may be empty
    std::vector<unsigned int> staticPorts;  // static-IP TCP ports (macOS only; Linux ignores)
};

// Linux DNS-leak protection intent. The helper snapshots the OS-default resolvers and installs a
// drop chain blocking port 53 to every snapshotted resolver except the tunnel's own DNS servers
// (allowedDnsServers). vpnInterfaceName identifies the tunnel link so the helper excludes it from the
// resolver snapshot and skips its default route; the blocking is destination-based, with no
// interface-egress RETURN exemption (see dnsleakprotect.cpp). The helper is the sole author of the
// rules; only this validated intent crosses the IPC boundary.
struct DnsLeakProtectConfig
{
    bool enabled = false;
    std::string vpnInterfaceName;            // tunnel adapter (e.g. wgN/tunN), may be empty
    std::vector<std::string> allowedDnsServers;  // tunnel DNS servers to never blacklist (v4/v6)
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
void save(Archive &ar, const types::IpAddress &ip, const unsigned int /*version*/)
{
    uint8_t family = static_cast<uint8_t>(ip.family());
    bool valid = ip.isValid();
    ar & family;
    ar & valid;
    if (valid) {
        std::size_t sz = ip.bytesSize();
        for (std::size_t i = 0; i < sz; ++i) {
            uint8_t b = ip.bytes()[i];
            ar & b;
        }
    }
}

template<class Archive>
void load(Archive &ar, types::IpAddress &ip, const unsigned int /*version*/)
{
    uint8_t family;
    bool valid;
    ar & family;
    ar & valid;
    if (valid) {
        auto f = static_cast<types::IpAddress::Family>(family);
        std::size_t sz = (f == types::IpAddress::IPv6) ? 16 : 4;
        uint8_t buf[16] = {};
        for (std::size_t i = 0; i < sz; ++i)
            ar & buf[i];
        ip = types::IpAddress(f, buf, sz);
    } else {
        ip = types::IpAddress();
    }
}

template<class Archive>
void serialize(Archive &ar, types::IpAddress &ip, const unsigned int version)
{
    boost::serialization::split_free(ar, ip, version);
}

template<class Archive>
void serialize(Archive &ar, ADAPTER_GATEWAY_INFO &a, const unsigned int /*version*/)
{
    ar & a.adapterName;
    ar & a.adapterNameV6;
    ar & a.adapterIp;
    ar & a.adapterIpV6;
    ar & a.gatewayIp;
    ar & a.gatewayIpV6;
    ar & a.dnsServers;
    ar & a.ifIndex;
}

template<class Archive>
void serialize(Archive &ar, FirewallConfig &c, const unsigned int /*version*/)
{
    ar & c.connectingIp;
    ar & c.allowedIps;
    ar & c.allowLanTraffic;
    ar & c.isCustomConfig;
    ar & c.vpnInterfaceName;
    ar & c.staticPorts;
}

template<class Archive>
void serialize(Archive &ar, DnsLeakProtectConfig &c, const unsigned int /*version*/)
{
    ar & c.enabled;
    ar & c.vpnInterfaceName;
    ar & c.allowedDnsServers;
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

// Throws on malformed or empty input rather than swallowing it. The previous version logged and
// returned, which left args default- or partly-constructed and let the handler run on garbage
// parameters. Every platform's dispatch wraps processCommand in a try/catch (linux/server.cpp,
// windows/helper.cpp, macos/server.cpp), so a throw here aborts the command and yields an empty
// answer before the handler body executes — the handler never acts on partially-deserialized data.
template<typename... Args>
void deserializePars(const std::string &data, Args&... args)
{
    if constexpr (sizeof...(Args) > 0) {
        if (data.empty()) {
            throw std::runtime_error("deserializePars: empty payload for a command that expects parameters");
        }
        std::istringstream iss(data, std::ios::binary);
        boost::archive::binary_iarchive archive(iss, boost::archive::no_header);
        (archive >> ... >> args);
    }
}
