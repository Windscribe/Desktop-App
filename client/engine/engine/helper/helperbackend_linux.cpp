#include "helperbackend_linux.h"
#include <QMutexLocker>
#include "utils/log/categories.h"

#define SOCK_PATH "/var/run/windscribe/helper.sock"

HelperBackend_linux::HelperBackend_linux(QObject *parent) :
    IHelperBackend(parent),
    ep_(SOCK_PATH)
{

}

HelperBackend_linux::~HelperBackend_linux()
{
    io_service_.stop();
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
    io_service_.reset();
    reconnectElapsedTimer_.start();
    socket_.reset(new boost::asio::local::stream_protocol::socket(io_service_));
    socket_->async_connect(ep_, std::bind(&HelperBackend_linux::connectHandler, this, boost::asio::placeholders::error));
    io_service_.run();
}

void HelperBackend_linux::connectHandler(const boost::system::error_code &ec)
{
    if (!ec) {
        // we connected
        curState_ = State::kConnected;
        qCInfo(LOG_BASIC) << "connected to helper socket";
    } else {
        if (reconnectElapsedTimer_.elapsed() > kMaxWaitHelper) {
            qCCritical(LOG_BASIC) << "Error while connecting to helper: " << ec.value();
            curState_ = State::kFailedConnect;
        } else {
            // try reconnect
            msleep(10);
            {
                QMutexLocker locker(&mutexSocket_);
                socket_->async_connect(ep_, std::bind(&HelperBackend_linux::connectHandler, this, boost::asio::placeholders::error));
            }
        }
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
    } else {
        std::vector<char> buff(length);
        ec = safeRead(&buff[0], length);
        if (ec) {
            logError(ec);
            return false;
        }
        answer = std::string(buff.begin(), buff.end());
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

