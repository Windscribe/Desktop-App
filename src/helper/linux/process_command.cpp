#include "process_command.h"

#include <fcntl.h>
#include <fstream>
#include <net/if.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include "../common/ovpn_mgmt_posix.h"
#include "../common/ovpn_posix.h"
#include "../common/spawn_posix.h"
#include "../common/validation_posix.h"
#include "clear_wifi_history/clear_wifi_history.h"
#include "dnsleakprotect.h"
#include "firewallcontroller.h"
#include "firewallonboot.h"
#include "installer_verifier.h"
#include "network_marks.h"
#include "split_tunneling/split_tunneling.h"
#include "tunnel_interface_names.h"
#include "utils.h"
#include "utils/log/debug_trace.h"
#include "wireguard/wireguardcontroller.h"

std::string processCommand(HelperCommand cmdId, const std::string &pars, uid_t clientUid)
{
    // executeOpenVPN is the only command that needs the caller's kernel-verified uid (to lock the
    // OpenVPN management socket to that user), so it's dispatched directly with the uid rather than
    // through kCommands (whose handlers take only the serialized parameters). Passing the uid as a
    // call argument — instead of a shared mutable static — keeps concurrent command dispatch safe: a
    // second client's command can no longer overwrite the uid mid-flight. clientUid is (uid_t)-1 when
    // the server could not determine the peer uid; executeOpenVPN treats that as fatal rather than
    // securing the socket to the wrong user.
    if (cmdId == HelperCommand::executeOpenVPN) {
        return executeOpenVPN(pars, clientUid);
    }

    const auto command = kCommands.find(cmdId);
    if (command == kCommands.end()) {
        spdlog::error("Unknown command id: {}", (int)cmdId);
        return std::string();
    }
    return (command->second)(pars);
}

std::string getHelperVersion(const std::string &pars)
{
    return serializeResult(std::string("Linux helper"));
}

std::string setSplitTunnelingSettings(const std::string &pars)
{
    WS_DEBUG_TRACE_SCOPE("setSplitTunnelingSettings");
    bool isActive, isExclude, isAllowLanTraffic;
    std::vector<std::string> files, ips, hosts;
    deserializePars(pars, isActive, isExclude, isAllowLanTraffic, files, ips, hosts);

    for (const auto &ip : ips) {
        if (!Validation::isValidIpCidr(ip)) {
            spdlog::error("setSplitTunnelingSettings: invalid IP \"{}\", rejecting", ip);
            return std::string();
        }
    }

    SplitTunneling::instance().setSplitTunnelingParams(isActive, isExclude, files, ips, hosts, isAllowLanTraffic);
    return std::string();
}

std::string sendConnectStatus(const std::string &pars)
{
    WS_DEBUG_TRACE_SCOPE("sendConnectStatus");
    ConnectStatus cs;
    deserializePars(pars, cs.isConnected, cs.protocol, cs.defaultAdapter, cs.vpnAdapter, cs.connectedIp, cs.remoteIp, cs.dnsWhitelistIps);

    // IP fields are types::IpAddress; deserialization already yields either a valid address
    // or one with valid_ == false. Downstream code handles invalid addresses gracefully,
    // so no explicit IP validation is needed here. Interface-name validation is still
    // required to guard against shell injection in code paths that pass adapter names
    // to shell commands.
    auto checkIface = [](const std::string &name, const char *label) {
        if (!name.empty() && !Validation::isValidInterfaceName(name)) {
            spdlog::error("sendConnectStatus: invalid {} \"{}\", rejecting", label, name);
            return false;
        }
        return true;
    };
    if (!checkIface(cs.defaultAdapter.adapterName, "defaultAdapter.adapterName") ||
        !checkIface(cs.defaultAdapter.adapterNameV6, "defaultAdapter.adapterNameV6") ||
        !checkIface(cs.vpnAdapter.adapterName, "vpnAdapter.adapterName") ||
        !checkIface(cs.vpnAdapter.adapterNameV6, "vpnAdapter.adapterNameV6")) {
        return serializeResult(false);
    }

    bool success = SplitTunneling::instance().setConnectParams(cs);

    // DNS-leak protection: snapshot the OS-default resolvers and (re)install the dnsleaks chain on
    // connect, remove it on disconnect. This is the uniform trigger for both WireGuard and OpenVPN —
    // cs carries the tunnel adapter name and its DNS servers for either protocol. The snapshot
    // excludes the tunnel link and its DNS servers, and the tunnel's own DNS egresses the (accepted)
    // tunnel interface, so blocking is limited to the physical link's OS-default resolvers.
    //
    // The DNS-leak result is deliberately NOT folded into `success`. This command's return is the
    // split-tunnel start result: the engine maps a false return solely to splitTunnelingStartFailed()
    // (engine.cpp), which alerts the user that split tunneling failed and disables it in the UI. A
    // DNS-leak nft failure has nothing to do with split tunneling, so reporting it through that bool
    // would show the wrong alert, wrongly disable split tunneling, and mask the real cause. Log the
    // DNS-leak failure here instead (NftablesController::run also logs the underlying nft error).
    if (cs.isConnected) {
        std::vector<std::string> tunnelDns;
        for (const auto &d : cs.vpnAdapter.dnsServers) {
            if (d.isValid()) {
                tunnelDns.push_back(d.toString());
            }
        }
        // Also allow dnsWhitelistIps (the ctrld listen IP plus its plain-DNS :53 upstreams) through
        // leak protection, so split-DNS to a legacy RFC1918 upstream isn't dropped. DnsLeakProtect::enable
        // treats allowedDnsServers as a never-blacklist set (via DnsLeakParse::removeMatching), so these
        // only widen the allow set; the drop chain for the remaining OS resolvers is unchanged.
        for (const auto &ipStr : cs.dnsWhitelistIps) {
            types::IpAddress ip(ipStr);
            if (ip.isValid()) {
                tunnelDns.push_back(ip.toString());
            }
        }
        // The physical default gateway is captured by the engine pre-VPN; pass it through rather than
        // reading the live default route (which points at the tunnel by now).
        const std::string defaultGatewayV4 = cs.defaultAdapter.gatewayIp.isValid()
                                                 ? cs.defaultAdapter.gatewayIp.toString()
                                                 : std::string();
        if (!DnsLeakProtect::enable(cs.vpnAdapter.adapterName, tunnelDns, defaultGatewayV4)) {
            spdlog::error("sendConnectStatus: DNS-leak protection enable failed; OS resolvers may stay reachable");
        }
    } else {
        if (!DnsLeakProtect::disable()) {
            spdlog::error("sendConnectStatus: DNS-leak protection disable failed; port-53 drops may remain installed");
        }
    }

    return serializeResult(success);
}

std::string changeMtu(const std::string &pars)
{
    WS_DEBUG_TRACE_SCOPE("changeMtu");
    std::string adapterName;
    int mtu;
    deserializePars(pars, adapterName, mtu);

    if (!Validation::isValidInterfaceName(adapterName)) {
        spdlog::error("Invalid interface name: {}", adapterName);
        return std::string();
    }

    spdlog::info("Change MTU: {}", mtu);
    Utils::executeCommand("ip", {"link", "set", "dev", adapterName.c_str(), "mtu", std::to_string(mtu)});
    return std::string();
}

std::string executeOpenVPN(const std::string &pars, uid_t clientUid)
{
    WS_DEBUG_TRACE_SCOPE("executeOpenVPN");
    std::string config, httpProxy, socksProxy;
    unsigned int httpPort, socksPort;
    CmdDnsManager dnsManager;
    deserializePars(pars, config, httpProxy, httpPort, socksProxy, socksPort, dnsManager);

    std::string script = Utils::getDnsScript(dnsManager);
    if (script.empty()) {
        spdlog::error("Could not find appropriate DNS manager script");
        return serializeResult(false, std::string());
    }

    // The management socket is locked to the client's user (chown + 0600, plus OpenVPN's
    // management-client-user check). Without the client's uid we cannot secure it, and the run dir is
    // world-traversable (0711), so fail rather than fall back to a world-connectable socket.
    if (clientUid == (uid_t)-1) {
        spdlog::error("executeOpenVPN: client uid unknown; refusing to start OpenVPN");
        return serializeResult(false, std::string());
    }
    std::string mgmtClientUser;
    OvpnMgmt::mgmtClientUserForUid(clientUid, mgmtClientUser);

    // The proxy address must be an IP literal: the GUI restricts it to one (NetworkingValidation::
    // isIp) and no other path sets it. Validate here too rather than trust the client — an invalid
    // value is dropped so it can't reach the OpenVPN config the helper writes as root. The proxy
    // lines are passed on the OpenVPN command line after --config, so they aren't otherwise
    // filtered. isValidIpAddress also rejects whitespace, subsuming the previous whitespace guard.
    if (!httpProxy.empty() && !Validation::isValidIpAddress(httpProxy)) {
        spdlog::warn("executeOpenVPN: invalid httpProxy, ignoring");
        httpProxy = "";
    }
    if (!socksProxy.empty() && !Validation::isValidIpAddress(socksProxy)) {
        spdlog::warn("executeOpenVPN: invalid socksProxy, ignoring");
        socksProxy = "";
    }

    // Ensure any previous OpenVPN is fully gone before writeOVPNFile recreates the shared management
    // socket and we spawn a new instance.
    OvpnMgmt::killOpenVPN();

    // OpenVPN uses a fixed device name, so a device left behind (e.g. by an OpenVPN that was killed
    // before it could close it) would make the new one fail to open. The kernel used to pick a fresh
    // tunN instead.
    if (if_nametoindex(TunnelInterfaceNames::kOpenVpn) != 0) {
        spdlog::warn("executeOpenVPN: removing leftover device {}", TunnelInterfaceNames::kOpenVpn);
        if (Utils::executeCommand("ip", {"link", "del", TunnelInterfaceNames::kOpenVpn}) != 0) {
            spdlog::error("executeOpenVPN: could not remove {}; OpenVPN will fail to open it", TunnelInterfaceNames::kOpenVpn);
        }
    }

    std::vector<std::string> extraArgs;
    if (!OVPN::writeOVPNFile(script, config, httpProxy, httpPort, socksProxy, socksPort, mgmtClientUser, extraArgs)) {
        spdlog::error("Could not write OpenVPN config");
        return serializeResult(false, std::string());
    }

    std::string ovpnPath;
    if (!Utils::locateHelperExecutable(WS_PRODUCT_NAME_LOWER "openvpn", ovpnPath)) {
        return serializeResult(false, std::string());
    }

    Spawn::Options opts;
    opts.extraEnv = {{"OPENSSL_CONF", "/dev/null"}};
    opts.cwd = WS_LINUX_INSTALL_DIR;
    // --log before --config. OpenVPN fully parses the file when it sees --config, and a
    // non-whitelisted directive is fatal. --log is what attaches the file, so it has to be parsed
    // first or that fatal exits before anything is written. A log directive in the file cannot
    // replace it: the parser allowlist rejects that name, and the redirect ignores a second --log.
    // Later arguments still override the file.
    std::vector<std::string> args = {"--mark", std::to_string(marks::kOpenVpnFwMark),
                                     "--log", OVPN::logPath(),
                                     "--config", WS_LINUX_RUN_DIR "/config.ovpn"};
    args.insert(args.end(), extraArgs.begin(), extraArgs.end());
    pid_t ovpnPid = 0;
    WS_DEBUG_TRACE_EVENT("executeOpenVPN spawn");
    if (!Spawn::spawnDetached(ovpnPath, args, opts, &ovpnPid)) {
        return serializeResult(false, std::string());
    }

    // The non-root client must be able to connect() to the management socket OpenVPN creates as
    // root. If it never appears or can't be made connectable, this connection is unusable: report
    // failure and kill the OpenVPN we just spawned so the client's retry doesn't leave an orphaned
    // process fighting over the same socket path (mirrors the Windows path).
    std::string detail;
    if (!OvpnMgmt::waitAndRelaxPerms(clientUid, ovpnPid, OVPN::logPath(), detail)) {
        OvpnMgmt::killOpenVPN();
        return serializeResult(false, detail);
    }
    return serializeResult(true, std::string());
}

std::string setOpenVpnDcoMode(const std::string &pars)
{
    WS_DEBUG_TRACE_SCOPE("setOpenVpnDcoMode");
    bool useDco;
    deserializePars(pars, useDco);
    OVPN::setUseDco(useDco);
    spdlog::info("OpenVPN DCO mode: {}", useDco);
    return std::string();
}

std::string startWireGuard(const std::string &pars)
{
    WS_DEBUG_TRACE_SCOPE("startWireGuard");
    bool isAmneziaWG = false;
    bool verboseLogging = false;
    deserializePars(pars, isAmneziaWG, verboseLogging);
    bool success = WireGuardController::instance().start(isAmneziaWG, verboseLogging);
    return serializeResult(success);
}

std::string stopWireGuard(const std::string &pars)
{
    WS_DEBUG_TRACE_SCOPE("stopWireGuard");
    bool success = WireGuardController::instance().stop();
    return serializeResult(success);
}

std::string configureWireGuard(const std::string &pars)
{
    WS_DEBUG_TRACE_SCOPE("configureWireGuard");
    std::string clientPrivateKey, clientIpAddress, clientDnsAddressList, peerPublicKey,
                peerPresharedKey, peerEndpoint, allowedIps, persistentKeepalive;
    uint16_t listenPort;
    CmdDnsManager dnsManager;
    AmneziawgConfig amneziawgConfig;
    bool disconnectOnStaleHandshake = true;
    deserializePars(pars, clientPrivateKey, clientIpAddress, clientDnsAddressList, peerPublicKey,
                    peerPresharedKey, peerEndpoint, allowedIps, listenPort, dnsManager, amneziawgConfig, persistentKeepalive,
                    disconnectOnStaleHandshake);

    bool success = false;
    if (WireGuardController::instance().isInitialized()) {
        do {
            std::vector<std::string> allowed_ips_vector = WireGuardController::instance().splitAndDeduplicateAllowedIps(allowedIps);
            if (allowed_ips_vector.size() < 1) {
                spdlog::error("WireGuard: invalid AllowedIps \"{}\"", allowedIps);
                break;
            }

            if (!Validation::validateWireGuardConfig(clientIpAddress, clientDnsAddressList, allowed_ips_vector, peerEndpoint)) {
                spdlog::error("WireGuard: IP validation failed, rejecting config");
                break;
            }

            // The fwmark is fixed so the kill-switch firewall and the WG socket mark always agree;
            // only the routing-table id is chosen dynamically (it may already be occupied).
            const uint32_t fwmark = marks::kWireGuardFwMark;
            const uint32_t routingTable = WireGuardController::instance().getRoutingTable();
            spdlog::info("Fwmark = {}, routing table = {}", fwmark, routingTable);

            if (!WireGuardController::instance().configure(clientPrivateKey,
                                                           peerPublicKey, peerPresharedKey,
                                                           peerEndpoint, allowed_ips_vector,
                                                           fwmark, listenPort, amneziawgConfig, persistentKeepalive, disconnectOnStaleHandshake)) {
                spdlog::error("WireGuard: configure() failed");
                break;
            }

            if (!WireGuardController::instance().configureDefaultRouteMonitor(peerEndpoint)) {
                spdlog::error("WireGuard: configureDefaultRouteMonitor() failed");
                break;
            }
            std::string script = Utils::getDnsScript(dnsManager);
            if (script.empty()) {
                spdlog::error("WireGuard: could not find appropriate dns manager script");
                break;
            }
            if (!WireGuardController::instance().configureAdapter(clientIpAddress,
                                                                  clientDnsAddressList,
                                                                  script,
                                                                  allowed_ips_vector,
                                                                  fwmark,
                                                                  routingTable,
                                                                  peerEndpoint)) {
                spdlog::error("WireGuard: configureAdapter() failed");
                break;
            }
            success = true;
        } while (0);
    }
    return serializeResult(success);
}

std::string getWireGuardStatus(const std::string &pars)
{
    unsigned int errorCode = 0;
    unsigned long long bytesReceived = 0, bytesTransmitted = 0;
    WireGuardServiceState state = (WireGuardServiceState)WireGuardController::instance().getStatus(&errorCode, &bytesReceived, &bytesTransmitted);
    return serializeResult(errorCode, state, bytesReceived, bytesTransmitted);
}

std::string checkFirewallState(const std::string &pars)
{
    WS_DEBUG_TRACE_SCOPE("checkFirewallState");
    return serializeResult(FirewallController::instance().enabled());
}

std::string clearFirewallRules(const std::string &pars)
{
    WS_DEBUG_TRACE_SCOPE("clearFirewallRules");
    bool isKeepPfEnabled;
    deserializePars(pars, isKeepPfEnabled);
    spdlog::debug("Clear firewall rules");
    return serializeResult(FirewallController::instance().disable());
}

std::string setFirewallRules(const std::string &pars)
{
    WS_DEBUG_TRACE_SCOPE("setFirewallRules");
    FirewallConfig config;
    deserializePars(pars, config);

    // Sanitize every token the helper will interpolate into iptables rules. A single bad entry must
    // not drop the whole firewall update (that could leave the firewall off and leak), so invalid
    // values are stripped rather than aborting: the firewall still comes up from the valid remainder
    // and the offending allow-rule simply isn't emitted. The helper never builds a rule from
    // unvalidated client input.
    Validation::sanitizeFirewallConfig(config);

    spdlog::debug("Set firewall rules");
    return serializeResult(FirewallController::instance().enable(config));
}

std::string setFirewallOnBoot(const std::string &pars)
{
    WS_DEBUG_TRACE_SCOPE("setFirewallOnBoot");
    bool enabled, allowLanTraffic;
    std::vector<std::string> ipTable;
    deserializePars(pars, enabled, allowLanTraffic, ipTable);
    spdlog::debug("Set firewall on boot: {}", enabled ? "true" : "false");
    FirewallOnBootManager::instance().setIpTable(ipTable);
    FirewallOnBootManager::instance().setEnabled(enabled, allowLanTraffic);
    return std::string();
}

std::string startStunnel(const std::string &pars)
{
    WS_DEBUG_TRACE_SCOPE("startStunnel");
    std::string hostname;
    unsigned int port, localPort;
    bool extraPadding;
    std::string customSniDomain;
    deserializePars(pars, hostname, port, localPort, extraPadding, customSniDomain);

    spdlog::debug("Starting stunnel");

    if (!Validation::isValidIpv4Address(hostname)) {
        return serializeResult(false);
    }

    std::string wstunnelPath;
    if (!Utils::locateHelperExecutable(WS_PRODUCT_NAME_LOWER "wstunnel", wstunnelPath)) {
        return serializeResult(false);
    }

    std::vector<std::string> args = {
        "--listenAddress", "127.0.0.1:" + std::to_string(localPort),
        "--remoteAddress", "https://" + hostname + ":" + std::to_string(port),
        "--logFilePath", "",
    };
    if (!customSniDomain.empty()) {
        args.push_back("-s");
        args.push_back(customSniDomain);
    }
    if (extraPadding) {
        args.push_back("--extraTlsPadding");
    }
    args.push_back("--tunnelType");
    args.push_back("2");

    Spawn::Options opts;
    opts.runAsUser = WS_PRODUCT_NAME_LOWER;
    return serializeResult(Spawn::spawnDetached(wstunnelPath, args, opts));
}

std::string startWstunnel(const std::string &pars)
{
    WS_DEBUG_TRACE_SCOPE("startWstunnel");
    std::string hostname;
    unsigned int port, localPort;
    bool extraPadding;
    std::string customSniDomain;
    deserializePars(pars, hostname, port, localPort, extraPadding, customSniDomain);

    spdlog::debug("Starting wstunnel");

    if (!Validation::isValidIpv4Address(hostname)) {
        return serializeResult(false);
    }

    std::string wstunnelPath;
    if (!Utils::locateHelperExecutable(WS_PRODUCT_NAME_LOWER "wstunnel", wstunnelPath)) {
        return serializeResult(false);
    }

    std::vector<std::string> args = {
        "--listenAddress", "127.0.0.1:" + std::to_string(localPort),
        "--remoteAddress", "wss://" + hostname + ":" + std::to_string(port) + "/tcp/127.0.0.1/1194",
        "--logFilePath", "",
    };
    if (!customSniDomain.empty()) {
        args.push_back("-s");
        args.push_back(customSniDomain);
    }
    if (extraPadding) {
        args.push_back("--extraTlsPadding");
    }

    Spawn::Options opts;
    opts.runAsUser = WS_PRODUCT_NAME_LOWER;
    return serializeResult(Spawn::spawnDetached(wstunnelPath, args, opts));
}

std::string setDnsLeakProtectEnabled(const std::string &pars)
{
    WS_DEBUG_TRACE_SCOPE("setDnsLeakProtectEnabled");
    DnsLeakProtectConfig config;
    deserializePars(pars, config);
    spdlog::debug("Set DNS leak protect: {}", config.enabled ? "enabled" : "disabled");

    if (!config.enabled) {
        if (!DnsLeakProtect::disable()) {
            spdlog::error("setDnsLeakProtectEnabled: DNS-leak protection disable failed; port-53 drops may remain installed");
        }
        return std::string();
    }

    // INVARIANT: an enable here must carry the tunnel's resolvers in config.allowedDnsServers. They are
    // the ONLY thing that keeps the tunnel's own DNS out of the drop list — DnsLeakProtect::enable
    // snapshots the OS resolvers (which on a non-tunnel link can include the tunnel resolver) and strips
    // allowedDnsServers from that snapshot; nothing else exempts them. Enabling with an EMPTY
    // allowedDnsServers would blacklist the tunnel's own DNS and break all resolution on connect.
    //
    // This is why the enable path is driven helper-side from sendConnectStatus (which has the adapter,
    // its DNS servers, and the pre-VPN gateway), NOT from the engine's HelperCommand::setDnsLeakProtectEnabled
    // entry point — Helper_linux::setDnsLeakProtectEnabled only ever sends `enabled` (it is the explicit
    // disconnect teardown and only sends false). Do NOT wire any caller to send enable=true through this
    // command without also populating allowedDnsServers; if you think you need to, use sendConnectStatus.

    // DnsLeakProtect::enable validates/canonicalizes allowedDnsServers itself (dropping invalid tokens),
    // so pass the raw list through. The primary enable path is sendConnectStatus, which supplies the
    // physical default gateway; this command carries no gateway, so pass none here.
    DnsLeakProtect::enable(config.vpnInterfaceName, config.allowedDnsServers, std::string());
    return std::string();
}

std::string setGaiIpv4PriorityEnabled(const std::string &pars)
{
    WS_DEBUG_TRACE_SCOPE("setGaiIpv4PriorityEnabled");
    bool enabled;
    deserializePars(pars, enabled);
    spdlog::debug("Set gai IPv4 priority: {}", enabled ? "enabled" : "disabled");
    Utils::executeCommand(WS_LINUX_INSTALL_DIR "/scripts/gai-ipv4-priority", {enabled ? "up" : "down"});
    return std::string();
}

std::string clearWifiHistoryData(const std::string &pars)
{
    WS_DEBUG_TRACE_SCOPE("clearWifiHistoryData");
    spdlog::debug("clearWifiHistoryData");
    bool success = ClearWiFiHistory::clear();
    return serializeResult(success);
}

std::string installerStageAndVerify(const std::string &pars)
{
    WS_DEBUG_TRACE_SCOPE("installerStageAndVerify");
    std::string srcPath;
    deserializePars(pars, srcPath);
    spdlog::debug("installerStageAndVerify: src={}", srcPath);

    auto stagedPath = InstallerVerifier::stageAndVerify(srcPath);
    if (!stagedPath) {
        return serializeResult(std::string(), false);
    }
    return serializeResult(*stagedPath, true);
}

std::string installerCleanupStaged(const std::string &pars)
{
    WS_DEBUG_TRACE_SCOPE("installerCleanupStaged");
    spdlog::debug("installerCleanupStaged");
    InstallerVerifier::cleanupStaged();
    return std::string();
}

std::string setDebugTrace(const std::string &pars)
{
    bool enabled;
    deserializePars(pars, enabled);
    log_utils::setDebugTraceEnabled(enabled);
    if (enabled) {
        spdlog::info("Debug trace enabled");
    }
    return std::string();
}
