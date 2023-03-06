#pragma once

#include "../basefailover.h"

namespace failover {

class HardcodedDomainFailover : public BaseFailover
{
    Q_OBJECT
public:
    explicit HardcodedDomainFailover(QObject *parent, const QString &uniqueId, const QString &domain) : BaseFailover(parent, uniqueId), domain_(domain) {}

    void getData(bool /*bIgnoreSslErrors*/) override
    {
        emit finished(QVector<FailoverData>() << FailoverData(domain_));
    }

    QString name() const override
    {
        // the domain name has been reduced to 3 characters for log security
        return "hrd";
    }

private:
    QString domain_;
};

} // namespace failover
