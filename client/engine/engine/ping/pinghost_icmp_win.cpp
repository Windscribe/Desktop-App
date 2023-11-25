#include "pinghost_icmp_win.h"

#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <winternl.h>
#include <icmpapi.h>

#include "utils/ipvalidation.h"
#include "utils/logger.h"

#include <QPointer>

PingHost_ICMP_win::PingHost_ICMP_win(QObject *parent, const QString &ip)
    : IPingHost(parent), ip_(ip)
{
}

void PingHost_ICMP_win::ping()
{
    if (!IpValidation::isIp(ip_)) {
        qCDebug(LOG_PING) << "PingHost_ICMP_win::addHostForPing - invalid IP" << ip_;
        emit pingFinished(false, 0, ip_);
        return;
    }

    icmpFile_ = ::IcmpCreateFile();

    if (icmpFile_ == INVALID_HANDLE_VALUE) {
        qCDebug(LOG_PING) << "PingHost_ICMP_win IcmpCreateFile failed" << ::GetLastError();
        emit pingFinished(false, 0, ip_);
        return;
    }

    const char dataForSend[] = "HelloBufferBuffer";
    replySize_ = sizeof(ICMP_ECHO_REPLY) + sizeof(dataForSend) + 8;
    replyBuffer_.reset(new unsigned char[replySize_]);

    IN_ADDR ipAddr;
    if (inet_pton(AF_INET, qPrintable(ip_), &ipAddr) != 1) {
        qCDebug(LOG_PING) << "PingHost_ICMP_win inet_pton failed" << ::WSAGetLastError();
        emit pingFinished(false, 0, ip_);
        return;
    }

    timer_.start();

    // Since the callback can be called after the object has been deleted, we should use a QPointer to avoid working with the deleted object's data
    QPointer<PingHost_ICMP_win> *argThisPointer = new QPointer<PingHost_ICMP_win>(this);
    DWORD result = ::IcmpSendEcho2(icmpFile_, NULL, icmpCallback, argThisPointer, ipAddr.S_un.S_addr,
                                   (LPVOID)dataForSend, sizeof(dataForSend), NULL,
                                   replyBuffer_.get(), replySize_, 2000);
    if (result != 0) {
        qCDebug(LOG_PING) << "PingHost_ICMP_win IcmpSendEcho2 returned unexpected result" << result << ::GetLastError();
        emit pingFinished(false, 0, ip_);
        return;
    }
    else if (::GetLastError() != ERROR_IO_PENDING) {
        qCDebug(LOG_PING) << "PingHost_ICMP_win IcmpSendEcho2 failed" << ::GetLastError();
        emit pingFinished(false, 0, ip_);
        return;
    }
}

void PingHost_ICMP_win::onPingRequestFinished(bool success, int timems, const QString &ip)
{
    emit pingFinished(success, timems, ip);
}

void PingHost_ICMP_win::icmpCallback(PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, ULONG)
{
    QPointer<PingHost_ICMP_win> *this_ = static_cast<QPointer<PingHost_ICMP_win>*>(ApcContext);
    if (this_) {
        bool bSuccess = false;
        if (NT_SUCCESS(IoStatusBlock->Status) && (*this_)->replyBuffer_ && (*this_)->replySize_ > 0) {
            bSuccess = ::IcmpParseReplies((*this_)->replyBuffer_.get(), (*this_)->replySize_) > 0;
        }
        QMetaObject::invokeMethod((*this_), "onPingRequestFinished", Qt::QueuedConnection, Q_ARG(bool, bSuccess), Q_ARG(int, (*this_)->timer_.elapsed()), Q_ARG(QString, (*this_)->ip_));
    }
    // Don't forget to delete memory for QPointer<PingHost_ICMP_win>
    delete this_;
}
