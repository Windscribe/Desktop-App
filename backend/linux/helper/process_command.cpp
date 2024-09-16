#include "process_command.h"

#include <codecvt>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include "execute_cmd.h"
#include "firewallcontroller.h"
#include "firewallonboot.h"
#include "logger.h"
#include "ovpn.h"
#include "routes_manager/routes_manager.h"
#include "split_tunneling/split_tunneling.h"
#include "utils.h"
#include "utils/executable_signature/executable_signature.h"
#include "wireguard/wireguardcontroller.h"

CMD_ANSWER processCommand(int cmdId, const std::string packet)
{
    const auto command = kCommands.find(cmdId);
    if (command == kCommands.end()) {
        Logger::instance().out("Unknown command id: %d", cmdId);
        return CMD_ANSWER();
    }

    std::istringstream stream(packet);
    boost::archive::text_iarchive ia(stream, boost::archive::no_header);

    return (command->second)(ia);
}

CMD_ANSWER startOpenvpn(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    CMD_START_OPENVPN cmd;
    ia >> cmd;

    std::string script = Utils::getDnsScript(cmd.dnsManager);
    if (script.empty()) {
        Logger::instance().out("Could not find appropriate DNS manager script");
        answer.executed = 0;
        return answer;
    }

    // sanitize
    if (Utils::hasWhitespaceInString(cmd.httpProxy) ||
        Utils::hasWhitespaceInString(cmd.socksProxy))
    {
        cmd.httpProxy = "";
        cmd.socksProxy = "";
    }

    if (!OVPN::writeOVPNFile(script, cmd.port, cmd.config, cmd.httpProxy, cmd.httpPort, cmd.socksProxy, cmd.socksPort, cmd.isCustomConfig)) {
        Logger::instance().out("Could not write OpenVPN config");
        answer.executed = 0;
        return answer;
    }

    std::string fullCmd = Utils::getFullCommand(Utils::getExePath(), "windscribeopenvpn", "--config /etc/windscribe/config.ovpn");
    if (fullCmd.empty()) {
        // Something wrong with the command
        answer.executed = 0;
        return answer;
    }

    const std::string fullPath = Utils::getExePath() + "/windscribeopenvpn";
    ExecutableSignature sigCheck;
    if (!sigCheck.verify(fullPath)) {
        Logger::instance().out("OpenVPN executable signature incorrect: %s", sigCheck.lastError().c_str());
        answer.executed = 0;
    } else {
        answer.cmdId = ExecuteCmd::instance().execute(fullCmd, "/etc/windscribe");
        answer.executed = 1;
    }
    return answer;
}

CMD_ANSWER getCmdStatus(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    CMD_GET_CMD_STATUS cmd;
    ia >> cmd;

    bool bFinished;
    std::string log;
    ExecuteCmd::instance().getStatus(cmd.cmdId, bFinished, log);

    if (bFinished) {
        answer.executed = 1;
        answer.body = log;
    } else {
        answer.executed = 2;
    }
    return answer;
}

CMD_ANSWER clearCmds(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    CMD_CLEAR_CMDS cmd;
    ia >> cmd;

    ExecuteCmd::instance().clearCmds();
    answer.executed = 2;

    return answer;
}

CMD_ANSWER splitTunnelingSettings(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    CMD_SPLIT_TUNNELING_SETTINGS cmd;
    ia >> cmd;

    SplitTunneling::instance().setSplitTunnelingParams(cmd.isActive, cmd.isExclude, cmd.files, cmd.ips, cmd.hosts, cmd.isAllowLanTraffic);
    answer.executed = 1;

    return answer;
}

CMD_ANSWER sendConnectStatus(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    CMD_SEND_CONNECT_STATUS cmd;
    ia >> cmd;

    if (SplitTunneling::instance().setConnectParams(cmd)) {
        answer.executed = 0;
    } else {
        answer.executed = 1;
    }

    return answer;
}

CMD_ANSWER startWireGuard(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;

    if (WireGuardController::instance().start()) {
        answer.executed = 1;
    }
    return answer;
}

CMD_ANSWER stopWireGuard(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    if (WireGuardController::instance().stop()) {
        answer.executed = 1;
    }
    return answer;
}

CMD_ANSWER configureWireGuard(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    CMD_CONFIGURE_WIREGUARD cmd;
    ia >> cmd;

    answer.executed = 0;
    if (WireGuardController::instance().isInitialized()) {
        do {
            std::vector<std::string> allowed_ips_vector = WireGuardController::instance().splitAndDeduplicateAllowedIps(cmd.allowedIps);
            if (allowed_ips_vector.size() < 1) {
                Logger::instance().out("WireGuard: invalid AllowedIps \"%s\"", cmd.allowedIps.c_str());
                break;
            }

            uint32_t fwmark = WireGuardController::instance().getFwmark();
            Logger::instance().out("Fwmark = %u", fwmark);

            if (!WireGuardController::instance().configure(cmd.clientPrivateKey,
                                                cmd.peerPublicKey, cmd.peerPresharedKey,
                                                cmd.peerEndpoint, allowed_ips_vector,
                                                fwmark, cmd.listenPort)) {
                Logger::instance().out("WireGuard: configure() failed");
                break;
            }

            if (!WireGuardController::instance().configureDefaultRouteMonitor(cmd.peerEndpoint)) {
                Logger::instance().out("WireGuard: configureDefaultRouteMonitor() failed");
                break;
            }
            std::string script = Utils::getDnsScript(cmd.dnsManager);
            if (script.empty()) {
                Logger::instance().out("WireGuard: could not find appropriate dns manager script");
                break;
            }
            if (!WireGuardController::instance().configureAdapter(cmd.clientIpAddress,
                                                       cmd.clientDnsAddressList,
                                                       script,
                                                       allowed_ips_vector, fwmark)) {
                Logger::instance().out("WireGuard: configureAdapter() failed");
                break;
            }

            answer.executed = 1;
        } while (0);
    }
    return answer;
}

CMD_ANSWER getWireGuardStatus(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    unsigned int errorCode = 0;
    unsigned long long bytesReceived = 0, bytesTransmitted = 0;

    answer.executed = 1;
    answer.cmdId = WireGuardController::instance().getStatus(&errorCode, &bytesReceived, &bytesTransmitted);
    if (answer.cmdId == kWgStateError) {
        if (errorCode) {
            answer.customInfoValue[0] = errorCode;
        } else {
            answer.customInfoValue[0] = -1;
        }
    } else if (answer.cmdId == kWgStateActive) {
        answer.customInfoValue[0] = bytesReceived;
        answer.customInfoValue[1] = bytesTransmitted;
    }
    return answer;
}

CMD_ANSWER changeMtu(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    CMD_CHANGE_MTU cmd;
    ia >> cmd;
    Logger::instance().out("Change MTU: %d", cmd.mtu);

    answer.executed = 1;
    Utils::executeCommand("ip", {"link", "set", "dev", cmd.adapterName.c_str(), "mtu", std::to_string(cmd.mtu)});
    return answer;
}

CMD_ANSWER setDnsLeakProtectEnabled(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    CMD_SET_DNS_LEAK_PROTECT_ENABLED cmd;
    ia >> cmd;
    Logger::instance().out("Set DNS leak protect: %s", cmd.enabled ? "enabled" : "disabled");

    // We only handle the down case; the 'up' trigger for this script happens in the DNS manager script
    if (!cmd.enabled) {
        Utils::executeCommand("/etc/windscribe/dns-leak-protect", {"down"});
        answer.executed = 1;
    }
    return answer;
}

CMD_ANSWER clearFirewallRules(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    CMD_CLEAR_FIREWALL_RULES cmd;
    ia >> cmd;
    Logger::instance().out("Clear firewall rules");
    FirewallController::instance().disable();
    answer.executed = 1;
    return answer;
}

CMD_ANSWER checkFirewallState(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    CMD_CHECK_FIREWALL_STATE cmd;
    ia >> cmd;

    answer.executed = 1;
    answer.exitCode = FirewallController::instance().enabled(cmd.tag) ? 1 : 0;
    return answer;
}

CMD_ANSWER setFirewallRules(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    CMD_SET_FIREWALL_RULES cmd;
    ia >> cmd;
    Logger::instance().out("Set firewall rules");

    answer.executed = 1;
    answer.exitCode = FirewallController::instance().enable(cmd.ipVersion == kIpv6, cmd.rules) ? 1 : 0;

    return answer;
}

CMD_ANSWER getFirewallRules(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    CMD_GET_FIREWALL_RULES cmd;
    ia >> cmd;

    FirewallController::instance().getRules(cmd.ipVersion == kIpv6, &answer.body);
    answer.executed = 1;
    return answer;
}

CMD_ANSWER setFirewallOnBoot(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    CMD_SET_FIREWALL_ON_BOOT cmd;
    ia >> cmd;
    Logger::instance().out("Set firewall on boot: %s", cmd.enabled ? "true" : "false");

    FirewallOnBootManager::instance().setIpTable(cmd.ipTable);
    answer.executed = FirewallOnBootManager::instance().setEnabled(cmd.enabled, cmd.allowLanTraffic);

    return answer;
}

CMD_ANSWER taskKill(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    CMD_TASK_KILL cmd;
    ia >> cmd;

    if (cmd.target == kTargetWindscribe) {
        Logger::instance().out("Killing Windscribe processes");
        Utils::executeCommand("pkill", {"Windscribe"});
        Utils::executeCommand("pkill", {"WindscribeEngine"}); // For older 1.x clients
        answer.executed = 1;
    } else if (cmd.target == kTargetOpenVpn) {
        Logger::instance().out("Killing OpenVPN processes");
        const std::vector<std::string> exes = Utils::getOpenVpnExeNames();
        for (auto exe : exes) {
            Utils::executeCommand("pkill", {"-f", exe.c_str()});
        }
        answer.executed = 1;
    } else if (cmd.target == kTargetStunnel) {
        Logger::instance().out("Killing Stunnel processes");
        Utils::executeCommand("pkill", {"-f", "windscribewstunnel"});
        answer.executed = 1;
    } else if (cmd.target == kTargetWStunnel) {
        Logger::instance().out("Killing WStunnel processes");
        Utils::executeCommand("pkill", {"-f", "windscribewstunnel"});
        answer.executed = 1;
    } else if (cmd.target == kTargetWireGuard) {
        Logger::instance().out("Killing WireGuard processes");
        Utils::executeCommand("pkill", {"-f", "windscribewireguard"});
        answer.executed = 1;
    } else if (cmd.target == kTargetCtrld) {
        Logger::instance().out("Killing ctrld processes");
        Utils::executeCommand("pkill", {"-f", "windscribectrld"});
        answer.executed = 1;
    } else {
        Logger::instance().out("Did not kill processes for type %d", cmd.target);
        answer.executed = 0;
    }

    return answer;
}

CMD_ANSWER startCtrld(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    CMD_START_CTRLD cmd;
    ia >> cmd;

    Logger::instance().out("Starting ctrld");

    // Validate URLs
    if (cmd.upstream1.empty() || Utils::normalizeAddress(cmd.upstream1).empty() || (!cmd.upstream2.empty() && Utils::normalizeAddress(cmd.upstream2).empty())) {
        Logger::instance().out("Invalid upstream URL(s)");
        answer.executed = 0;
        return answer;
    }
    for (const auto domain: cmd.domains) {
        if (!Utils::isValidDomain(domain)) {
            Logger::instance().out("Invalid domain: %s", domain.c_str());
            answer.executed = 0;
            return answer;
        }
    }

    std::stringstream arguments;
    arguments << "run";
    arguments << " --daemon";
    arguments << " --listen=127.0.0.1:53";
    arguments << " --primary_upstream=" + cmd.upstream1;
    if (!cmd.upstream2.empty()) {
        arguments << " --secondary_upstream=" + cmd.upstream2;
        if (!cmd.domains.empty()) {
            std::stringstream domains;
            std::copy(cmd.domains.begin(), cmd.domains.end(), std::ostream_iterator<std::string>(domains, ","));

            std::string domainsStr = domains.str();
            domainsStr.erase(domainsStr.length() - 1); // remove last comma

            arguments << " --domains=" + domainsStr;
        }
    }
    if (cmd.isCreateLog) {
        arguments << " --log /var/log/windscribe/ctrld.log";
        arguments << " -vv";
    }

    std::string fullCmd = Utils::getFullCommand(Utils::getExePath(), "windscribectrld", arguments.str());
    if (fullCmd.empty()) {
        // Something wrong with the command
        answer.executed = 0;
        return answer;
    }

    const std::string fullPath = Utils::getExePath() + "/windscribectrld";
    ExecutableSignature sigCheck;
    if (!sigCheck.verify(fullPath)) {
        Logger::instance().out("ctrld executable signature incorrect: %s", sigCheck.lastError().c_str());
        answer.executed = 0;
    } else {
        answer.executed = Utils::executeCommand(fullCmd) ? 0 : 1;
    }
    return answer;
}

CMD_ANSWER startStunnel(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    CMD_START_STUNNEL cmd;
    ia >> cmd;
    Logger::instance().out("Starting stunnel");

    if (!Utils::isValidIpAddress(cmd.hostname)) {
        answer.executed = 0;
        return answer;
    }

    std::stringstream arguments;
    arguments << "--listenAddress :" << cmd.localPort;
    arguments << " --remoteAddress https://" << cmd.hostname << ":" << cmd.port;
    arguments << " --logFilePath \"\"";
    if (cmd.extraPadding) {
        arguments << " --extraTlsPadding";
    }
    arguments << " --tunnelType 2";
    //arguments << " --dev"; // enables verbose logging when necessary
    std::string fullCmd = Utils::getFullCommandAsUser("windscribe", Utils::getExePath(), "windscribewstunnel", arguments.str());
    if (fullCmd.empty()) {
        // Something wrong with the command
        answer.executed = 0;
        return answer;
    }

    const std::string fullPath = Utils::getExePath() + "/windscribewstunnel";
    ExecutableSignature sigCheck;
    if (!sigCheck.verify(fullPath)) {
        Logger::instance().out("stunnel executable signature incorrect: %s", sigCheck.lastError().c_str());
        answer.executed = 0;
    } else {
        answer.cmdId = ExecuteCmd::instance().execute(fullCmd, std::string(), true);
        answer.executed = 1;
    }
    return answer;
}

CMD_ANSWER startWstunnel(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    CMD_START_WSTUNNEL cmd;
    ia >> cmd;
    Logger::instance().out("Starting wstunnel");

    if (!Utils::isValidIpAddress(cmd.hostname)) {
        answer.executed = 0;
        return answer;
    }

    std::stringstream arguments;
    arguments << "--listenAddress :" << cmd.localPort;
    arguments << " --remoteAddress wss://" << cmd.hostname << ":" << cmd.port << "/tcp/127.0.0.1/1194";
    arguments << " --logFilePath \"\"";
    //arguments << " --dev"; // enables verbose logging when necessary
    std::string fullCmd = Utils::getFullCommandAsUser("windscribe", Utils::getExePath(), "windscribewstunnel", arguments.str());
    if (fullCmd.empty()) {
        // Something wrong with the command
        answer.executed = 0;
        return answer;
    }

    const std::string fullPath = Utils::getExePath() + "/windscribewstunnel";
    ExecutableSignature sigCheck;
    if (!sigCheck.verify(fullPath)) {
        Logger::instance().out("wstunnel executable signature incorrect: %s", sigCheck.lastError().c_str());
        answer.executed = 0;
    } else {
        answer.cmdId = ExecuteCmd::instance().execute(fullCmd, std::string(), true);
        answer.executed = 1;
    }
    return answer;
}
