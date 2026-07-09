#include "ws_branding.h"
#include "process_command.h"

#include <codecvt>
#include <filesystem>
#include <locale>
#include <spdlog/spdlog.h>
#include <sstream>

#include <WinSock2.h>
#include <ws2ipdef.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#include "active_processes.h"
#include "utils/executable_signature/executable_signature.h"
#include "utils/systemlibloader.h"
#include "utils/win32handle.h"
#include "changeics/icsmanager.h"
#include "clear_wifi_history/clear_wifi_history.h"
#include "close_tcp_connections.h"
#include "dns_firewall.h"
#include "dohdata.h"
#include "executecmd.h"
#include "firewallfilter.h"
#include "firewallonboot.h"
#include "hostsedit.h"
#include "ikev2ipsec.h"
#include "ikev2route.h"
#include "network_validation.h"
#include "ipv6_firewall.h"
#include "macaddressspoof.h"
#include "openvpncontroller.h"
#include "reinstall_wan_ikev2.h"
#include "remove_app_network_profiles.h"
#include "split_tunneling/split_tunneling.h"
#include "utils.h"
#include "utils/servicecontrolmanager.h"
#include "wireguard/wireguardcontroller.h"
#include "wmi_utils.h"

struct SPLIT_TUNNELING_PARS
{
    bool isEnabled;
    bool isExclude;
    std::vector<std::wstring> apps;
    bool isVpnConnected;

    SPLIT_TUNNELING_PARS() : isEnabled(false), isExclude(true), isVpnConnected(false) {}
} g_SplitTunnelingPars;

std::string processCommand(HelperCommand cmdId, const std::string &pars)
{
    const auto command = kCommands.find(cmdId);
    if (command == kCommands.end()) {
        spdlog::error("Unknown command id: {}", (int)cmdId);
        return std::string();
    }

    return (command->second)(pars);
}

std::string setSplitTunnelingSettings(const std::string &pars)
{
    bool isActive, isExclude, isAllowLanTraffic;
    std::vector<std::wstring> files, ips, hosts_wstr;
    deserializePars(pars, isActive, isExclude, isAllowLanTraffic, files, ips, hosts_wstr);

    // Rather than reject the whole request on bad input, sanitize it: drop any invalid IP/CIDR
    // and hostname entries and apply the valid remainder. This keeps split tunneling functional
    // for the well-formed entries instead of silently failing the entire operation.
    ips.erase(std::remove_if(ips.begin(), ips.end(),
                             [](const std::wstring &ip) {
                                 if (NetworkValidation::isValidIpOrCidr(ip)) {
                                     return false;
                                 }
                                 spdlog::error(L"setSplitTunnelingSettings: dropping invalid IP/CIDR: \"{}\"", ip);
                                 return true;
                             }),
              ips.end());

    hosts_wstr.erase(std::remove_if(hosts_wstr.begin(), hosts_wstr.end(),
                                    [](const std::wstring &hostname) {
                                        if (NetworkValidation::isValidHostname(hostname)) {
                                            return false;
                                        }
                                        spdlog::error(L"setSplitTunnelingSettings: dropping invalid hostname: \"{}\"", hostname);
                                        return true;
                                    }),
                     hosts_wstr.end());

    // convert hosts_wstr to UTF8 hosts vector
    std::vector<std::string> hosts;
    hosts.reserve(hosts_wstr.size());
    using Converter = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>;
    Converter converter;
    std::transform(hosts_wstr.begin(), hosts_wstr.end(), std::back_inserter(hosts),
                   [&converter](const std::wstring& wstr) {
                       return converter.to_bytes(wstr);
                   });

    spdlog::debug("setSplitTunnelingSettings, isEnabled: {}, isExclude: {},"
                  " isAllowLanTraffic: {}, cntApps: {}",
                  isActive, isExclude, isAllowLanTraffic, files.size());

    for (size_t i = 0; i < files.size(); ++i) {
        spdlog::debug(L"setSplitTunnelingSettings: {}", files[i].substr(files[i].find_last_of(L"/\\") + 1));
    }

    for (size_t i = 0; i < ips.size(); ++i) {
        spdlog::debug(L"setSplitTunnelingSettings: {}", ips[i]);
    }

    for (size_t i = 0; i < hosts.size(); ++i) {
        spdlog::debug("setSplitTunnelingSettings: {}", hosts[i]);
    }

    SplitTunneling::instance().setSettings(isActive, isExclude, files, ips, hosts, isAllowLanTraffic);
    g_SplitTunnelingPars.isEnabled = isActive;
    g_SplitTunnelingPars.isExclude = isExclude;
    g_SplitTunnelingPars.apps = files;

    return std::string();
}

std::string sendConnectStatus(const std::string &pars)
{
    ConnectStatus connectStatus;

    deserializePars(pars, connectStatus.isConnected, connectStatus.isTerminateSocket, connectStatus.isKeepLocalSocket,
                    connectStatus.protocol, connectStatus.defaultAdapter, connectStatus.vpnAdapter,
                    connectStatus.connectedIp, connectStatus.remoteIp);

    spdlog::debug("sendConnectStatus: {}", connectStatus.isConnected);

    bool success = SplitTunneling::instance().setConnectStatus(connectStatus);
    g_SplitTunnelingPars.isVpnConnected = connectStatus.isConnected;
    return serializeResult(success);
}

std::string changeMtu(const std::string &pars)
{
    std::wstring adapterName;
    int mtu;
    deserializePars(pars, adapterName, mtu);

    // mtu arrives as a signed int over IPC; the caller only checks mtu > 0. Range-check here
    // before the static_cast<ULONG> below so a negative or absurd value can't wrap into a huge
    // NlMtu. Bound to the IPv4 range: RFC 791 minimum (68) up to a jumbo frame (9000), which
    // covers every legitimate value the client sends.
    // Note: SetIpInterfaceEntry will still reject anything the kernel considers too small.
    if (mtu < 68 || mtu > 9000) {
        spdlog::error(L"changeMtu: rejecting out-of-range mtu={} for '{}'", mtu, adapterName);
        return std::string();
    }

    NET_LUID luid = {};
    DWORD ret = ConvertInterfaceAliasToLuid(adapterName.c_str(), &luid);
    if (ret != NO_ERROR) {
        spdlog::error(L"changeMtu: ConvertInterfaceAliasToLuid failed for '{}' ({})", adapterName, ret);
        return std::string();
    }

    MIB_IPINTERFACE_ROW row = {};
    row.Family = AF_INET;
    row.InterfaceLuid = luid;
    ret = GetIpInterfaceEntry(&row);
    if (ret != NO_ERROR) {
        spdlog::error(L"changeMtu: GetIpInterfaceEntry failed for '{}' ({})", adapterName, ret);
        return std::string();
    }

    row.NlMtu = static_cast<ULONG>(mtu);
    // SetIpInterfaceEntry rejects SitePrefixLength > 32 for IPv4, but GetIpInterfaceEntry can return 64.
    row.SitePrefixLength = 0;
    ret = SetIpInterfaceEntry(&row);
    if (ret != NO_ERROR) {
        spdlog::error(L"changeMtu: SetIpInterfaceEntry failed for '{}' mtu={} ({})", adapterName, mtu, ret);
        return std::string();
    }

    spdlog::debug(L"changeMtu: adapter='{}' mtu={}", adapterName, mtu);
    return std::string();
}

std::string executeOpenVPN(const std::string &pars)
{
    std::wstring config, httpProxy, socksProxy;
    unsigned int port, httpPort, socksPort;
    deserializePars(pars, config, port, httpProxy, httpPort, socksProxy, socksPort);

    const auto res = OpenVPNController::instance().runOpenvpn(config, port, httpProxy, httpPort, socksProxy, socksPort);
    return serializeResult(res.success);
}

std::string executeTaskKill(const std::string &pars)
{
    CmdKillTarget target;
    deserializePars(pars, target);

    ExecuteCmdResult res;
    if (target == kTargetOpenVpn) {
        std::wstringstream killCmd;
        killCmd << Utils::getSystemDir() << L"\\taskkill.exe /f /t /im " WS_PRODUCT_NAME_LOWER_W L"openvpn.exe";
        spdlog::debug(L"executeTaskKill, cmd={}", killCmd.str());
        res = ExecuteCmd::instance().executeBlockingCmd(killCmd.str());
    }
    else {
        spdlog::error(L"executeTaskKill, invalid ID received {}", (int)target);
    }

    return serializeResult(res.success, res.exitCode, res.output);
}

std::string startWireGuard(const std::string &pars)
{
    bool useAmneziaWG;
    deserializePars(pars, useAmneziaWG);
    bool success = WireGuardController::instance().installService(useAmneziaWG);
    spdlog::debug("startWireGuard: success = {}", success);
    return serializeResult(success);
}

std::string stopWireGuard(const std::string &pars)
{
    bool success = WireGuardController::instance().deleteService();
    spdlog::debug("stopWireGuard");
    return serializeResult(success);
}

std::string configureWireGuard(const std::string &pars)
{
    std::wstring config;
    deserializePars(pars, config);
    bool success = WireGuardController::instance().configure(config);
    spdlog::debug("configureWireGuard: success = {}", success);
    return serializeResult(success);
}

std::string getWireGuardStatus(const std::string &pars)
{
   ULONG64 lastHandshake, txBytes, rxBytes;
   bool success = (WireGuardController::instance().getStatus(lastHandshake, txBytes, rxBytes) != kWgStateError);
   return serializeResult(success, lastHandshake, txBytes, rxBytes);
}

std::string firewallOn(const std::string &pars)
{
    std::wstring connectingIp;
    std::vector<std::wstring> ips;
    bool bAllowLanTraffic;
    bool bIsCustomConfig;
    deserializePars(pars, connectingIp, ips, bAllowLanTraffic, bIsCustomConfig);

    // connectingIp may be empty (always-on firewall before any connection); when present it must
    // be a single IP. Rather than reject the whole request on bad input, sanitize it: drop an
    // invalid connectingIp and any invalid IP/CIDR entries from the list. This keeps the firewall
    // enabled (fail-closed) instead of leaving the user unprotected.
    if (!connectingIp.empty() && !NetworkValidation::isValidIpAddress(connectingIp)) {
        spdlog::error(L"firewallOn: clearing invalid connectingIp: \"{}\"", connectingIp);
        connectingIp.clear();
    }

    ips.erase(std::remove_if(ips.begin(), ips.end(),
                             [](const std::wstring &ip) {
                                 if (NetworkValidation::isValidIpOrCidr(ip)) {
                                     return false;
                                 }
                                 spdlog::error(L"firewallOn: dropping invalid IP/CIDR: \"{}\"", ip);
                                 return true;
                             }),
              ips.end());

    bool prevStatus = FirewallFilter::instance().currentStatus();
    FirewallFilter::instance().on(connectingIp.c_str(), ips, bAllowLanTraffic, bIsCustomConfig);
    if (!prevStatus) {
        SplitTunneling::instance().updateState();
    }
    spdlog::debug("firewallOn, AllowLocalTraffic={}", bAllowLanTraffic);
    return std::string();
}

std::string firewallOff(const std::string &pars)
{
    bool prevStatus = FirewallFilter::instance().currentStatus();
    FirewallFilter::instance().off();
    if (prevStatus) {
        SplitTunneling::instance().updateState();
    }
    spdlog::debug("firewallOff");
    return std::string();
}

std::string firewallActualState(const std::string &pars)
{
    bool state;
    if (FirewallFilter::instance().currentStatus()) {
        spdlog::debug("firewallActualState, On");
        state = true;
    } else {
        spdlog::debug("firewallActualState, Off");
        state = false;
    }
    return serializeResult(state);
}

std::string setCustomDnsWhileConnected(const std::string &pars)
{
    unsigned long ifIndex;
    std::wstring overrideDnsIpAddress;
    deserializePars(pars, ifIndex, overrideDnsIpAddress);

    if (!NetworkValidation::isValidIpAddress(overrideDnsIpAddress)) {
        spdlog::error(L"setCustomDnsWhileConnected: invalid IP address: \"{}\"", overrideDnsIpAddress);
        return serializeResult(false);
    }

    // Prefer the structured SetInterfaceDnsSettings API (Win10 2004+) so the IP is passed
    // through a typed field instead of being interpolated into a command line. The IP is
    // validated above, so command injection isn't reachable today, but the typed call
    // closes the netsh parsing sink entirely as defense-in-depth.
    //
    // Resolve the symbol at runtime rather than calling it directly: it exists only on
    // build 19041+, and a static import would prevent the service from starting on the
    // older Windows 10 builds we still support. The netsh path below is the downlevel fallback.
    try {
        wsl::SystemLibLoader iphlpapi("iphlpapi.dll");
        const auto pSetInterfaceDnsSettings = iphlpapi.getFunction<DWORD WINAPI(GUID, const DNS_INTERFACE_SETTINGS *)>("SetInterfaceDnsSettings");

        NET_LUID luid = {};
        DWORD ret = ::ConvertInterfaceIndexToLuid(ifIndex, &luid);
        if (ret != NO_ERROR) {
            spdlog::error("setCustomDnsWhileConnected: ConvertInterfaceIndexToLuid failed ({})", ret);
            return serializeResult(false);
        }

        GUID guid = {};
        ret = ::ConvertInterfaceLuidToGuid(&luid, &guid);
        if (ret != NO_ERROR) {
            spdlog::error("setCustomDnsWhileConnected: ConvertInterfaceLuidToGuid failed ({})", ret);
            return serializeResult(false);
        }

        IN6_ADDR addr6;
        const bool isV6 = (::InetPtonW(AF_INET6, overrideDnsIpAddress.c_str(), &addr6) == 1);

        // DNS_INTERFACE_SETTINGS::NameServer is PWSTR; copy into a non-const buffer so we
        // don't const_cast away the std::wstring's immutability.
        std::wstring nameServer = overrideDnsIpAddress;
        DNS_INTERFACE_SETTINGS settings = {};
        settings.Version = DNS_INTERFACE_SETTINGS_VERSION1;
        settings.Flags = DNS_SETTING_NAMESERVER | (isV6 ? DNS_SETTING_IPV6 : 0);
        settings.NameServer = nameServer.data();

        ret = pSetInterfaceDnsSettings(guid, &settings);
        if (ret != NO_ERROR) {
            spdlog::error(L"setCustomDnsWhileConnected: SetInterfaceDnsSettings failed for ifIndex={} dns=\"{}\" ({})", ifIndex, overrideDnsIpAddress, ret);
            return serializeResult(false);
        }

        spdlog::debug(L"setCustomDnsWhileConnected: ifIndex={} dns={}", ifIndex, overrideDnsIpAddress);
        return serializeResult(true);
    }
    catch (const std::system_error &ex) {
        // SetInterfaceDnsSettings unavailable on this OS — fall through to the netsh path below.
        spdlog::debug("setCustomDnsWhileConnected: SetInterfaceDnsSettings API unavailable ({}), falling back to netsh", ex.what());
    }

    std::wstring netshCmd = Utils::getSystemDir() + L"\\netsh.exe interface ipv4 set dns \"" + std::to_wstring(ifIndex) + L"\" static " + overrideDnsIpAddress;
    auto res = ExecuteCmd::instance().executeBlockingCmd(netshCmd);

    std::wstring logBuf = L"setCustomDnsWhileConnected: " + netshCmd + L" " + (res.success ? L"Success" : L"Failure") + L" " + std::to_wstring(res.exitCode);
    spdlog::debug(L"{}", logBuf);
    return serializeResult(res.exitCode == 0);
}

std::string executeSetMetric(const std::string &pars)
{
    ADDRESS_FAMILY family;
    std::wstring interfaceName;
    ULONG metric;
    deserializePars(pars, family, interfaceName, metric);

    if (family != AF_INET && family != AF_INET6) {
        spdlog::error("executeSetMetric: invalid address family ({})", family);
        return serializeResult(false);
    }

    NET_LUID luid = {};
    DWORD ret = ConvertInterfaceAliasToLuid(interfaceName.c_str(), &luid);
    if (ret != NO_ERROR) {
        spdlog::error(L"executeSetMetric: ConvertInterfaceAliasToLuid failed for '{}' ({})", interfaceName, ret);
        return serializeResult(false);
    }

    MIB_IPINTERFACE_ROW row = {};
    row.Family = family;
    row.InterfaceLuid = luid;
    ret = GetIpInterfaceEntry(&row);
    if (ret != NO_ERROR) {
        spdlog::error(L"executeSetMetric: GetIpInterfaceEntry failed for '{}' ({})", interfaceName, ret);
        return serializeResult(false);
    }

    row.Metric = metric;
    row.UseAutomaticMetric = FALSE;
    if (family == AF_INET) {
        // SetIpInterfaceEntry rejects SitePrefixLength > 32 for IPv4, but GetIpInterfaceEntry can return 64.
        row.SitePrefixLength = 0;
    }
    ret = SetIpInterfaceEntry(&row);
    if (ret != NO_ERROR) {
        spdlog::error(L"executeSetMetric: SetIpInterfaceEntry failed for '{}' metric={} ({})", interfaceName, metric, ret);
        return serializeResult(false);
    }

    spdlog::debug(L"executeSetMetric: family={} adapter='{}' metric={}", family, interfaceName, metric);
    return serializeResult(true);
}

std::string isWanIkev2AdapterDisabled(const std::string &pars)
{
    return serializeResult(WmiUtils::isWanIkev2AdapterDisabled());
}

std::string isIcsSupported(const std::string &pars)
{
    spdlog::debug("isIcsSupported");
    bool supported = IcsManager::instance().isSupported();
    return serializeResult(supported);
}

std::string startIcs(const std::string &pars)
{
    // do nothing in the current implementation
    spdlog::debug("startIcs");
    return serializeResult(true);
}

std::string changeIcs(const std::string &pars)
{
    std::wstring adapterName;
    deserializePars(pars, adapterName);

    spdlog::debug("changeIcs");
    bool success = IcsManager::instance().change(adapterName);
    return serializeResult(success);
}

std::string stopIcs(const std::string &pars)
{
    spdlog::debug("stopIcs");
    bool success = IcsManager::instance().stop();
    return serializeResult(success);
}

std::string enableBFE(const std::string &pars)
{
    spdlog::debug("enableBFE");

    bool success = false;
    try {
        wsl::ServiceControlManager scm;
        scm.openSCM(SC_MANAGER_CONNECT);
        scm.openService(L"BFE", SERVICE_QUERY_STATUS | SERVICE_START);
        scm.startService(6000);
        success = true;
    }
    catch (std::system_error& ex) {
        spdlog::error("enableBFE - {}", ex.what());
    }

    return serializeResult(success);
}

std::string queryBFEStatus(const std::string &pars)
{
    spdlog::debug("queryBFEStatus");

    DWORD status = 0;
    try {
        wsl::ServiceControlManager scm;
        scm.openSCM(SC_MANAGER_CONNECT);
        scm.openService(L"BFE", SERVICE_QUERY_STATUS);
        status = scm.queryServiceStatus();
    }
    catch (std::system_error& ex) {
        spdlog::error("queryBFEStatus - {}", ex.what());
    }

    return serializeResult(status);
}

std::string resetAndStartRAS(const std::string &pars)
{
    spdlog::debug("resetAndStartRAS");

    std::wstring exe = Utils::getSystemDir() + L"\\sc.exe";
    ExecuteCmd::instance().executeBlockingCmd(exe + L" config RasMan start= demand");
    ExecuteCmd::instance().executeBlockingCmd(exe + L" stop RasMan");
    ExecuteCmd::instance().executeBlockingCmd(exe + L" start RasMan");
    ExecuteCmd::instance().executeBlockingCmd(exe + L" config SstpSvc start= demand");
    ExecuteCmd::instance().executeBlockingCmd(exe + L" stop SstpSvc");
    auto res = ExecuteCmd::instance().executeBlockingCmd(exe + L" start SstpSvc");
    return serializeResult(res.output);
}

std::string setIPv6EnabledInFirewall(const std::string &pars)
{
    bool bEnable, bAllowLanTraffic, bIsCustomConfig;
    deserializePars(pars, bEnable, bAllowLanTraffic, bIsCustomConfig);
    spdlog::debug("setIPv6EnabledInFirewall, enable={}, allowLanTraffic={}, isCustomConfig={}", bEnable, bAllowLanTraffic, bIsCustomConfig);
    if (bEnable) {
        Ipv6Firewall::instance().enableIPv6();
    } else {
        Ipv6Firewall::instance().disableIPv6(bAllowLanTraffic, bIsCustomConfig);
    }
    return std::string();
}

std::string setFirewallOnBoot(const std::string &pars)
{
    bool bEnable;
    deserializePars(pars, bEnable);

    spdlog::debug("setFirewallOnBoot: {}", bEnable ? "enabled" : "disabled");
    FirewallOnBootManager::instance().setEnabled(bEnable);
    return std::string();
}

std::string clearWifiHistoryData(const std::string &pars)
{
    spdlog::debug("clearWifiHistoryData");
    bool success = ClearWiFiHistory::clear();
    return serializeResult(success);
}

std::string addHosts(const std::string &pars)
{
    std::wstring ip, hostname;
    deserializePars(pars, ip, hostname);

    if (!NetworkValidation::isValidIpAddress(ip)) {
        spdlog::error(L"addHosts invalid IP address: \"{}\"", ip);
        return serializeResult(false);
    }
    if (!NetworkValidation::isValidHostname(hostname)) {
        spdlog::error(L"addHosts invalid hostname: \"{}\"", hostname);
        return serializeResult(false);
    }

    spdlog::debug("addHosts");
    bool success = HostsEdit::instance().addHosts(ip, hostname);
    return serializeResult(success);
}

std::string removeHosts(const std::string &pars)
{
    spdlog::debug("removeHosts");
    bool success = HostsEdit::instance().removeHosts();
    return serializeResult(success);
}

std::string closeAllTcpConnections(const std::string &pars)
{
    bool isKeepLocalSockets;
    deserializePars(pars, isKeepLocalSockets);

    spdlog::debug("closeAllTcpConnections, KeepLocalSockets = {}", isKeepLocalSockets);
    if (g_SplitTunnelingPars.isEnabled && g_SplitTunnelingPars.isVpnConnected) {
        CloseTcpConnections::closeAllTcpConnections(isKeepLocalSockets, g_SplitTunnelingPars.isExclude, g_SplitTunnelingPars.apps);
    } else {
        CloseTcpConnections::closeAllTcpConnections(isKeepLocalSockets);
    }
    return std::string();
}

std::string getProcessesList(const std::string &pars)
{
    spdlog::debug("getProcessesList");
    std::vector<std::wstring> list = ActiveProcesses::instance().getList();
    return serializeResult(list);
}

std::string whitelistPorts(const std::string &pars)
{
    std::wstring ports;
    deserializePars(pars, ports);

    if (!NetworkValidation::isValidPortList(ports)) {
        spdlog::error(L"whitelistPorts: invalid port list: \"{}\"", ports);
        return serializeResult(false);
    }

    // Design note: replacing this netsh call with structured APIs requires too much code and no extra protection.
    std::wstring strCmd = Utils::getSystemDir() + L"\\netsh.exe advfirewall firewall add rule name=\"" WS_APP_IDENTIFIER_W L"StaticIpTcp\" protocol=TCP dir=in localport=\"";
    strCmd += ports;
    strCmd += L"\" action=allow";
    spdlog::debug(L"whitelistPorts, cmd={}", strCmd);
    auto res1 = ExecuteCmd::instance().executeBlockingCmd(strCmd);

    strCmd = Utils::getSystemDir() + L"\\netsh.exe advfirewall firewall add rule name=\"" WS_APP_IDENTIFIER_W L"StaticIpUdp\" protocol=UDP dir=in localport=\"";
    strCmd += ports;
    strCmd += L"\" action=allow";
    spdlog::debug(L"whitelistPorts, cmd={}", strCmd);
    auto res2 = ExecuteCmd::instance().executeBlockingCmd(strCmd);
    return serializeResult(res1.success && res2.success);
}

std::string deleteWhitelistPorts(const std::string &pars)
{
    // Design note: replacing this netsh call with structured APIs requires too much code and no extra protection.
    std::wstring strCmd = Utils::getSystemDir() + L"\\netsh.exe advfirewall firewall delete rule name=\"" WS_APP_IDENTIFIER_W L"StaticIpTcp\" dir=in";
    spdlog::debug(L"deleteWhitelistPorts, cmd={}", strCmd);
    auto res1 = ExecuteCmd::instance().executeBlockingCmd(strCmd);

    strCmd = Utils::getSystemDir() + L"\\netsh.exe advfirewall firewall delete rule name=\"" WS_APP_IDENTIFIER_W L"StaticIpUdp\" dir=in";
    spdlog::debug(L"deleteWhitelistPorts, cmd={}", strCmd);
    auto res2 =  ExecuteCmd::instance().executeBlockingCmd(strCmd);
    return serializeResult(res1.success && res2.success);
}

std::string enableDnsLeaksProtection(const std::string &pars)
{
    std::vector<std::wstring> customDnsIp;
    deserializePars(pars, customDnsIp);

    // Rather than reject the whole request on bad input, sanitize it: drop any invalid IP entries
    // and apply the valid remainder. These IPs are only excluded from the DNS firewall block, so
    // dropping a malformed one fails closed (that IP simply stays blocked).
    customDnsIp.erase(std::remove_if(customDnsIp.begin(), customDnsIp.end(),
                                     [](const std::wstring &ip) {
                                         if (NetworkValidation::isValidIpAddress(ip)) {
                                             return false;
                                         }
                                         spdlog::error(L"enableDnsLeaksProtection: dropping invalid IP address: \"{}\"", ip);
                                         return true;
                                     }),
                      customDnsIp.end());

    spdlog::debug("enableDnsLeaksProtection");
    DnsFirewall::instance().setExcludeIps(customDnsIp);
    DnsFirewall::instance().enable();
    return std::string();
}

std::string disableDnsLeaksProtection(const std::string &pars)
{
    spdlog::debug("disableDnsLeaksProtection");
    DnsFirewall::instance().disable();
    return std::string();
}

std::string reinstallWanIkev2(const std::string &pars)
{
    spdlog::debug("reinstallWanIkev2");
    ReinstallWanIkev2::uninstallDevice();
    ReinstallWanIkev2::scanForHardwareChanges();
    return std::string();
}

std::string enableWanIkev2(const std::string &pars)
{
    spdlog::debug("enableWanIkev2");
    ReinstallWanIkev2::enableDevice();
    return std::string();
}

std::string setMacAddressSpoof(const std::string &pars)
{
    std::wstring interfaceName, value;
    deserializePars(pars, interfaceName, value);
    MacAddressSpoof::set(interfaceName, value);
    return std::string();
}

std::string removeMacAddressSpoof(const std::string &pars)
{
    std::wstring interfaceName;
    deserializePars(pars, interfaceName);
    MacAddressSpoof::remove(interfaceName);
    return std::string();
}

std::string resetNetworkAdapter(const std::string &pars)
{
    ULONG ifIndex;
    bool bringBackUp;
    deserializePars(pars, ifIndex, bringBackUp);

    spdlog::debug("resetNetworkAdapter: Disable ifIndex={}", ifIndex);
    Utils::setNetworkAdapterState(ifIndex, false);

    if (bringBackUp) {
        spdlog::debug("resetNetworkAdapter: Enable ifIndex={}", ifIndex);
        Utils::setNetworkAdapterState(ifIndex, true);
    }
    return std::string();
}

std::string addIKEv2DefaultRoute(const std::string &pars)
{
    bool success = IKEv2Route::addRouteForIKEv2();
    spdlog::debug("addIKEv2DefaultRoute: success = {}", success);
    return std::string();
}

std::string removeAppNetworkProfiles(const std::string &pars)
{
    spdlog::debug("removeAppNetworkProfiles");
    RemoveAppNetworkProfiles::remove();
    return std::string();
}

std::string setIKEv2IPSecParameters(const std::string &pars)
{
    bool success = IKEv2IPSec::setIKEv2IPSecParameters();
    spdlog::debug("setIKEv2IPSecParameters: success = {}", success);
    return std::string();
}

std::string makeHostsFileWritable(const std::string &pars)
{
    bool success = HostsEdit::instance().makeHostsFileWritable();
    return serializeResult(success);
}

std::string createOpenVpnAdapter(const std::string &pars)
{
    bool useDCODriver;
    deserializePars(pars, useDCODriver);

    spdlog::debug(L"createOpenVpnAdapter: creating '{}' adapter", (useDCODriver ? L"ovpn-dco" : L"tap-windows6"));
    OpenVPNController::instance().createAdapter(useDCODriver);
    return std::string();
}

std::string removeOpenVpnAdapter(const std::string &pars)
{
    spdlog::debug("removeOpenVpnAdapter");
    OpenVPNController::instance().removeAdapter();
    return std::string();
}

std::string disableDohSettings(const std::string &pars)
{
    spdlog::debug("disableDohSettings");
    DohData::instance().disableDohSettings();
    return std::string();
}

std::string enableDohSettings(const std::string &pars)
{
    spdlog::debug("enableDohSettings");
    DohData::instance().enableDohSettings();
    return std::string();
}

std::string ssidFromInterfaceGUID(const std::string &pars)
{
    std::wstring interfaceGUID;
    deserializePars(pars, interfaceGUID);

    if (!NetworkValidation::isValidGuid(interfaceGUID)) {
        spdlog::error(L"ssidFromInterfaceGUID: invalid GUID: \"{}\"", interfaceGUID);
        return serializeResult(false, static_cast<unsigned long>(ERROR_INVALID_PARAMETER), std::string());
    }

    std::string output;
    bool success = false;
    unsigned long exitCode = 0;

    try {
        output = Utils::ssidFromInterfaceGUID(interfaceGUID);
        success = true;
    }
    catch (std::system_error &ex) {
        exitCode = ex.code().value();
        if (exitCode != ERROR_ACCESS_DENIED && exitCode != ERROR_NOT_FOUND && exitCode != ERROR_INVALID_STATE) {
            // Only log 'abnormal' errors so we do not spam the log.  We'll receive ERROR_ACCESS_DENIED if location services
            // are blocked on the computer, and the other two when an adapter is being reset.
            spdlog::debug("ssidFromInterfaceGUID - {}", ex.what());
        }
    }

    return serializeResult(success, exitCode, output);
}

std::string installerStageAndVerify(const std::string &pars)
{
    std::wstring srcPath;
    deserializePars(pars, srcPath);

    // Open the downloaded installer once and bind every subsequent check and the copy itself to
    // this exact handle. FILE_FLAG_OPEN_REPARSE_POINT opens a reparse-point source as the reparse
    // point itself instead of following it, so we can reject symlinks/junctions on the handle.
    // This closes a source-side TOCTOU: srcPath lives in the per-user, user-writable download dir,
    // so a same-user attacker could otherwise swap it for a symlink between a path-based attribute
    // check and a path-based CopyFileW, redirecting the SYSTEM helper into reading an arbitrary
    // file it can access but the user cannot — and staging it where Users have read access.
    // FILE_SHARE_READ only (no write/delete) pins the object for the lifetime of the copy.
    wsl::Win32Handle srcHandle(::CreateFileW(srcPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
                                             OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_SEQUENTIAL_SCAN, NULL));
    if (!srcHandle.isValid()) {
        spdlog::error("installerStageAndVerify: failed to open source ({})", ::GetLastError());
        return serializeResult(std::wstring(), false);
    }

    BY_HANDLE_FILE_INFORMATION srcInfo;
    if (!::GetFileInformationByHandle(srcHandle.getHandle(), &srcInfo)) {
        spdlog::error("installerStageAndVerify: GetFileInformationByHandle failed ({})", ::GetLastError());
        return serializeResult(std::wstring(), false);
    }
    if (srcInfo.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
        spdlog::error("installerStageAndVerify: refusing reparse-point source");
        return serializeResult(std::wstring(), false);
    }
    // A hard link is not a reparse point, so the attribute check above does not catch it. A
    // freshly downloaded installer always has exactly one link; anything more means the source
    // name is an extra link aliasing a file we wouldn't otherwise read (the classic hard-link
    // redirect). Modern Windows already blocks creating a hard link to a file the caller can't
    // write, but this keeps the guarantee independent of OS version.
    if (srcInfo.nNumberOfLinks != 1) {
        spdlog::error("installerStageAndVerify: refusing multi-link source ({} links)", srcInfo.nNumberOfLinks);
        return serializeResult(std::wstring(), false);
    }

    // Resolve the protected update dir once (SHGetKnownFolderPath isn't free), then
    // wipe and recreate. This clears any prior staging state — symmetric with
    // installerCleanupStaged below — and is also the primary self-healing path if
    // the engine ever crashed before persisting the pendingInstallerCleanup flag.
    const std::wstring updateDir = Utils::getUpdatePath(false);
    if (updateDir.empty()) {
        spdlog::error("installerStageAndVerify: failed to obtain protected update dir");
        return serializeResult(std::wstring(), false);
    }
    std::error_code dirEc;
    std::filesystem::remove_all(updateDir, dirEc);
    if (dirEc) {
        spdlog::warn("installerStageAndVerify: failed to wipe update dir: {}", dirEc.message());
    }
    std::filesystem::create_directories(updateDir, dirEc);
    if (dirEc) {
        spdlog::error("installerStageAndVerify: failed to recreate update dir: {}", dirEc.message());
        return serializeResult(std::wstring(), false);
    }

    const std::wstring stagedPath = updateDir + L"\\installer.exe";

    // Copy bytes from the validated handle rather than re-opening srcPath by name. Re-opening
    // (as CopyFileW does internally) would reintroduce the swap window the handle above closes.
    // The destination lives in the Admins-only update dir we just recreated, so its path is not
    // attacker-controllable; the new file inherits the Program Files ACL just as CopyFileW's would.
    bool copyOk = false;
    {
        wsl::Win32Handle dstHandle(::CreateFileW(stagedPath.c_str(), GENERIC_WRITE, 0, NULL,
                                                 CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL));
        if (!dstHandle.isValid()) {
            spdlog::error("installerStageAndVerify: failed to create staged file ({})", ::GetLastError());
            return serializeResult(std::wstring(), false);
        }

        copyOk = true;
        std::vector<BYTE> buf(64 * 1024);
        for (;;) {
            DWORD bytesRead = 0;
            if (!::ReadFile(srcHandle.getHandle(), buf.data(), static_cast<DWORD>(buf.size()), &bytesRead, NULL)) {
                spdlog::error("installerStageAndVerify: ReadFile failed ({})", ::GetLastError());
                copyOk = false;
                break;
            }
            if (bytesRead == 0) {
                break;
            }
            DWORD offset = 0;
            while (offset < bytesRead) {
                DWORD bytesWritten = 0;
                if (!::WriteFile(dstHandle.getHandle(), buf.data() + offset, bytesRead - offset, &bytesWritten, NULL)) {
                    spdlog::error("installerStageAndVerify: WriteFile failed ({})", ::GetLastError());
                    copyOk = false;
                    break;
                }
                offset += bytesWritten;
            }
            if (!copyOk) {
                break;
            }
        }
    }

    if (!copyOk) {
        ::DeleteFileW(stagedPath.c_str());
        return serializeResult(std::wstring(), false);
    }

    // The staged file inherits the Program Files ACL: Users RX (so the launching
    // non-elevated user can ShellExecuteEx the file through UAC) and Admins/SYSTEM Full.
    // Users have no W, which is what closes the TOCTOU window — the bytes can't be
    // swapped between the signature check and the runas launch.

    ExecutableSignature sigCheck;
    if (!sigCheck.verify(stagedPath)) {
        spdlog::error("installerStageAndVerify: signature verification failed: {}", sigCheck.lastError());
        ::DeleteFileW(stagedPath.c_str());
        return serializeResult(std::wstring(), false);
    }

    return serializeResult(stagedPath, true);
}

std::string installerCleanupStaged(const std::string &pars)
{
    // Pass create=false: we're about to remove the directory, so recreating it first
    // would be pointless and racey.
    const std::wstring updateDir = Utils::getUpdatePath(false);
    if (!updateDir.empty()) {
        std::error_code ec;
        std::filesystem::remove_all(updateDir, ec);
        if (ec) {
            spdlog::info("installerCleanupStaged: {}", ec.message());
        }
    }
    return std::string();
}
