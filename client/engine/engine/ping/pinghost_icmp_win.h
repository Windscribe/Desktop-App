#pragma once

#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <winternl.h>
#include <icmpapi.h>
#include <QElapsedTimer>
#include "ipinghost.h"

// todo proxy support for icmp ping
class PingHost_ICMP_win : public IPingHost
{
    Q_OBJECT
public:
    explicit PingHost_ICMP_win(QObject *parent, const QString &ip);

    void ping() override;

private slots:
    void onPingRequestFinished(bool success, int timems, const QString &ip);

private:
    QString ip_;
    HANDLE icmpFile_ = INVALID_HANDLE_VALUE;
    std::unique_ptr<unsigned char[]> replyBuffer_;
    DWORD replySize_ = 0;
    QElapsedTimer timer_;

    static VOID NTAPI icmpCallback(IN PVOID ApcContext, IN PIO_STATUS_BLOCK IoStatusBlock, IN ULONG /*Reserved*/);
};
