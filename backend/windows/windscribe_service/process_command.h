#pragma once

#include <conio.h>
#include <functional>
#include <map>

#include "all_headers.h"

#include "active_processes.h"
#include "dns_firewall.h"
#include "firewallfilter.h"
#include "fwpm_wrapper.h"
#include "hostsedit.h"
#include "ipc/servicecommunication.h"
#include "ipc/serialize_structs.h"
#include "ipv6_firewall.h"
#include "split_tunneling/split_tunneling.h"
#include "sys_ipv6_controller.h"
#include "wireguard/wireguardcontroller.h"

struct SPLIT_TUNNELING_PARS
{
    bool isEnabled;
    bool isExclude;
    std::vector<std::wstring> apps;
    bool isVpnConnected;

    SPLIT_TUNNELING_PARS() : isEnabled(false), isExclude(true), isVpnConnected(false) {}
};

MessagePacketResult firewallOn(boost::archive::text_iarchive &ia);
MessagePacketResult firewallOff(boost::archive::text_iarchive &ia);
MessagePacketResult firewallStatus(boost::archive::text_iarchive &ia);
MessagePacketResult firewallIpv6Enable(boost::archive::text_iarchive &ia);
MessagePacketResult firewallIpv6Disable(boost::archive::text_iarchive &ia);
MessagePacketResult removeFromHosts(boost::archive::text_iarchive &ia);
MessagePacketResult checkUnblockingCmdStatus(boost::archive::text_iarchive &ia);
MessagePacketResult getUnblockingCmdCount(boost::archive::text_iarchive &ia);
MessagePacketResult clearUnblockingCmd(boost::archive::text_iarchive &ia);
MessagePacketResult getHelperVersion(boost::archive::text_iarchive &ia);
MessagePacketResult ipv6State(boost::archive::text_iarchive &ia);
MessagePacketResult ipv6Enable(boost::archive::text_iarchive &ia);
MessagePacketResult ipv6Disable(boost::archive::text_iarchive &ia);
MessagePacketResult icsIsSupported(boost::archive::text_iarchive &ia);
MessagePacketResult icsStart(boost::archive::text_iarchive &ia);
MessagePacketResult icsChange(boost::archive::text_iarchive &ia);
MessagePacketResult icsStop(boost::archive::text_iarchive &ia);
MessagePacketResult addHosts(boost::archive::text_iarchive &ia);
MessagePacketResult removeHosts(boost::archive::text_iarchive &ia);
MessagePacketResult disableDnsTraffic(boost::archive::text_iarchive &ia);
MessagePacketResult enableDnsTraffic(boost::archive::text_iarchive &ia);
MessagePacketResult reinstallWanIkev2(boost::archive::text_iarchive &ia);
MessagePacketResult enableWanIkev2(boost::archive::text_iarchive &ia);
MessagePacketResult closeTcpConnections(boost::archive::text_iarchive &ia);
MessagePacketResult enumProcesses(boost::archive::text_iarchive &ia);
MessagePacketResult taskKill(boost::archive::text_iarchive &ia);
MessagePacketResult setMetric(boost::archive::text_iarchive &ia);
MessagePacketResult wmicGetConfigErrorCode(boost::archive::text_iarchive &ia);
MessagePacketResult enableBfe(boost::archive::text_iarchive &ia);
MessagePacketResult runOpenvpn(boost::archive::text_iarchive &ia);
MessagePacketResult whitelistPorts(boost::archive::text_iarchive &ia);
MessagePacketResult deleteWhitelistPorts(boost::archive::text_iarchive &ia);
MessagePacketResult setMacAddressRegistryValueSz(boost::archive::text_iarchive &ia);
MessagePacketResult removeMacAddressRegistryProperty(boost::archive::text_iarchive &ia);
MessagePacketResult resetNetworkAdapter(boost::archive::text_iarchive &ia);
MessagePacketResult splitTunnelingSettings(boost::archive::text_iarchive &ia);
MessagePacketResult addIkev2DefaultRoute(boost::archive::text_iarchive &ia);
MessagePacketResult removeWindscribeNetworkProfiles(boost::archive::text_iarchive &ia);
MessagePacketResult changeMtu(boost::archive::text_iarchive &ia);
MessagePacketResult resetAndStartRas(boost::archive::text_iarchive &ia);
MessagePacketResult setIkev2IpsecParameters(boost::archive::text_iarchive &ia);
MessagePacketResult startWireGuard(boost::archive::text_iarchive &ia);
MessagePacketResult configureWireGuard(boost::archive::text_iarchive &ia);
MessagePacketResult stopWireGuard(boost::archive::text_iarchive &ia);
MessagePacketResult getWireGuardStatus(boost::archive::text_iarchive &ia);
MessagePacketResult connectStatus(boost::archive::text_iarchive &ia);
MessagePacketResult suspendUnblockingCmd(boost::archive::text_iarchive &ia);
MessagePacketResult connectedDns(boost::archive::text_iarchive &ia);
MessagePacketResult makeHostsFileWritable(boost::archive::text_iarchive &ia);
MessagePacketResult createOpenVPNAdapter(boost::archive::text_iarchive &ia);
MessagePacketResult removeOpenVPNAdapter(boost::archive::text_iarchive &ia);
MessagePacketResult disableDohSettings(boost::archive::text_iarchive &ia);
MessagePacketResult enableDohSettings(boost::archive::text_iarchive &ia);
MessagePacketResult ssidFromInterfaceGUID(boost::archive::text_iarchive &ia);

static const std::map<const int, std::function<MessagePacketResult(boost::archive::text_iarchive &)>> kCommands = {
    { AA_COMMAND_FIREWALL_ON, firewallOn },
    { AA_COMMAND_FIREWALL_OFF, firewallOff },
    { AA_COMMAND_FIREWALL_STATUS, firewallStatus },
    { AA_COMMAND_FIREWALL_IPV6_ENABLE, firewallIpv6Enable },
    { AA_COMMAND_FIREWALL_IPV6_DISABLE, firewallIpv6Disable },
    { AA_COMMAND_REMOVE_WINDSCRIBE_FROM_HOSTS, removeFromHosts },
    { AA_COMMAND_CHECK_UNBLOCKING_CMD_STATUS, checkUnblockingCmdStatus },
    { AA_COMMAND_GET_UNBLOCKING_CMD_COUNT, getUnblockingCmdCount },
    { AA_COMMAND_CLEAR_UNBLOCKING_CMD, clearUnblockingCmd },
    { AA_COMMAND_GET_HELPER_VERSION, getHelperVersion },
    { AA_COMMAND_OS_IPV6_STATE, ipv6State },
    { AA_COMMAND_OS_IPV6_ENABLE, ipv6Enable },
    { AA_COMMAND_OS_IPV6_DISABLE, ipv6Disable },
    { AA_COMMAND_ICS_IS_SUPPORTED, icsIsSupported },
    { AA_COMMAND_ICS_START, icsStart },
    { AA_COMMAND_ICS_CHANGE, icsChange },
    { AA_COMMAND_ICS_STOP, icsStop },
    { AA_COMMAND_ADD_HOSTS, addHosts },
    { AA_COMMAND_REMOVE_HOSTS, removeHosts },
    { AA_COMMAND_DISABLE_DNS_TRAFFIC, disableDnsTraffic },
    { AA_COMMAND_ENABLE_DNS_TRAFFIC, enableDnsTraffic },
    { AA_COMMAND_REINSTALL_WAN_IKEV2, reinstallWanIkev2 },
    { AA_COMMAND_ENABLE_WAN_IKEV2, enableWanIkev2 },
    { AA_COMMAND_CLOSE_TCP_CONNECTIONS, closeTcpConnections },
    { AA_COMMAND_ENUM_PROCESSES, enumProcesses },
    { AA_COMMAND_TASK_KILL, taskKill },
    { AA_COMMAND_SET_METRIC, setMetric },
    { AA_COMMAND_WMIC_GET_CONFIG_ERROR_CODE, wmicGetConfigErrorCode },
    { AA_COMMAND_ENABLE_BFE, enableBfe },
    { AA_COMMAND_RUN_OPENVPN, runOpenvpn },
    { AA_COMMAND_WHITELIST_PORTS, whitelistPorts },
    { AA_COMMAND_DELETE_WHITELIST_PORTS, deleteWhitelistPorts },
    { AA_COMMAND_SET_MAC_ADDRESS_REGISTRY_VALUE_SZ, setMacAddressRegistryValueSz },
    { AA_COMMAND_REMOVE_MAC_ADDRESS_REGISTRY_PROPERTY, removeMacAddressRegistryProperty },
    { AA_COMMAND_RESET_NETWORK_ADAPTER, resetNetworkAdapter },
    { AA_COMMAND_SPLIT_TUNNELING_SETTINGS, splitTunnelingSettings },
    { AA_COMMAND_ADD_IKEV2_DEFAULT_ROUTE, addIkev2DefaultRoute },
    { AA_COMMAND_REMOVE_WINDSCRIBE_NETWORK_PROFILES, removeWindscribeNetworkProfiles },
    { AA_COMMAND_CHANGE_MTU, changeMtu },
    { AA_COMMAND_RESET_AND_START_RAS, resetAndStartRas },
    { AA_COMMAND_SET_IKEV2_IPSEC_PARAMETERS, setIkev2IpsecParameters },
    { AA_COMMAND_START_WIREGUARD, startWireGuard },
    { AA_COMMAND_CONFIGURE_WIREGUARD, configureWireGuard },
    { AA_COMMAND_STOP_WIREGUARD, stopWireGuard },
    { AA_COMMAND_GET_WIREGUARD_STATUS, getWireGuardStatus },
    { AA_COMMAND_CONNECT_STATUS, connectStatus },
    { AA_COMMAND_SUSPEND_UNBLOCKING_CMD, suspendUnblockingCmd },
    { AA_COMMAND_CONNECTED_DNS, connectedDns },
    { AA_COMMAND_MAKE_HOSTS_FILE_WRITABLE, makeHostsFileWritable },
    { AA_COMMAND_CREATE_OPENVPN_ADAPTER, createOpenVPNAdapter },
    { AA_COMMAND_REMOVE_OPENVPN_ADAPTER, removeOpenVPNAdapter },
    { AA_COMMAND_DISABLE_DOH_SETTINGS, disableDohSettings },
    { AA_COMMAND_ENABLE_DOH_SETTINGS, enableDohSettings },
    { AA_COMMAND_SSID_FROM_INTERFACE_GUID, ssidFromInterfaceGUID }
};

MessagePacketResult processCommand(int cmdId, const std::string &packet);
