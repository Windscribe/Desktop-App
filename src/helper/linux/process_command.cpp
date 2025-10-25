#include "process_command.h"

#include <codecvt>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <spdlog/spdlog.h>
#include "execute_cmd.h"
#include "firewallcontroller.h"
#include "firewallonboot.h"
#include "ovpn.h"
#include "split_tunneling/split_tunneling.h"
#include "utils.h"
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
    return serializeResult(std::string("Linux helper"));
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
    SplitTunneling::instance().setSplitTunnelingParams(isActive, isExclude, files, ips, hosts, isAllowLanTraffic);
    return std::string();
}

std::string sendConnectStatus(const std::string &pars)
{
    ConnectStatus cs;
    deserializePars(pars, cs.isConnected, cs.protocol, cs.defaultAdapter, cs.vpnAdapter, cs.connectedIp, cs.remoteIp);
    bool success = !SplitTunneling::instance().setConnectParams(cs);
    return serializeResult(success);
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
    Utils::executeCommand("ip", {"link", "set", "dev", adapterName.c_str(), "mtu", std::to_string(mtu)});
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
    std::string script = Utils::getDnsScript(dnsManager);
    if (script.empty()) {
        spdlog::error("Could not find appropriate DNS manager script");
        return serializeResult(false, cmdId);
    }

    // sanitize
    if (Utils::hasWhitespaceInString(httpProxy) ||
        Utils::hasWhitespaceInString(socksProxy))
    {
        httpProxy = "";
        socksProxy = "";
    }

    if (!OVPN::writeOVPNFile(script, port, config, httpProxy, httpPort, socksProxy, socksPort, isCustomConfig)) {
        spdlog::error("Could not write OpenVPN config");
        return serializeResult(false, cmdId);
    }

    std::string fullCmd = Utils::getFullCommand(Utils::getExePath(), "windscribeopenvpn", "--config /var/run/windscribe/config.ovpn");
    if (fullCmd.empty()) {
        // Something wrong with the command
        return serializeResult(false, cmdId);
    }

    cmdId = ExecuteCmd::instance().execute(fullCmd, "/opt/windscribe");
    return serializeResult(true, cmdId);
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

            uint32_t fwmark = WireGuardController::instance().getFwmark();
            spdlog::info("Fwmark = {}", fwmark);

            if (!WireGuardController::instance().configure(clientPrivateKey,
                                                           peerPublicKey, peerPresharedKey,
                                                           peerEndpoint, allowed_ips_vector,
                                                           fwmark, listenPort)) {
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
        arguments << " --log /var/log/windscribe/ctrld.log";
        arguments << " -vv";
    }

    std::string fullCmd = Utils::getFullCommand(Utils::getExePath(), "windscribectrld", arguments.str());
    if (fullCmd.empty()) {
        // Something wrong with the command
        return serializeResult(false);
    }

    bool executed = Utils::executeCommand(fullCmd) == 0;
    return serializeResult(executed);
}

std::string checkFirewallState(const std::string &pars)
{
    std::string tag;
    deserializePars(pars, tag);
    return serializeResult(FirewallController::instance().enabled(tag));
}

std::string clearFirewallRules(const std::string &pars)
{
    bool isKeepPfEnabled;
    deserializePars(pars, isKeepPfEnabled);
    spdlog::debug("Clear firewall rules");
    FirewallController::instance().disable();
    return std::string();
}

std::string setFirewallRules(const std::string &pars)
{
    CmdIpVersion ipVersion;
    std::string table, group, rules;
    deserializePars(pars, ipVersion, table, group, rules);
    spdlog::debug("Set firewall rules");
    FirewallController::instance().enable(ipVersion == kIpv6, rules);
    return std::string();
}

std::string getFirewallRules(const std::string &pars)
{
    CmdIpVersion ipVersion;
    std::string table, group;
    deserializePars(pars, ipVersion, table, group);
    std::string rules;
    FirewallController::instance().getRules(ipVersion == kIpv6, &rules);
    return serializeResult(rules);
}

std::string setFirewallOnBoot(const std::string &pars)
{
    bool enabled, allowLanTraffic;
    std::string ipTable;
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

    ExecuteCmd::instance().execute(fullCmd, std::string(), true);
    return serializeResult(true);
}

std::string startWstunnel(const std::string &pars)
{
    std::string hostname;
    unsigned int port, localPort;
    deserializePars(pars, hostname, port, localPort);

    spdlog::debug("Starting wstunnel");

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

    ExecuteCmd::instance().execute(fullCmd, std::string(), true);
    return serializeResult(true);
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
    if (isWifi) {
        Utils::executeCommand("nmcli", {"connection", "modify", network, "wifi.cloned-mac-address", mac}, &out);
    } else {
        Utils::executeCommand("nmcli", {"connection", "modify", network, "ethernet.cloned-mac-address", mac}, &out);
    }
    // restart the connection
    Utils::executeCommand("nmcli", {"connection", "up", network});
#endif
    return serializeResult(true);
}

std::string setDnsLeakProtectEnabled(const std::string &pars)
{
    bool enabled;
    deserializePars(pars, enabled);
    spdlog::debug("Set DNS leak protect: {}", enabled ? "enabled" : "disabled");
    // We only handle the down case; the 'up' trigger for this script happens in the DNS manager script
    if (!enabled) {
        Utils::executeCommand("/opt/windscribe/scripts/dns-leak-protect", {"down"});
    }
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
