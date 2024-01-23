#include "server.h"

#define BOOST_BIND_GLOBAL_PLACEHOLDERS 1
#include <boost/bind.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <codecvt>
#include <mach-o/dyld.h>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "execute_cmd.h"
#include "logger.h"
#include "ipc/helper_security.h"
#include "macutils.h"
#include "process_command.h"
#include "utils.h"
#include "utils/executable_signature/executable_signature.h"

#define SOCK_PATH "/var/run/windscribe_helper_socket2"

Server::Server()
{
    acceptor_ = NULL;
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
    outCmdAnswer = processCommand(cmdId, str);

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
    if (Utils::isAppUninstalled()) {
        Utils::deleteSelf();
        return;
    }

    system("mkdir -p /var/run");
    system("mkdir -p /etc/windscribe");
    Utils::createWindscribeUserAndGroup();

    ::unlink(SOCK_PATH);

    boost::asio::local::stream_protocol::endpoint ep(SOCK_PATH);
    acceptor_ = new boost::asio::local::stream_protocol::acceptor(service_, ep);

    chmod(SOCK_PATH, 0777);
    startAccept();

    service_.run();
}
