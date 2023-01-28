#include "pinghost_icmp_win.h"
#include "Utils/ipvalidation.h"
#include "Utils/utils.h"
#include "icmp_header.h"
#include "ipv4_header.h"

#include <winsock2.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include "Utils/logger.h"

PingHost_ICMP_win::PingHost_ICMP_win(QObject *parent, IConnectStateController *stateController) : QObject(parent),
    mutex_(QRecursiveMutex()), connectStateController_(stateController)
{

}

PingHost_ICMP_win::~PingHost_ICMP_win()
{
    clearPings();
}

void PingHost_ICMP_win::addHostForPing(const QString &ip)
{
    QMutexLocker locker(&mutex_);
    if (!hostAlreadyPingingOrInWaitingQueue(ip))
    {
        waitingPingsQueue_.enqueue(ip);
        processNextPings();
    }
}

void PingHost_ICMP_win::clearPings()
{
    QMutexLocker locker(&mutex_);
    for (QMap<QString, PingInfo *>::iterator it = pingingHosts_.begin(); it != pingingHosts_.end(); ++it)
    {
        IcmpCloseHandle(it.value()->hIcmpFile);
        delete it.value();
    }
    pingingHosts_.clear();
    waitingPingsQueue_.clear();
}

void PingHost_ICMP_win::setProxySettings(const ProxySettings & /*proxySettings*/)
{
    //todo
}

void PingHost_ICMP_win::disableProxy()
{
    //todo
}

void PingHost_ICMP_win::enableProxy()
{
    //todo
}

bool PingHost_ICMP_win::hostAlreadyPingingOrInWaitingQueue(const QString &ip)
{
    return pingingHosts_.find(ip) != pingingHosts_.end() || waitingPingsQueue_.indexOf(ip) != -1;
}

VOID NTAPI PingHost_ICMP_win::icmpCallback(IN PVOID ApcContext, IN PIO_STATUS_BLOCK IoStatusBlock, IN ULONG /*Reserved*/)
{
    USER_ARG *userArg = static_cast<USER_ARG *>(ApcContext);
    QMutexLocker locker(&(userArg->this_->mutex_));
    int timeMs = -1;
    if (IoStatusBlock->Status == 0) // success
    {
        if (IcmpParseReplies(userArg->pingInfo->replyBuffer, userArg->pingInfo->replySize) > 0)
        {
            timeMs = userArg->pingInfo->elapsedTimer.elapsed();
        }
    }

    QString ip = userArg->pingInfo->ip;
    auto it = userArg->this_->pingingHosts_.find(ip);
    if (it != userArg->this_->pingingHosts_.end())
    {
        PingInfo *pingInfo = it.value();
        bool bFromDisconnectedState = userArg->pingInfo->isFromDisconnectedState_;
        userArg->this_->pingingHosts_.remove(ip);
        IcmpCloseHandle(pingInfo->hIcmpFile);
        delete pingInfo;
        if (timeMs != -1)
        {
            Q_EMIT userArg->this_->pingFinished(true, timeMs, ip, bFromDisconnectedState);
        }
        else
        {
            Q_EMIT userArg->this_->pingFinished(false, 0, ip, bFromDisconnectedState);
        }
    }
    userArg->this_->waitingPingsQueue_.removeAll(ip);
    userArg->this_->processNextPings();

    delete userArg;
}

void PingHost_ICMP_win::processNextPings()
{
    if (pingingHosts_.count() < MAX_PARALLEL_PINGS && !waitingPingsQueue_.isEmpty())
    {
        QString ip = waitingPingsQueue_.dequeue();
        Q_ASSERT(IpValidation::instance().isIp(ip));

        const char dataForSend[] = "HelloBufferBuffer";

        PingInfo *pingInfo = new PingInfo();
        pingInfo->ip = ip;

        if (connectStateController_)
        {
            pingInfo->isFromDisconnectedState_ = (connectStateController_->currentState() == CONNECT_STATE_DISCONNECTED || connectStateController_->currentState() == CONNECT_STATE_CONNECTING);
        }
        else
        {
            pingInfo->isFromDisconnectedState_ = true;
        }

        pingInfo->hIcmpFile = IcmpCreateFile();
        if (pingInfo->hIcmpFile == INVALID_HANDLE_VALUE)
        {
            qCDebug(LOG_PING) << "IcmpCreateFile failed:" << GetLastError();

            Q_EMIT pingFinished(false, 0, ip, pingInfo->isFromDisconnectedState_);
            delete pingInfo;
            waitingPingsQueue_.removeAll(ip);
            processNextPings();
            return;
        }

        pingInfo->replySize = sizeof(ICMP_ECHO_REPLY) + sizeof(dataForSend) + 8;
        pingInfo->replyBuffer = new unsigned char[pingInfo->replySize];

        unsigned long ipaddr = inet_addr(ip.toStdString().c_str());

        pingingHosts_[ip] = pingInfo;

        USER_ARG *userArg = new USER_ARG();
        userArg->this_ = this;
        userArg->pingInfo = pingInfo;

        pingInfo->elapsedTimer.start();
        IcmpSendEcho2(pingInfo->hIcmpFile, NULL, icmpCallback, userArg,
                                 ipaddr, (LPVOID)dataForSend, sizeof(dataForSend), NULL,
                                 pingInfo->replyBuffer, pingInfo->replySize, 2000);
    }
}


