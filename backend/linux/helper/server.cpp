#include "server.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string>
#include <codecvt>
#include <fstream>

#include <boost/bind.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include "logger.h"
#include "ovpn.h"
#include "execute_cmd.h"
#include "../../posix_common/helper_commands_serialize.h"
#include "../../../client/common/utils/executable_signature/executable_signature.h"

#include "utils.h"
#include "ipc/helper_security.h"

// For debugging
//#define SOCK_PATH "/tmp/windscribe_helper_socket2"

#define SOCK_PATH "/var/run/windscribe_helper_socket2"

Server::Server()
{
    acceptor_ = NULL;
    //files_ = NULL;
}

Server::~Server()
{
    service_.stop();

    if (acceptor_) {
        delete acceptor_;
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

    struct ucred peerCred;
    socklen_t lenPeerCred = sizeof(peerCred);
    int retCode = getsockopt(sock->native_handle(), SOL_SOCKET, SO_PEERCRED, &peerCred, &lenPeerCred);

    if ((retCode != 0) || (lenPeerCred != sizeof(peerCred))) {
        Logger::instance().out("getsockopt(SO_PEERCRED) failed (%d).", errno);
        return false;
    }

    // check process id
    if (!HelperSecurity::instance().verifyProcessId(peerCred.pid)) {
        return false;
    }

    std::vector<char> vector(length);
    memcpy(&vector[0], bufPtr + headerSize, length);
    std::string str(vector.begin(), vector.end());
    std::istringstream stream(str);
    boost::archive::text_iarchive ia(stream, boost::archive::no_header);

    if (cmdId == HELPER_CMD_START_OPENVPN) {
        CMD_START_OPENVPN cmd;
        ia >> cmd;

        std::string script = Utils::getDnsScript(cmd.dnsManager);
        if (script.empty()) {
            Logger::instance().out("Could not find appropriate DNS manager script");
            outCmdAnswer.executed = 0;
        } else {
            if (!OVPN::writeOVPNFile(script, cmd.config, cmd.isCustomConfig)) {
               Logger::instance().out("Could not write OpenVPN config");
               outCmdAnswer.executed = 0;
            } else {
                std::string fullCmd = Utils::getFullCommand(cmd.exePath, cmd.executable, "--config /etc/windscribe/config.ovpn " + cmd.arguments);
                if (fullCmd.empty()) {
                    // Something wrong with the command
                    outCmdAnswer.executed = 0;
                } else {
                    const std::string fullPath = cmd.exePath + "/" + cmd.executable;
                    ExecutableSignature sigCheck;
                    if (!sigCheck.verify(std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(fullPath))) {
                        Logger::instance().out("OpenVPN executable signature incorrect: %s", sigCheck.lastError().c_str());
                        outCmdAnswer.executed = 0;
                    } else {
                        outCmdAnswer.cmdId = ExecuteCmd::instance().execute(fullCmd, "/etc/windscribe");
                        outCmdAnswer.executed = 1;
                    }
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
    } else if (cmdId == HELPER_CMD_SEND_CONNECT_STATUS) {
        CMD_SEND_CONNECT_STATUS cmd;
        ia >> cmd;

        routesManager_.updateState(cmd);
        outCmdAnswer.executed = 1;
    } else if (cmdId == HELPER_CMD_START_WIREGUARD) {
        CMD_START_WIREGUARD cmd;
        ia >> cmd;

        if (wireGuardController_.start(cmd.exePath, cmd.executable, cmd.deviceName)) {
            outCmdAnswer.executed = 1;
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
            if (!sigCheck.verify(std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(fullPath))) {
                Logger::instance().out("ctrld executable signature incorrect: %s", sigCheck.lastError().c_str());
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
                    Logger::instance().out("WireGuard: invalid AllowedIps \"%s\"", cmd.allowedIps.c_str());
                    break;
                }

                uint32_t fwmark = wireGuardController_.getFwmark();
                Logger::instance().out("Fwmark = %u", fwmark);

                if (!wireGuardController_.configure(cmd.clientPrivateKey,
                                                    cmd.peerPublicKey, cmd.peerPresharedKey,
                                                    cmd.peerEndpoint, allowed_ips_vector, fwmark)) {
                    Logger::instance().out("WireGuard: configure() failed");
                    break;
                }

                if (!wireGuardController_.configureDefaultRouteMonitor(cmd.peerEndpoint)) {
                    Logger::instance().out("WireGuard: configureDefaultRouteMonitor() failed");
                    break;
                }
                std::string script = Utils::getDnsScript(cmd.dnsManager);
                if (script.empty()) {
                    Logger::instance().out("WireGuard: could not find appropriate dns manager script");
                    break;
                }
                if (!wireGuardController_.configureAdapter(cmd.clientIpAddress,
                                                           cmd.clientDnsAddressList,
                                                           script,
                                                           allowed_ips_vector, fwmark)) {
                    Logger::instance().out("WireGuard: configureAdapter() failed");
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
    } else if (cmdId == HELPER_CMD_CHANGE_MTU) {
        CMD_CHANGE_MTU cmd;
        ia >> cmd;
        Logger::instance().out("Change MTU: %d", cmd.mtu);

        outCmdAnswer.executed = 1;
        Utils::executeCommand("ip", {"link", "set", "dev", cmd.adapterName.c_str(), "mtu", std::to_string(cmd.mtu)});
    } else if (cmdId == HELPER_CMD_SET_DNS_LEAK_PROTECT_ENABLED) {
        CMD_SET_DNS_LEAK_PROTECT_ENABLED cmd;
        ia >> cmd;
        Logger::instance().out("Set DNS leak protect: %s", cmd.enabled ? "enabled" : "disabled");

        // We only handle the down case; the 'up' trigger for this script happens in the DNS manager script
        if (!cmd.enabled) {
            Utils::executeCommand("/etc/windscribe/dns-leak-protect", {"down"});
            outCmdAnswer.executed = 1;
        }
    } else if (cmdId == HELPER_CMD_TASK_KILL) {
        CMD_TASK_KILL cmd;
        ia >> cmd;

        if (cmd.target == kTargetWindscribe) {
            Logger::instance().out("Killing Windscribe processes");
            Utils::executeCommand("pkill", {"Windscribe"});
            Utils::executeCommand("pkill", {"WindscribeEngine"}); // For older 1.x clients
            outCmdAnswer.executed = 1;
        } else if (cmd.target == kTargetOpenVpn) {
            Logger::instance().out("Killing OpenVPN processes");
            const std::vector<std::string> exes = Utils::getOpenVpnExeNames();
            for (auto exe : exes) {
                Utils::executeCommand("pkill", {"-f", exe.c_str()});
            }
            outCmdAnswer.executed = 1;
        } else if (cmd.target == kTargetStunnel) {
            Logger::instance().out("Killing Stunnel processes");
            Utils::executeCommand("pkill", {"-f", "windscribestunnel"});
            outCmdAnswer.executed = 1;
        } else if (cmd.target == kTargetWStunnel) {
            Logger::instance().out("Killing WStunnel processes");
            Utils::executeCommand("pkill", {"-f", "windscribewstunnel"});
            outCmdAnswer.executed = 1;
        } else if (cmd.target == kTargetWireGuard) {
            Logger::instance().out("Killing WireGuard processes");
            Utils::executeCommand("pkill", {"-f", "windscribewireguard"});
            outCmdAnswer.executed = 1;
        } else if (cmd.target == kTargetCtrld) {
            Logger::instance().out("Killing ctrld processes");
            Utils::executeCommand("pkill", {"-f", "windscribectrld"});
            outCmdAnswer.executed = 1;
        } else {
            Logger::instance().out("Did not kill processes for type %d", cmd.target);
            outCmdAnswer.executed = 0;
        }
    } else if (cmdId == HELPER_CMD_CHECK_FOR_WIREGUARD_KERNEL_MODULE) {
        outCmdAnswer.executed = Utils::executeCommand("modprobe", {"wireguard"}) ? 0 : 1;
        Logger::instance().out("WireGuard kernel module: %s", outCmdAnswer.executed ? "available" : "not available");
    } else if (cmdId == HELPER_CMD_CLEAR_FIREWALL_RULES) {
        CMD_CLEAR_FIREWALL_RULES cmd;
        ia >> cmd;
        Logger::instance().out("Clear firewall rules");
        Utils::executeCommand("rm", {"-f", "/etc/windscribe/rules.v4"});
        Utils::executeCommand("rm", {"-f", "/etc/windscribe/rules.v6"});
        outCmdAnswer.executed = 1;
    } else if (cmdId == HELPER_CMD_CHECK_FIREWALL_STATE) {
        CMD_CHECK_FIREWALL_STATE cmd;
        ia >> cmd;

        outCmdAnswer.executed = 1;
        if (Utils::executeCommand("iptables", {"--check", "INPUT", "-j", "windscribe_input", "-m", "comment", "--comment", cmd.tag.c_str()}, &outCmdAnswer.body)) {
            outCmdAnswer.exitCode = 0;
        } else {
            outCmdAnswer.exitCode = 1;
        }
    } else if (cmdId == HELPER_CMD_SET_FIREWALL_RULES) {
        CMD_SET_FIREWALL_RULES cmd;
        ia >> cmd;
        Logger::instance().out("Set firewall rules");

        int fd;
        if (cmd.ipVersion == kIpv4) {
            fd = open("/etc/windscribe/rules.v4", O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU | S_IRGRP | S_IROTH);
        } else {
            fd = open("/etc/windscribe/rules.v6", O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU | S_IRGRP | S_IROTH);
        }

        if (fd < 0) {
            Logger::instance().out("Could not open firewall rules for writing");
            outCmdAnswer.executed = 0;
        } else {
            int bytes = write(fd, cmd.rules.c_str(), cmd.rules.length());
            close(fd);
            if (bytes <= 0) {
                Logger::instance().out("Could not write rules");
                outCmdAnswer.executed = 0;
            } else {
                if (cmd.ipVersion == kIpv4) {
                    outCmdAnswer.exitCode = Utils::executeCommand("iptables-restore", {"-n", "/etc/windscribe/rules.v4"});
                } else {
                    outCmdAnswer.exitCode = Utils::executeCommand("ip6tables-restore", {"-n", "/etc/windscribe/rules.v6"});
                }
                outCmdAnswer.executed = 1;
            }
        }
    } else if (cmdId == HELPER_CMD_GET_FIREWALL_RULES) { CMD_GET_FIREWALL_RULES cmd;
        ia >> cmd;
        std::string filename;

        if (cmd.ipVersion == kIpv4) {
            filename = "/etc/windscribe/rules.v4";
            outCmdAnswer.exitCode = Utils::executeCommand("iptables-save", {"-f", filename.c_str()});
        } else {
            filename = "/etc/windscribe/rules.v6";
            outCmdAnswer.exitCode = Utils::executeCommand("ip6tables-save", {"-f", filename.c_str()});
        }

        std::ifstream ifs(filename.c_str());
        std::stringstream buffer;
        buffer << ifs.rdbuf();
        outCmdAnswer.body = buffer.str();
        outCmdAnswer.executed = 1;
    } else if (cmdId == HELPER_CMD_START_STUNNEL) {
        CMD_START_STUNNEL cmd;
        ia >> cmd;
        Logger::instance().out("Starting stunnel");

        std::string fullCmd = Utils::getFullCommandAsUser("windscribe", cmd.exePath, cmd.executable, "/etc/windscribe/stunnel.conf");
        if (fullCmd.empty()) {
            // Something wrong with the command
            outCmdAnswer.executed = 0;
        } else {
            const std::string fullPath = cmd.exePath + "/" + cmd.executable;
            ExecutableSignature sigCheck;
            if (!sigCheck.verify(std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(fullPath))) {
                Logger::instance().out("stunnel executable signature incorrect: %s", sigCheck.lastError().c_str());
                outCmdAnswer.executed = 0;
            } else {
                outCmdAnswer.cmdId = ExecuteCmd::instance().execute(fullCmd, std::string());
                outCmdAnswer.executed = 1;
            }
        }
    } else if (cmdId == HELPER_CMD_CONFIGURE_STUNNEL) {
        CMD_CONFIGURE_STUNNEL cmd;
        ia >> cmd;

        std::stringstream conf;
        conf << "[openvpn]\n";
        conf << "client = yes\n";
        conf << "accept = 127.0.0.1:" << cmd.localPort << "\n";
        conf << "connect = " << cmd.hostname << ":" << cmd.port << "\n";

        int fd = open("/etc/windscribe/stunnel.conf", O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);
        if (fd < 0) {
            Logger::instance().out("Could not open stunnel config for writing");
            outCmdAnswer.executed = 0;
        } else {
            write(fd, conf.str().c_str(), conf.str().length());
            close(fd);
            outCmdAnswer.executed = 1;
            outCmdAnswer.cmdId = Utils::executeCommand("chown", {"windscribe:windscribe", "/etc/windscribe/stunnel.conf"});
        }
    } else if (cmdId == HELPER_CMD_START_WSTUNNEL) {
        CMD_START_WSTUNNEL cmd;
        ia >> cmd;
        Logger::instance().out("Starting wstunnel");

        std::string arguments = "--localToRemote 127.0.0.1:" + std::to_string(cmd.localPort) + ":127.0.0.1:1194 wss://" + cmd.hostname + ":" + std::to_string(cmd.port) + " --verbose --upgradePathPrefix=/";
        if (cmd.isUdp) {
            arguments += " --udp";
        }
        std::string fullCmd = Utils::getFullCommandAsUser("windscribe", cmd.exePath, cmd.executable, arguments);
        if (fullCmd.empty()) {
            // Something wrong with the command
            outCmdAnswer.executed = 0;
        } else {
            const std::string fullPath = cmd.exePath + "/" + cmd.executable;
            ExecutableSignature sigCheck;
            if (!sigCheck.verify(std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(fullPath))) {
                Logger::instance().out("wstunnel executable signature incorrect: %s", sigCheck.lastError().c_str());
                outCmdAnswer.executed = 0;
            } else {
                outCmdAnswer.cmdId = ExecuteCmd::instance().execute(fullCmd, std::string());
                outCmdAnswer.executed = 1;
            }
        }
    } else {
        // these commands are not used in Linux:
        //
        // HELPER_CMD_SET_KEYCHAIN_ITEM
        // HELPER_CMD_SPLIT_TUNNELING_SETTINGS
        // HELPER_CMD_DELETE_OLD_HELPER
        // HELPER_CMD_SET_FIREWALL_ON_BOOT
        // HELPER_CMD_SET_MAC_SPOOFING_ON_BOOT
        // HELPER_CMD_SET_MAC_ADDRESS
        // HELPER_CMD_SET_KEXT_PATH
        // HELPER_CMD_SET_DNS_SCRIPT_ENABLED
        // HELPER_CMD_DELETE_ROUTE
        // HELPER_CMD_INSTALLER_SET_PATH
        // HELPER_CMD_INSTALLER_EXECUTE_COPY_FILE
        // HELPER_CMD_APPLY_CUSTOM_DNS
    }

    buf->consume(headerSize + length);

    return true;
}

void Server::receiveCmdHandle(socket_ptr sock, boost::shared_ptr<boost::asio::streambuf> buf, const boost::system::error_code& ec, std::size_t bytes_transferred)
{
    UNUSED(bytes_transferred);

    if (!ec.value())
    {
        // read and handle commands
        while (true)
        {
            CMD_ANSWER cmdAnswer;
            if (!readAndHandleCommand(sock, buf.get(), cmdAnswer))
            {
                // goto receive next commands
                boost::asio::async_read(*sock, *buf, boost::asio::transfer_at_least(1),
                                        boost::bind(&Server::receiveCmdHandle, this, sock, buf, _1, _2));
                break;
            }
            else
            {
                if (!sendAnswerCmd(sock, cmdAnswer))
                {
                    Logger::instance().out("client app disconnected");
                    HelperSecurity::instance().reset();
                    return;
                }
            }
        }
    }
    else
    {
        Logger::instance().out("client app disconnected");
        HelperSecurity::instance().reset();
    }
}

void Server::acceptHandler(const boost::system::error_code & ec, socket_ptr sock)
{
    if (!ec.value())
    {
        Logger::instance().out("client app connected");

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
    auto res = system("mkdir -p /var/run"); // res is necessary to avoid no-discard warning.
    UNUSED(res);
    Utils::createWindscribeUserAndGroup();

    ::unlink(SOCK_PATH);

    boost::asio::local::stream_protocol::endpoint ep(SOCK_PATH);
    acceptor_ = new boost::asio::local::stream_protocol::acceptor(service_, ep);

    chmod(SOCK_PATH, 0777);
    startAccept();

    boost::thread_group g;
    for (int i = 0; i < 4; i++) {
        g.add_thread(new boost::thread( boost::bind(&Server::runService, this) ));
    }
    g.join_all();
}

void Server::stop()
{
    service_.stop();
}
