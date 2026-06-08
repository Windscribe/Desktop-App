#include "server.h"

#include <boost/bind.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <codecvt>
#include <filesystem>
#include <grp.h>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <spdlog/spdlog.h>

#include "../common/io_posix.h"
#include "firewallonboot.h"
#include "process_command.h"
#include "utils.h"

#define SOCK_PATH WS_LINUX_RUN_DIR "/helper.sock"

namespace {
// Scope-local umask override. Restores the previous umask in the destructor so an exception during the guarded
// operation can't leave the process-wide umask in an unexpected state.
struct UmaskGuard {
    explicit UmaskGuard(mode_t mask) : prev_(umask(mask)) {}
    ~UmaskGuard() { umask(prev_); }
    mode_t prev_;
};
} // namespace

Server::Server()
{
    acceptor_ = NULL;
}

Server::~Server()
{
    io_context_.stop();

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

    const char *bufPtr = static_cast<const char*>(buf->data().data());
    size_t headerSize = 0;
    int cmdId;
    memcpy(&cmdId, bufPtr + headerSize, sizeof(cmdId));
    headerSize += sizeof(cmdId);
    // pid field is reserved in the protocol but unused by the helper
    headerSize += sizeof(pid_t);
    int length;
    memcpy(&length, bufPtr + headerSize, sizeof(length));
    headerSize += sizeof(length);

    // Reject malformed or oversized frames before any size-derived allocation.
    // A negative length would otherwise promote to a huge size_t in the
    // completeness check and the vector constructor below, crashing the helper.
    if (length < 0 || static_cast<size_t>(length) > kMaxHelperFrameSize) {
        spdlog::error("Helper IPC: rejecting frame with invalid length {}", length);
        boost::system::error_code closeEc;
        sock->close(closeEc);
        return false;
    }

    // not enough data for read command
    if (buf->size() < (headerSize + static_cast<size_t>(length))) {
        return false;
    }

    std::string str;
    try {
        str.assign(bufPtr + headerSize, length);
    } catch (const std::bad_alloc &) {
        spdlog::error("Helper IPC: bad_alloc for frame size {}", length);
        boost::system::error_code closeEc;
        sock->close(closeEc);
        return false;
    }
    buf->consume(headerSize + length);

    try {
        outCmdAnswer = processCommand((HelperCommand)cmdId, str);
    } catch (const std::exception &ex) {
        spdlog::error("Helper IPC: processCommand({}) exception: {}", cmdId, ex.what());
        outCmdAnswer.clear();
    }

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
        spdlog::info("client app disconnected, code {}", ec.message());
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
    socket_ptr sock(new boost::asio::local::stream_protocol::socket(io_context_));
    acceptor_->async_accept(*sock, boost::bind(&Server::acceptHandler, this, boost::asio::placeholders::error, sock));
}

bool Server::sendAnswerCmd(socket_ptr sock, const std::string &answer)
{
    int length = (int)answer.length();
    // send answer to client
    boost::system::error_code er = safeBlockingWrite(sock, &length, sizeof(length));
    if (er.value()) {
        spdlog::warn("Server::sendAnswerCmd write error: {}", er.message());
        return false;
    } else {
        er = safeBlockingWrite(sock, answer.data(), answer.length());
        if (er.value()) {
            spdlog::warn("Server::sendAnswerCmd write error: {}", er.message());
            return false;
        }
    }
    return true;
}

boost::system::error_code Server::safeBlockingWrite(socket_ptr &sock, const void *data, size_t size)
{
    const char *buf = static_cast<const char *>(data);
    boost::system::error_code ec;
    size_t total_written = 0;
    while (total_written < size) {
        size_t sz = boost::asio::write(*sock, boost::asio::buffer(buf + total_written, size - total_written), boost::asio::transfer_exactly(size - total_written), ec);
        if (ec == boost::asio::error::interrupted) {    // EINTR signal, should be ignored on POSIX systems
            continue;
        } else if (ec) {
            return ec;
        }
        total_written += sz;
    }
    return ec;
}

void Server::run()
{

    std::error_code ec;
    {
#ifndef WINDSCRIBE_DEV_MODE
        UmaskGuard guard(0067);
#else
        UmaskGuard guard(0);
#endif
        std::filesystem::create_directories(WS_LINUX_RUN_DIR, ec);
    }
    if (ec) {
        spdlog::error("Failed to create " WS_LINUX_RUN_DIR ": {}", ec.message());
    }
#ifndef WINDSCRIBE_DEV_MODE // secure (non-dev)
    Utils::createAppUserAndGroup();
    // Force uid=0 (not (uid_t)-1) so a pre-existing dir with a wrong owner is self-healed before
    // permissions runs — otherwise the chmod could hand owner-write to that wrong owner.
    if (struct group *grp = getgrnam(WS_PRODUCT_NAME_LOWER)) {
        if (::lchown(WS_LINUX_RUN_DIR, 0, grp->gr_gid) != 0) {
            spdlog::error("Failed to chown " WS_LINUX_RUN_DIR ": {}", IO::strerror(errno));
        }
    } else {
        spdlog::error("Could not get group info for " WS_PRODUCT_NAME_LOWER);
    }
    std::filesystem::permissions(WS_LINUX_RUN_DIR,
        std::filesystem::perms::owner_all | std::filesystem::perms::group_exec,
        ec);
#else // dev mode
    std::filesystem::permissions(WS_LINUX_RUN_DIR,
        std::filesystem::perms::owner_all | std::filesystem::perms::group_all | std::filesystem::perms::others_all,
        ec);
#endif
    if (ec) {
        spdlog::error("Failed to set permissions on " WS_LINUX_RUN_DIR ": {}", ec.message());
    }
    {
#ifndef WINDSCRIBE_DEV_MODE
        UmaskGuard guard(0077);
#else
        UmaskGuard guard(0);
#endif
        std::filesystem::create_directories(WS_LINUX_TMP_DIR, ec);
    }
    if (ec) {
        spdlog::error("Failed to create " WS_LINUX_TMP_DIR ": {}", ec.message());
    }
#ifndef WINDSCRIBE_DEV_MODE // secure (non-dev)
    if (struct group *grp = getgrnam(WS_PRODUCT_NAME_LOWER)) {
        if (::lchown(WS_LINUX_TMP_DIR, 0, grp->gr_gid) != 0) {
            spdlog::error("Failed to chown " WS_LINUX_TMP_DIR ": {}", IO::strerror(errno));
        }
    } else {
        spdlog::error("Could not get group info for " WS_PRODUCT_NAME_LOWER);
    }
    std::filesystem::permissions(WS_LINUX_TMP_DIR, std::filesystem::perms::owner_all, ec);
#else // dev mode
    std::filesystem::permissions(WS_LINUX_TMP_DIR,
        std::filesystem::perms::owner_all | std::filesystem::perms::group_all | std::filesystem::perms::others_all,
        ec);
#endif
    if (ec) {
        spdlog::error("Failed to set permissions on " WS_LINUX_TMP_DIR ": {}", ec.message());
    }
    spdlog::info(WS_LINUX_RUN_DIR " and " WS_LINUX_TMP_DIR " created");

    unlink(SOCK_PATH);
    boost::asio::local::stream_protocol::endpoint ep(SOCK_PATH);
    {
#ifndef WINDSCRIBE_DEV_MODE
        UmaskGuard guard(0007);
#else
        UmaskGuard guard(0);
#endif
        acceptor_ = new boost::asio::local::stream_protocol::acceptor(io_context_, ep);
    }

#ifndef WINDSCRIBE_DEV_MODE
    struct group *grp = getgrnam(WS_PRODUCT_NAME_LOWER);
    if (!grp || chown(SOCK_PATH, (uid_t)-1, grp->gr_gid) != 0) {
        // Do not have a group, or chown failed.
        unlink(SOCK_PATH);
        return;
    }
#endif

    // Cause the FireallOnBootManager to be constructed here, so that on-boot rules are processed,
    // even if the app/service does not start.
    FirewallOnBootManager::instance();

    startAccept();

    io_context_.run();
}
