#include "helperbackend_linux.h"
#include <QMutexLocker>
#include "utils/crashhandler.h"

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

void HelperBackend_linux::startInstallHelper()
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
    boost::system::error_code ec;

    // first 4 bytes - cmdId
    boost::asio::write(*socket_, boost::asio::buffer(&cmdId, sizeof(cmdId)), boost::asio::transfer_exactly(sizeof(cmdId)), ec);
    if (ec) {
        return false;
    }
    // second 4 bytes - pid
    const auto pid = getpid();
    boost::asio::write(*socket_, boost::asio::buffer(&pid, sizeof(pid)), boost::asio::transfer_exactly(sizeof(pid)), ec);
    if (ec) {
        return false;
    }
    // third 4 bytes - size of buffer
    boost::asio::write(*socket_, boost::asio::buffer(&length, sizeof(length)), boost::asio::transfer_exactly(sizeof(length)), ec);
    if (ec) {
        return false;
    }
    // body of message
    boost::asio::write(*socket_, boost::asio::buffer(data.data(), length), boost::asio::transfer_exactly(length), ec);
    if (ec) {
        return false;
    }
    return true;
}

bool HelperBackend_linux::readAnswer(std::string &answer)
{
    boost::system::error_code ec;
    int length;
    boost::asio::read(*socket_, boost::asio::buffer(&length, sizeof(length)),
                      boost::asio::transfer_exactly(sizeof(length)), ec);
    if (ec) {
        return false;
    } else {
        std::vector<char> buff(length);
        boost::asio::read(*socket_, boost::asio::buffer(&buff[0], length),
                          boost::asio::transfer_exactly(length), ec);
        if (ec) {
            return false;
        }
        answer = std::string(buff.begin(), buff.end());
    }
    return true;
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

