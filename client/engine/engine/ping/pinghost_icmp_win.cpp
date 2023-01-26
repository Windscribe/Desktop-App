#include "pinghost_icmp_win.h"

#include <QElapsedTimer>

#include <ws2tcpip.h>
#include <iphlpapi.h>

#include <winternl.h>
#include <icmpapi.h>

#include "utils/ipvalidation.h"
#include "utils/logger.h"

static constexpr int MAX_PARALLEL_PINGS = 10;

class PingRequest : public QObject
{
    Q_OBJECT
public:
    explicit PingRequest(QObject *parent, const QString &ip);
    ~PingRequest();

    bool parseReply();
    bool sendRequest();

    bool isFromDisconnectedState() const { return isFromDisconnectedState_; }
    void setFromDisconnectedState(bool disconncted) { isFromDisconnectedState_ = disconncted; }

signals:
    void requestFinished(bool success, const QString ip, qint64 elapsed);

private:
    const QString ip_;
    bool isFromDisconnectedState_ = true;
    HANDLE icmpFile_ = INVALID_HANDLE_VALUE;

    std::unique_ptr<unsigned char[]> replyBuffer_;
    DWORD replySize_ = 0;

    QElapsedTimer timer_;

private:
    static VOID NTAPI icmpCallback(IN PVOID ApcContext, IN PIO_STATUS_BLOCK IoStatusBlock, IN ULONG /*Reserved*/);
};

PingRequest::PingRequest(QObject *parent, const QString &ip)
    : QObject(parent),
      ip_(ip)
{
}

PingRequest::~PingRequest()
{
    if (icmpFile_ != INVALID_HANDLE_VALUE) {
        ::IcmpCloseHandle(icmpFile_);
    }
}

bool PingRequest::sendRequest()
{
    icmpFile_ = ::IcmpCreateFile();

    if (icmpFile_ == INVALID_HANDLE_VALUE) {
        qCDebug(LOG_PING) << "PingHost_ICMP_win IcmpCreateFile failed" << ::GetLastError();
        return false;
    }

    const char dataForSend[] = "HelloBufferBuffer";
    replySize_ = sizeof(ICMP_ECHO_REPLY) + sizeof(dataForSend) + 8;
    replyBuffer_.reset(new unsigned char[replySize_]);

    IN_ADDR ipAddr;
    if (inet_pton(AF_INET, qPrintable(ip_), &ipAddr) != 1) {
        qCDebug(LOG_PING) << "PingHost_ICMP_win inet_pton failed" << ::WSAGetLastError();
        return false;
    }

    timer_.start();

    DWORD result = ::IcmpSendEcho2(icmpFile_, NULL, icmpCallback, this, ipAddr.S_un.S_addr,
                                   (LPVOID)dataForSend, sizeof(dataForSend), NULL,
                                   replyBuffer_.get(), replySize_, 2000);
    if (result != 0) {
        qCDebug(LOG_PING) << "PingHost_ICMP_win IcmpSendEcho2 returned unexpected result" << result << ::GetLastError();
        return false;
    }
    else if (::GetLastError() != ERROR_IO_PENDING) {
        qCDebug(LOG_PING) << "PingHost_ICMP_win IcmpSendEcho2 failed" << ::GetLastError();
        return false;
    }

    return true;
}

bool PingRequest::parseReply()
{
    if (replyBuffer_ && replySize_ > 0) {
        return (::IcmpParseReplies(replyBuffer_.get(), replySize_) > 0);
    }

    return false;
}

VOID NTAPI PingRequest::icmpCallback(IN PVOID ApcContext, IN PIO_STATUS_BLOCK IoStatusBlock, IN ULONG /*Reserved*/)
{
    PingRequest *request = static_cast<PingRequest*>(ApcContext);
    Q_EMIT request->requestFinished((IoStatusBlock->Status == 0), request->ip_, request->timer_.elapsed());
}


PingHost_ICMP_win::PingHost_ICMP_win(QObject *parent, IConnectStateController *stateController)
    : QObject(parent),
      connectStateController_(stateController)
{
}

PingHost_ICMP_win::~PingHost_ICMP_win()
{
    clearPings();
}

void PingHost_ICMP_win::addHostForPing(const QString &ip)
{
    if (!IpValidation::isIp(ip)) {
        qCDebug(LOG_PING) << "PingHost_ICMP_win::addHostForPing - invalid IP" << ip;
        return;
    }

    QMutexLocker locker(&mutex_);
    if (!pingingHosts_.contains(ip) && !waitingPingsQueue_.contains(ip)) {
        waitingPingsQueue_.enqueue(ip);
        sendNextPing();
    }
}

void PingHost_ICMP_win::clearPings()
{
    QMutexLocker locker(&mutex_);
    for (auto request : qAsConst(pingingHosts_)) {
        delete request;
    }
    pingingHosts_.clear();
    waitingPingsQueue_.clear();
}

void PingHost_ICMP_win::setProxySettings(const types::ProxySettings &proxySettings)
{
    Q_UNUSED(proxySettings)
}

void PingHost_ICMP_win::disableProxy()
{
    //todo
}

void PingHost_ICMP_win::enableProxy()
{
    //todo
}

void PingHost_ICMP_win::sendNextPing()
{
    if (pingingHosts_.size() < MAX_PARALLEL_PINGS && !waitingPingsQueue_.isEmpty()) {
        const QString ip = waitingPingsQueue_.dequeue();
        auto request = std::make_unique<PingRequest>(nullptr, ip);

        if (connectStateController_) {
            request->setFromDisconnectedState(connectStateController_->currentState() == CONNECT_STATE_DISCONNECTED ||
                                              connectStateController_->currentState() == CONNECT_STATE_CONNECTING);
        }

        connect(request.get(), &PingRequest::requestFinished, this, &PingHost_ICMP_win::onPingRequestFinished);

        if (!request->sendRequest()) {
            Q_EMIT pingFinished(false, 0, ip, request->isFromDisconnectedState());
            waitingPingsQueue_.removeAll(ip);
            sendNextPing();
            return;
        }

        pingingHosts_[ip] = request.release();
    }
}

void PingHost_ICMP_win::onPingRequestFinished(bool success, const QString ip, qint64 elapsed)
{
    QMutexLocker locker(&mutex_);
    auto it = pingingHosts_.find(ip);

    if (it != pingingHosts_.end()) {
        std::unique_ptr<PingRequest> request(it.value());
        pingingHosts_.remove(ip);

        if (success && request->parseReply()) {
            Q_EMIT pingFinished(true, elapsed, ip, request->isFromDisconnectedState());
        }
        else {
            Q_EMIT pingFinished(false, 0, ip, request->isFromDisconnectedState());
        }
    }

    waitingPingsQueue_.removeAll(ip);
    sendNextPing();
}

// Following line required if the Q_OBJECT macro is used within a cpp file.
#include "pinghost_icmp_win.moc"
