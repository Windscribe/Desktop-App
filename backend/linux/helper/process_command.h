#pragma once

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <map>
#include <string>

#include "helper_commands.h"
#include "helper_commands_serialize.h"

CMD_ANSWER startOpenvpn(boost::archive::text_iarchive &ia);
CMD_ANSWER getCmdStatus(boost::archive::text_iarchive &ia);
CMD_ANSWER clearCmds(boost::archive::text_iarchive &ia);
CMD_ANSWER splitTunnelingSettings(boost::archive::text_iarchive &ia);
CMD_ANSWER sendConnectStatus(boost::archive::text_iarchive &ia);
CMD_ANSWER startWireGuard(boost::archive::text_iarchive &ia);
CMD_ANSWER stopWireGuard(boost::archive::text_iarchive &ia);
CMD_ANSWER configureWireGuard(boost::archive::text_iarchive &ia);
CMD_ANSWER getWireGuardStatus(boost::archive::text_iarchive &ia);
CMD_ANSWER changeMtu(boost::archive::text_iarchive &ia);
CMD_ANSWER setDnsLeakProtectEnabled(boost::archive::text_iarchive &ia);
CMD_ANSWER clearFirewallRules(boost::archive::text_iarchive &ia);
CMD_ANSWER checkFirewallState(boost::archive::text_iarchive &ia);
CMD_ANSWER setFirewallRules(boost::archive::text_iarchive &ia);
CMD_ANSWER getFirewallRules(boost::archive::text_iarchive &ia);
CMD_ANSWER setFirewallOnBoot(boost::archive::text_iarchive &ia);
CMD_ANSWER setMacAddress(boost::archive::text_iarchive &ia);
CMD_ANSWER taskKill(boost::archive::text_iarchive &ia);
CMD_ANSWER startCtrld(boost::archive::text_iarchive &ia);
CMD_ANSWER startStunnel(boost::archive::text_iarchive &ia);
CMD_ANSWER startWstunnel(boost::archive::text_iarchive &ia);
CMD_ANSWER resetMacAddresses(boost::archive::text_iarchive &ia);

static const std::map<const int, std::function<CMD_ANSWER(boost::archive::text_iarchive &)>> kCommands = {
    { HELPER_CMD_START_OPENVPN, startOpenvpn },
    { HELPER_CMD_GET_CMD_STATUS, getCmdStatus },
    { HELPER_CMD_CLEAR_CMDS, clearCmds },
    { HELPER_CMD_SPLIT_TUNNELING_SETTINGS, splitTunnelingSettings },
    { HELPER_CMD_SEND_CONNECT_STATUS, sendConnectStatus },
    { HELPER_CMD_START_WIREGUARD, startWireGuard },
    { HELPER_CMD_STOP_WIREGUARD, stopWireGuard },
    { HELPER_CMD_CONFIGURE_WIREGUARD, configureWireGuard },
    { HELPER_CMD_GET_WIREGUARD_STATUS, getWireGuardStatus },
    { HELPER_CMD_CHANGE_MTU, changeMtu },
    { HELPER_CMD_SET_DNS_LEAK_PROTECT_ENABLED, setDnsLeakProtectEnabled },
    { HELPER_CMD_CLEAR_FIREWALL_RULES, clearFirewallRules },
    { HELPER_CMD_CHECK_FIREWALL_STATE, checkFirewallState },
    { HELPER_CMD_SET_FIREWALL_RULES, setFirewallRules },
    { HELPER_CMD_GET_FIREWALL_RULES, getFirewallRules },
    { HELPER_CMD_SET_FIREWALL_ON_BOOT, setFirewallOnBoot },
    { HELPER_CMD_SET_MAC_ADDRESS, setMacAddress },
    { HELPER_CMD_TASK_KILL, taskKill },
    { HELPER_CMD_START_CTRLD, startCtrld },
    { HELPER_CMD_START_STUNNEL, startStunnel },
    { HELPER_CMD_START_WSTUNNEL, startWstunnel },
    { HELPER_CMD_RESET_MAC_ADDRESSES, resetMacAddresses },
};

CMD_ANSWER processCommand(int cmdId, const std::string packet);
