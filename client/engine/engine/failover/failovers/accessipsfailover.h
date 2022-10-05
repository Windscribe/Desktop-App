#pragma once

#include "basefailover.h"
#include "engine/connectstatecontroller/iconnectstatecontroller.h"
#include "engine/connectstatecontroller/connectstatewatcher.h"

namespace failover {

class AccessIpsFailover : public BaseFailover
{
    Q_OBJECT
public:
    explicit AccessIpsFailover(QObject *parent, NetworkAccessManager *networkAccessManager, const QString &ip, IConnectStateController *connectStateController) :
        BaseFailover(parent, networkAccessManager),
        ip_(ip),
        connectStateController_(connectStateController),
        connectStateWatcher_(nullptr)
    {}
    void getHostnames(bool bIgnoreSslErrors) override;
    QString name() const override;

private slots:
    void onNetworkRequestFinished();

private:
    static constexpr int kTimeout = 5000;          // timeout 5 sec by default
    QString ip_;
    IConnectStateController *connectStateController_;
    ConnectStateWatcher *connectStateWatcher_;

    QStringList handleRequest(const QByteArray &arr);
};

} // namespace failover

