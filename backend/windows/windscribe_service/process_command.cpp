#include "process_command.h"

#include <sstream>

#include "close_tcp_connections.h"
#include "changeics/icsmanager.h"
#include "executecmd.h"
#include "ikev2ipsec.h"
#include "ikev2route.h"
#include "ipc/servicecommunication.h"
#include "logger.h"
#include "ovpn.h"
#include "registry.h"
#include "reinstall_wan_ikev2.h"
#include "remove_windscribe_network_profiles.h"
#include "utils.h"
#include "wintuncontroller.h"

SPLIT_TUNNELING_PARS g_SplitTunnelingPars;

MessagePacketResult processCommand(int cmdId, const std::string &packet)
{
    const auto command = kCommands.find(cmdId);
    if (command == kCommands.end()) {
        Logger::instance().out("Unknown command id: %d", cmdId);
        return MessagePacketResult();
    }

    std::istringstream stream(packet);
    boost::archive::text_iarchive ia(stream, boost::archive::no_header);

    return (command->second)(ia);
}

MessagePacketResult firewallOn(boost::archive::text_iarchive &ia)
{
    MessagePacketResult mpr;

    CMD_FIREWALL_ON cmdFirewallOn;
    ia >> cmdFirewallOn;

    bool prevStatus = FirewallFilter::instance().currentStatus();
    FirewallFilter::instance().on(cmdFirewallOn.connectingIp.c_str(), cmdFirewallOn.ip.c_str(), cmdFirewallOn.allowLanTraffic, cmdFirewallOn.isCustomConfig);
    if (!prevStatus) {
        SplitTunneling::instance().updateState();
    }
    Logger::instance().out(L"AA_COMMAND_FIREWALL_ON, AllowLocalTraffic=%d, IsCustomConfig=%d", cmdFirewallOn.allowLanTraffic, cmdFirewallOn.isCustomConfig);
    mpr.success = true;
    mpr.exitCode = 0;

    return mpr;
}

MessagePacketResult firewallOff(boost::archive::text_iarchive &ia)
{
    MessagePacketResult mpr;

    bool prevStatus = FirewallFilter::instance().currentStatus();
    FirewallFilter::instance().off();
    if (prevStatus) {
        SplitTunneling::instance().updateState();
    }
    Logger::instance().out(L"AA_COMMAND_FIREWALL_OFF");
    mpr.success = true;
    mpr.exitCode = 0;

    return mpr;
}

MessagePacketResult firewallStatus(boost::archive::text_iarchive &ia)
{
    MessagePacketResult mpr;
    mpr.success = true;

    if (FirewallFilter::instance().currentStatus()) {
        Logger::instance().out(L"AA_COMMAND_FIREWALL_STATUS, On");
        mpr.exitCode = 1;
    } else {
        Logger::instance().out(L"AA_COMMAND_FIREWALL_STATUS, Off");
        mpr.exitCode = 0;
    }
    return mpr;
}

MessagePacketResult firewallIpv6Enable(boost::archive::text_iarchive &ia)
{
    MessagePacketResult mpr;

    Logger::instance().out(L"AA_COMMAND_FIREWALL_IPV6_ENABLE");
    Ipv6Firewall::instance().enableIPv6();
    mpr.success = true;
    mpr.exitCode = 0;
    return mpr;
}

MessagePacketResult firewallIpv6Disable(boost::archive::text_iarchive &ia)
{
    MessagePacketResult mpr;

    Logger::instance().out(L"AA_COMMAND_FIREWALL_IPV6_DISABLE");
    Ipv6Firewall::instance().disableIPv6();
    mpr.success = true;
    mpr.exitCode = 0;
    return mpr;
}

MessagePacketResult removeFromHosts(boost::archive::text_iarchive &ia)
{
    MessagePacketResult mpr;

    Logger::instance().out(L"AA_COMMAND_REMOVE_WINDSCRIBE_FROM_HOSTS");
    mpr.success = HostsEdit::instance().removeWindscribeHosts();
    mpr.exitCode = 0;
    return mpr;
}

MessagePacketResult checkUnblockingCmdStatus(boost::archive::text_iarchive &ia)
{
    CMD_CHECK_UNBLOCKING_CMD_STATUS checkUnblockingCmdStatus;
    ia >> checkUnblockingCmdStatus;
    return ExecuteCmd::instance().getUnblockingCmdStatus(checkUnblockingCmdStatus.cmdId);
}

MessagePacketResult getUnblockingCmdCount(boost::archive::text_iarchive &ia)
{
    return ExecuteCmd::instance().getActiveUnblockingCmdCount();
}

MessagePacketResult clearUnblockingCmd(boost::archive::text_iarchive &ia)
{
    CMD_CLEAR_UNBLOCKING_CMD cmdClearUnblockingCmd;
    ia >> cmdClearUnblockingCmd;
    return ExecuteCmd::instance().clearUnblockingCmd(cmdClearUnblockingCmd.blockingCmdId);
}

MessagePacketResult getHelperVersion(boost::archive::text_iarchive &ia)
{
    MessagePacketResult mpr;

    wchar_t szModPath[MAX_PATH];
    szModPath[0] = '\0';
    GetModuleFileName(NULL, szModPath, sizeof(szModPath));

    DWORD dwArg;
    DWORD dwInfoSize = GetFileVersionInfoSize(szModPath, &dwArg);
    if (dwInfoSize == 0) {
        mpr.success = false;
        return mpr;
    }

    std::unique_ptr<unsigned char> arr(new unsigned char[dwInfoSize]);
    if (GetFileVersionInfo(szModPath, 0, dwInfoSize, arr.get()) == 0) {
        mpr.success = false;
        return mpr;
    }
    VS_FIXEDFILEINFO *vInfo;

    UINT uInfoSize;
    if (VerQueryValue(arr.get(), TEXT("\\"), (LPVOID*)&vInfo, &uInfoSize) == 0) {
        mpr.success = false;
        return mpr;
    }
    WORD v1 = HIWORD(vInfo->dwFileVersionMS);
    WORD v2 = LOWORD(vInfo->dwFileVersionMS);
    WORD v3 = HIWORD(vInfo->dwFileVersionLS);
    WORD v4 = LOWORD(vInfo->dwFileVersionLS);

    mpr.success = true;
    mpr.additionalString = std::to_string(v1) + "." + std::to_string(v2) + "." + std::to_string(v3) + "." + std::to_string(v4);
    return mpr;
}

MessagePacketResult ipv6State(boost::archive::text_iarchive &ia)
{
    MessagePacketResult mpr;

    mpr.exitCode = SysIpv6Controller::instance().isIpv6Enabled();
    Logger::instance().out(L"AA_COMMAND_OS_IPV6_STATE, %d", mpr.exitCode);
    mpr.success = true;

    return mpr;
}

MessagePacketResult ipv6Enable(boost::archive::text_iarchive &ia)
{
    MessagePacketResult mpr;

    Logger::instance().out(L"AA_COMMAND_OS_IPV6_ENABLE");
    SysIpv6Controller::instance().setIpv6Enabled(true);
    mpr.success = true;

    return mpr;
}

MessagePacketResult ipv6Disable(boost::archive::text_iarchive &ia)
{
    MessagePacketResult mpr;

    Logger::instance().out(L"AA_COMMAND_OS_IPV6_DISABLE");
    SysIpv6Controller::instance().setIpv6Enabled(false);
    mpr.success = true;

    return mpr;
}

MessagePacketResult icsIsSupported(boost::archive::text_iarchive &ia)
{
    MessagePacketResult mpr;

    Logger::instance().out(L"AA_COMMAND_ICS_IS_SUPPORTED");
    mpr.exitCode = IcsManager::instance().isSupported();
    mpr.success = true;

    return mpr;
}

MessagePacketResult icsStart(boost::archive::text_iarchive &ia)
{
    MessagePacketResult mpr;

    // do nothing in the current implementation
    Logger::instance().out(L"AA_COMMAND_ICS_START");
    mpr.exitCode = true;
    mpr.success = true;

    return mpr;
}

MessagePacketResult icsStop(boost::archive::text_iarchive &ia)
{
    MessagePacketResult mpr;

    Logger::instance().out(L"AA_COMMAND_ICS_STOP");
    mpr.exitCode = IcsManager::instance().stop();
    mpr.success = true;

    return mpr;
}

MessagePacketResult icsChange(boost::archive::text_iarchive &ia)
{
    MessagePacketResult mpr;

    CMD_ICS_CHANGE cmdIcsChange;
    ia >> cmdIcsChange;
    Logger::instance().out(L"AA_COMMAND_ICS_CHANGE");
    mpr.exitCode = IcsManager::instance().change(cmdIcsChange.szAdapterName);
    mpr.success = true;

    return mpr;
}

MessagePacketResult addHosts(boost::archive::text_iarchive &ia)
{
    MessagePacketResult mpr;

    Logger::instance().out(L"AA_COMMAND_ADD_HOSTS");
    CMD_ADD_HOSTS cmdAddHosts;
    ia >> cmdAddHosts;
    mpr.success = HostsEdit::instance().addHosts(cmdAddHosts.hosts);
    mpr.exitCode = 0;

    return mpr;
}

MessagePacketResult removeHosts(boost::archive::text_iarchive &ia)
{
    MessagePacketResult mpr;

    Logger::instance().out(L"AA_COMMAND_REMOVE_HOSTS");
    mpr.success = HostsEdit::instance().removeHosts();
    mpr.exitCode = 0;

    return mpr;
}

MessagePacketResult disableDnsTraffic(boost::archive::text_iarchive &ia)
{
    MessagePacketResult mpr;

    CMD_DISABLE_DNS_TRAFFIC cmdDisableDnsTraffic;
    ia >> cmdDisableDnsTraffic;

    Logger::instance().out(L"AA_COMMAND_DISABLE_DNS_TRAFFIC");
    DnsFirewall::instance().setExcludeIps(cmdDisableDnsTraffic.excludedIps);
    DnsFirewall::instance().enable();
    mpr.success = true;
    mpr.exitCode = 0;

    return mpr;
}

MessagePacketResult enableDnsTraffic(boost::archive::text_iarchive &ia)
{
    MessagePacketResult mpr;

    Logger::instance().out(L"AA_COMMAND_ENABLE_DNS_TRAFFIC");
    DnsFirewall::instance().disable();
    mpr.success = true;
    mpr.exitCode = 0;

    return mpr;
}

MessagePacketResult reinstallWanIkev2(boost::archive::text_iarchive &ia)
{
    MessagePacketResult mpr;

    Logger::instance().out(L"AA_COMMAND_REINSTALL_WAN_IKEV2");
    bool b1 = ReinstallWanIkev2::uninstallDevice();
    bool b2 = ReinstallWanIkev2::scanForHardwareChanges();
    mpr.exitCode = b1 && b2;
    mpr.success = true;

    return mpr;
}

MessagePacketResult enableWanIkev2(boost::archive::text_iarchive &ia)
{
    MessagePacketResult mpr;

    Logger::instance().out(L"AA_COMMAND_ENABLE_WAN_IKEV2");
    mpr.exitCode = ReinstallWanIkev2::enableDevice();
    mpr.success = true;

    return mpr;
}

MessagePacketResult closeTcpConnections(boost::archive::text_iarchive &ia)
{
    MessagePacketResult mpr;

    CMD_CLOSE_TCP_CONNECTIONS cmdCloseTcpConnections;
    ia >> cmdCloseTcpConnections;
    Logger::instance().out(L"AA_COMMAND_CLOSE_TCP_CONNECTIONS, KeepLocalSockets = %d", cmdCloseTcpConnections.isKeepLocalSockets);

    if (g_SplitTunnelingPars.isEnabled && g_SplitTunnelingPars.isVpnConnected) {
        CloseTcpConnections::closeAllTcpConnections(cmdCloseTcpConnections.isKeepLocalSockets, g_SplitTunnelingPars.isExclude, g_SplitTunnelingPars.apps);
    } else {
        CloseTcpConnections::closeAllTcpConnections(cmdCloseTcpConnections.isKeepLocalSockets);
    }
    mpr.success = true;

    return mpr;
}

MessagePacketResult enumProcesses(boost::archive::text_iarchive &ia)
{
    MessagePacketResult mpr;

    Logger::instance().out(L"AA_COMMAND_ENUM_PROCESSES");

    std::vector<std::wstring> list = ActiveProcesses::instance().getList();

    size_t overallCharactersCount = 0;
    for (auto it = list.begin(); it != list.end(); ++it) {
        overallCharactersCount += it->length();
        overallCharactersCount += 1;  // add null character after each process name
    }

    if (overallCharactersCount > 0) {
        std::vector<char> v(overallCharactersCount * sizeof(wchar_t));
        size_t curPos = 0;
        wchar_t *p = (wchar_t *)(&v[0]);
        for (auto it = list.begin(); it != list.end(); ++it) {
            memcpy(p + curPos, it->c_str(), it->length() * sizeof(wchar_t));
            curPos += it->length();
            p[curPos] = L'\0';
            curPos++;
        }
        mpr.additionalString = std::string(v.begin(), v.end());
    }

    mpr.success = true;
    return mpr;
}

MessagePacketResult taskKill(boost::archive::text_iarchive &ia)
{
    CMD_TASK_KILL cmdTaskKill;
    ia >> cmdTaskKill;

    std::wstring killCmd = Utils::getSystemDir() + L"\\taskkill.exe /f /t /im " + cmdTaskKill.szExecutableName;
    Logger::instance().out(L"AA_COMMAND_TASK_KILL, cmd=%s", killCmd.c_str());
    return ExecuteCmd::instance().executeBlockingCmd(killCmd);
}

MessagePacketResult setMetric(boost::archive::text_iarchive &ia)
{
    CMD_SET_METRIC cmdSetMetric;
    ia >> cmdSetMetric;

    std::wstring setMetricCmd = Utils::getSystemDir() + L"\\netsh.exe int " + cmdSetMetric.szInterfaceType + L" set interface interface=\"" +
                                cmdSetMetric.szInterfaceName + L"\" metric=" + cmdSetMetric.szMetricNumber;
    Logger::instance().out(L"AA_COMMAND_SET_METRIC, cmd=%s", setMetricCmd.c_str());
    return ExecuteCmd::instance().executeBlockingCmd(setMetricCmd);
}

MessagePacketResult wmicEnable(boost::archive::text_iarchive &ia)
{
    CMD_WMIC_ENABLE cmdWmicEnable;
    ia >> cmdWmicEnable;

    std::wstring wmicCmd = Utils::getSystemDir() + L"\\wbem\\wmic.exe path win32_networkadapter where description=\"" + cmdWmicEnable.szAdapterName + L"\" call enable";
    Logger::instance().out(L"AA_COMMAND_WMIC_ENABLE, cmd=%s", wmicCmd.c_str());
    return ExecuteCmd::instance().executeBlockingCmd(wmicCmd);
}

MessagePacketResult wmicGetConfigErrorCode(boost::archive::text_iarchive &ia)
{
    CMD_WMIC_GET_CONFIG_ERROR_CODE cmdWmicGetConfigErrorCode;
    ia >> cmdWmicGetConfigErrorCode;

    std::wstring wmicCmd = Utils::getSystemDir() + L"\\wbem\\wmic.exe path win32_networkadapter where description=\"" + cmdWmicGetConfigErrorCode.szAdapterName + L"\" get ConfigManagerErrorCode";
    Logger::instance().out(L"AA_COMMAND_WMIC_GET_CONFIG_ERROR_CODE, cmd=%s", wmicCmd.c_str());
    return ExecuteCmd::instance().executeBlockingCmd(wmicCmd);
}

MessagePacketResult enableBfe(boost::archive::text_iarchive &ia)
{
    Logger::instance().out(L"AA_COMMAND_ENABLE_BFE");

    std::wstring exe = Utils::getSystemDir() + L"\\sc.exe";
    ExecuteCmd::instance().executeBlockingCmd(exe + L" config BFE start= auto");
    return ExecuteCmd::instance().executeBlockingCmd(exe + L" start BFE");
}

MessagePacketResult runOpenvpn(boost::archive::text_iarchive &ia)
{
    MessagePacketResult mpr;

    CMD_RUN_OPENVPN cmdRunOpenVpn;
    ia >> cmdRunOpenVpn;

    // check input parameters
    if (Utils::isValidFileName(cmdRunOpenVpn.szOpenVpnExecutable) &&
        Utils::noSpacesInString(cmdRunOpenVpn.szHttpProxy) &&
        Utils::noSpacesInString(cmdRunOpenVpn.szSocksProxy))
    {
        std::wstring filename;
        int ret = OVPN::writeOVPNFile(filename, cmdRunOpenVpn.szConfig);
        if (ret) {
            // make openvpn command
            std::wstring strCmd = L"\"" + Utils::getExePath() + L"\\" + cmdRunOpenVpn.szOpenVpnExecutable + L"\"";
            strCmd += L" --config \"" + filename + L"\" --management 127.0.0.1 ";
            strCmd += std::to_wstring(cmdRunOpenVpn.portNumber) + L" --management-query-passwords --management-hold --verb 3";

            if (wcslen(cmdRunOpenVpn.szHttpProxy.c_str()) > 0) {
                strCmd += L" --http-proxy " + cmdRunOpenVpn.szHttpProxy + L" " + std::to_wstring(cmdRunOpenVpn.httpPortNumber) + L" auto";
            }
            if (wcslen(cmdRunOpenVpn.szSocksProxy.c_str()) > 0) {
                strCmd += L" --socks-proxy " + cmdRunOpenVpn.szSocksProxy + L" " + std::to_wstring(cmdRunOpenVpn.socksPortNumber);
            }

            // Ensure OpenVPN will only use the wintun adapter instance we created (AA_COMMAND_CREATE_WINTUN_ADAPTER).
            strCmd += L" --dev tun --windows-driver wintun --dev-node ";
            strCmd += WintunController::adapterName();
            return ExecuteCmd::instance().executeUnblockingCmd(strCmd, L"", Utils::getDirPathFromFullPath(filename));
        }
    }

    mpr.success = false;
    return mpr;
}

MessagePacketResult whitelistPorts(boost::archive::text_iarchive &ia)
{
    CMD_WHITELIST_PORTS cmdWhitelistPorts;
    ia >> cmdWhitelistPorts;

    std::wstring strCmd = Utils::getSystemDir() + L"\\netsh.exe advfirewall firewall add rule name=\"WindscribeStaticIpTcp\" protocol=TCP dir=in localport=\"";
    strCmd += cmdWhitelistPorts.ports;
    strCmd += L"\" action=allow";
    Logger::instance().out(L"AA_COMMAND_WHITELIST_PORTS, cmd=%s", strCmd.c_str());
    ExecuteCmd::instance().executeBlockingCmd(strCmd);

    strCmd = Utils::getSystemDir() + L"\\netsh.exe advfirewall firewall add rule name=\"WindscribeStaticIpUdp\" protocol=UDP dir=in localport=\"";
    strCmd += cmdWhitelistPorts.ports;
    strCmd += L"\" action=allow";
    Logger::instance().out(L"AA_COMMAND_WHITELIST_PORTS, cmd=%s", strCmd.c_str());
    return ExecuteCmd::instance().executeBlockingCmd(strCmd);
}

MessagePacketResult deleteWhitelistPorts(boost::archive::text_iarchive &ia)
{
    std::wstring strCmd = Utils::getSystemDir() + L"\\netsh.exe advfirewall firewall delete rule name=\"WindscribeStaticIpTcp\" dir=in";
    Logger::instance().out(L"AA_COMMAND_DELETE_WHITELIST_PORTS, cmd=%s", strCmd.c_str());
    ExecuteCmd::instance().executeBlockingCmd(strCmd);

    strCmd = Utils::getSystemDir() + L"\\netsh.exe advfirewall firewall delete rule name=\"WindscribeStaticIpUdp\" dir=in";
    Logger::instance().out(L"AA_COMMAND_DELETE_WHITELIST_PORTS, cmd=%s", strCmd.c_str());
    return ExecuteCmd::instance().executeBlockingCmd(strCmd);
}

MessagePacketResult setMacAddressRegistryValueSz(boost::archive::text_iarchive &ia)
{
    MessagePacketResult mpr;

    CMD_SET_MAC_ADDRESS_REGISTRY_VALUE_SZ cmdSetMacAddressRegistryValueSz;
    ia >> cmdSetMacAddressRegistryValueSz;

    std::wstring keyPath = L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\" + cmdSetMacAddressRegistryValueSz.szInterfaceName;
    std::wstring propertyName = L"NetworkAddress";
    std::wstring propertyValue = cmdSetMacAddressRegistryValueSz.szValue;

    mpr.success = Registry::regWriteSzProperty(HKEY_LOCAL_MACHINE, keyPath.c_str(), propertyName, propertyValue);

    if (mpr.success) {
        Registry::regWriteDwordProperty(HKEY_LOCAL_MACHINE, keyPath, L"WindscribeMACSpoofed", 1);
    }

    Logger::instance().out(L"AA_COMMAND_SET_MAC_ADDRESS_REGISTRY_VALUE_SZ, path=%s, name=%s, value=%s", keyPath.c_str(), propertyName.c_str(), propertyValue.c_str());
    return mpr;
}

MessagePacketResult removeMacAddressRegistryProperty(boost::archive::text_iarchive &ia)
{
    MessagePacketResult mpr;

    CMD_REMOVE_MAC_ADDRESS_REGISTRY_PROPERTY cmdRemoveMacAddressRegistryProperty;
    ia >> cmdRemoveMacAddressRegistryProperty;

    std::wstring propertyName = L"NetworkAddress";
    std::wstring keyPath = L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\" + cmdRemoveMacAddressRegistryProperty.szInterfaceName;

    wchar_t keyPathSz[128];
    wcsncpy_s(keyPathSz, 128, keyPath.c_str(), _TRUNCATE);

    bool success1 = Registry::regDeleteProperty(HKEY_LOCAL_MACHINE, keyPathSz, propertyName);
    bool success2 = false;

    if (success1) {
        success2 = Registry::regDeleteProperty(HKEY_LOCAL_MACHINE, keyPathSz, L"WindscribeMACSpoofed");
    }

    mpr.success = success1 && success2;

    Logger::instance().out(L"AA_COMMAND_REMOVE_MAC_ADDRESS_REGISTRY_PROPERTY, key=%s, name=%s", keyPathSz, propertyName.c_str());
    return mpr;
}

MessagePacketResult resetNetworkAdapter(boost::archive::text_iarchive &ia)
{
    MessagePacketResult mpr;

    CMD_RESET_NETWORK_ADAPTER cmdResetNetworkAdapter;
    ia >> cmdResetNetworkAdapter;

    std::wstring adapterRegistryName = cmdResetNetworkAdapter.szInterfaceName;

    Logger::instance().out(L"AA_COMMAND_RESET_NETWORK_ADAPTER, Disable %s", adapterRegistryName.c_str());
    Utils::callNetworkAdapterMethod(L"Disable", adapterRegistryName);

    if (cmdResetNetworkAdapter.bringBackUp) {
        Logger::instance().out(L"AA_COMMAND_RESET_NETWORK_ADAPTER, Enable %s", adapterRegistryName.c_str());
        Utils::callNetworkAdapterMethod(L"Enable", adapterRegistryName);
    }

    mpr.success = true;
    return mpr;
}

MessagePacketResult splitTunnelingSettings(boost::archive::text_iarchive &ia)
{
    MessagePacketResult mpr;

    CMD_SPLIT_TUNNELING_SETTINGS cmdSplitTunnelingSettings;
    ia >> cmdSplitTunnelingSettings;

    Logger::instance().out(L"AA_COMMAND_SPLIT_TUNNELING_SETTINGS, isEnabled: %d, isExclude: %d,"
                           L" isAllowLanTraffic: %d, cntApps: %d",
                           cmdSplitTunnelingSettings.isActive,
                           cmdSplitTunnelingSettings.isExclude,
                           cmdSplitTunnelingSettings.isAllowLanTraffic,
                           cmdSplitTunnelingSettings.files.size());

    for (size_t i = 0; i < cmdSplitTunnelingSettings.files.size(); ++i) {
        Logger::instance().out(L"AA_COMMAND_SPLIT_TUNNELING_SETTINGS: %s",
            cmdSplitTunnelingSettings.files[i].substr(cmdSplitTunnelingSettings.files[i].find_last_of(L"/\\") + 1).c_str());
    }

    for (size_t i = 0; i < cmdSplitTunnelingSettings.ips.size(); ++i) {
        Logger::instance().out(L"AA_COMMAND_SPLIT_TUNNELING_SETTINGS: %s",
            cmdSplitTunnelingSettings.ips[i].c_str());
    }

    for (size_t i = 0; i < cmdSplitTunnelingSettings.hosts.size(); ++i) {
        Logger::instance().out("AA_COMMAND_SPLIT_TUNNELING_SETTINGS: %s",
            cmdSplitTunnelingSettings.hosts[i].c_str());
    }


    SplitTunneling::instance().setSettings(cmdSplitTunnelingSettings.isActive,
                                           cmdSplitTunnelingSettings.isExclude,
                                           cmdSplitTunnelingSettings.files,
                                           cmdSplitTunnelingSettings.ips,
                                           cmdSplitTunnelingSettings.hosts,
                                           cmdSplitTunnelingSettings.isAllowLanTraffic);

    g_SplitTunnelingPars.isEnabled = cmdSplitTunnelingSettings.isActive;
    g_SplitTunnelingPars.isExclude = cmdSplitTunnelingSettings.isExclude;
    g_SplitTunnelingPars.apps = cmdSplitTunnelingSettings.files;

    mpr.success = true;
    return mpr;
}

MessagePacketResult addIkev2DefaultRoute(boost::archive::text_iarchive &ia)
{
    MessagePacketResult mpr;

    mpr.success = IKEv2Route::addRouteForIKEv2();
    Logger::instance().out(L"AA_COMMAND_ADD_IKEV2_DEFAULT_ROUTE: success = %d", mpr.success);
    return mpr;
}

MessagePacketResult removeWindscribeNetworkProfiles(boost::archive::text_iarchive &ia)
{
    MessagePacketResult mpr;

    Logger::instance().out(L"AA_COMMAND_REMOVE_WINDSCRIBE_NETWORK_PROFILES");
    RemoveWindscribeNetworkProfiles::remove();
    mpr.success = true;
    return mpr;
}

MessagePacketResult changeMtu(boost::archive::text_iarchive &ia)
{
    CMD_CHANGE_MTU cmdChangeMtu;
    ia >> cmdChangeMtu;

    std::wstring netshCmd = Utils::getSystemDir() + L"\\netsh.exe interface ipv4 set subinterface \"" + cmdChangeMtu.szAdapterName + L"\" mtu=" + std::to_wstring(cmdChangeMtu.mtu) + L" store=";
    if (cmdChangeMtu.storePersistent) {
        netshCmd += L"persistent";
    } else {
        netshCmd += L"active";
    }

    Logger::instance().out(L"AA_COMMAND_CHANGE_MTU: " + netshCmd);
    return ExecuteCmd::instance().executeBlockingCmd(netshCmd);
}

MessagePacketResult resetAndStartRas(boost::archive::text_iarchive &ia)
{
    Logger::instance().out(L"AA_COMMAND_RESET_AND_START_RAS");

    std::wstring exe = Utils::getSystemDir() + L"\\sc.exe";
    ExecuteCmd::instance().executeBlockingCmd(exe + L" config RasMan start= demand");
    ExecuteCmd::instance().executeBlockingCmd(exe + L" stop RasMan");
    ExecuteCmd::instance().executeBlockingCmd(exe + L" start RasMan");
    ExecuteCmd::instance().executeBlockingCmd(exe + L" config SstpSvc start= demand");
    ExecuteCmd::instance().executeBlockingCmd(exe + L" stop SstpSvc");
    return ExecuteCmd::instance().executeBlockingCmd(exe + L" start SstpSvc");
}

MessagePacketResult setIkev2IpsecParameters(boost::archive::text_iarchive &ia)
{
    MessagePacketResult mpr;

    mpr.success = IKEv2IPSec::setIKEv2IPSecParameters();
    Logger::instance().out(L"AA_COMMAND_SET_IKEV2_IPSEC_PARAMETERS: success = %d", mpr.success);

    return mpr;
}

MessagePacketResult startWireGuard(boost::archive::text_iarchive &ia)
{
    MessagePacketResult mpr;

    CMD_START_WIREGUARD cmdStartWireGuard;
    ia >> cmdStartWireGuard;

    mpr.success = WireGuardController::instance().installService(cmdStartWireGuard.szExecutable, cmdStartWireGuard.szDeviceName);
    Logger::instance().out(L"AA_COMMAND_START_WIREGUARD: success = %d", mpr.success);

    return mpr;
}

MessagePacketResult stopWireGuard(boost::archive::text_iarchive &ia)
{
    MessagePacketResult mpr;

    mpr.success = WireGuardController::instance().deleteService();
    Logger::instance().out(L"AA_COMMAND_STOP_WIREGUARD");

    return mpr;
}

MessagePacketResult getWireGuardStatus(boost::archive::text_iarchive &ia)
{
    MessagePacketResult mpr;

    mpr.success = true;
    mpr.exitCode = WireGuardController::instance().getStatus(mpr.customInfoValue[0], mpr.customInfoValue[1], mpr.customInfoValue[2]);

    return mpr;
}

MessagePacketResult connectStatus(boost::archive::text_iarchive &ia)
{
    MessagePacketResult mpr;

    CMD_CONNECT_STATUS cmdConnectStatus;
    ia >> cmdConnectStatus;
    Logger::instance().out(L"AA_COMMAND_CONNECT_STATUS: %d", cmdConnectStatus.isConnected);
    mpr.success = SplitTunneling::instance().setConnectStatus(cmdConnectStatus);
    g_SplitTunnelingPars.isVpnConnected = cmdConnectStatus.isConnected;

    return mpr;
}

MessagePacketResult suspendUnblockingCmd(boost::archive::text_iarchive &ia)
{
    CMD_SUSPEND_UNBLOCKING_CMD cmdSuspendUnblockingCmd;
    ia >> cmdSuspendUnblockingCmd;
    return ExecuteCmd::instance().suspendUnblockingCmd(cmdSuspendUnblockingCmd.blockingCmdId);
}

MessagePacketResult connectedDns(boost::archive::text_iarchive &ia)
{
    MessagePacketResult mpr;

    CMD_CONNECTED_DNS cmdDnsWhileConnected;
    ia >> cmdDnsWhileConnected;

    std::wstring netshCmd = Utils::getSystemDir() + L"\\netsh.exe interface ipv4 set dns \"" + std::to_wstring(cmdDnsWhileConnected.ifIndex) + L"\" static " + cmdDnsWhileConnected.szDnsIpAddress;
    mpr = ExecuteCmd::instance().executeBlockingCmd(netshCmd);

    std::wstring logBuf = L"AA_COMMAND_CONNECTED_DNS: " + netshCmd + L" " + (mpr.success ? L"Success" : L"Failure") + L" " + std::to_wstring(mpr.exitCode);
    Logger::instance().out(logBuf);

    return mpr;
}

MessagePacketResult makeHostsFileWritable(boost::archive::text_iarchive &ia)
{
    MessagePacketResult mpr;

    const auto hostsPath = L"C:\\Windows\\System32\\drivers\\etc\\hosts";
    if (SetFileAttributes(hostsPath, GetFileAttributes(hostsPath) & ~FILE_ATTRIBUTE_READONLY)) {
        mpr.success = true;
    } else {
        Logger::instance().out(L"Can't change permissions of \"hosts\" file.");
        mpr.success = false;
    }

    return mpr;
}

MessagePacketResult createWintunAdapter(boost::archive::text_iarchive &ia) {
    MessagePacketResult mpr;

    mpr.success = WintunController::instance().createAdapter();
    return mpr;
}

MessagePacketResult removeWintunAdapter(boost::archive::text_iarchive &ia) {
    MessagePacketResult mpr;

    WintunController::instance().removeAdapter();
    mpr.success = true;
    return mpr;
}
