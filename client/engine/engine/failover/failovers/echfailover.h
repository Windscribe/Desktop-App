#pragma once

#include "../basefailover.h"

namespace failover {

class EchFailover : public BaseFailover
{
    Q_OBJECT
public:
    explicit EchFailover(QObject *parent, const QString &uniqueId, NetworkAccessManager *networkAccessManager, const QString &urlString, const QString &configDomainName, const QString &domainName, bool isFallback) :
        BaseFailover(parent, uniqueId, networkAccessManager),
        urlString_(urlString),
        configDomainName_(configDomainName),
        domainName_(domainName),
        isFallback_(isFallback)
    {}

    void getData(bool bIgnoreSslErrors) override;
    QString name() const override;

private:
    QString urlString_;
    QString configDomainName_;
    QString domainName_;
    bool isFallback_;

    QVector<failover::FailoverData> parseDataFromJson(const QByteArray &arr);
};

} // namespace failover

