#include "process_command.h"

#include <codecvt>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include "execute_cmd.h"
#include "firewallcontroller.h"
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
    } else {
        if (!OVPN::writeOVPNFile(script, cmd.config, cmd.isCustomConfig)) {
            Logger::instance().out("Could not write OpenVPN config");
            answer.executed = 0;
        } else {
            std::string fullCmd = Utils::getFullCommand(cmd.exePath, cmd.executable, "--config /etc/windscribe/config.ovpn " + cmd.arguments);
            if (fullCmd.empty()) {
                // Something wrong with the command
                answer.executed = 0;
            } else {
                const std::string fullPath = cmd.exePath + "/" + cmd.executable;
                ExecutableSignature sigCheck;
                if (!sigCheck.verify(std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(fullPath))) {
                    Logger::instance().out("OpenVPN executable signature incorrect: %s", sigCheck.lastError().c_str());
                    answer.executed = 0;
                } else {
                    answer.cmdId = ExecuteCmd::instance().execute(fullCmd, "/etc/windscribe");
                    answer.executed = 1;
                }
            }
        }
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

    SplitTunneling::instance().setSplitTunnelingParams(cmd.isActive, cmd.isExclude, cmd.files, cmd.ips, cmd.hosts);
    answer.executed = 1;

    return answer;
}

CMD_ANSWER sendConnectStatus(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    CMD_SEND_CONNECT_STATUS cmd;
    ia >> cmd;

    SplitTunneling::instance().setConnectParams(cmd);
    answer.executed = 1;

    return answer;
}

CMD_ANSWER startWireGuard(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    CMD_START_WIREGUARD cmd;
    ia >> cmd;

    if (WireGuardController::instance().start(cmd.exePath, cmd.executable, cmd.deviceName)) {
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
                                                cmd.peerEndpoint, allowed_ips_vector, fwmark)) {
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

CMD_ANSWER checkForWireGuardKernelModule(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    answer.executed = Utils::executeCommand("modprobe", {"wireguard"}) ? 0 : 1;
    Logger::instance().out("WireGuard kernel module: %s", answer.executed ? "available" : "not available");
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

    std::string fullCmd = Utils::getFullCommand(cmd.exePath, cmd.executable, cmd.parameters);
    if (fullCmd.empty()) {
        // Something wrong with the command
        answer.executed = 0;
    } else {
        const std::string fullPath = cmd.exePath + "/" + cmd.executable;
        ExecutableSignature sigCheck;
        if (!sigCheck.verify(std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(fullPath))) {
            Logger::instance().out("ctrld executable signature incorrect: %s", sigCheck.lastError().c_str());
            answer.executed = 0;
        } else {
            answer.cmdId = ExecuteCmd::instance().execute(fullCmd, std::string());
            answer.executed = 1;
        }
    }
    return answer;
}

CMD_ANSWER startStunnel(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    CMD_START_STUNNEL cmd;
    ia >> cmd;
    Logger::instance().out("Starting stunnel");

    std::stringstream arguments;
    arguments << "--listenAddress :" << cmd.localPort;
    arguments << " --remoteAddress https://" << cmd.hostname << ":" << cmd.port;
    arguments << " --logFilePath \"\"";
    if (cmd.extraPadding) {
        arguments << " --extraTlsPadding";
    }
    arguments << " --tunnelType 2";
    //arguments << " --dev"; // enables verbose logging when necessary
    std::string fullCmd = Utils::getFullCommandAsUser("windscribe", cmd.exePath, cmd.executable, arguments.str());
    if (fullCmd.empty()) {
        // Something wrong with the command
        answer.executed = 0;
    } else {
        const std::string fullPath = cmd.exePath + "/" + cmd.executable;
        ExecutableSignature sigCheck;
        if (!sigCheck.verify(std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(fullPath))) {
            Logger::instance().out("stunnel executable signature incorrect: %s", sigCheck.lastError().c_str());
            answer.executed = 0;
        } else {
            answer.cmdId = ExecuteCmd::instance().execute(fullCmd, std::string());
            answer.executed = 1;
        }
    }
    return answer;
}

CMD_ANSWER startWstunnel(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    CMD_START_WSTUNNEL cmd;
    ia >> cmd;
    Logger::instance().out("Starting wstunnel");

    std::stringstream arguments;
    arguments << "--listenAddress :" << cmd.localPort;
    arguments << " --remoteAddress wss://" << cmd.hostname << ":" << cmd.port << "/tcp/127.0.0.1/1194";
    arguments << " --logFilePath \"\"";
    //arguments << " --dev"; // enables verbose logging when necessary
    std::string fullCmd = Utils::getFullCommandAsUser("windscribe", cmd.exePath, cmd.executable, arguments.str());
    if (fullCmd.empty()) {
        // Something wrong with the command
        answer.executed = 0;
    } else {
        const std::string fullPath = cmd.exePath + "/" + cmd.executable;
        ExecutableSignature sigCheck;
        if (!sigCheck.verify(std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(fullPath))) {
            Logger::instance().out("wstunnel executable signature incorrect: %s", sigCheck.lastError().c_str());
            answer.executed = 0;
        } else {
            answer.cmdId = ExecuteCmd::instance().execute(fullCmd, std::string());
            answer.executed = 1;
        }
    }
    return answer;
}
