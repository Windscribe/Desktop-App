#include "process_command.h"

#include <codecvt>
#include <fcntl.h>
#include <filesystem>
#include <grp.h>
#include <pwd.h>
#include <sstream>

#include "execute_cmd.h"
#include "files_manager.h"
#include "firewallcontroller.h"
#include "firewallonboot.h"
#include "ipv6/ipv6manager.h"
#include "logger.h"
#include "macspoofingonboot.h"
#include "macutils.h"
#include "ovpn.h"
#include "split_tunneling/split_tunneling.h"
#include "utils.h"
#include "utils/executable_signature/executable_signature.h"
#include "wireguard/wireguardcontroller.h"

CMD_ANSWER processCommand(int cmdId, const std::string &packet)
{
    const auto command = kCommands.find(cmdId);
    if (command == kCommands.end()) {
        LOG("Unknown command id: %d", cmdId);
        return CMD_ANSWER();
    }

    std::istringstream stream(packet);
    boost::archive::text_iarchive ia(stream, boost::archive::no_header);

    return (command->second)(ia);
}

CMD_ANSWER startOpenvpn(boost::archive::text_iarchive &ia)
{
    CMD_START_OPENVPN cmd;
    CMD_ANSWER answer;
    ia >> cmd;

    // sanitize
    if (Utils::hasWhitespaceInString(cmd.httpProxy) ||
        Utils::hasWhitespaceInString(cmd.socksProxy))
    {
        cmd.httpProxy = "";
        cmd.socksProxy = "";
    }

    if (!OVPN::writeOVPNFile(MacUtils::resourcePath() + "dns.sh", cmd.port, cmd.config, cmd.httpProxy, cmd.httpPort, cmd.socksProxy, cmd.socksPort, cmd.isCustomConfig)) {
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
        LOG("OpenVPN executable signature incorrect: %s", sigCheck.lastError().c_str());
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

    if (WireGuardController::instance().start()) {
        answer.executed = 1;
    } else {
        answer.executed = 0;
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
                LOG("WireGuard: invalid AllowedIps \"%s\"", cmd.allowedIps.c_str());
                break;
            }

            if (!WireGuardController::instance().configureAdapter(cmd.clientIpAddress,
                                                       cmd.clientDnsAddressList,
                                                       MacUtils::resourcePath() + "/dns.sh",
                                                       allowed_ips_vector)) {
                LOG("WireGuard: configureAdapter() failed");
                break;
            }

            if (!WireGuardController::instance().configureDefaultRouteMonitor(cmd.peerEndpoint)) {
                LOG("WireGuard: configureDefaultRouteMonitor() failed");
                break;
            }
            if (!WireGuardController::instance().configure(cmd.clientPrivateKey,
                                                cmd.peerPublicKey, cmd.peerPresharedKey,
                                                cmd.peerEndpoint, allowed_ips_vector,
                                                cmd.listenPort)) {
                LOG("WireGuard: configureDaemon() failed");
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

CMD_ANSWER installerSetPath(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    CMD_INSTALLER_FILES_SET_PATH cmd;
    ia >> cmd;

    FilesManager::instance().setFiles(new Files(cmd.archivePath, cmd.installPath));
    answer.executed = 1;

    return answer;
}

CMD_ANSWER installerExecuteCopyFile(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    Files *files = FilesManager::instance().files();

    if (files) {
        answer.executed = files->executeStep();
        if (answer.executed == -1) {
            answer.body = files->getLastError();
        }

        FilesManager::instance().setFiles(nullptr);
    }

    return answer;
}

CMD_ANSWER installerRemoveOldInstall(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    CMD_INSTALLER_REMOVE_OLD_INSTALL cmd;
    ia >> cmd;

    // sanity check that path at least contains our binary.  We can't assume this path is in /Applications
    // because versions < 2.6 allowed custom dir installs.  This check at least disallows user to try to
    // delete paths that they can't write to.
    std::stringstream path;
    path << cmd.path << "/Contents/MacOS/Windscribe";

    if (access(path.str().c_str(), F_OK) == 0) {
        LOG("Remove old install: %s", cmd.path.c_str());
        Utils::executeCommand("rm", {"-rf", cmd.path.c_str()});
        answer.executed = 1;
    } else {
        LOG("Old install at %s not removed", cmd.path.c_str());
        answer.executed = 0;
    }

    return answer;
}


CMD_ANSWER applyCustomDns(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    CMD_APPLY_CUSTOM_DNS cmd;
    ia >> cmd;

    if (MacUtils::setDnsOfDynamicStoreEntry(cmd.ipAddress, cmd.networkService)) {
        answer.executed = 1;
    } else {
        answer.executed = 0;
    }
    return answer;
}

CMD_ANSWER changeMtu(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    CMD_CHANGE_MTU cmd;
    ia >> cmd;
    LOG("Change MTU: %d", cmd.mtu);

    answer.executed = 1;
    Utils::executeCommand("ifconfig", {cmd.adapterName.c_str(), "mtu", std::to_string(cmd.mtu)});

    return answer;
}

CMD_ANSWER deleteRoute(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    CMD_DELETE_ROUTE cmd;
    ia >> cmd;
    LOG("Delete route: %s/%d gw %s", cmd.range.c_str(), cmd.mask, cmd.gateway.c_str());

    answer.executed = 1;
    std::stringstream str;
    str << cmd.range << "/" << cmd.mask;
    Utils::executeCommand("route", {"-n", "delete", str.str().c_str(), cmd.gateway.c_str()});

    return answer;
}

CMD_ANSWER setIpv6Enabled(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    CMD_SET_IPV6_ENABLED cmd;
    ia >> cmd;
    LOG("Set IPv6: %s", cmd.enabled ? "enabled" : "disabled");

    answer.executed = Ipv6Manager::instance().setEnabled(cmd.enabled);
    return answer;
}

CMD_ANSWER setDnsScriptEnabled(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    CMD_SET_DNS_SCRIPT_ENABLED cmd;
    ia >> cmd;
    LOG("Set DNS script: %s", cmd.enabled ? "enabled" : "disabled");

    answer.executed = 1;
    std::string out;

    // We only handle the down case; the 'up' trigger happens elsewhere
    if (!cmd.enabled) {
        Utils::executeCommand(MacUtils::resourcePath() + "/dns.sh", {"-down"}, &out);
        LOG("%s", out.c_str());
    }

    return answer;
}

CMD_ANSWER clearFirewallRules(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    CMD_CLEAR_FIREWALL_RULES cmd;
    ia >> cmd;
    LOG("Clear firewall rules");

    FirewallController::instance().disable(cmd.isKeepPfEnabled);
    answer.executed = 1;

    return answer;
}

CMD_ANSWER checkFirewallState(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    answer.exitCode = FirewallController::instance().enabled();
    answer.executed = 1;

    return answer;
}

CMD_ANSWER setFirewallRules(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    LOG("Set firewall rules");
    CMD_SET_FIREWALL_RULES cmd;
    ia >> cmd;

    if (FirewallController::instance().enable(cmd.rules, cmd.table, cmd.group)) {
        answer.executed = 1;
    } else {
        answer.executed = 0;
    }
    return answer;
}

CMD_ANSWER getFirewallRules(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    CMD_GET_FIREWALL_RULES cmd;
    ia >> cmd;

    FirewallController::instance().getRules(cmd.table, cmd.group, &answer.body);
    answer.executed = 1;

    return answer;
}

CMD_ANSWER deleteOldHelper(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    LOG("Delete old helper");
    Utils::executeCommand("rm", {"-f", "/Library/PrivilegedHelperTools/com.windscribe.helper.macos"});
    Utils::executeCommand("rm", {"-f", "/Library/Logs/com.windscribe.helper.macos/helper_log.txt"});

    // remove helper from version 1
    Utils::executeCommand("launchctl", {"unload", "/Library/LaunchDaemons/com.aaa.windscribe.OVPNHelper.plist"});
    Utils::executeCommand("rm", {"-f", "/Library/LaunchDaemons/com.aaa.windscribe.OVPNHelper.plist"});
    Utils::executeCommand("rm", {"-f", "/Library/PrivilegedHelperTools/com.aaa.windscribe.OVPNHelper"});

    answer.executed = 1;
    return answer;
}

CMD_ANSWER setFirewallOnBoot(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    CMD_SET_FIREWALL_ON_BOOT cmd;
    ia >> cmd;
    LOG("Set firewall on boot: %s", cmd.enabled ? "true" : "false");

    FirewallOnBootManager::instance().setIpTable(cmd.ipTable);
    answer.executed = FirewallOnBootManager::instance().setEnabled(cmd.enabled, cmd.allowLanTraffic);

    return answer;
}

CMD_ANSWER setMacSpoofingOnBoot(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    CMD_SET_MAC_SPOOFING_ON_BOOT cmd;
    ia >> cmd;
    LOG("Set mac spoofing on boot: %s", cmd.enabled ? "true" : "false");

    answer.executed = MacSpoofingOnBootManager::instance().setEnabled(cmd.enabled, cmd.interface, cmd.macAddress);
    return answer;
}

CMD_ANSWER setMacAddress(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    CMD_SET_MAC_ADDRESS cmd;
    ia >> cmd;
    LOG("Set mac address on %s: %s", cmd.interface.c_str(), cmd.macAddress.c_str());

    Utils::executeCommand("/System/Library/PrivateFrameworks/Apple80211.framework/Resources/airport", {"-z"});
    answer.executed = Utils::executeCommand("ifconfig", {cmd.interface.c_str(), "ether", cmd.macAddress.c_str()});
    Utils::executeCommand("ifconfig", {cmd.interface.c_str(), "up"});
    return answer;
}

CMD_ANSWER taskKill(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    CMD_TASK_KILL cmd;
    ia >> cmd;

    if (cmd.target == kTargetWindscribe) {
        LOG("Killing Windscribe processes");
        Utils::executeCommand("pkill", {"Windscribe"});
        Utils::executeCommand("pkill", {"WindscribeEngine"}); // For older 1.x clients
        answer.executed = 1;
    } else if (cmd.target == kTargetOpenVpn) {
        LOG("Killing OpenVPN processes");
        const std::vector<std::string> exes = Utils::getOpenVpnExeNames();
        for (auto exe : exes) {
            Utils::executeCommand("pkill", {"-f", exe.c_str()});
        }
        answer.executed = 1;
    } else if (cmd.target == kTargetStunnel) {
        LOG("Killing Stunnel processes");
        Utils::executeCommand("pkill", {"-f", "windscribewstunnel"});
        answer.executed = 1;
    } else if (cmd.target == kTargetWStunnel) {
        LOG("Killing WStunnel processes");
        Utils::executeCommand("pkill", {"-f", "windscribewstunnel"});
        answer.executed = 1;
    } else if (cmd.target == kTargetWireGuard) {
        LOG("Killing WireGuard processes");
        Utils::executeCommand("pkill", {"-f", "windscribewireguard"});
        answer.executed = 1;
    } else if (cmd.target == kTargetCtrld) {
        LOG("Killing ctrld processes");
        Utils::executeCommand("pkill", {"-f", "windscribectrld"});
        answer.executed = 1;
    } else {
        LOG("Did not kill processes for type %d", cmd.target);
        answer.executed = 0;
    }

    return answer;
}

CMD_ANSWER startCtrld(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    CMD_START_CTRLD cmd;
    ia >> cmd;

    // Validate URLs
    if (cmd.upstream1.empty() || Utils::normalizeAddress(cmd.upstream1).empty() || (!cmd.upstream2.empty() && Utils::normalizeAddress(cmd.upstream2).empty())) {
        LOG("Invalid upstream URL(s)");
        answer.executed = 0;
        return answer;
    }
    for (const auto domain: cmd.domains) {
        if (!Utils::isValidDomain(domain)) {
            LOG("Invalid domain: %s", domain.c_str());
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
        arguments << " --log /Library/Logs/com.windscribe.helper.macos/ctrld.log";
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
        LOG("ctrld executable signature incorrect: %s", sigCheck.lastError().c_str());
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
    LOG("Starting stunnel");

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
        LOG("stunnel executable signature incorrect: %s", sigCheck.lastError().c_str());
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
    LOG("Starting wstunnel");

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
        LOG("wstunnel executable signature incorrect: %s", sigCheck.lastError().c_str());
        answer.executed = 0;
    } else {
        answer.cmdId = ExecuteCmd::instance().execute(fullCmd, std::string(), true);
        answer.executed = 1;
    }

    return answer;
}

CMD_ANSWER installerCreateCliSymlinkDir(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    CMD_INSTALLER_CREATE_CLI_SYMLINK_DIR cmd;
    ia >> cmd;

    std::error_code err;
    if (!std::filesystem::is_directory("/usr/local/bin", err)) {
        LOG("Creating CLI symlink dir");

        if (!std::filesystem::create_directory("/usr/local/bin", err)) {
            LOG("Failed to create CLI directory: %s", err.message().c_str());
            answer.executed = 0;
            return answer;
        }
        const struct group *grp = getgrnam("admin");
        if (grp == nullptr) {
            LOG("Could not get group info");
            answer.executed = 0;
            return answer;
        }
        int rc = chown("/usr/local/bin", cmd.uid, grp->gr_gid);
        if (rc != 0) {
            LOG("Failed to set owner: %s", strerror(errno));
            answer.executed = 0;
            return answer;
        }
        std::filesystem::permissions("/usr/local/bin",
            std::filesystem::perms::owner_all |
            std::filesystem::perms::group_all |
            std::filesystem::perms::others_read | std::filesystem::perms::others_exec,
            err);
        if (err) {
            LOG("Failed to set permissions: %s", err.message().c_str());
            answer.executed = 0;
            return answer;
        }
    }

    answer.executed = 1;
    return answer;
}

CMD_ANSWER getHelperVersion(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    answer.body = MacUtils::bundleVersionFromPlist();
    answer.executed = 1;
    return answer;
}

CMD_ANSWER getInterfaceSsid(boost::archive::text_iarchive &ia)
{
    CMD_ANSWER answer;
    CMD_GET_INTERFACE_SSID cmd;
    ia >> cmd;
    LOG("Get interface SSID for %s", cmd.interface.c_str());

    std::string output;

    answer.executed = Utils::executeCommand("/usr/bin/wdutil", {"info"}, &output);

    std::istringstream stream(output);
    std::string line;

    // Skip all the lines until we see our interface
    while (getline(stream, line)) {
        if (line.find("Interface Name") != std::string::npos && line.find(cmd.interface) != std::string::npos) {
            continue;
        }
        break;
    }
    // Now look for the SSID
    while (getline(stream, line)) {
        if (line.find("SSID") != std::string::npos) {
            answer.body = line.substr(line.find(":") + 2);
            LOG("Found SSID for %s: %s", cmd.interface.c_str(), answer.body.c_str());
            return answer;
        }
    }
    LOG("No SSID for %s", cmd.interface.c_str());
    return answer;
}
