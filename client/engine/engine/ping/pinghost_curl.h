#pragma once

#include "engine/networkaccessmanager/networkaccessmanager.h"
#include "ipinghost.h"

class PingHost_Curl : public IPingHost
{
    Q_OBJECT
public:
    explicit PingHost_Curl(QObject *parent, NetworkAccessManager *networkAccessManager, const QString &ip, const QString &hostname);

    void ping() override;

private slots:
    void onNetworkRequestFinished();

private:
    enum {PING_TIMEOUT = 2000};

    NetworkAccessManager *networkAccessManager_;
    QMap<QString, NetworkReply *> pingingHosts_;
    QString ip_;
    QString hostname_;
    bool isAlreadyPinging_ = false;

    // Parse a string like {"rtt": "155457"} and return a rtt integer value. Return -1 if failed
    int parseReplyString(const QByteArray &arr);
};

