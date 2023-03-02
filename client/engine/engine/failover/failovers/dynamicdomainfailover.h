#pragma once

#include "../basefailover.h"

namespace failover {

class DynamicDomainFailover : public BaseFailover
{
    Q_OBJECT
public:
    explicit DynamicDomainFailover(QObject *parent, const QString &uniqueId, NetworkAccessManager *networkAccessManager, const QString &urlString, const QString &domainName) :
        BaseFailover(parent, uniqueId, networkAccessManager),
        urlString_(urlString),
        domainName_(domainName)
    {}

    void getData(bool bIgnoreSslErrors) override;
    QString name() const override;

private:
    QString urlString_;
    QString domainName_;

    QString parseHostnameFromJson(const QByteArray &arr);
};

} // namespace failover

