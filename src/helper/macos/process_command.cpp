#include "process_command.h"

#include <copyfile.h>
#include <fcntl.h>
#include <filesystem>
#include <grp.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>
#include <spdlog/spdlog.h>

#include "files_manager.h"
#include "firewallcontroller.h"
#include "firewallonboot.h"
#include "ipc/helper_security.h"
#include "macspoofingonboot.h"
#include "macutils.h"
#include "ovpn.h"
#include "routes_manager/routes_manager.h"
#include "utils.h"
#include "utils/executable_signature/executable_signature.h"
#include "utils/wsscopeguard.h"
#include "wireguard/wireguardcontroller.h"
#include "../common/io_posix.h"
#include "../common/ovpn_mgmt_posix.h"
#include "../common/spawn_posix.h"
#include "../common/validation_posix.h"
#include "clear_wifi_history/clear_wifi_history.h"

std::string processCommand(HelperCommand cmdId, const std::string &pars, uid_t clientUid)
{
    // executeOpenVPN is the only command that needs the caller's kernel-verified uid (to lock the
    // OpenVPN management socket to that user), so it's dispatched directly with the uid rather than
    // through kCommands (whose handlers take only the serialized parameters). Passing the uid as a
    // call argument — instead of a shared static as implemented in the Linux helper — is what makes
    // concurrent command dispatch on the default concurrent XPC queue safe: a second client's
    // command (which we do not currently support, this is future-proofing) can no longer overwrite
    // the uid mid-flight. clientUid is (uid_t)-1 when the server could not determine the peer uid.
    // executeOpenVPN treats that as fatal rather than securing the socket to the wrong user.
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
    return serializeResult(MacUtils::bundleVersionFromPlist());
}

std::string setSplitTunnelingSettings(const std::string &pars)
{
    bool isActive, isExclude, isAllowLanTraffic;
    std::vector<std::string> files, ips, hosts;
    deserializePars(pars, isActive, isExclude, isAllowLanTraffic, files, ips, hosts);
    RoutesManager::instance().setSplitTunnelSettings(isActive, isExclude);
    return std::string();
}

std::string sendConnectStatus(const std::string &pars)
{
    ConnectStatus cs;
    deserializePars(pars, cs.isConnected, cs.protocol, cs.defaultAdapter, cs.vpnAdapter, cs.connectedIp, cs.remoteIp, cs.dnsWhitelistIps);

    // IP fields are types::IpAddress; deserialization already yields either a valid address
    // or one with valid_ == false. Downstream code (RoutesManager / FirewallController) handles
    // invalid addresses gracefully, so no explicit IP validation is needed here. Interface-name
    // validation is still required to guard against shell injection in code paths that pass
    // adapter names to shell commands (ifconfig, route, pfctl).
    auto checkIface = [](const std::string &name, const char *label) {
        if (!name.empty() && !Validation::isValidInterfaceName(name)) {
            spdlog::error("sendConnectStatus: invalid {} \"{}\", rejecting", label, name);
            return false;
        }
        return true;
    };
    if (!checkIface(cs.vpnAdapter.adapterName, "vpnAdapter.adapterName") ||
        !checkIface(cs.defaultAdapter.adapterName, "defaultAdapter.adapterName")) {
        return serializeResult(false);
    }

    // Forward the full dual-stack DNS list to the firewall pf table. setVpnDns drops
    // invalid entries internally; an empty list (disconnected, or all entries invalid) flushes
    // <windscribe_dns> back to "no DNS permitted through the firewall".
    //
    // When connected, also fold in dnsWhitelistIps (the ctrld listen IP plus its plain-DNS :53
    // upstreams) so split-DNS to a legacy RFC1918 upstream isn't dropped by leak protection. These
    // go into the allow table only; the <disallowed_dns> block for the remaining OS resolvers is
    // untouched, so no new leak surface beyond the user-configured upstream.
    std::vector<types::IpAddress> allowedDns;
    if (cs.isConnected) {
        allowedDns = cs.vpnAdapter.dnsServers;
        for (const auto &ipStr : cs.dnsWhitelistIps) {
            types::IpAddress ip(ipStr);
            if (ip.isValid()) {
                allowedDns.push_back(ip);
            }
        }
    }
    FirewallController::instance().setVpnDns(allowedDns);
    RoutesManager::instance().setConnectStatus(cs);
    return serializeResult(true);
}

std::string changeMtu(const std::string &pars)
{
    std::string adapterName;
    int mtu;
    deserializePars(pars, adapterName, mtu);

    if (!Validation::isValidInterfaceName(adapterName)) {
        spdlog::error("Invalid interface name: {}", adapterName);
        return std::string();
    }

    spdlog::info("Change MTU: {}", mtu);
    Utils::executeCommand("ifconfig", {adapterName, "mtu", std::to_string(mtu)});
    return std::string();
}

std::string executeOpenVPN(const std::string &pars, uid_t clientUid)
{
    std::string config, httpProxy, socksProxy;
    unsigned int httpPort, socksPort;
    CmdDnsManager dnsManager;
    deserializePars(pars, config, httpProxy, httpPort, socksProxy, socksPort, dnsManager);

    // The proxy address must be an IP literal: the GUI restricts it to one (NetworkingValidation::
    // isIp) and no other path sets it. Validate here too rather than trust the client — an invalid
    // value is dropped so it can't reach the OpenVPN config the helper writes as root. The proxy
    // lines are appended after OvpnDirectiveWhitelist::filterConfig, so they aren't otherwise
    // filtered. isValidIpAddress also rejects whitespace, subsuming the previous whitespace guard.
    if (!httpProxy.empty() && !Validation::isValidIpAddress(httpProxy)) {
        spdlog::warn("executeOpenVPN: invalid httpProxy, ignoring");
        httpProxy = "";
    }
    if (!socksProxy.empty() && !Validation::isValidIpAddress(socksProxy)) {
        spdlog::warn("executeOpenVPN: invalid socksProxy, ignoring");
        socksProxy = "";
    }

    // The management socket is locked to the client's user (chown + 0600, plus OpenVPN's
    // management-client-user check). Without the client's uid we cannot secure it, and the config dir
    // is world-traversable, so fail rather than fall back to a world-connectable socket.
    if (clientUid == (uid_t)-1) {
        spdlog::error("executeOpenVPN: client uid unknown; refusing to start OpenVPN");
        return serializeResult(false);
    }
    std::string mgmtClientUser;
    OvpnMgmt::mgmtClientUserForUid(clientUid, mgmtClientUser);

    // Ensure any previous OpenVPN is fully gone before writeOVPNFile recreates the shared management
    // socket and we spawn a new instance.
    OvpnMgmt::killOpenVPN();

    if (!OVPN::writeOVPNFile(MacUtils::resourcePath() + "dns.sh", config, httpProxy, httpPort, socksProxy, socksPort, mgmtClientUser)) {
        spdlog::error("Could not write OpenVPN config");
        return serializeResult(false);
    }

    std::string ovpnPath;
    if (!Utils::resolveExePath(Utils::getExePath(), WS_PRODUCT_NAME_LOWER "openvpn", ovpnPath)) {
        return serializeResult(false);
    }

    ExecutableSignature sigCheck;
    if (!sigCheck.verify(ovpnPath)) {
        spdlog::error("OpenVPN executable signature incorrect: {}", sigCheck.lastError());
        return serializeResult(false);
    }

    Spawn::Options opts;
    opts.cwd = WS_POSIX_CONFIG_DIR;
    std::vector<std::string> args = {"--config", WS_POSIX_CONFIG_DIR "/config.ovpn"};
    pid_t ovpnPid = 0;
    if (!Spawn::spawnDetached(ovpnPath, args, opts, &ovpnPid)) {
        return serializeResult(false);
    }

    // The non-root client must be able to connect() to the management socket OpenVPN creates as
    // root. If it never appears or can't be made connectable, this connection is unusable: report
    // failure and kill the OpenVPN we just spawned so the client's retry doesn't leave an orphaned
    // process fighting over the same socket path (mirrors the Windows path).
    if (!OvpnMgmt::waitAndRelaxPerms(clientUid, ovpnPid)) {
        OvpnMgmt::killOpenVPN();
        return serializeResult(false);
    }
    return serializeResult(true);
}

std::string executeTaskKill(const std::string &pars)
{
    CmdKillTarget target;
    deserializePars(pars, target);

    bool success = false;
    if (target == kTargetApp) {
        spdlog::info("Killing " WS_PRODUCT_NAME " processes");
        Utils::executeCommand("pkill", {WS_APP_EXECUTABLE_NAME});
#ifdef WS_IS_WINDSCRIBE
        Utils::executeCommand("pkill", {WS_APP_IDENTIFIER "Engine"}); // For older 1.x clients
#endif
        success = true;
    } else if (target == kTargetOpenVpn) {
        spdlog::info("Killing OpenVPN processes");
        OvpnMgmt::killOpenVPN();
        success = true;
    } else if (target == kTargetStunnel) {
        spdlog::info("Killing Stunnel processes");
        Utils::executeCommand("pkill", {"-f", WS_PRODUCT_NAME_LOWER "wstunnel"});
        success = true;
    } else if (target == kTargetWStunnel) {
        spdlog::info("Killing WStunnel processes");
        Utils::executeCommand("pkill", {"-f", WS_PRODUCT_NAME_LOWER "wstunnel"});
        success = true;
    } else if (target == kTargetWireGuard) {
        spdlog::info("Killing WireGuard processes");
        Utils::executeCommand("pkill", {"-f", WS_PRODUCT_NAME_LOWER "wireguard"});
        success = true;
    } else if (target == kTargetCtrld) {
        spdlog::info("Killing ctrld processes");
        Utils::executeCommand("pkill", {"-f", WS_PRODUCT_NAME_LOWER "ctrld"});
        success = true;
    } else {
        spdlog::error("Did not kill processes for type {}", (int)target);
        success = false;
    }
    return serializeResult(success);
}

std::string startWireGuard(const std::string &pars)
{
    bool verboseLogging = false;
    deserializePars(pars, verboseLogging);
    bool success = WireGuardController::instance().start(verboseLogging);
    return serializeResult(success);
}

std::string stopWireGuard(const std::string &pars)
{
    bool success = WireGuardController::instance().stop();
    return serializeResult(success);
}

std::string configureWireGuard(const std::string &pars)
{
    std::string clientPrivateKey, clientIpAddress, clientDnsAddressList, peerPublicKey,
                peerPresharedKey, peerEndpoint, allowedIps;
    uint16_t listenPort;
    CmdDnsManager dnsManager;
    AmneziawgConfig amneziawgConfig;
    deserializePars(pars, clientPrivateKey, clientIpAddress, clientDnsAddressList, peerPublicKey,
                    peerPresharedKey, peerEndpoint, allowedIps, listenPort, dnsManager, amneziawgConfig);

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

            if (!WireGuardController::instance().configureAdapter(clientIpAddress,
                                                                  clientDnsAddressList,
                                                                  MacUtils::resourcePath() + "/dns.sh",
                                                                  allowed_ips_vector)) {
                spdlog::error("WireGuard: configureAdapter() failed");
                break;
            }

            if (!WireGuardController::instance().configureDefaultRouteMonitor(peerEndpoint)) {
                spdlog::error("WireGuard: configureDefaultRouteMonitor() failed");
                break;
            }
            if (!WireGuardController::instance().configure(clientPrivateKey,
                                                           peerPublicKey, peerPresharedKey,
                                                           peerEndpoint, allowed_ips_vector,
                                                           listenPort, amneziawgConfig)) {
                spdlog::error("WireGuard: configureDaemon() failed");
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

std::string startCtrld(const std::string &pars)
{
    std::string upstream1, upstream2;
    std::vector<std::string> domains;
    bool isCreateLog;
    deserializePars(pars, upstream1, upstream2, domains, isCreateLog);

    // Validate URLs
    if (upstream1.empty() || Validation::normalizeAddress(upstream1).empty() || (!upstream2.empty() && Validation::normalizeAddress(upstream2).empty())) {
        spdlog::error("Invalid upstream URL(s)");
        return serializeResult(false);
    }
    for (const auto &domain: domains) {
        if (!Validation::isValidDomain(domain)) {
            spdlog::error("Invalid domain: {}", domain);
            return serializeResult(false);
        }
    }

    std::string ctrldPath;
    if (!Utils::resolveExePath(Utils::getExePath(), WS_PRODUCT_NAME_LOWER "ctrld", ctrldPath)) {
        return serializeResult(false);
    }

    ExecutableSignature sigCheck;
    if (!sigCheck.verify(ctrldPath)) {
        spdlog::error("ctrld executable signature incorrect: {}", sigCheck.lastError());
        return serializeResult(false);
    }

    std::vector<std::string> args = {"run", "--daemon", "--listen=127.0.0.1:53",
                                     "--primary_upstream=" + upstream1};
    if (!upstream2.empty()) {
        args.push_back("--secondary_upstream=" + upstream2);
        if (!domains.empty()) {
            std::string domainsStr;
            for (size_t i = 0; i < domains.size(); ++i) {
                if (i > 0) domainsStr += ",";
                domainsStr += domains[i];
            }
            args.push_back("--domains=" + domainsStr);
        }
    }
    if (isCreateLog) {
        args.push_back("--log");
        args.push_back("/Library/Logs/" WS_MAC_HELPER_BUNDLE_ID "/ctrld.log");
        args.push_back("-vv");
    }

    bool executed = Utils::executeCommand(ctrldPath, args) == 0;
    return serializeResult(executed);
}

std::string checkFirewallState(const std::string &pars)
{
    return serializeResult(FirewallController::instance().enabled());
}

std::string clearFirewallRules(const std::string &pars)
{
    bool isKeepPfEnabled;
    deserializePars(pars, isKeepPfEnabled);
    spdlog::debug("Clear firewall rules");
    return serializeResult(FirewallController::instance().disable(isKeepPfEnabled));
}

std::string setFirewallRules(const std::string &pars)
{
    FirewallConfig config;
    deserializePars(pars, config);

    // Sanitize every token the helper will interpolate into pf rules. A single bad entry must not
    // drop the whole firewall update (that could leave the firewall off and leak), so invalid
    // values are stripped rather than aborting: the firewall still comes up from the valid remainder
    // and the offending allow-rule simply isn't emitted. The helper never builds a rule from
    // unvalidated client input.
    Validation::sanitizeFirewallConfig(config);

    spdlog::debug("Set firewall rules");
    return serializeResult(FirewallController::instance().enable(config));
}

std::string getFirewallRules(const std::string &pars)
{
    CmdFirewallRulesQuery query;
    deserializePars(pars, query);
    std::string rules;
    FirewallController::instance().getRules(query, &rules);
    return serializeResult(rules);
}

std::string setFirewallOnBoot(const std::string &pars)
{
    bool enabled, allowLanTraffic;
    std::vector<std::string> ipTable;
    deserializePars(pars, enabled, allowLanTraffic, ipTable);
    spdlog::info("Set firewall on boot: {}", enabled ? "true" : "false");
    FirewallOnBootManager::instance().setIpTable(ipTable);
    FirewallOnBootManager::instance().setEnabled(enabled, allowLanTraffic);
    return std::string();
}

std::string startStunnel(const std::string &pars)
{
    std::string hostname;
    unsigned int port, localPort;
    bool extraPadding;
    std::string customSniDomain;
    deserializePars(pars, hostname, port, localPort, extraPadding, customSniDomain);

    spdlog::info("Starting stunnel");

    if (!Validation::isValidIpv4Address(hostname)) {
        return serializeResult(false);
    }

    std::string wstunnelPath;
    if (!Utils::resolveExePath(Utils::getExePath(), WS_PRODUCT_NAME_LOWER "wstunnel", wstunnelPath)) {
        return serializeResult(false);
    }

    ExecutableSignature sigCheck;
    if (!sigCheck.verify(wstunnelPath)) {
        spdlog::error("stunnel executable signature incorrect: {}", sigCheck.lastError());
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
    if (!Spawn::spawnDetached(wstunnelPath, args, opts)) {
        return serializeResult(false);
    }

    if (!Utils::isPortListening(localPort)) {
        spdlog::error("stunnel failed to bind to port {}", localPort);
        return serializeResult(false);
    }

    return serializeResult(true);
}

std::string startWstunnel(const std::string &pars)
{
    std::string hostname;
    unsigned int port, localPort;
    std::string customSniDomain;
    deserializePars(pars, hostname, port, localPort, customSniDomain);

    spdlog::info("Starting wstunnel");

    if (!Validation::isValidIpv4Address(hostname)) {
        return serializeResult(false);
    }

    std::string wstunnelPath;
    if (!Utils::resolveExePath(Utils::getExePath(), WS_PRODUCT_NAME_LOWER "wstunnel", wstunnelPath)) {
        return serializeResult(false);
    }

    ExecutableSignature sigCheck;
    if (!sigCheck.verify(wstunnelPath)) {
        spdlog::error("wstunnel executable signature incorrect: {}", sigCheck.lastError());
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

    Spawn::Options opts;
    opts.runAsUser = WS_PRODUCT_NAME_LOWER;
    if (!Spawn::spawnDetached(wstunnelPath, args, opts)) {
        return serializeResult(false);
    }

    if (!Utils::isPortListening(localPort)) {
        spdlog::error("wstunnel failed to bind to port {}", localPort);
        return serializeResult(false);
    }

    return serializeResult(true);
}

std::string setMacAddress(const std::string &pars)
{
    std::string interface, macAddress, network;
    bool isWifi;
    deserializePars(pars, interface, macAddress, network, isWifi);

    if (!Validation::isValidInterfaceName(interface)) {
        spdlog::error("Invalid interface name: {}", interface);
        return serializeResult(false);
    }

    if (!Validation::isValidMacAddress(macAddress)) {
        spdlog::error("Invalid MAC address: {}", macAddress);
        return serializeResult(false);
    }

    spdlog::info("Set mac address on {}: {}", interface, macAddress);
    Utils::executeCommand("/System/Library/PrivateFrameworks/Apple80211.framework/Resources/airport", {"-z"});
    bool success = (Utils::executeCommand("ifconfig", {interface, "ether", macAddress}) == 0);
    Utils::executeCommand("ifconfig", {interface, "up"});
    return serializeResult(success);
}

std::string setDnsScriptEnabled(const std::string &pars)
{
    bool enabled;
    deserializePars(pars, enabled);

    spdlog::info("Set DNS script: {}", enabled ? "enabled" : "disabled");
    std::string out;
    // We only handle the down case; the 'up' trigger happens elsewhere
    if (!enabled) {
        Utils::executeCommand(MacUtils::resourcePath() + "/dns.sh", {"-down"}, &out);
        spdlog::info("{}", out.c_str());
    }
    return std::string();
}

std::string enableMacSpoofingOnBoot(const std::string &pars)
{
    bool enabled;
    std::string interface, macAddress;
    deserializePars(pars, enabled, interface, macAddress);

    if (enabled) {
        if (!Validation::isValidInterfaceName(interface)) {
            spdlog::error("enableMacSpoofingOnBoot: invalid interface \"{}\", rejecting", interface);
            return std::string();
        }
        if (!Validation::isValidMacAddress(macAddress)) {
            spdlog::error("enableMacSpoofingOnBoot: invalid MAC address \"{}\", rejecting", macAddress);
            return std::string();
        }
    }

    spdlog::info("Set mac spoofing on boot: {}", enabled ? "true" : "false");
    MacSpoofingOnBootManager::instance().setEnabled(enabled, interface, macAddress);
    return std::string();
}

std::string setDnsOfDynamicStoreEntry(const std::string &pars)
{
    std::string ipAddress, networkService;
    deserializePars(pars, ipAddress, networkService);

    if (!Validation::isValidIpAddress(ipAddress)) {
        spdlog::error("setDnsOfDynamicStoreEntry: invalid DNS IP \"{}\", rejecting", ipAddress);
        return serializeResult(false);
    }
    if (!Utils::isValidDnsDynamicStoreEntry(networkService)) {
        spdlog::error("setDnsOfDynamicStoreEntry: invalid network service entry \"{}\", rejecting", networkService);
        return serializeResult(false);
    }

    return serializeResult(MacUtils::setDnsOfDynamicStoreEntry(ipAddress, networkService));
}

// Deprecated since the macOS IPv6 rewrite (Ipv6Manager removed): system-wide v6 toggling via
// networksetup is replaced by per-interface pf rules in firewallcontroller_mac. The wire-format
// slot stays so an older client (built before the rewrite) talking to a newer helper doesn't
// hit "unknown command id" — we just no-op-and-log instead.
std::string setIpv6Enabled(const std::string &pars)
{
    bool enabled;
    deserializePars(pars, enabled);
    spdlog::warn("setIpv6Enabled (enabled={}) is a no-op since the macOS IPv6 rewrite", enabled);
    return std::string();
}

std::string deleteRoute(const std::string &pars)
{
    std::string range, gateway;
    int mask;
    deserializePars(pars, range, mask, gateway);

    if (!Validation::isValidIpv4Address(range)) {
        spdlog::error("Invalid IP address range: {}", range);
        return std::string();
    }

    if (!Validation::isValidIpv4Address(gateway)) {
        spdlog::error("Invalid gateway IP address: {}", gateway);
        return std::string();
    }

    spdlog::info("Delete route: {}/{} gw {}", range, mask, gateway);
    std::stringstream str;
    str << range << "/" << mask;
    Utils::executeCommand("route", {"-n", "delete", str.str(), gateway});
    return std::string();
}

std::string removeOldInstall(const std::string &pars)
{
    std::string installPath;
    deserializePars(pars, installPath);

    // The install location is fixed; we never accept an attacker-controlled path here.
    // The IPC parameter is retained for wire compat but ignored. Legacy non-/Applications
    // installs are removed by the installer running as the user, not by the helper.
    if (!installPath.empty() && installPath != WS_MAC_APP_DIR) {
        spdlog::warn("removeOldInstall: ignoring caller-supplied path \"{}\", using \"{}\"",
                     installPath, WS_MAC_APP_DIR);
    }

    const char *binaryPath = WS_MAC_APP_DIR "/Contents/MacOS/" WS_APP_EXECUTABLE_NAME;
    if (access(binaryPath, F_OK) == 0) {
        spdlog::info("Remove old install: " WS_MAC_APP_DIR);
        std::error_code ec;
        std::filesystem::remove_all(WS_MAC_APP_DIR, ec);
        if (ec) {
            spdlog::error("Failed to remove old install: {}", ec.message());
            return serializeResult(false);
        }
        return serializeResult(true);
    } else {
        spdlog::error("Old install at " WS_MAC_APP_DIR " not removed");
        return serializeResult(false);
    }
}

static bool stageArchiveFromCallerBundle(std::string &outTempPath)
{
    // Resolve the calling installer's bundle from the validated SecCodeRef.
    // The IPC layer (HelperSecurity::isValidXpcConnection) has already enforced
    // that the caller is our installer/GUI bundle signed with our Developer ID.
    SecCodeRef secCode = HelperSecurity::currentCallerSecCode();
    if (!secCode) {
        spdlog::error("setInstallerPaths: no validated caller SecCode available");
        return false;
    }

    CFURLRef bundleURL = NULL;
    OSStatus s = SecCodeCopyPath(secCode, kSecCSDefaultFlags, &bundleURL);
    if (s != errSecSuccess || !bundleURL) {
        spdlog::error("setInstallerPaths: SecCodeCopyPath failed: {}", (int)s);
        return false;
    }
    auto bundleGuard = wsl::wsScopeGuard([bundleURL]{ CFRelease(bundleURL); });

    char bundlePath[PATH_MAX] = {0};
    if (!CFURLGetFileSystemRepresentation(bundleURL, true, (UInt8 *)bundlePath, sizeof(bundlePath))) {
        spdlog::error("setInstallerPaths: CFURLGetFileSystemRepresentation failed");
        return false;
    }

    const std::string archivePath =
        std::string(bundlePath) + "/Contents/Resources/" WS_PRODUCT_NAME_LOWER ".tar.lzma";

    int srcFd = open(archivePath.c_str(), O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    if (srcFd < 0) {
        spdlog::error("setInstallerPaths: open archive at \"{}\" failed: {}", archivePath, IO::strerror(errno));
        return false;
    }
    auto srcGuard = wsl::wsScopeGuard([srcFd]{ close(srcFd); });

    struct stat st;
    if (fstat(srcFd, &st) != 0 || !S_ISREG(st.st_mode)) {
        spdlog::error("setInstallerPaths: archive at \"{}\" is not a regular file", archivePath);
        return false;
    }

    // Stage the archive into a root-owned temp file in a root-only directory.
    // mkstemps prevents pre-creation/guess attacks on the destination.
    const char *kStageDir = "/private/var/root/.windscribe-installer";
    std::error_code ec;
    std::filesystem::create_directories(kStageDir, ec);
    if (ec) {
        spdlog::error("setInstallerPaths: create_directories(\"{}\") failed: {}", kStageDir, ec.message());
        return false;
    }
    chmod(kStageDir, S_IRWXU);

    char tmpl[] = "/private/var/root/.windscribe-installer/wsinstaller-XXXXXX.tar.lzma";
    int dstFd = mkstemps(tmpl, /* suffix length of ".tar.lzma" */ 9);
    if (dstFd < 0) {
        spdlog::error("setInstallerPaths: mkstemps failed: {}", IO::strerror(errno));
        return false;
    }
    fchmod(dstFd, S_IRUSR | S_IWUSR);

    int rc = fcopyfile(srcFd, dstFd, NULL, COPYFILE_DATA);
    fsync(dstFd);
    close(dstFd);

    if (rc != 0) {
        spdlog::error("setInstallerPaths: fcopyfile failed: {}", IO::strerror(errno));
        unlink(tmpl);
        return false;
    }

    // Re-validate the caller's signature now that the copy is complete. If the
    // bundle on disk was tampered with at any point during the copy (e.g. an
    // attacker on a writable mount swapping the archive), CodeResources hashing
    // makes that show up here and we discard the staged copy.
    if (!HelperSecurity::recheckCurrentCaller()) {
        spdlog::error("setInstallerPaths: caller signature became invalid after staging; rejecting");
        unlink(tmpl);
        return false;
    }

    outTempPath = tmpl;
    return true;
}

std::string setInstallerPaths(const std::string &pars)
{
    // Wire-format compat: deserialize and ignore. Both the install destination
    // and the archive source are derived from the helper-trusted state (a
    // hardcoded /Applications path and the verified caller bundle), never from
    // these caller-supplied strings.
    std::wstring archivePathFromIpc, installPathFromIpc;
    deserializePars(pars, archivePathFromIpc, installPathFromIpc);

    std::string stagedPath;
    if (!stageArchiveFromCallerBundle(stagedPath)) {
        return serializeResult(false);
    }
    FilesManager::instance().setFiles(new Files(stagedPath));
    return serializeResult(true);
}

std::string executeFilesStep(const std::string &pars)
{
    Files *files = FilesManager::instance().files();
    if (files) {
        bool success = (files->executeStep() == 1);
        FilesManager::instance().setFiles(nullptr);
        return serializeResult(success);
    }
    return serializeResult(false);
}

std::string createCliSymlink(const std::string &pars)
{
    uid_t uid;
    deserializePars(pars, uid);

    std::error_code err;
    if (!std::filesystem::is_directory("/usr/local/bin", err)) {
        spdlog::debug("Creating CLI symlink dir");

        if (!std::filesystem::create_directory("/usr/local/bin", err)) {
            spdlog::error("Failed to create CLI directory: {}", err.message());
            return serializeResult(false);
        }
        const struct group *grp = getgrnam("admin");
        if (grp == nullptr) {
            spdlog::error("Could not get group info");
            return serializeResult(false);
        }
        int rc = chown("/usr/local/bin", uid, grp->gr_gid);
        if (rc != 0) {
            spdlog::error("Failed to set owner: {}", IO::strerror(errno));
            return serializeResult(false);
        }
        std::filesystem::permissions("/usr/local/bin",
                                     std::filesystem::perms::owner_all |
                                         std::filesystem::perms::group_all |
                                         std::filesystem::perms::others_read | std::filesystem::perms::others_exec,
                                     err);
        if (err) {
            spdlog::error("Failed to set permissions: {}", err.message());
            return serializeResult(false);
        }
    }

    spdlog::debug("Creating CLI symlink");
    std::string filepath = Utils::getExePath() + "/../MacOS/" WS_CLI_EXECUTABLE_NAME;
    std::string sympath = "/usr/local/bin/" WS_CLI_EXECUTABLE_NAME;
    std::filesystem::remove(sympath, err);
    if (err) {
        spdlog::error("Failed to remove existing CLI symlink: {}", err.message());
        return serializeResult(false);
    }
    std::filesystem::create_symlink(filepath, sympath, err);
    if (err) {
        spdlog::error("Failed to create CLI symlink: {}", err.message());
        return serializeResult(false);
    }

    return serializeResult(true);
}

std::string clearWifiHistoryData(const std::string &pars)
{
    bool success = ClearWiFiHistory::clear();
    return serializeResult(success);
}

std::string installerStageAndVerify(const std::string &pars)
{
    std::string srcPath;
    deserializePars(pars, srcPath);

    if (srcPath.empty()) {
        spdlog::error("installerStageAndVerify: empty source path");
        return serializeResult(std::string(), false);
    }

    // Reject anything that isn't a real .app bundle. is_directory() follows symlinks, so we
    // use symlink_status() explicitly on each path component — a bundle whose Contents or
    // Contents/MacOS is a symlink would get its symlinks preserved by std::filesystem::copy
    // (with copy_symlinks) and the staged copy could escape kStageDir via the symlink target.
    // Rejecting up front gives a clearer error than waiting for signature verification to fail.
    std::error_code ec;
    auto isRealDir = [](const std::string &p) {
        std::error_code lec;
        auto st = std::filesystem::symlink_status(p, lec);
        return !lec && std::filesystem::is_directory(st) && !std::filesystem::is_symlink(st);
    };
    if (!isRealDir(srcPath) ||
        !isRealDir(srcPath + "/Contents") ||
        !isRealDir(srcPath + "/Contents/MacOS")) {
        spdlog::error("installerStageAndVerify: source \"{}\" is not a real .app bundle", srcPath);
        return serializeResult(std::string(), false);
    }

    // Staging in a root-owned directory closes the TOCTOU window: a same-user attacker
    // cannot replace the verified bundle between signature check and posix_spawn since
    // the directory is root-owned and 0711 (traverse-only) for everyone else. The
    // verification is done on the *staged* copy for the same reason.
    static const std::string kStageDir = Utils::kInstallerStageDir;
    static const std::string kStagedBundle = kStageDir + "/installer.app";

    // Clear any prior staging state — symmetric with installerCleanupStaged below.
    // Wipes the entire stage dir (not just installer.app)
    // so any debris from a previous run can't survive into a new stage.
    std::filesystem::remove_all(kStageDir, ec);

    std::filesystem::create_directories(kStageDir, ec);
    if (ec) {
        spdlog::error("installerStageAndVerify: create_directories failed: {}", ec.message());
        return serializeResult(std::string(), false);
    }
    if (chown(kStageDir.c_str(), 0, 0) != 0) {
        spdlog::error("installerStageAndVerify: chown stage dir failed: {}", IO::strerror(errno));
        return serializeResult(std::string(), false);
    }
    // 0711: root can write, everyone else can only traverse (no listing). The GUI runs as
    // the user and only needs +x on each path component to posix_spawn the inner binary;
    // it does not need to readdir() the staging dir.
    if (chmod(kStageDir.c_str(), S_IRWXU | S_IXGRP | S_IXOTH) != 0) {
        spdlog::error("installerStageAndVerify: chmod stage dir failed: {}", IO::strerror(errno));
        return serializeResult(std::string(), false);
    }

    // Pre-create the staged bundle root with root-only (0700) perms so the bundle is never
    // user-accessible during the copy or verification windows. A malicious source bundle can
    // contain hard links to root-only-readable files (e.g. another user's ssh keys); the
    // recursive copy reads those bytes as root and writes regular files into the staged
    // bundle. If the bundle were user-traversable during the verify window (which includes
    // a full re-hash of the bundle by SecStaticCodeCheckValidity), an attacker could open
    // the leaked path and read the contents. 0700 on the bundle root makes that impossible.
    if (mkdir(kStagedBundle.c_str(), S_IRWXU) != 0) {
        spdlog::error("installerStageAndVerify: mkdir staged bundle failed: {}", IO::strerror(errno));
        std::filesystem::remove_all(kStagedBundle, ec);
        return serializeResult(std::string(), false);
    }

    // copy_symlinks (rather than the default follow-symlinks) is important: app bundles
    // contain internal symlinks (notably inside .framework Versions) that must be preserved
    // as symlinks rather than dereferenced into duplicate copies. With a pre-existing dst
    // directory, std::filesystem::copy descends into srcPath and copies the contents into
    // our 0700-locked kStagedBundle.
    std::filesystem::copy(srcPath, kStagedBundle,
                          std::filesystem::copy_options::recursive |
                          std::filesystem::copy_options::copy_symlinks,
                          ec);
    if (ec) {
        spdlog::error("installerStageAndVerify: copy failed: {}", ec.message());
        std::filesystem::remove_all(kStagedBundle, ec);
        return serializeResult(std::string(), false);
    }

    // Belt-and-braces: even if libcxx's copy explicitly chmod()s the bundle root from the
    // source's mode, re-lock to 0700 before verification.
    if (chmod(kStagedBundle.c_str(), S_IRWXU) != 0) {
        spdlog::error("installerStageAndVerify: chmod staged bundle failed: {}", IO::strerror(errno));
        std::filesystem::remove_all(kStagedBundle, ec);
        return serializeResult(std::string(), false);
    }

    ExecutableSignature sigCheck;
    if (!sigCheck.verify(kStagedBundle)) {
        spdlog::error("installerStageAndVerify: signature verify failed: {}", sigCheck.lastError());
        std::filesystem::remove_all(kStagedBundle, ec);
        return serializeResult(std::string(), false);
    }

    // Verification succeeded — the bundle is the genuine Windscribe installer. Widen perms
    // so the GUI (running as the user) can posix_spawn the inner Mach-O. Walk the *destination*
    // tree (root-owned, no race with the user-writable source) and apply each entry's source
    // mode with go-w stripped. If the source disappeared meanwhile, fall back to sensible
    // defaults — the dest is root-owned regardless, so we still control everything.
    auto stripGoWrite = [](std::filesystem::perms p) {
        return p & ~(std::filesystem::perms::group_write | std::filesystem::perms::others_write);
    };
    auto applyFinalMode = [&](const std::filesystem::path &src, const std::filesystem::path &dst) {
        std::error_code permEc;
        auto dstStatus = std::filesystem::symlink_status(dst, permEc);
        if (permEc || std::filesystem::is_symlink(dstStatus)) {
            return;
        }
        std::filesystem::perms perms;
        auto srcStatus = std::filesystem::symlink_status(src, permEc);
        if (!permEc && !std::filesystem::is_symlink(srcStatus)) {
            perms = srcStatus.permissions();
        } else if (std::filesystem::is_directory(dstStatus)) {
            perms = std::filesystem::perms::owner_all
                  | std::filesystem::perms::group_read | std::filesystem::perms::group_exec
                  | std::filesystem::perms::others_read | std::filesystem::perms::others_exec;
        } else {
            perms = std::filesystem::perms::owner_read | std::filesystem::perms::owner_write
                  | std::filesystem::perms::group_read
                  | std::filesystem::perms::others_read;
        }
        std::filesystem::permissions(dst, stripGoWrite(perms),
                                     std::filesystem::perm_options::replace, permEc);
    };
    applyFinalMode(srcPath, kStagedBundle);
    for (auto it = std::filesystem::recursive_directory_iterator(kStagedBundle, ec);
         !ec && it != std::filesystem::recursive_directory_iterator(); it.increment(ec)) {
        applyFinalMode(std::filesystem::path(srcPath) / it->path().lexically_relative(kStagedBundle),
                       it->path());
    }

    return serializeResult(kStagedBundle, true);
}

std::string installerCleanupStaged(const std::string &pars)
{
    std::error_code ec;
    std::filesystem::remove_all(Utils::kInstallerStageDir, ec);
    if (ec) {
        spdlog::info("installerCleanupStaged: {}", ec.message());
    }
    return std::string();
}
