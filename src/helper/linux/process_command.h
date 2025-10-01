#pragma once

#include <sstream>
#include <map>
#include <string>
#include "../common/helper_commands.h"

std::string getHelperVersion(const std::string &pars);
std::string getUnblockingCmdStatus(const std::string &pars);
std::string clearUnblockingCmd(const std::string &pars);
std::string setSplitTunnelingSettings(const std::string &pars);
std::string sendConnectStatus(const std::string &pars);
std::string changeMtu(const std::string &pars);
std::string executeOpenVPN(const std::string &pars);
std::string executeTaskKill(const std::string &pars);
std::string startWireGuard(const std::string &pars);
std::string stopWireGuard(const std::string &pars);
std::string configureWireGuard(const std::string &pars);
std::string getWireGuardStatus(const std::string &pars);
std::string startCtrld(const std::string &pars);
std::string checkFirewallState(const std::string &pars);
std::string clearFirewallRules(const std::string &pars);
std::string setFirewallRules(const std::string &pars);
std::string getFirewallRules(const std::string &pars);
std::string setFirewallOnBoot(const std::string &pars);
std::string startStunnel(const std::string &pars);
std::string startWstunnel(const std::string &pars);
std::string setMacAddress(const std::string &pars);
std::string setDnsLeakProtectEnabled(const std::string &pars);
std::string resetMacAddresses(const std::string &pars);

static const std::map<const HelperCommand, std::function<std::string(const std::string &)>> kCommands = {
    { HelperCommand::getHelperVersion, getHelperVersion },
    { HelperCommand::getUnblockingCmdStatus, getUnblockingCmdStatus },
    { HelperCommand::clearUnblockingCmd, clearUnblockingCmd },
    { HelperCommand::setSplitTunnelingSettings, setSplitTunnelingSettings },
    { HelperCommand::sendConnectStatus, sendConnectStatus },
    { HelperCommand::changeMtu, changeMtu },
    { HelperCommand::executeOpenVPN, executeOpenVPN },
    { HelperCommand::executeTaskKill, executeTaskKill },
    { HelperCommand::startWireGuard, startWireGuard },
    { HelperCommand::stopWireGuard, stopWireGuard },
    { HelperCommand::configureWireGuard, configureWireGuard },
    { HelperCommand::getWireGuardStatus, getWireGuardStatus },
    { HelperCommand::startCtrld, startCtrld },
    { HelperCommand::checkFirewallState, checkFirewallState },
    { HelperCommand::clearFirewallRules, clearFirewallRules },
    { HelperCommand::setFirewallRules, setFirewallRules },
    { HelperCommand::getFirewallRules, getFirewallRules },
    { HelperCommand::setFirewallOnBoot, setFirewallOnBoot },
    { HelperCommand::startStunnel, startStunnel },
    { HelperCommand::startWstunnel, startWstunnel },
    { HelperCommand::setMacAddress, setMacAddress },
    { HelperCommand::setDnsLeakProtectEnabled, setDnsLeakProtectEnabled },
    { HelperCommand::resetMacAddresses, resetMacAddresses }
};

std::string processCommand(HelperCommand cmdId, const std::string &pars);
