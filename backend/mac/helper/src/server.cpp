#include "server.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string>
#include <mach-o/dyld.h>

#include <boost/bind.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include "logger.h"
#include "execute_cmd.h"
#include "keychain_utils.h"
#include "ipc/helper_commands_serialize.h"
#include "ipc/helper_security.h"
#include "macutils.h"

#define SOCK_PATH "/var/run/windscribe_helper_socket2"

Server::Server()
{
    acceptor_ = NULL;
    files_ = NULL;
}

Server::~Server()
{
    service_.stop();
    
    if (acceptor_)
    {
        delete acceptor_;
    }
    
    if (files_)
    {
        delete files_;
    }

    unlink(SOCK_PATH);
}

bool Server::readAndHandleCommand(boost::asio::streambuf *buf, CMD_ANSWER &outCmdAnswer)
{
    // not enough data for read command
    if (buf->size() < sizeof(int)*3)
    {
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
    if (buf->size() < (headerSize + length))
    {
        return false;
    }

    // check process id
    if (!HelperSecurity::instance().verifyProcessId(pid))
    {
        return false;
    }

    std::vector<char> vector(length);
    memcpy(&vector[0], bufPtr + headerSize, length);
    std::string str(vector.begin(), vector.end());
    std::istringstream stream(str);
    boost::archive::text_iarchive ia(stream, boost::archive::no_header);
    
    if (cmdId == HELPER_CMD_EXECUTE)
    {
        CMD_EXECUTE cmdExecute;
        ia >> cmdExecute;
        
        std::string strReply;
        
        FILE *file = popen(cmdExecute.cmdline.c_str(), "r");
        if (file)
        {
            char szLine[4096];
            while(fgets(szLine, sizeof(szLine), file) != 0)
            {
                strReply += szLine;
            }
            pclose(file);
            outCmdAnswer.executed = 1;
            outCmdAnswer.body = strReply;
        }
        else
        {
            outCmdAnswer.executed = 0;
        }
    }
    else if (cmdId == HELPER_CMD_EXECUTE_OPENVPN)
    {
        CMD_EXECUTE_OPENVPN cmdExecuteOpenVpn;
        ia >> cmdExecuteOpenVpn;
        
        outCmdAnswer.cmdId = ExecuteCmd::instance().execute(cmdExecuteOpenVpn.cmdline.c_str());
        outCmdAnswer.executed = 1;
        
    }
    else if (cmdId == HELPER_CMD_GET_CMD_STATUS)
    {
        CMD_GET_CMD_STATUS cmd;
        ia >> cmd;
        
        bool bFinished;
        std::string log;
        ExecuteCmd::instance().getStatus(cmd.cmdId, bFinished, log);
        
        if (bFinished)
        {
            outCmdAnswer.executed = 1;
            outCmdAnswer.body = log;
        }
        else
        {
            outCmdAnswer.executed = 2;
        }
    }
    else if (cmdId == HELPER_CMD_CLEAR_CMDS)
    {
        CMD_CLEAR_CMDS cmd;
        ia >> cmd;
        
        ExecuteCmd::instance().clearCmds();
        outCmdAnswer.executed = 2;
    }
    else if (cmdId == HELPER_CMD_SET_KEYCHAIN_ITEM)
    {
        CMD_SET_KEYCHAIN_ITEM cmd;
        ia >> cmd;
        
        bool bSuccess = KeyChainUtils::setUsernameAndPassword("Windscribe IKEv2", "Windscribe IKEv2", "Windscribe IKEv2 password", cmd.username.c_str(), cmd.password.c_str());
        
        if (bSuccess)
        {
            outCmdAnswer.executed = 1;
        }
        else
        {
            outCmdAnswer.executed = 0;
        }
    }
    else if (cmdId == HELPER_CMD_SPLIT_TUNNELING_SETTINGS)
    {
        CMD_SPLIT_TUNNELING_SETTINGS cmd;
        ia >> cmd;
        
        splitTunneling_.setSplitTunnelingParams(cmd.isActive, cmd.isExclude, cmd.files, cmd.ips, cmd.hosts);
        outCmdAnswer.executed = 1;
    }
    else if (cmdId == HELPER_CMD_SEND_CONNECT_STATUS)
    {
        CMD_SEND_CONNECT_STATUS cmd;
        ia >> cmd;
        
        splitTunneling_.setConnectParams(cmd);
        outCmdAnswer.executed = 1;
    }
    else if (cmdId == HELPER_CMD_SET_KEXT_PATH)
    {
        CMD_SET_KEXT_PATH cmd;
        ia >> cmd;
        
        splitTunneling_.setKextPath(cmd.kextPath);
        outCmdAnswer.executed = 1;
    }
    else if (cmdId == HELPER_CMD_START_WIREGUARD)
    {
        CMD_START_WIREGUARD cmd;
        ia >> cmd;
        const std::string strFullCmd(cmd.exePath + " -f " + cmd.deviceName);
        outCmdAnswer.cmdId = ExecuteCmd::instance().execute(strFullCmd.c_str());
        wireGuardController_.init(cmd.deviceName, outCmdAnswer.cmdId);
        outCmdAnswer.executed = 1;
    }
    else if (cmdId == HELPER_CMD_STOP_WIREGUARD)
    {
        bool is_daemon_dead = true;
        std::string log;
        ExecuteCmd::instance().getStatus(wireGuardController_.getDaemonCmdId(), is_daemon_dead,
                                         log);
        if (is_daemon_dead) {
            wireGuardController_.reset();
            outCmdAnswer.executed = 1;
            outCmdAnswer.body = log;
        }
    }
    else if (cmdId == HELPER_CMD_CONFIGURE_WIREGUARD)
    {
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
                                                           cmd.clientDnsScriptName,
                                                           allowed_ips_vector)) {
                    LOG("WireGuard: configureAdapter() failed");
                    break;
                }
                
                if (!wireGuardController_.configureDefaultRouteMonitor(cmd.peerEndpoint)) {
                    LOG("WireGuard: configureDefaultRouteMonitor() failed");
                    break;
                }
                if (!wireGuardController_.configureDaemon(cmd.clientPrivateKey,
                                                          cmd.peerPublicKey, cmd.peerPresharedKey,
                                                          cmd.peerEndpoint, allowed_ips_vector)) {
                    LOG("WireGuard: configureDaemon() failed");
                    break;
                }
                outCmdAnswer.executed = 1;
            } while (0);
        }
    }
    else if (cmdId == HELPER_CMD_GET_WIREGUARD_STATUS)
    {
        outCmdAnswer.executed = 1;
        if (!wireGuardController_.isInitialized()) {
            outCmdAnswer.cmdId = WIREGUARD_STATE_NONE;
        } else {
            bool is_daemon_dead = true;
            std::string log;
            ExecuteCmd::instance().getStatus(wireGuardController_.getDaemonCmdId(), is_daemon_dead,
                                             log);
            if (is_daemon_dead) {
                outCmdAnswer.cmdId = WIREGUARD_STATE_ERROR;
                // Special error code means the daemon is dead.
                outCmdAnswer.customInfoValue[0] = 666u;
                outCmdAnswer.body = log;
            } else {
                unsigned int errorCode = 0;
                unsigned long long bytesReceived = 0, bytesTransmitted = 0;
                outCmdAnswer.cmdId = wireGuardController_.getStatus(
                    &errorCode, &bytesReceived, &bytesTransmitted);
                if (outCmdAnswer.cmdId == WIREGUARD_STATE_ERROR) {
                    outCmdAnswer.customInfoValue[0] = errorCode;
                } else if (outCmdAnswer.cmdId == WIREGUARD_STATE_ACTIVE) {
                    outCmdAnswer.customInfoValue[0] = bytesReceived;
                    outCmdAnswer.customInfoValue[1] = bytesTransmitted;
                }
            }
        }
    }
    else if (cmdId == HELPER_CMD_KILL_PROCESS)
    {
        CMD_KILL_PROCESS cmd;
        ia >> cmd;
        outCmdAnswer.executed = (kill(cmd.processId, SIGTERM) == 0);
    }
    else if (cmdId == HELPER_CMD_INSTALLER_SET_PATH)
    {
        CMD_INSTALLER_FILES_SET_PATH cmd;
        ia >> cmd;
        
        if (files_)
        {
            delete files_;
        }
        files_ = new Files(cmd.archivePath, cmd.installPath, cmd.userId, cmd.groupId);
        outCmdAnswer.executed = 1;
    }
    else if (cmdId == HELPER_CMD_INSTALLER_EXECUTE_COPY_FILE)
    {
        if (files_)
        {
            outCmdAnswer.executed = files_->executeStep();
            if (outCmdAnswer.executed == -1)
            {
                outCmdAnswer.body = files_->getLastError();
            }
            if (outCmdAnswer.executed == -1 || outCmdAnswer.executed == 100)
            {
                delete files_;
                files_ = NULL;
            }
        }
    }
    else if (cmdId == HELPER_CMD_APPLY_CUSTOM_DNS)
    {
        CMD_APPLY_CUSTOM_DNS cmd;
        ia >> cmd;
        
        MacUtils::setDnsOfDynamicStoreEntry(cmd.ipAddress, cmd.networkService);
        outCmdAnswer.executed = 1;
    }

    buf->consume(headerSize + length);

    return true;
}

void Server::receiveCmdHandle(socket_ptr sock, boost::shared_ptr<boost::asio::streambuf> buf, const boost::system::error_code& ec, std::size_t bytes_transferred)
{
    if (!ec.value())
    {
        // read and handle commands
        while (true)
        {
            CMD_ANSWER cmdAnswer;
            if (!readAndHandleCommand(buf.get(), cmdAnswer))
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
                    LOG("client app disconnected");
                    HelperSecurity::instance().reset();
                    return;
                }
            }
        }
    }
    else
    {
        LOG("client app disconnected");
        HelperSecurity::instance().reset();
    }
}

void Server::acceptHandler(const boost::system::error_code & ec, socket_ptr sock)
{
    if (!ec.value())
    {
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
    if (er.value())
    {
        return false;
    }
    else
    {
        boost::asio::write(*sock, boost::asio::buffer(str.data(), str.length()), boost::asio::transfer_exactly(str.length()), er);
        if (er.value())
        {
            return false;
        }
    }
    return true;
}

void Server::run()
{
    system("mkdir -p /var/run");

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




