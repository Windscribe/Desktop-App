#include "Server.h"
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
#include "Logger.h"
#include "ExecuteCmd.h"
#include "KeychainUtils.h"
#include "IPC/HelperCommandsSerialize.h"

#define SOCK_PATH "/var/run/windscribe_helper_socket2"

Server::Server()
{
    acceptor_ = NULL;
}

Server::~Server()
{
    service_.stop();
    
    if (acceptor_)
    {
        delete acceptor_;
    }

    unlink(SOCK_PATH);
}

bool Server::readAndHandleCommand(boost::asio::streambuf *buf, CMD_ANSWER &outCmdAnswer)
{
    // not enough data for read command
    if (buf->size() < sizeof(int)*2)
    {
        return false;
    }
    
    const char *bufPtr = boost::asio::buffer_cast<const char*>(buf->data());
    int cmdId, length;
    memcpy(&cmdId, bufPtr, sizeof(int));
    memcpy(&length, bufPtr + sizeof(int), sizeof(int));
    
    // not enough data for read command
    if (buf->size() < (sizeof(int)*2 + length))
    {
        return false;
    }
    
    std::vector<char> vector(length);
    memcpy(&vector[0], bufPtr + sizeof(int)*2, length);
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
    
    buf->consume(sizeof(int)*2 + length);
    
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
                    return;
                }
            }
        }
    }
    else
    {
        LOG("client app disconnected");
    }
}

void Server::acceptHandler(const boost::system::error_code & ec, socket_ptr sock)
{
    if (!ec.value())
    {
        LOG("client app connected");
                
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




