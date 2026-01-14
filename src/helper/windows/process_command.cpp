#include "process_command.h"

#include <sstream>
#include <spdlog/spdlog.h>
#include <codecvt>
#include <locale>

#include "close_tcp_connections.h"
#include "changeics/icsmanager.h"
#include "dohdata.h"
#include "executecmd.h"
#include "firewallfilter.h"
#include "firewallonboot.h"
#include "ikev2ipsec.h"
#include "ikev2route.h"
#include "macaddressspoof.h"
#include "openvpncontroller.h"
#include "reinstall_wan_ikev2.h"
#include "remove_windscribe_network_profiles.h"
#include "split_tunneling/split_tunneling.h"
#include "wireguard/wireguardcontroller.h"
#include "ipv6_firewall.h"
#include "hostsedit.h"
#include "active_processes.h"
#include "dns_firewall.h"
#include "utils.h"
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
    std::vector<std::string> hosts;
    deserializePars(pars, isActive, isExclude, isAllowLanTraffic, files, ips, hosts_wstr);

    // convert hosts_wstr to UTF8 hosts vector
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
    std::wstring netshCmd = Utils::getSystemDir() + L"\\netsh.exe interface ipv4 set subinterface \"" + adapterName + L"\" mtu=" + std::to_wstring(mtu) + L" store=active";
    spdlog::debug(L"changeMtu: {}", netshCmd);
    ExecuteCmd::instance().executeBlockingCmd(netshCmd);
    return std::string();
}

std::string executeOpenVPN(const std::string &pars)
{
    std::wstring config, httpProxy, socksProxy;
    unsigned int port, httpPort, socksPort;
    bool isCustomConfig;
    deserializePars(pars, config, port, httpProxy, httpPort, socksProxy, socksPort, isCustomConfig);

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
        killCmd << Utils::getSystemDir() << L"\\taskkill.exe /f /t /im windscribeopenvpn.exe";
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
    bool success = WireGuardController::instance().installService();
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
    std::wstring connectingIp, ip;
    bool bAllowLanTraffic, bIsCustomConfig;
    deserializePars(pars, connectingIp, ip, bAllowLanTraffic, bIsCustomConfig);

    bool prevStatus = FirewallFilter::instance().currentStatus();
    FirewallFilter::instance().on(connectingIp.c_str(), ip.c_str(), bAllowLanTraffic, bIsCustomConfig);
    if (!prevStatus) {
        SplitTunneling::instance().updateState();
    }
    spdlog::debug("firewallOn, AllowLocalTraffic={}, IsCustomConfig={}", bAllowLanTraffic, bIsCustomConfig);
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

    std::wstring netshCmd = Utils::getSystemDir() + L"\\netsh.exe interface ipv4 set dns \"" + std::to_wstring(ifIndex) + L"\" static " + overrideDnsIpAddress;
    auto res = ExecuteCmd::instance().executeBlockingCmd(netshCmd);

    std::wstring logBuf = L"setCustomDnsWhileConnected: " + netshCmd + L" " + (res.success ? L"Success" : L"Failure") + L" " + std::to_wstring(res.exitCode);
    spdlog::debug(L"{}", logBuf);
    return serializeResult(res.exitCode == 0);
}

std::string executeSetMetric(const std::string &pars)
{
    std::wstring interfaceType, interfaceName, metricNumber;
    deserializePars(pars, interfaceType, interfaceName, metricNumber);

    std::wstring setMetricCmd = Utils::getSystemDir() + L"\\netsh.exe int " + interfaceType + L" set interface interface=\"" +
                                interfaceName + L"\" metric=" + metricNumber;
    spdlog::debug(L"executeSetMetric, cmd={}", setMetricCmd);
    auto res =  ExecuteCmd::instance().executeBlockingCmd(setMetricCmd);
    return serializeResult(res.output);
}

std::string executeWmicGetConfigManagerErrorCode(const std::string &pars)
{
    std::wstring adapterName;
    deserializePars(pars, adapterName);

    std::wstring result = WmiUtils::getNetworkAdapterConfigManagerErrorCode(adapterName);
    return serializeResult(result);
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

    std::wstring exe = Utils::getSystemDir() + L"\\sc.exe";
    ExecuteCmd::instance().executeBlockingCmd(exe + L" config BFE start= auto");
    auto res = ExecuteCmd::instance().executeBlockingCmd(exe + L" start BFE");
    return serializeResult(res.output);
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
    bool bEnable;
    deserializePars(pars, bEnable);
    spdlog::debug("setIPv6EnabledInFirewall, {}", bEnable);
    if (bEnable) {
        Ipv6Firewall::instance().enableIPv6();
    } else {
        Ipv6Firewall::instance().disableIPv6();
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

std::string addHosts(const std::string &pars)
{
    std::wstring hosts;
    deserializePars(pars, hosts);

    spdlog::debug("addHosts");
    bool success = HostsEdit::instance().addHosts(hosts);
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

    std::wstring strCmd = Utils::getSystemDir() + L"\\netsh.exe advfirewall firewall add rule name=\"WindscribeStaticIpTcp\" protocol=TCP dir=in localport=\"";
    strCmd += ports;
    strCmd += L"\" action=allow";
    spdlog::debug(L"whitelistPorts, cmd={}", strCmd);
    auto res1 = ExecuteCmd::instance().executeBlockingCmd(strCmd);

    strCmd = Utils::getSystemDir() + L"\\netsh.exe advfirewall firewall add rule name=\"WindscribeStaticIpUdp\" protocol=UDP dir=in localport=\"";
    strCmd += ports;
    strCmd += L"\" action=allow";
    spdlog::debug(L"whitelistPorts, cmd={}", strCmd);
    auto res2 = ExecuteCmd::instance().executeBlockingCmd(strCmd);
    return serializeResult(res1.success && res2.success);
}

std::string deleteWhitelistPorts(const std::string &pars)
{
    std::wstring strCmd = Utils::getSystemDir() + L"\\netsh.exe advfirewall firewall delete rule name=\"WindscribeStaticIpTcp\" dir=in";
    spdlog::debug(L"deleteWhitelistPorts, cmd={}", strCmd);
    auto res1 = ExecuteCmd::instance().executeBlockingCmd(strCmd);

    strCmd = Utils::getSystemDir() + L"\\netsh.exe advfirewall firewall delete rule name=\"WindscribeStaticIpUdp\" dir=in";
    spdlog::debug(L"deleteWhitelistPorts, cmd={}", strCmd);
    auto res2 =  ExecuteCmd::instance().executeBlockingCmd(strCmd);
    return serializeResult(res1.success && res2.success);
}

std::string enableDnsLeaksProtection(const std::string &pars)
{
    std::vector<std::wstring> customDnsIp;
    deserializePars(pars, customDnsIp);
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
    std::wstring adapterRegistryName;
    bool bringBackUp;
    deserializePars(pars, adapterRegistryName, bringBackUp);

    spdlog::debug(L"resetNetworkAdapter, Disable {}", adapterRegistryName);
    Utils::callNetworkAdapterMethod(L"Disable", adapterRegistryName);

    if (bringBackUp) {
        spdlog::debug(L"resetNetworkAdapter, Enable {}", adapterRegistryName);
        Utils::callNetworkAdapterMethod(L"Enable", adapterRegistryName);
    }
    return std::string();
}

std::string addIKEv2DefaultRoute(const std::string &pars)
{
    bool success = IKEv2Route::addRouteForIKEv2();
    spdlog::debug("addIKEv2DefaultRoute: success = {}", success);
    return std::string();
}

std::string removeWindscribeNetworkProfiles(const std::string &pars)
{
    spdlog::debug("removeWindscribeNetworkProfiles");
    RemoveWindscribeNetworkProfiles::remove();
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
    bool success = true;

    const auto hostsFile = Utils::getSystemDir() + L"\\drivers\\etc\\hosts";
    const auto attributes = ::GetFileAttributes(hostsFile.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        spdlog::error("GetFileAttributes of hosts file failed ({})", ::GetLastError());
        success = false;
    } else if (::SetFileAttributes(hostsFile.c_str(), attributes & ~FILE_ATTRIBUTE_READONLY) == FALSE) {
        spdlog::error("SetFileAttributes of hosts file failed ({})", ::GetLastError());
        success = false;
    }
    return serializeResult(success);
}

std::string createOpenVpnAdapter(const std::string &pars)
{
    bool useDCODriver;
    deserializePars(pars, useDCODriver);

    spdlog::debug(L"createOpenVpnAdapter: creating '{}' adapter", (useDCODriver ? L"ovpn-dco" : L"wintun"));
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
