#pragma once

#include <string>
#include <map>
#include <functional>
#include "../common/helper_commands.h"

std::string setSplitTunnelingSettings(const std::string &pars);
std::string sendConnectStatus(const std::string &pars);
std::string changeMtu(const std::string &pars);
std::string executeOpenVPN(const std::string &pars);
std::string executeTaskKill(const std::string &pars);
std::string startWireGuard(const std::string &pars);
std::string stopWireGuard(const std::string &pars);
std::string configureWireGuard(const std::string &pars);
std::string getWireGuardStatus(const std::string &pars);
std::string firewallOn(const std::string &pars);
std::string firewallOff(const std::string &pars);
std::string firewallActualState(const std::string &pars);
std::string setCustomDnsWhileConnected(const std::string &pars);
std::string executeSetMetric(const std::string &pars);
std::string executeWmicGetConfigManagerErrorCode(const std::string &pars);
std::string isIcsSupported(const std::string &pars);
std::string startIcs(const std::string &pars);
std::string changeIcs(const std::string &pars);
std::string stopIcs(const std::string &pars);
std::string enableBFE(const std::string &pars);
std::string resetAndStartRAS(const std::string &pars);
std::string setIPv6EnabledInFirewall(const std::string &pars);
std::string setFirewallOnBoot(const std::string &pars);
std::string addHosts(const std::string &pars);
std::string removeHosts(const std::string &pars);
std::string closeAllTcpConnections(const std::string &pars);
std::string getProcessesList(const std::string &pars);
std::string whitelistPorts(const std::string &pars);
std::string deleteWhitelistPorts(const std::string &pars);
std::string enableDnsLeaksProtection(const std::string &pars);
std::string disableDnsLeaksProtection(const std::string &pars);
std::string reinstallWanIkev2(const std::string &pars);
std::string enableWanIkev2(const std::string &pars);
std::string setMacAddressSpoof(const std::string &pars);
std::string removeMacAddressSpoof(const std::string &pars);
std::string resetNetworkAdapter(const std::string &pars);
std::string addIKEv2DefaultRoute(const std::string &pars);
std::string removeWindscribeNetworkProfiles(const std::string &pars);
std::string setIKEv2IPSecParameters(const std::string &pars);
std::string makeHostsFileWritable(const std::string &pars);
std::string createOpenVpnAdapter(const std::string &pars);
std::string removeOpenVpnAdapter(const std::string &pars);
std::string disableDohSettings(const std::string &pars);
std::string enableDohSettings(const std::string &pars);
std::string ssidFromInterfaceGUID(const std::string &pars);

static const std::map<const HelperCommand, std::function<std::string(const std::string &)>> kCommands = {
    { HelperCommand::setSplitTunnelingSettings, setSplitTunnelingSettings },
    { HelperCommand::sendConnectStatus, sendConnectStatus },
    { HelperCommand::changeMtu, changeMtu },
    { HelperCommand::executeOpenVPN, executeOpenVPN },
    { HelperCommand::executeTaskKill, executeTaskKill },
    { HelperCommand::startWireGuard, startWireGuard },
    { HelperCommand::stopWireGuard, stopWireGuard },
    { HelperCommand::configureWireGuard, configureWireGuard },
    { HelperCommand::getWireGuardStatus, getWireGuardStatus },
    { HelperCommand::firewallOn, firewallOn },
    { HelperCommand::firewallOff, firewallOff },
    { HelperCommand::firewallActualState, firewallActualState },
    { HelperCommand::setCustomDnsWhileConnected, setCustomDnsWhileConnected },
    { HelperCommand::executeSetMetric, executeSetMetric },
    { HelperCommand::executeWmicGetConfigManagerErrorCode, executeWmicGetConfigManagerErrorCode },
    { HelperCommand::isIcsSupported, isIcsSupported },
    { HelperCommand::startIcs, startIcs },
    { HelperCommand::changeIcs, changeIcs },
    { HelperCommand::stopIcs, stopIcs },
    { HelperCommand::enableBFE, enableBFE },
    { HelperCommand::resetAndStartRAS, resetAndStartRAS },
    { HelperCommand::setIPv6EnabledInFirewall, setIPv6EnabledInFirewall },
    { HelperCommand::setFirewallOnBoot, setFirewallOnBoot },
    { HelperCommand::addHosts, addHosts },
    { HelperCommand::removeHosts, removeHosts },
    { HelperCommand::closeAllTcpConnections, closeAllTcpConnections },
    { HelperCommand::getProcessesList, getProcessesList },
    { HelperCommand::whitelistPorts, whitelistPorts },
    { HelperCommand::deleteWhitelistPorts, deleteWhitelistPorts },
    { HelperCommand::enableDnsLeaksProtection, enableDnsLeaksProtection },
    { HelperCommand::disableDnsLeaksProtection, disableDnsLeaksProtection },
    { HelperCommand::reinstallWanIkev2, reinstallWanIkev2 },
    { HelperCommand::enableWanIkev2, enableWanIkev2 },
    { HelperCommand::setMacAddressRegistryValueSz, setMacAddressSpoof },
    { HelperCommand::removeMacAddressRegistryProperty, removeMacAddressSpoof },
    { HelperCommand::resetNetworkAdapter, resetNetworkAdapter },
    { HelperCommand::addIKEv2DefaultRoute, addIKEv2DefaultRoute },
    { HelperCommand::removeWindscribeNetworkProfiles, removeWindscribeNetworkProfiles },
    { HelperCommand::setIKEv2IPSecParameters, setIKEv2IPSecParameters },
    { HelperCommand::makeHostsFileWritable, makeHostsFileWritable },
    { HelperCommand::createOpenVpnAdapter, createOpenVpnAdapter },
    { HelperCommand::removeOpenVpnAdapter, removeOpenVpnAdapter },
    { HelperCommand::disableDohSettings, disableDohSettings },
    { HelperCommand::enableDohSettings, enableDohSettings },
    { HelperCommand::ssidFromInterfaceGUID, ssidFromInterfaceGUID },
};

std::string processCommand(HelperCommand cmdId, const std::string &pars);
