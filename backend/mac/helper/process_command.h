#pragma once

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <map>
#include <string>

#include "../../posix_common/helper_commands.h"
#include "../../posix_common/helper_commands_serialize.h"

CMD_ANSWER startOpenvpn(boost::archive::text_iarchive &ia);
CMD_ANSWER getCmdStatus(boost::archive::text_iarchive &ia);
CMD_ANSWER clearCmds(boost::archive::text_iarchive &ia);
CMD_ANSWER splitTunnelingSettings(boost::archive::text_iarchive &ia);
CMD_ANSWER sendConnectStatus(boost::archive::text_iarchive &ia);
CMD_ANSWER startWireGuard(boost::archive::text_iarchive &ia);
CMD_ANSWER stopWireGuard(boost::archive::text_iarchive &ia);
CMD_ANSWER configureWireGuard(boost::archive::text_iarchive &ia);
CMD_ANSWER getWireGuardStatus(boost::archive::text_iarchive &ia);
CMD_ANSWER installerSetPath(boost::archive::text_iarchive &ia);
CMD_ANSWER installerExecuteCopyFile(boost::archive::text_iarchive &ia);
CMD_ANSWER installerRemoveOldInstall(boost::archive::text_iarchive &ia);
CMD_ANSWER applyCustomDns(boost::archive::text_iarchive &ia);
CMD_ANSWER changeMtu(boost::archive::text_iarchive &ia);
CMD_ANSWER deleteRoute(boost::archive::text_iarchive &ia);
CMD_ANSWER setIpv6Enabled(boost::archive::text_iarchive &ia);
CMD_ANSWER setDnsScriptEnabled(boost::archive::text_iarchive &ia);
CMD_ANSWER clearFirewallRules(boost::archive::text_iarchive &ia);
CMD_ANSWER checkFirewallState(boost::archive::text_iarchive &ia);
CMD_ANSWER setFirewallRules(boost::archive::text_iarchive &ia);
CMD_ANSWER getFirewallRules(boost::archive::text_iarchive &ia);
CMD_ANSWER deleteOldHelper(boost::archive::text_iarchive &ia);
CMD_ANSWER setFirewallOnBoot(boost::archive::text_iarchive &ia);
CMD_ANSWER setMacSpoofingOnBoot(boost::archive::text_iarchive &ia);
CMD_ANSWER setMacAddress(boost::archive::text_iarchive &ia);
CMD_ANSWER taskKill(boost::archive::text_iarchive &ia);
CMD_ANSWER startCtrld(boost::archive::text_iarchive &ia);
CMD_ANSWER startStunnel(boost::archive::text_iarchive &ia);
CMD_ANSWER startWstunnel(boost::archive::text_iarchive &ia);
CMD_ANSWER installerCreateCliSymlinkDir(boost::archive::text_iarchive &ia);
CMD_ANSWER getHelperVersion(boost::archive::text_iarchive &ia);
CMD_ANSWER getInterfaceSsid(boost::archive::text_iarchive &ia);

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
    { HELPER_CMD_INSTALLER_SET_PATH, installerSetPath },
    { HELPER_CMD_INSTALLER_EXECUTE_COPY_FILE, installerExecuteCopyFile },
    { HELPER_CMD_INSTALLER_REMOVE_OLD_INSTALL, installerRemoveOldInstall },
    { HELPER_CMD_APPLY_CUSTOM_DNS, applyCustomDns },
    { HELPER_CMD_CHANGE_MTU, changeMtu },
    { HELPER_CMD_DELETE_ROUTE, deleteRoute },
    { HELPER_CMD_SET_IPV6_ENABLED, setIpv6Enabled },
    { HELPER_CMD_SET_DNS_SCRIPT_ENABLED, setDnsScriptEnabled },
    { HELPER_CMD_CLEAR_FIREWALL_RULES, clearFirewallRules },
    { HELPER_CMD_CHECK_FIREWALL_STATE, checkFirewallState },
    { HELPER_CMD_SET_FIREWALL_RULES, setFirewallRules },
    { HELPER_CMD_GET_FIREWALL_RULES, getFirewallRules },
    { HELPER_CMD_DELETE_OLD_HELPER, deleteOldHelper },
    { HELPER_CMD_SET_FIREWALL_ON_BOOT, setFirewallOnBoot },
    { HELPER_CMD_SET_MAC_SPOOFING_ON_BOOT, setMacSpoofingOnBoot },
    { HELPER_CMD_SET_MAC_ADDRESS, setMacAddress },
    { HELPER_CMD_TASK_KILL, taskKill },
    { HELPER_CMD_START_CTRLD, startCtrld },
    { HELPER_CMD_START_STUNNEL, startStunnel },
    { HELPER_CMD_START_WSTUNNEL, startWstunnel },
    { HELPER_CMD_INSTALLER_CREATE_CLI_SYMLINK_DIR, installerCreateCliSymlinkDir },
    { HELPER_CMD_HELPER_VERSION, getHelperVersion },
    { HELPER_CMD_GET_INTERFACE_SSID, getInterfaceSsid },
};

CMD_ANSWER processCommand(int cmdId, const std::string &packet);
