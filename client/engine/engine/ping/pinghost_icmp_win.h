#pragma once

#include <QObject>
#include "engine/ConnectStateController/iconnectstatecontroller.h"
#include "utils/boost_includes.h"
#include "types/proxysettings.h"
#include <QQueue>
#include <QMap>
#include <QMutex>
#include <QElapsedTimer>
#include <winternl.h>

// todo proxy support for icmp ping
class PingHost_ICMP_win : public QObject
{
    Q_OBJECT
public:
    explicit PingHost_ICMP_win(QObject *parent, IConnectStateController *stateController);
    virtual ~PingHost_ICMP_win();

    void addHostForPing(const QString &ip);
    void clearPings();

    void setProxySettings(const types::ProxySettings &proxySettings);
    void disableProxy();
    void enableProxy();

signals:
    void pingFinished(bool bSuccess, int timems, const QString &ip, bool isFromDisconnectedState);

private:

    struct PingInfo
    {
        QString ip;
        bool isFromDisconnectedState_ = false;
        QElapsedTimer elapsedTimer;
        HANDLE hIcmpFile = NULL;

        unsigned char* replyBuffer = nullptr;
        DWORD replySize = 0;

        PingInfo() {}

        ~PingInfo()
        {
            if (replyBuffer) {
                delete[] replyBuffer;
            }
        }
    };

    QRecursiveMutex mutex_;
    IConnectStateController *connectStateController_;

    static constexpr int MAX_PARALLEL_PINGS = 10;
    QMap<QString, PingInfo *> pingingHosts_;
    QQueue<QString> waitingPingsQueue_;

    struct USER_ARG
    {
        PingHost_ICMP_win *this_;
        PingInfo *pingInfo;
    };

    bool hostAlreadyPingingOrInWaitingQueue(const QString &ip);
    static VOID NTAPI icmpCallback(IN PVOID ApcContext, IN PIO_STATUS_BLOCK IoStatusBlock, IN ULONG Reserved);
    void processNextPings();
};
