#pragma once

#include "types/proxysettings.h"
#include "engine/networkaccessmanager/networkaccessmanager.h"

class IConnectStateController;

class PingHost_Curl : public QObject
{
    Q_OBJECT
public:
    // stateController can be NULL, in this case not used
    explicit PingHost_Curl(QObject *parent, IConnectStateController *stateController, NetworkAccessManager *networkAccessManager);
    virtual ~PingHost_Curl();

    void addHostForPing(const QString &ip, const QString &hostame);
    void clearPings();

    void setProxySettings(const types::ProxySettings &proxySettings);
    void disableProxy();
    void enableProxy();

signals:
    void pingFinished(bool bSuccess, int timems, const QString &ip, bool isFromDisconnectedState);

private slots:
    void onNetworkRequestFinished();

private:
    enum {PING_TIMEOUT = 2000};

    IConnectStateController *connectStateController_;
    NetworkAccessManager *networkAccessManager_;

    QMap<QString, NetworkReply *> pingingHosts_;

    // Parse a string like {"rtt": "155457"} and return a rtt integer value. Return -1 if failed
    int parseReplyString(const QByteArray &arr);
};

