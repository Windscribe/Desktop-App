#include "process_command.h"

#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <spdlog/spdlog.h>
#include "clear_wifi_history/clear_wifi_history.h"
#include "firewallcontroller.h"
#include "firewallonboot.h"
#include "installer_verifier.h"
#include "ovpn.h"
#include "split_tunneling/split_tunneling.h"
#include "utils.h"
#include "wireguard/wireguardcontroller.h"
#include "../common/spawn_posix.h"
#include "../common/validation_posix.h"

std::string processCommand(HelperCommand cmdId, const std::string &pars)
{
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
    ConnectStatus cs;
    deserializePars(pars, cs.isConnected, cs.protocol, cs.defaultAdapter, cs.vpnAdapter, cs.connectedIp, cs.remoteIp);

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
    return serializeResult(success);
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
    Utils::executeCommand("ip", {"link", "set", "dev", adapterName.c_str(), "mtu", std::to_string(mtu)});
    return std::string();
}

std::string executeOpenVPN(const std::string &pars)
{
    std::string config, httpProxy, socksProxy;
    unsigned int port, httpPort, socksPort;
    CmdDnsManager dnsManager;
    deserializePars(pars, config, port, httpProxy, httpPort, socksProxy, socksPort, dnsManager);

    std::string script = Utils::getDnsScript(dnsManager);
    if (script.empty()) {
        spdlog::error("Could not find appropriate DNS manager script");
        return serializeResult(false);
    }

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

    if (!OVPN::writeOVPNFile(script, port, config, httpProxy, httpPort, socksProxy, socksPort)) {
        spdlog::error("Could not write OpenVPN config");
        return serializeResult(false);
    }

    std::string ovpnPath;
    if (!Utils::resolveExePath(Utils::getExePath(), WS_PRODUCT_NAME_LOWER "openvpn", ovpnPath)) {
        return serializeResult(false);
    }

    Spawn::Options opts;
    opts.extraEnv = {{"OPENSSL_CONF", "/dev/null"}};
    opts.cwd = WS_LINUX_INSTALL_DIR;
    std::vector<std::string> args = {"--mark", "20310", "--config", WS_LINUX_RUN_DIR "/config.ovpn"};
    return serializeResult(Spawn::spawnDetached(ovpnPath, args, opts));
}

std::string setOpenVpnDcoMode(const std::string &pars)
{
    bool useDco;
    deserializePars(pars, useDco);
    OVPN::setUseDco(useDco);
    spdlog::info("OpenVPN DCO mode: {}", useDco);
    return std::string();
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
        const std::vector<std::string> exes = Utils::getOpenVpnExeNames();
        for (const auto &exe : exes) {
            Utils::executeCommand("pkill", {"-f", exe.c_str()});
        }
        OVPN::setUseDco(true);
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
    bool isAmneziaWG = false;
    bool verboseLogging = false;
    deserializePars(pars, isAmneziaWG, verboseLogging);
    bool success = WireGuardController::instance().start(isAmneziaWG, verboseLogging);
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

            uint32_t fwmark = WireGuardController::instance().getFwmark();
            spdlog::info("Fwmark = {}", fwmark);

            if (!WireGuardController::instance().configure(clientPrivateKey,
                                                           peerPublicKey, peerPresharedKey,
                                                           peerEndpoint, allowed_ips_vector,
                                                           fwmark, listenPort, amneziawgConfig)) {
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
                                                                  allowed_ips_vector, fwmark)) {
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

std::string startCtrld(const std::string &pars)
{
    std::string upstream1, upstream2;
    std::vector<std::string> domains;
    bool isCreateLog;
    deserializePars(pars, upstream1, upstream2, domains, isCreateLog);


    spdlog::debug("Starting ctrld");

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
        args.push_back(WS_LINUX_LOG_DIR "/ctrld.log");
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
    return serializeResult(FirewallController::instance().disable());
}

std::string setFirewallRules(const std::string &pars)
{
    FirewallConfig config;
    deserializePars(pars, config);

    // Sanitize every token the helper will interpolate into iptables rules. A single bad entry must
    // not drop the whole kill-switch update (that could leave the firewall off and leak), so invalid
    // values are stripped rather than aborting: the firewall still comes up from the valid remainder
    // and the offending allow-rule simply isn't emitted. The helper never builds a rule from
    // unvalidated client input.
    Validation::sanitizeFirewallConfig(config);

    spdlog::debug("Set firewall rules");
    return serializeResult(FirewallController::instance().enable(config));
}

std::string setFirewallOnBoot(const std::string &pars)
{
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
    std::string hostname;
    unsigned int port, localPort;
    bool extraPadding;
    deserializePars(pars, hostname, port, localPort, extraPadding);

    spdlog::debug("Starting stunnel");

    if (!Validation::isValidIpv4Address(hostname)) {
        return serializeResult(false);
    }

    std::string wstunnelPath;
    if (!Utils::resolveExePath(Utils::getExePath(), WS_PRODUCT_NAME_LOWER "wstunnel", wstunnelPath)) {
        return serializeResult(false);
    }

    std::vector<std::string> args = {
        "--listenAddress", "127.0.0.1:" + std::to_string(localPort),
        "--remoteAddress", "https://" + hostname + ":" + std::to_string(port),
        "--logFilePath", "",
    };
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
    std::string hostname;
    unsigned int port, localPort;
    deserializePars(pars, hostname, port, localPort);

    spdlog::debug("Starting wstunnel");

    if (!Validation::isValidIpv4Address(hostname)) {
        return serializeResult(false);
    }

    std::string wstunnelPath;
    if (!Utils::resolveExePath(Utils::getExePath(), WS_PRODUCT_NAME_LOWER "wstunnel", wstunnelPath)) {
        return serializeResult(false);
    }

    std::vector<std::string> args = {
        "--listenAddress", "127.0.0.1:" + std::to_string(localPort),
        "--remoteAddress", "wss://" + hostname + ":" + std::to_string(port) + "/tcp/127.0.0.1/1194",
        "--logFilePath", "",
    };

    Spawn::Options opts;
    opts.runAsUser = WS_PRODUCT_NAME_LOWER;
    return serializeResult(Spawn::spawnDetached(wstunnelPath, args, opts));
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

    // network is passed to nmcli; validating here also guards the resetMacAddresses() call below.
    if (!Validation::isValidNetworkManagerConnectionName(network)) {
        spdlog::error("Invalid network connection name: {}", network);
        return serializeResult(false);
    }

    std::string mac =
        macAddress.substr(0, 2) + ":" +
        macAddress.substr(2, 2) + ":" +
        macAddress.substr(4, 2) + ":" +
        macAddress.substr(6, 2) + ":" +
        macAddress.substr(8, 2) + ":" +
        macAddress.substr(10, 2);

    spdlog::debug("Set MAC address on {} ({} - {}): {}", interface, network, (isWifi ? "wifi" : "ethernet"),  mac);

    // reset addresses on other networks
    Utils::resetMacAddresses(network);

#ifdef CLI_ONLY
    // Must bring interface down to change the MAC address
    Utils::executeCommand("ip", {"link", "set", "dev", interface, "down"});
    Utils::executeCommand("ip", {"link", "set", "dev", interface, "address", mac});
    Utils::executeCommand("ip", {"link", "set", "dev", interface, "up"});
#else
    std::string out;
    // Pass the connection name with an explicit "id" selector so nmcli always treats it as a name,
    // never as one of its own selector keywords (id/uuid/path) that would otherwise consume the
    // following token. isValidNetworkManagerConnectionName already blocks a leading '-'; this closes
    // the same argv-semantics gap for a connection literally named "id"/"uuid"/"path".
    if (isWifi) {
        Utils::executeCommand("nmcli", {"connection", "modify", "id", network, "wifi.cloned-mac-address", mac}, &out);
    } else {
        Utils::executeCommand("nmcli", {"connection", "modify", "id", network, "ethernet.cloned-mac-address", mac}, &out);
    }
    // restart the connection
    Utils::executeCommand("nmcli", {"connection", "up", "id", network});
#endif
    return serializeResult(true);
}

std::string setDnsLeakProtectEnabled(const std::string &pars)
{
    bool enabled;
    deserializePars(pars, enabled);
    spdlog::debug("Set DNS leak protect: {}", enabled ? "enabled" : "disabled");
    // We only handle the down case; the 'up' trigger (chain install) happens in
    // the DNS manager scripts at connect time. OUTPUT rule ordering on a late
    // firewall enable is handled by FirewallController::outputJumpRule when the
    // helper builds the rules, so there is no helper-driven replay.
    if (!enabled) {
        Utils::executeCommand(WS_LINUX_INSTALL_DIR "/scripts/dns-leak-protect", {"down"});
    }
    return std::string();
}

std::string setGaiIpv4PriorityEnabled(const std::string &pars)
{
    bool enabled;
    deserializePars(pars, enabled);
    spdlog::debug("Set gai IPv4 priority: {}", enabled ? "enabled" : "disabled");
    Utils::executeCommand(WS_LINUX_INSTALL_DIR "/scripts/gai-ipv4-priority", {enabled ? "up" : "down"});
    return std::string();
}

std::string resetMacAddresses(const std::string &pars)
{
    std::string ignoreNetwork;
    deserializePars(pars, ignoreNetwork);
    spdlog::debug("Resetting MAC addresses");
    Utils::resetMacAddresses(ignoreNetwork);
    return std::string();
}

std::string clearWifiHistoryData(const std::string &pars)
{
    spdlog::debug("clearWifiHistoryData");
    bool success = ClearWiFiHistory::clear();
    return serializeResult(success);
}

std::string installerStageAndVerify(const std::string &pars)
{
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
    spdlog::debug("installerCleanupStaged");
    InstallerVerifier::cleanupStaged();
    return std::string();
}
