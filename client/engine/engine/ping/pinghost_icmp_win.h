#pragma once

#include <QElapsedTimer>
#include <qt_windows.h>

#include "ipinghost.h"

struct _IO_STATUS_BLOCK;

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

    static VOID NTAPI icmpCallback(IN PVOID ApcContext, IN _IO_STATUS_BLOCK *IoStatusBlock, IN ULONG /*Reserved*/);
};
