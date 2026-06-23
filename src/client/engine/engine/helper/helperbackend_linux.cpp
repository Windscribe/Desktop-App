#include "helperbackend_linux.h"

#include <unistd.h>

#include <QMutexLocker>
#include "../../../../helper/common/helper_commands.h"
#include "utils/helperconnector_linux.h"
#include "utils/log/categories.h"

HelperBackend_linux::HelperBackend_linux(QObject *parent) :
    IHelperBackend(parent)
{

}

HelperBackend_linux::~HelperBackend_linux()
{
    stopRequested_ = true;
    wait();
}

void HelperBackend_linux::startInstallHelper(bool /*bForceDeleteOld*/)
{
    start(QThread::LowPriority);
}

HelperBackend_linux::State HelperBackend_linux::currentState() const
{
    QMutexLocker locker(&mutex_);
    return curState_;
}

bool HelperBackend_linux::reinstallHelper()
{
    return false;
}

void HelperBackend_linux::run()
{
    stopRequested_ = false;
    {
        QMutexLocker locker(&mutex_);
        curState_ = State::kInit;
    }
    socket_.reset(new boost::asio::local::stream_protocol::socket(io_context_));

    // The helper connection was opened at startup, before this process dropped the 'windscribe'
    // group (see HelperConnector / main.cpp). Adopt that fd here rather than connecting, since this
    // thread no longer holds the group. Poll so the destructor's stop request can break the wait.
    int fd = -1;
    while (!HelperConnector::tryGetConnectedFd(fd)) {
        if (stopRequested_) {
            // Give up before the fd was handed over; close it if/when the connect completes.
            HelperConnector::abandon();
            QMutexLocker locker(&mutex_);
            curState_ = State::kFailedConnect;
            return;
        }
        msleep(10);
    }

    boost::system::error_code ec;
    if (fd >= 0) {
        socket_->assign(boost::asio::local::stream_protocol(), fd, ec);
        if (ec) {
            // assign() did not take ownership of the fd; close it so it is not leaked.
            ::close(fd);
        }
    } else {
        ec = boost::asio::error::not_connected;
    }

    QMutexLocker locker(&mutex_);
    if (!ec) {
        curState_ = State::kConnected;
        qCInfo(LOG_BASIC) << "connected to helper socket";
    } else {
        qCCritical(LOG_BASIC) << "Error while connecting to helper: " << ec.value();
        curState_ = State::kFailedConnect;
    }
}

bool HelperBackend_linux::sendCmdToHelper(int cmdId, const std::string &data)
{
    int length = data.size();
    auto logError = [](const boost::system::error_code &ec) {
        qCWarning(LOG_BASIC) << "HelperBackend_linux::sendCmdToHelper, write error:" << QString::fromStdString(ec.message());
    };

    // first 4 bytes - cmdId
    boost::system::error_code ec = safeWrite(&cmdId, sizeof(cmdId));
    if (ec) {
        logError(ec);
        return false;
    }
    // second 4 bytes - pid
    const auto pid = getpid();
    ec = safeWrite(&pid, sizeof(pid));
    if (ec) {
        logError(ec);
        return false;
    }
    // third 4 bytes - size of buffer
    ec = safeWrite(&length, sizeof(length));
    if (ec) {
        logError(ec);
        return false;
    }
    // body of message
    ec = safeWrite(data.data(), length);
    if (ec) {
        logError(ec);
        return false;
    }
    return true;
}

bool HelperBackend_linux::readAnswer(std::string &answer)
{
    int length;
    auto logError = [](const boost::system::error_code &ec) {
        qCWarning(LOG_BASIC) << "HelperBackend_linux::readAnswer, read error:" << QString::fromStdString(ec.message());
    };

    boost::system::error_code ec = safeRead(&length, sizeof(length));
    if (ec) {
        logError(ec);
        return false;
    }

    // Reject malformed or oversized lengths before allocation. Defends the engine
    // against a compromised/spoofed helper sending a bogus reply length.
    if (length < 0 || static_cast<size_t>(length) > kMaxHelperFrameSize) {
        qCWarning(LOG_BASIC) << "HelperBackend_linux::readAnswer, invalid length:" << length;
        return false;
    }

    if (length == 0) {
        answer.clear();
        return true;
    }

    try {
        std::vector<char> buff(length);
        ec = safeRead(buff.data(), length);
        if (ec) {
            logError(ec);
            return false;
        }
        answer = std::string(buff.begin(), buff.end());
    } catch (const std::bad_alloc &) {
        qCWarning(LOG_BASIC) << "HelperBackend_linux::readAnswer, bad_alloc for length:" << length;
        return false;
    }
    return true;
}

boost::system::error_code HelperBackend_linux::safeRead(void *data, size_t size)
{
    char *buf = static_cast<char *>(data);
    boost::system::error_code ec;
    size_t total_read = 0;
    while (total_read < size) {
        size_t sz = boost::asio::read(*socket_, boost::asio::buffer(buf + total_read, size - total_read), boost::asio::transfer_exactly(size - total_read), ec);
        if (ec == boost::asio::error::interrupted) {    // EINTR signal, should be ignored on POSIX systems
            continue;
        } else if (ec) {
            return ec;
        }
        total_read += sz;
    }
    return ec;
}

boost::system::error_code HelperBackend_linux::safeWrite(const void *data, size_t size)
{
    const char *buf = static_cast<const char *>(data);
    boost::system::error_code ec;
    size_t total_written = 0;
    while (total_written < size) {
        size_t sz = boost::asio::write(*socket_, boost::asio::buffer(buf + total_written, size - total_written), boost::asio::transfer_exactly(size - total_written), ec);
        if (ec == boost::asio::error::interrupted) {    // EINTR signal, should be ignored on POSIX systems
            continue;
        } else if (ec) {
            return ec;
        }
        total_written += sz;
    }
    return ec;
}

std::string HelperBackend_linux::sendCmd(int cmdId, const std::string &data)
{
    QMutexLocker locker(&mutex_);

    bool ret = sendCmdToHelper(cmdId, data);
    if (!ret) {
        qCWarning(LOG_BASIC) << "Disconnected from helper socket";
        emit lostConnectionToHelper();
        return std::string();
    }

    std::string answer;
    if (!readAnswer(answer)) {
        qCWarning(LOG_BASIC) << "Disconnected from helper socket";
        emit lostConnectionToHelper();
        return std::string();
    }
    return answer;
}

