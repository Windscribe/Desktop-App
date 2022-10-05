#pragma once

#include "basefailover.h"
#include "engine/connectstatecontroller/iconnectstatecontroller.h"
#include "engine/connectstatecontroller/connectstatewatcher.h"

namespace failover {

class DynamicDomainFailover : public BaseFailover
{
    Q_OBJECT
public:
    explicit DynamicDomainFailover(QObject *parent, NetworkAccessManager *networkAccessManager, const QString &urlString, const QString &domainName, IConnectStateController *connectStateController) :
        BaseFailover(parent, networkAccessManager),
        urlString_(urlString),
        domainName_(domainName),
        connectStateController_(connectStateController),
        connectStateWatcher_(nullptr)
    {}

    void getHostnames(bool bIgnoreSslErrors) override;
    QString name() const override;

private:
    QString urlString_;
    QString domainName_;
    IConnectStateController *connectStateController_;
    ConnectStateWatcher *connectStateWatcher_;

    QString parseHostnameFromJson(const QByteArray &arr);
};

} // namespace failover

