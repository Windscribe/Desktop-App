#include "process_command.h"

#include <copyfile.h>
#include <fcntl.h>
#include <filesystem>
#include <grp.h>
#include <pwd.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>
#include <spdlog/spdlog.h>

#include "execute_cmd.h"
#include "files_manager.h"
#include "firewallcontroller.h"
#include "firewallonboot.h"
#include "ipc/helper_security.h"
#include "ipv6/ipv6manager.h"
#include "macspoofingonboot.h"
#include "macutils.h"
#include "ovpn.h"
#include "routes_manager/routes_manager.h"
#include "utils.h"
#include "utils/executable_signature/executable_signature.h"
#include "utils/wsscopeguard.h"
#include "wireguard/wireguardcontroller.h"
#include "../common/ip_validation.h"
#include "../common/shellquote.h"
#include "clear_wifi_history/clear_wifi_history.h"

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

    auto checkIp = [](const std::string &ip, const char *label) {
        if (!ip.empty() && !IpValidation::isValidIpAddress(ip)) {
            spdlog::error("sendConnectStatus: invalid {} \"{}\", rejecting", label, ip);
            return false;
        }
        return true;
    };
    auto checkIface = [](const std::string &name, const char *label) {
        if (!name.empty() && !Utils::isValidInterfaceName(name)) {
            spdlog::error("sendConnectStatus: invalid {} \"{}\", rejecting", label, name);
            return false;
        }
        return true;
    };
    if (!checkIp(cs.remoteIp, "remoteIp") ||
        !checkIp(cs.defaultAdapter.gatewayIp, "defaultAdapter.gatewayIp") ||
        !checkIp(cs.vpnAdapter.gatewayIp, "vpnAdapter.gatewayIp") ||
        !checkIface(cs.vpnAdapter.adapterName, "vpnAdapter.adapterName") ||
        !checkIface(cs.defaultAdapter.adapterName, "defaultAdapter.adapterName")) {
        return serializeResult(false);
    }
    for (const auto &dns : cs.vpnAdapter.dnsServers) {
        if (!IpValidation::isValidIpAddress(dns)) {
            spdlog::error("sendConnectStatus: invalid vpnAdapter.dnsServers entry \"{}\", rejecting", dns);
            return serializeResult(false);
        }
    }

    const std::string vpnDns = (cs.isConnected && !cs.vpnAdapter.dnsServers.empty())
        ? cs.vpnAdapter.dnsServers.front()
        : "";
    FirewallController::instance().setVpnDns(vpnDns);
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
    CmdDnsManager dnsManager;
    deserializePars(pars, config, port, httpProxy, httpPort, socksProxy, socksPort, dnsManager);

    // sanitize
    if (Utils::hasWhitespaceInString(httpProxy) ||
        Utils::hasWhitespaceInString(socksProxy))
    {
        httpProxy = "";
        socksProxy = "";
    }

    if (!OVPN::writeOVPNFile(MacUtils::resourcePath() + "dns.sh", port, config, httpProxy, httpPort, socksProxy, socksPort)) {
        spdlog::error("Could not write OpenVPN config");
        return serializeResult(false);
    }

    std::string fullCmd = Utils::getFullCommand(Utils::getExePath(), WS_PRODUCT_NAME_LOWER "openvpn", "--config " WS_POSIX_CONFIG_DIR "/config.ovpn");
    if (fullCmd.empty()) {
        // Something wrong with the command
        return serializeResult(false);
    }

    const std::string fullPath = Utils::getExePath() + "/" WS_PRODUCT_NAME_LOWER "openvpn";
    ExecutableSignature sigCheck;
    if (!sigCheck.verify(fullPath)) {
        spdlog::error("OpenVPN executable signature incorrect: {}", sigCheck.lastError());
        return serializeResult(false);
    } else {
        ExecuteCmd::instance().execute(fullCmd, WS_POSIX_CONFIG_DIR);
        return serializeResult(true);
    }
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

            if (!IpValidation::validateWireGuardConfig(clientIpAddress, clientDnsAddressList, allowed_ips_vector, peerEndpoint)) {
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
    arguments << " --primary_upstream=" + ShellQuote::quote(upstream1);
    if (!upstream2.empty()) {
        arguments << " --secondary_upstream=" + ShellQuote::quote(upstream2);
        if (!domains.empty()) {
            std::stringstream domainsStream;
            std::copy(domains.begin(), domains.end(), std::ostream_iterator<std::string>(domainsStream, ","));

            std::string domainsStr = domainsStream.str();
            domainsStr.erase(domainsStr.length() - 1); // remove last comma

            arguments << " --domains=" + ShellQuote::quote(domainsStr);
        }
    }
    if (isCreateLog) {
        arguments << " --log /Library/Logs/" WS_MAC_HELPER_BUNDLE_ID "/ctrld.log";
        arguments << " -vv";
    }

    std::string fullCmd = Utils::getFullCommand(Utils::getExePath(), WS_PRODUCT_NAME_LOWER "ctrld", arguments.str());
    if (fullCmd.empty()) {
        // Something wrong with the command
        return serializeResult(false);
    }

    const std::string fullPath = Utils::getExePath() + "/" WS_PRODUCT_NAME_LOWER "ctrld";
    ExecutableSignature sigCheck;
    if (!sigCheck.verify(fullPath)) {
        spdlog::error("ctrld executable signature incorrect: {}", sigCheck.lastError());
        return serializeResult(false);
    }

    bool executed = Utils::executeCommand(fullCmd) == 0;
    return serializeResult(executed);
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
    std::string fullCmd = Utils::getFullCommandAsUser(WS_PRODUCT_NAME_LOWER, Utils::getExePath(), WS_PRODUCT_NAME_LOWER "wstunnel", arguments.str());
    if (fullCmd.empty()) {
        // Something wrong with the command
        return serializeResult(false);
    }

    const std::string fullPath = Utils::getExePath() + "/" WS_PRODUCT_NAME_LOWER "wstunnel";
    ExecutableSignature sigCheck;
    if (!sigCheck.verify(fullPath)) {
        spdlog::error("stunnel executable signature incorrect: {}", sigCheck.lastError());
        return serializeResult(false);
    }

    ExecuteCmd::instance().execute(fullCmd);

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
    std::string fullCmd = Utils::getFullCommandAsUser(WS_PRODUCT_NAME_LOWER, Utils::getExePath(), WS_PRODUCT_NAME_LOWER "wstunnel", arguments.str());
    if (fullCmd.empty()) {
        // Something wrong with the command
        return serializeResult(false);
    }

    const std::string fullPath = Utils::getExePath() + "/" WS_PRODUCT_NAME_LOWER "wstunnel";
    ExecutableSignature sigCheck;
    if (!sigCheck.verify(fullPath)) {
        spdlog::error("wstunnel executable signature incorrect: {}", sigCheck.lastError());
        return serializeResult(false);
    }

    ExecuteCmd::instance().execute(fullCmd);

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

    if (enabled) {
        if (!Utils::isValidInterfaceName(interface)) {
            spdlog::error("enableMacSpoofingOnBoot: invalid interface \"{}\", rejecting", interface);
            return std::string();
        }
        if (!Utils::isValidMacAddress(macAddress)) {
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

    if (!IpValidation::isValidIpAddress(ipAddress)) {
        spdlog::error("setDnsOfDynamicStoreEntry: invalid DNS IP \"{}\", rejecting", ipAddress);
        return serializeResult(false);
    }
    if (!Utils::isValidDnsDynamicStoreEntry(networkService)) {
        spdlog::error("setDnsOfDynamicStoreEntry: invalid network service entry \"{}\", rejecting", networkService);
        return serializeResult(false);
    }

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
        Utils::executeCommand("rm", {"-rf", WS_MAC_APP_DIR});
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
        spdlog::error("setInstallerPaths: open archive at \"{}\" failed: {}", archivePath, strerror(errno));
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
        spdlog::error("setInstallerPaths: mkstemps failed: {}", strerror(errno));
        return false;
    }
    fchmod(dstFd, S_IRUSR | S_IWUSR);

    int rc = fcopyfile(srcFd, dstFd, NULL, COPYFILE_DATA);
    fsync(dstFd);
    close(dstFd);

    if (rc != 0) {
        spdlog::error("setInstallerPaths: fcopyfile failed: {}", strerror(errno));
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
