#include "server.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <codecvt>
#include <string>
#include <mach-o/dyld.h>

#define BOOST_BIND_GLOBAL_PLACEHOLDERS 1

#include <boost/bind.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include "logger.h"
#include "utils/executable_signature/executable_signature.h"
#include "execute_cmd.h"
#include "keychain_utils.h"
#include "../../posix_common/helper_commands_serialize.h"
#include "ipc/helper_security.h"
#include "macutils.h"
#include "firewallonboot.h"
#include "ovpn.h"
#include "utils.h"

#define SOCK_PATH "/var/run/windscribe_helper_socket2"

Server::Server()
{
    acceptor_ = NULL;
    files_ = NULL;
}

Server::~Server()
{
    service_.stop();

    if (acceptor_) {
        delete acceptor_;
    }

    if (files_) {
        delete files_;
    }

    unlink(SOCK_PATH);
}

bool Server::readAndHandleCommand(socket_ptr sock, boost::asio::streambuf *buf, CMD_ANSWER &outCmdAnswer)
{
    // not enough data for read command
    if (buf->size() < sizeof(int)*3) {
        return false;
    }

    const char *bufPtr = boost::asio::buffer_cast<const char*>(buf->data());
    size_t headerSize = 0;
    int cmdId;
    memcpy(&cmdId, bufPtr + headerSize, sizeof(cmdId));
    headerSize += sizeof(cmdId);
    pid_t pid;
    memcpy(&pid, bufPtr + headerSize, sizeof(pid));
    headerSize += sizeof(pid);
    int length;
    memcpy(&length, bufPtr + headerSize, sizeof(length));
    headerSize += sizeof(length);

    // not enough data for read command
    if (buf->size() < (headerSize + length)) {
        return false;
    }

    pid_t pidPeer = -1;
    socklen_t lenPidPeer = sizeof(pidPeer);
    if (getsockopt(sock->native_handle(), SOL_LOCAL, LOCAL_PEERPID, &pidPeer, &lenPidPeer) != 0) {
        LOG("getsockopt(LOCAL_PEERID) failed (%d).", errno);
        return false;
    }

    // check process id
    if (!HelperSecurity::instance().verifyProcessId(pidPeer)) {
        return false;
    }

    std::string str(bufPtr + headerSize, length);
    std::istringstream stream(str);
    boost::archive::text_iarchive ia(stream, boost::archive::no_header);

    if (cmdId == HELPER_CMD_START_OPENVPN) {
        CMD_START_OPENVPN cmd;
        ia >> cmd;

        if (!OVPN::writeOVPNFile(MacUtils::resourcePath() + "dns.sh", cmd.config, cmd.isCustomConfig)) {
           LOG("Could not write OpenVPN config");
           outCmdAnswer.executed = 0;
        } else {
            std::string fullCmd = Utils::getFullCommand(cmd.exePath, cmd.executable, "--config /etc/windscribe/config.ovpn " + cmd.arguments);
            if (fullCmd.empty()) {
                // Something wrong with the command
                outCmdAnswer.executed = 0;
            } else {
                const std::string fullPath = cmd.exePath + "/" + cmd.executable;
                ExecutableSignature sigCheck;
                if (!sigCheck.verifyWithSignCheck(std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(fullPath))) {
                    LOG("OpenVPN executable signature incorrect: %s", sigCheck.lastError().c_str());
                    outCmdAnswer.executed = 0;
                } else {
                    outCmdAnswer.cmdId = ExecuteCmd::instance().execute(fullCmd, "/etc/windscribe");
                    outCmdAnswer.executed = 1;
                }
            }
        }
    } else if (cmdId == HELPER_CMD_GET_CMD_STATUS) {
        CMD_GET_CMD_STATUS cmd;
        ia >> cmd;

        bool bFinished;
        std::string log;
        ExecuteCmd::instance().getStatus(cmd.cmdId, bFinished, log);

        if (bFinished) {
            outCmdAnswer.executed = 1;
            outCmdAnswer.body = log;
        } else {
            outCmdAnswer.executed = 2;
        }
    } else if (cmdId == HELPER_CMD_CLEAR_CMDS) {
        CMD_CLEAR_CMDS cmd;
        ia >> cmd;

        ExecuteCmd::instance().clearCmds();
        outCmdAnswer.executed = 2;
    } else if (cmdId == HELPER_CMD_SET_KEYCHAIN_ITEM) {
        CMD_SET_KEYCHAIN_ITEM cmd;
        ia >> cmd;

        bool bSuccess = KeyChainUtils::setUsernameAndPassword("Windscribe IKEv2", "Windscribe IKEv2", "Windscribe IKEv2 password", cmd.username.c_str(), cmd.password.c_str());

        if (bSuccess) {
            outCmdAnswer.executed = 1;
        } else {
            outCmdAnswer.executed = 0;
        }
    } else if (cmdId == HELPER_CMD_SPLIT_TUNNELING_SETTINGS) {
        CMD_SPLIT_TUNNELING_SETTINGS cmd;
        ia >> cmd;

        splitTunneling_.setSplitTunnelingParams(cmd.isActive, cmd.isExclude, cmd.files, cmd.ips, cmd.hosts);
        outCmdAnswer.executed = 1;
    } else if (cmdId == HELPER_CMD_SEND_CONNECT_STATUS) {
        CMD_SEND_CONNECT_STATUS cmd;
        ia >> cmd;

        splitTunneling_.setConnectParams(cmd);
        outCmdAnswer.executed = 1;
    } else if (cmdId == HELPER_CMD_START_WIREGUARD) {
        CMD_START_WIREGUARD cmd;
        ia >> cmd;

        if (wireGuardController_.start(cmd.exePath, cmd.executable, cmd.deviceName)) {
            outCmdAnswer.executed = 1;
        } else {
            outCmdAnswer.executed = 0;
        }
    } else if (cmdId == HELPER_CMD_STOP_WIREGUARD) {
        if (wireGuardController_.stop()) {
            outCmdAnswer.executed = 1;
        }
    } else if (cmdId == HELPER_CMD_START_CTRLD) {
        CMD_START_CTRLD cmd;
        ia >> cmd;

        std::string fullCmd = Utils::getFullCommand(cmd.exePath, cmd.executable, cmd.parameters);
        if (fullCmd.empty()) {
            // Something wrong with the command
            outCmdAnswer.executed = 0;
        } else {
            const std::string fullPath = cmd.exePath + "/" + cmd.executable;
            ExecutableSignature sigCheck;
            if (!sigCheck.verifyWithSignCheck(std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(fullPath))) {
                LOG("ctrld executable signature incorrect: %s", sigCheck.lastError().c_str());
                outCmdAnswer.executed = 0;
            } else {
                outCmdAnswer.cmdId = ExecuteCmd::instance().execute(fullCmd, std::string());
                outCmdAnswer.executed = 1;
            }
        }
    } else if (cmdId == HELPER_CMD_CONFIGURE_WIREGUARD) {
        CMD_CONFIGURE_WIREGUARD cmd;
        ia >> cmd;

        outCmdAnswer.executed = 0;
        if (wireGuardController_.isInitialized()) {
            do {
                std::vector<std::string> allowed_ips_vector =
                    wireGuardController_.splitAndDeduplicateAllowedIps(cmd.allowedIps);
                if (allowed_ips_vector.size() < 1) {
                    LOG("WireGuard: invalid AllowedIps \"%s\"", cmd.allowedIps.c_str());
                    break;
                }

                if (!wireGuardController_.configureAdapter(cmd.clientIpAddress,
                                                           cmd.clientDnsAddressList,
                                                           MacUtils::resourcePath() + "/dns.sh",
                                                           allowed_ips_vector)) {
                    LOG("WireGuard: configureAdapter() failed");
                    break;
                }

                if (!wireGuardController_.configureDefaultRouteMonitor(cmd.peerEndpoint)) {
                    LOG("WireGuard: configureDefaultRouteMonitor() failed");
                    break;
                }
                if (!wireGuardController_.configure(cmd.clientPrivateKey,
                                                    cmd.peerPublicKey, cmd.peerPresharedKey,
                                                    cmd.peerEndpoint, allowed_ips_vector)) {
                    LOG("WireGuard: configureDaemon() failed");
                    break;
                }
                outCmdAnswer.executed = 1;
            } while (0);
        }
    } else if (cmdId == HELPER_CMD_GET_WIREGUARD_STATUS) {
        unsigned int errorCode = 0;
        unsigned long long bytesReceived = 0, bytesTransmitted = 0;

        outCmdAnswer.executed = 1;
        outCmdAnswer.cmdId = wireGuardController_.getStatus(&errorCode, &bytesReceived, &bytesTransmitted);
        if (outCmdAnswer.cmdId == kWgStateError) {
            if (errorCode) {
                outCmdAnswer.customInfoValue[0] = errorCode;
            } else {
                outCmdAnswer.customInfoValue[0] = -1;
            }
        } else if (outCmdAnswer.cmdId == kWgStateActive) {
            outCmdAnswer.customInfoValue[0] = bytesReceived;
            outCmdAnswer.customInfoValue[1] = bytesTransmitted;
        }
    } else if (cmdId == HELPER_CMD_INSTALLER_SET_PATH) {
        CMD_INSTALLER_FILES_SET_PATH cmd;
        ia >> cmd;

        if (files_) {
            delete files_;
        }
        files_ = new Files(cmd.archivePath, cmd.installPath, cmd.userId, cmd.groupId);
        outCmdAnswer.executed = 1;
    } else if (cmdId == HELPER_CMD_INSTALLER_EXECUTE_COPY_FILE) {
        if (files_) {
            outCmdAnswer.executed = files_->executeStep();
            if (outCmdAnswer.executed == -1) {
                outCmdAnswer.body = files_->getLastError();
            }
            if (outCmdAnswer.executed == -1 || outCmdAnswer.executed == 100) {
                delete files_;
                files_ = NULL;
            }
        }
    } else if (cmdId == HELPER_CMD_APPLY_CUSTOM_DNS) {
        CMD_APPLY_CUSTOM_DNS cmd;
        ia >> cmd;

        if (MacUtils::setDnsOfDynamicStoreEntry(cmd.ipAddress, cmd.networkService)) {
            outCmdAnswer.executed = 1;
        } else {
            outCmdAnswer.executed = 0;
        }
    } else if (cmdId == HELPER_CMD_CHANGE_MTU) {
        CMD_CHANGE_MTU cmd;
        ia >> cmd;
        LOG("Change MTU: %d", cmd.mtu);

        outCmdAnswer.executed = 1;
        Utils::executeCommand("ifconfig", {cmd.adapterName.c_str(), "mtu", std::to_string(cmd.mtu)});
    } else if (cmdId == HELPER_CMD_DELETE_ROUTE) {
        CMD_DELETE_ROUTE cmd;
        ia >> cmd;
        LOG("Delete route: %s/%d gw %s", cmd.range.c_str(), cmd.mask, cmd.gateway.c_str());

        outCmdAnswer.executed = 1;
        std::stringstream str;
        str << cmd.range << "/" << cmd.mask;
        Utils::executeCommand("route", {"-n", "delete", str.str().c_str(), cmd.gateway.c_str()});
    } else if (cmdId == HELPER_CMD_SET_IPV6_ENABLED) {
        CMD_SET_IPV6_ENABLED cmd;
        ia >> cmd;
        LOG("Set IPv6: %s", cmd.enabled ? "enabled" : "disabled");

        outCmdAnswer.executed = ipv6Manager_.setEnabled(cmd.enabled);
    } else if (cmdId == HELPER_CMD_SET_DNS_SCRIPT_ENABLED) {
        CMD_SET_DNS_SCRIPT_ENABLED cmd;
        ia >> cmd;
        LOG("Set DNS script: %s", cmd.enabled ? "enabled" : "disabled");

        outCmdAnswer.executed = 1;
        std::string out;

        // We only handle the down case; the 'up' trigger happens elsewhere
        if (!cmd.enabled) {
            Utils::executeCommand(MacUtils::resourcePath() + "/dns.sh", {"-down"}, &out);
            LOG("%s", out.c_str());
        }
    } else if (cmdId == HELPER_CMD_TASK_KILL) {
        CMD_TASK_KILL cmd;
        ia >> cmd;

        if (cmd.target == kTargetWindscribe) {
            LOG("Killing Windscribe processes");
            Utils::executeCommand("pkill", {"Windscribe"});
            Utils::executeCommand("pkill", {"WindscribeEngine"}); // For older 1.x clients
            outCmdAnswer.executed = 1;
        } else if (cmd.target == kTargetOpenVpn) {
            LOG("Killing OpenVPN processes");
            const std::vector<std::string> exes = Utils::getOpenVpnExeNames();
            for (auto exe : exes) {
                Utils::executeCommand("pkill", {"-f", exe.c_str()});
            }
            outCmdAnswer.executed = 1;
        } else if (cmd.target == kTargetStunnel) {
            LOG("Killing Stunnel processes");
            Utils::executeCommand("pkill", {"-f", "windscribestunnel"});
            outCmdAnswer.executed = 1;
        } else if (cmd.target == kTargetWStunnel) {
            LOG("Killing WStunnel processes");
            Utils::executeCommand("pkill", {"-f", "windscribewstunnel"});
            outCmdAnswer.executed = 1;
        } else if (cmd.target == kTargetWireGuard) {
            LOG("Killing WireGuard processes");
            Utils::executeCommand("pkill", {"-f", "windscribewireguard"});
            outCmdAnswer.executed = 1;
        } else if (cmd.target == kTargetCtrld) {
            LOG("Killing ctrld processes");
            Utils::executeCommand("pkill", {"-f", "windscribectrld"});
            outCmdAnswer.executed = 1;
        } else {
            LOG("Did not kill processes for type %d", cmd.target);
            outCmdAnswer.executed = 0;
        }
    } else if (cmdId == HELPER_CMD_CLEAR_FIREWALL_RULES) {
        LOG("Clear firewall rules");
        Utils::executeCommand("pfctl", {"-v", "-F", "all", "-f", "/etc/pf.conf"});
        Utils::executeCommand("pfctl", {"-d"});
        outCmdAnswer.executed = 1;
    } else if (cmdId == HELPER_CMD_CHECK_FIREWALL_STATE) {
        std::string output;

        Utils::executeCommand("pfctl", {"-si"}, &output);
        outCmdAnswer.exitCode = (output.find("Status: Enabled") != std::string::npos);
        outCmdAnswer.executed = 1;
    } else if (cmdId == HELPER_CMD_SET_FIREWALL_RULES) {
        LOG("Set firewall rules");
        CMD_SET_FIREWALL_RULES cmd;
        ia >> cmd;

        int fd = open("/etc/windscribe/pf.conf", O_CREAT | O_WRONLY | O_TRUNC);
        if (fd < 0) {
            LOG("Could not open firewall rules for writing");
            outCmdAnswer.executed = 0;
        } else {
            write(fd, cmd.rules.c_str(), cmd.rules.length());
            close(fd);

            if (cmd.table.empty() && cmd.group.empty()) {
                Utils::executeCommand("pfctl", {"-v", "-F", "all", "-f", "/etc/windscribe/pf.conf"});
                Utils::executeCommand("pfctl", {"-e"});
            } else if (!cmd.table.empty()) {
                Utils::executeCommand("pfctl", {"-T", "load", "-f", "/etc/windscribe/pf.conf"});
            } else if (!cmd.group.empty()) {
                Utils::executeCommand("pfctl", {"-a", cmd.group.c_str(), "-f", "/etc/windscribe/pf.conf"});
            }
            outCmdAnswer.executed = 1;
        }
    } else if (cmdId == HELPER_CMD_GET_FIREWALL_RULES) {
        CMD_GET_FIREWALL_RULES cmd;
        ia >> cmd;

        if (cmd.table.empty() && cmd.group.empty()) {
            Utils::executeCommand("pfctl", {"-s", "rules"}, &outCmdAnswer.body, false);
        } else if (!cmd.table.empty()) {
            Utils::executeCommand("pfctl", {"-t", cmd.table.c_str(), "-T", "show"}, &outCmdAnswer.body, false);
        } else if (!cmd.group.empty()) {
            Utils::executeCommand("pfctl", {"-a", cmd.group.c_str(), "-s", "rules"}, &outCmdAnswer.body, false);
        }
        outCmdAnswer.executed = 1;
    } else if (cmdId == HELPER_CMD_DELETE_OLD_HELPER) {
        LOG("Delete old helper");
        Utils::executeCommand("rm", {"-f", "/Library/PrivilegedHelperTools/com.windscribe.helper.macos"});
        Utils::executeCommand("rm", {"-f", "/Library/Logs/com.windscribe.helper.macos/helper_log.txt"});

        // remove helper from version 1
        Utils::executeCommand("launchctl", {"unload", "/Library/LaunchDaemons/com.aaa.windscribe.OVPNHelper.plist"});
        Utils::executeCommand("rm", {"-f", "/Library/LaunchDaemons/com.aaa.windscribe.OVPNHelper.plist"});
        Utils::executeCommand("rm", {"-f", "/Library/PrivilegedHelperTools/com.aaa.windscribe.OVPNHelper"});

        outCmdAnswer.executed = 1;
    } else if (cmdId == HELPER_CMD_INSTALLER_REMOVE_OLD_INSTALL) {
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
            outCmdAnswer.executed = 1;
        } else {
            LOG("Old install at %s not removed", cmd.path.c_str());
            outCmdAnswer.executed = 0;
        }
    } else if (cmdId == HELPER_CMD_SET_FIREWALL_ON_BOOT) {
        CMD_SET_FIREWALL_ON_BOOT cmd;
        ia >> cmd;
        LOG("Set firewall on boot: %s", cmd.enabled ? "true" : "false");

        outCmdAnswer.executed = firewallOnBoot_.setEnabled(cmd.enabled);
    } else if (cmdId == HELPER_CMD_SET_MAC_SPOOFING_ON_BOOT) {
        CMD_SET_MAC_SPOOFING_ON_BOOT cmd;
        ia >> cmd;
        LOG("Set mac spoofing on boot%s: %s", cmd.robustMethod ? " (robust method)" : "", cmd.enabled ? "true" : "false");

        outCmdAnswer.executed = macSpoofingOnBoot_.setEnabled(cmd.enabled, cmd.interface, cmd.macAddress, cmd.robustMethod);
    } else if (cmdId == HELPER_CMD_SET_MAC_ADDRESS) {
        CMD_SET_MAC_ADDRESS cmd;
        ia >> cmd;
        LOG("Set mac address on %s: %s%s", cmd.interface.c_str(), cmd.macAddress.c_str(), cmd.robustMethod ? " (robust method)" : "");

        if (cmd.robustMethod) {
            Utils::executeCommand("/System/Library/PrivateFrameworks/Apple80211.framework/Resources/airport", {"-z"});
        }
        outCmdAnswer.executed = Utils::executeCommand("ifconfig", {cmd.interface.c_str(), "ether", cmd.macAddress.c_str()});
        if (cmd.robustMethod) {
            Utils::executeCommand("ifconfig", {cmd.interface.c_str(), "up"});
        }
    } else {
        // these commands are not used in MacOS:
        //
        // HELPER_CMD_SET_DNS_LEAK_PROTECT_ENABLED
        // HELPER_CMD_CHECK_FOR_WIREGUARD_KERNEL_MODULE
    }

    buf->consume(headerSize + length);

    return true;
}

void Server::receiveCmdHandle(socket_ptr sock, boost::shared_ptr<boost::asio::streambuf> buf, const boost::system::error_code& ec, std::size_t bytes_transferred)
{
    if (!ec.value()) {
        // read and handle commands
        while (true) {
            CMD_ANSWER cmdAnswer;
            if (!readAndHandleCommand(sock, buf.get(), cmdAnswer)) {
                // goto receive next commands
                boost::asio::async_read(*sock, *buf, boost::asio::transfer_at_least(1),
                                        boost::bind(&Server::receiveCmdHandle, this, sock, buf, _1, _2));
                break;
            } else {
                if (!sendAnswerCmd(sock, cmdAnswer)) {
                    LOG("client app disconnected");
                    HelperSecurity::instance().reset();
                    return;
                }
            }
        }
    } else {
        LOG("client app disconnected");
        HelperSecurity::instance().reset();
    }
}

void Server::acceptHandler(const boost::system::error_code & ec, socket_ptr sock)
{
    if (!ec.value()) {
        LOG("client app connected");

        HelperSecurity::instance().reset();
        boost::shared_ptr<boost::asio::streambuf> buf(new boost::asio::streambuf);
        boost::asio::async_read(*sock, *buf, boost::asio::transfer_at_least(1),
                                    boost::bind(&Server::receiveCmdHandle, this, sock, buf, _1, _2));
    }

    startAccept();
}

void Server::startAccept()
{
    socket_ptr sock(new boost::asio::local::stream_protocol::socket(service_));
    acceptor_->async_accept(*sock, boost::bind(&Server::acceptHandler, this, boost::asio::placeholders::error, sock));
}

void Server::runService()
{
    service_.run();
}

bool Server::sendAnswerCmd(socket_ptr sock, const CMD_ANSWER &cmdAnswer)
{
    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmdAnswer;
    std::string str = stream.str();
    int length = (int)str.length();
    // send answer to client
    boost::system::error_code er;
    boost::asio::write(*sock, boost::asio::buffer(&length, sizeof(length)), boost::asio::transfer_exactly(sizeof(length)), er);
    if (er.value()) {
        return false;
    } else {
        boost::asio::write(*sock, boost::asio::buffer(str.data(), str.length()), boost::asio::transfer_exactly(str.length()), er);
        if (er.value()) {
            return false;
        }
    }
    return true;
}

void Server::run()
{
    system("mkdir -p /var/run");
    system("mkdir -p /etc/windscribe");

    ::unlink(SOCK_PATH);

    boost::asio::local::stream_protocol::endpoint ep(SOCK_PATH);
    acceptor_ = new boost::asio::local::stream_protocol::acceptor(service_, ep);

    chmod(SOCK_PATH, 0777);
    startAccept();

    boost::thread_group g;
    for (int i = 0; i < 4; i++)
    {
        g.add_thread(new boost::thread( boost::bind(&Server::runService, this) ));
    }
    g.join_all();
}
