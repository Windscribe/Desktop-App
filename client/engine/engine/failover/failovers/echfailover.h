#pragma once

#include "../basefailover.h"
#include "engine/connectstatecontroller/iconnectstatecontroller.h"
#include "engine/connectstatecontroller/connectstatewatcher.h"

namespace failover {

class EchFailover : public BaseFailover
{
    Q_OBJECT
public:
    explicit EchFailover(QObject *parent, const QString &uniqueId, NetworkAccessManager *networkAccessManager, const QString &urlString, const QString &configDomainName, const QString &domainName, IConnectStateController *connectStateController) :
        BaseFailover(parent, uniqueId, networkAccessManager),
        urlString_(urlString),
        configDomainName_(configDomainName),
        domainName_(domainName),
        connectStateController_(connectStateController),
        connectStateWatcher_(nullptr)
    {}

    void getData(bool bIgnoreSslErrors) override;
    QString name() const override;
    bool isEch() const override { return true; }

private:
    QString urlString_;
    QString configDomainName_;
    QString domainName_;
    IConnectStateController *connectStateController_;
    ConnectStateWatcher *connectStateWatcher_;

    QVector<failover::FailoverData> parseDataFromJson(const QByteArray &arr);
};

} // namespace failover

