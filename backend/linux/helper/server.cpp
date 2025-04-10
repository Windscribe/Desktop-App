#include "server.h"

#include <boost/bind.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <codecvt>
#include <grp.h>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <spdlog/spdlog.h>

#include "execute_cmd.h"
#include "firewallcontroller.h"
#include "ipc/helper_security.h"
#include "ovpn.h"
#include "process_command.h"
#include "utils.h"
#include "utils/executable_signature/executable_signature.h"

#define SOCK_PATH "/var/run/windscribe/helper.sock"

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

bool Server::readAndHandleCommand(socket_ptr sock, boost::asio::streambuf *buf, std::string &outCmdAnswer)
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
        spdlog::error("getsockopt(SO_PEERCRED) failed ({}).", errno);
        return false;
    }

    if (!HelperSecurity::instance().verifySignature()) {
        return false;
    }

    std::vector<char> vector(length);
    memcpy(&vector[0], bufPtr + headerSize, length);
    std::string str(vector.begin(), vector.end());
    std::istringstream stream(str);
    outCmdAnswer = processCommand((HelperCommand)cmdId, str);

    buf->consume(headerSize + length);

    return true;
}

void Server::receiveCmdHandle(socket_ptr sock, boost::shared_ptr<boost::asio::streambuf> buf, const boost::system::error_code& ec, std::size_t bytes_transferred)
{
    UNUSED(bytes_transferred);

    if (!ec.value()) {
        // read and handle commands
        while (true) {
            std::string answer;
            if (!readAndHandleCommand(sock, buf.get(), answer)) {
                // goto receive next commands
                boost::asio::async_read(*sock, *buf, boost::asio::transfer_at_least(1),
                                        boost::bind(&Server::receiveCmdHandle, this, sock, buf, _1, _2));
                break;
            } else {
                if (!sendAnswerCmd(sock, answer)) {
                    spdlog::info("client app disconnected");
                    return;
                }
            }
        }
    } else {
        spdlog::info("client app disconnected");
    }
}

void Server::acceptHandler(const boost::system::error_code & ec, socket_ptr sock)
{
    if (!ec.value()) {
        spdlog::info("client app connected");

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

bool Server::sendAnswerCmd(socket_ptr sock, const std::string &answer)
{
    int length = (int)answer.length();
    // send answer to client
    boost::system::error_code er;
    boost::asio::write(*sock, boost::asio::buffer(&length, sizeof(length)), boost::asio::transfer_exactly(sizeof(length)), er);
    if (er.value()) {
        return false;
    } else {
        boost::asio::write(*sock, boost::asio::buffer(answer.data(), answer.length()), boost::asio::transfer_exactly(answer.length()), er);
        if (er.value()) {
            return false;
        }
    }
    return true;
}

void Server::run()
{
    Utils::createWindscribeUserAndGroup();

    auto res = system("mkdir -p /var/run/windscribe && chown :windscribe /var/run/windscribe && chmod 775 /var/run/windscribe"); // res is necessary to avoid no-discard warning.
    UNUSED(res);

    ::unlink(SOCK_PATH);

    boost::asio::local::stream_protocol::endpoint ep(SOCK_PATH);
    acceptor_ = new boost::asio::local::stream_protocol::acceptor(service_, ep);

    struct group *grp = getgrnam("windscribe");
    if (!grp || chmod(SOCK_PATH, 0770) || chown(SOCK_PATH, (uid_t)-1, grp->gr_gid)) {
        // Do not have a group, or chmod/chown failed.
        ::unlink(SOCK_PATH);
        return;
    }

    // Cause the FirewallController to be constructed here, so that on-boot rules are processed, even if the Windscribe app/service does not start.
    FirewallController::instance();

    startAccept();

    service_.run();
}
