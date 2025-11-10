#include "process_command.h"

#include <codecvt>
#include <fcntl.h>
#include <filesystem>
#include <grp.h>
#include <pwd.h>
#include <stdint.h>
#include <spdlog/spdlog.h>

#include "execute_cmd.h"
#include "files_manager.h"
#include "firewallcontroller.h"
#include "firewallonboot.h"
#include "ipv6/ipv6manager.h"
#include "macspoofingonboot.h"
#include "macutils.h"
#include "ovpn.h"
#include "routes_manager/routes_manager.h"
#include "utils.h"
#include "utils/executable_signature/executable_signature.h"
#include "wireguard/wireguardcontroller.h"

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
    return serializeResult(MacUtils::bundleVersionFromPlist());
}

std::string getUnblockingCmdStatus(const std::string &pars)
{
    unsigned long cmdId;
    deserializePars(pars, cmdId);

    bool bFinished;
    std::string log;
    ExecuteCmd::instance().getStatus(cmdId, bFinished, log);
    return serializeResult(bFinished, log);
}

std::string clearUnblockingCmd(const std::string &pars)
{
    unsigned long cmdId;
    deserializePars(pars, cmdId);
    ExecuteCmd::instance().clearCmds();
    return std::string();
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
    deserializePars(pars, cs.isConnected, cs.protocol, cs.defaultAdapter, cs.vpnAdapter, cs.connectedIp, cs.remoteIp);
    FirewallController::instance().setVpnDns(cs.isConnected ? cs.vpnAdapter.dnsServers.front() : "");
    RoutesManager::instance().setConnectStatus(cs);
    return serializeResult(true);
}

std::string changeMtu(const std::string &pars)
{
    std::string adapterName;
    int mtu;
    deserializePars(pars, adapterName, mtu);

    if (!Utils::isValidInterfaceName(adapterName)) {
        spdlog::error("Invalid interface name: {}", adapterName);
        return std::string();
    }

    spdlog::info("Change MTU: {}", mtu);
    Utils::executeCommand("ifconfig", {adapterName, "mtu", std::to_string(mtu)});
    return std::string();
}

std::string executeOpenVPN(const std::string &pars)
{
    std::string config, httpProxy, socksProxy;
    unsigned int port, httpPort, socksPort;
    bool isCustomConfig;
    CmdDnsManager dnsManager;
    deserializePars(pars, config, port, httpProxy, httpPort, socksProxy, socksPort, isCustomConfig, dnsManager);

    unsigned long cmdId = 0;
    // sanitize
    if (Utils::hasWhitespaceInString(httpProxy) ||
        Utils::hasWhitespaceInString(socksProxy))
    {
        httpProxy = "";
        socksProxy = "";
    }

    if (!OVPN::writeOVPNFile(MacUtils::resourcePath() + "dns.sh", port, config, httpProxy, httpPort, socksProxy, socksPort, isCustomConfig)) {
        spdlog::error("Could not write OpenVPN config");
        return serializeResult(false, cmdId);
    }

    std::string fullCmd = Utils::getFullCommand(Utils::getExePath(), "windscribeopenvpn", "--config /etc/windscribe/config.ovpn");
    if (fullCmd.empty()) {
        // Something wrong with the command
        return serializeResult(false, cmdId);
    }

    const std::string fullPath = Utils::getExePath() + "/windscribeopenvpn";
    ExecutableSignature sigCheck;
    if (!sigCheck.verify(fullPath)) {
        spdlog::error("OpenVPN executable signature incorrect: {}", sigCheck.lastError());
        return serializeResult(false, cmdId);
    } else {
        cmdId = ExecuteCmd::instance().execute(fullCmd, "/etc/windscribe");
        return serializeResult(true, cmdId);
    }
}

std::string executeTaskKill(const std::string &pars)
{
    CmdKillTarget target;
    deserializePars(pars, target);

    bool success = false;
    if (target == kTargetWindscribe) {
        spdlog::info("Killing Windscribe processes");
        Utils::executeCommand("pkill", {"Windscribe"});
        Utils::executeCommand("pkill", {"WindscribeEngine"}); // For older 1.x clients
        success = true;
    } else if (target == kTargetOpenVpn) {
        spdlog::info("Killing OpenVPN processes");
        const std::vector<std::string> exes = Utils::getOpenVpnExeNames();
        for (auto exe : exes) {
            Utils::executeCommand("pkill", {"-f", exe.c_str()});
        }
        success = true;
    } else if (target == kTargetStunnel) {
        spdlog::info("Killing Stunnel processes");
        Utils::executeCommand("pkill", {"-f", "windscribewstunnel"});
        success = true;
    } else if (target == kTargetWStunnel) {
        spdlog::info("Killing WStunnel processes");
        Utils::executeCommand("pkill", {"-f", "windscribewstunnel"});
        success = true;
    } else if (target == kTargetWireGuard) {
        spdlog::info("Killing WireGuard processes");
        Utils::executeCommand("pkill", {"-f", "windscribewireguard"});
        success = true;
    } else if (target == kTargetCtrld) {
        spdlog::info("Killing ctrld processes");
        Utils::executeCommand("pkill", {"-f", "windscribectrld"});
        success = true;
    } else {
        spdlog::error("Did not kill processes for type {}", (int)target);
        success = false;
    }
    return serializeResult(success);
}

std::string startWireGuard(const std::string &pars)
{
    bool success = WireGuardController::instance().start();
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
    deserializePars(pars, clientPrivateKey, clientIpAddress, clientDnsAddressList, peerPublicKey,
                    peerPresharedKey, peerEndpoint, allowedIps, listenPort, dnsManager);

    bool success = false;
    if (WireGuardController::instance().isInitialized()) {
        do {
            std::vector<std::string> allowed_ips_vector = WireGuardController::instance().splitAndDeduplicateAllowedIps(allowedIps);
            if (allowed_ips_vector.size() < 1) {
                spdlog::error("WireGuard: invalid AllowedIps \"{}\"", allowedIps);
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
                                                           listenPort)) {
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
    if (upstream1.empty() || Utils::normalizeAddress(upstream1).empty() || (!upstream2.empty() && Utils::normalizeAddress(upstream2).empty())) {
        spdlog::error("Invalid upstream URL(s)");
        return serializeResult(false);
    }
    for (const auto &domain: domains) {
        if (!Utils::isValidDomain(domain)) {
            spdlog::error("Invalid domain: {}", domain);
            return serializeResult(false);
        }
    }

    std::stringstream arguments;
    arguments << "run";
    arguments << " --daemon";
    arguments << " --listen=127.0.0.1:53";
    arguments << " --primary_upstream=" + upstream1;
    if (!upstream2.empty()) {
        arguments << " --secondary_upstream=" + upstream2;
        if (!domains.empty()) {
            std::stringstream domainsStream;
            std::copy(domains.begin(), domains.end(), std::ostream_iterator<std::string>(domainsStream, ","));

            std::string domainsStr = domainsStream.str();
            domainsStr.erase(domainsStr.length() - 1); // remove last comma

            arguments << " --domains=" + domainsStr;
        }
    }
    if (isCreateLog) {
        arguments << " --log /Library/Logs/com.windscribe.helper.macos/ctrld.log";
        arguments << " -vv";
    }

    std::string fullCmd = Utils::getFullCommand(Utils::getExePath(), "windscribectrld", arguments.str());
    if (fullCmd.empty()) {
        // Something wrong with the command
        return serializeResult(false);
    }

    const std::string fullPath = Utils::getExePath() + "/windscribectrld";
    ExecutableSignature sigCheck;
    if (!sigCheck.verify(fullPath)) {
        spdlog::error("ctrld executable signature incorrect: {}", sigCheck.lastError());
        return serializeResult(false);
    } else {
        bool executed = Utils::executeCommand(fullCmd) == 0;
        return serializeResult(executed);
    }
}

std::string checkFirewallState(const std::string &pars)
{
    std::string tag;
    deserializePars(pars, tag);
    return serializeResult(FirewallController::instance().enabled());
}

std::string clearFirewallRules(const std::string &pars)
{
    bool isKeepPfEnabled;
    deserializePars(pars, isKeepPfEnabled);
    spdlog::debug("Clear firewall rules");
    FirewallController::instance().disable(isKeepPfEnabled);
    return std::string();
}

std::string setFirewallRules(const std::string &pars)
{
    CmdIpVersion ipVersion;
    std::string table, group, rules;
    deserializePars(pars, ipVersion, table, group, rules);

    spdlog::debug("Set firewall rules");
    FirewallController::instance().enable(rules, table, group);
    return std::string();
}

std::string getFirewallRules(const std::string &pars)
{
    CmdIpVersion ipVersion;
    std::string table, group;
    deserializePars(pars, ipVersion, table, group);
    std::string rules;
    FirewallController::instance().getRules(table, group, &rules);
    return serializeResult(rules);
}

std::string setFirewallOnBoot(const std::string &pars)
{
    bool enabled, allowLanTraffic;
    std::string ipTable;
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
    deserializePars(pars, hostname, port, localPort, extraPadding);

    spdlog::info("Starting stunnel");

    if (!Utils::isValidIpAddress(hostname)) {
        return serializeResult(false);
    }

    std::stringstream arguments;
    arguments << "--listenAddress :" << localPort;
    arguments << " --remoteAddress https://" << hostname << ":" << port;
    arguments << " --logFilePath \"\"";
    if (extraPadding) {
        arguments << " --extraTlsPadding";
    }
    arguments << " --tunnelType 2";
    //arguments << " --dev"; // enables verbose logging when necessary
    std::string fullCmd = Utils::getFullCommandAsUser("windscribe", Utils::getExePath(), "windscribewstunnel", arguments.str());
    if (fullCmd.empty()) {
        // Something wrong with the command
        return serializeResult(false);
    }

    const std::string fullPath = Utils::getExePath() + "/windscribewstunnel";
    ExecutableSignature sigCheck;
    if (!sigCheck.verify(fullPath)) {
        spdlog::error("stunnel executable signature incorrect: {}", sigCheck.lastError());
        return serializeResult(false);
    } else {
        ExecuteCmd::instance().execute(fullCmd, std::string(), true);
        return serializeResult(true);
    }
}

std::string startWstunnel(const std::string &pars)
{
    std::string hostname;
    unsigned int port, localPort;
    deserializePars(pars, hostname, port, localPort);

    spdlog::info("Starting wstunnel");

    if (!Utils::isValidIpAddress(hostname)) {
        return serializeResult(false);
    }

    std::stringstream arguments;
    arguments << "--listenAddress :" << localPort;
    arguments << " --remoteAddress wss://" << hostname << ":" << port << "/tcp/127.0.0.1/1194";
    arguments << " --logFilePath \"\"";
    //arguments << " --dev"; // enables verbose logging when necessary
    std::string fullCmd = Utils::getFullCommandAsUser("windscribe", Utils::getExePath(), "windscribewstunnel", arguments.str());
    if (fullCmd.empty()) {
        // Something wrong with the command
        return serializeResult(false);
    }

    const std::string fullPath = Utils::getExePath() + "/windscribewstunnel";
    ExecutableSignature sigCheck;
    if (!sigCheck.verify(fullPath)) {
        spdlog::error("wstunnel executable signature incorrect: {}", sigCheck.lastError());
        return serializeResult(false);
    } else {
        ExecuteCmd::instance().execute(fullCmd, std::string(), true);
        return serializeResult(true);
    }
}

std::string setMacAddress(const std::string &pars)
{
    std::string interface, macAddress, network;
    bool isWifi;
    deserializePars(pars, interface, macAddress, network, isWifi);

    if (!Utils::isValidInterfaceName(interface)) {
        spdlog::error("Invalid interface name: {}", interface);
        return serializeResult(false);
    }

    if (!Utils::isValidMacAddress(macAddress)) {
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

    spdlog::info("Set mac spoofing on boot: {}", enabled ? "true" : "false");
    MacSpoofingOnBootManager::instance().setEnabled(enabled, interface, macAddress);
    return std::string();
}

std::string setDnsOfDynamicStoreEntry(const std::string &pars)
{
    std::string ipAddress, networkService;
    deserializePars(pars, ipAddress, networkService);
    return serializeResult(MacUtils::setDnsOfDynamicStoreEntry(ipAddress, networkService));
}

std::string setIpv6Enabled(const std::string &pars)
{
    bool enabled;
    deserializePars(pars, enabled);
    spdlog::debug("Set IPv6: {}", enabled ? "enabled" : "disabled");
    Ipv6Manager::instance().setEnabled(enabled);
    return std::string();
}

std::string deleteRoute(const std::string &pars)
{
    std::string range, gateway;
    int mask;
    deserializePars(pars, range, mask, gateway);

    if (!Utils::isValidIpAddress(range)) {
        spdlog::error("Invalid IP address range: {}", range);
        return std::string();
    }

    if (!Utils::isValidIpAddress(gateway)) {
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

    // sanity check that path at least contains our binary.  We can't assume this path is in /Applications
    // because versions < 2.6 allowed custom dir installs.  This check at least disallows user to try to
    // delete paths that they can't write to.
    std::stringstream path;
    path << installPath << "/Contents/MacOS/Windscribe";

    if (access(path.str().c_str(), F_OK) == 0) {
        spdlog::info("Remove old install: {}", installPath);
        Utils::executeCommand("rm", {"-rf", installPath});
        return serializeResult(true);
    } else {
        spdlog::error("Old install at {} not removed", installPath);
        return serializeResult(false);
    }
}

std::string setInstallerPaths(const std::string &pars)
{
    std::wstring archivePath, installPath;
    deserializePars(pars, archivePath, installPath);
    FilesManager::instance().setFiles(new Files(archivePath, installPath));
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
            spdlog::error("Failed to set owner: {}", strerror(errno));
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
    std::string filepath = Utils::getExePath() + "/../MacOS/windscribe-cli";
    std::string sympath = "/usr/local/bin/windscribe-cli";
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
