#pragma once

#include "../basefailover.h"

namespace failover {

class CdnDomainFailover : public BaseFailover
{
    Q_OBJECT
public:
    explicit CdnDomainFailover(QObject *parent, const QString &uniqueId, const QString &domain, const QString &sniDomain) : BaseFailover(parent, uniqueId),
                                domain_(domain), sniDomain_(sniDomain) {}

    void getData(bool /*bIgnoreSslErrors*/) override
    {
        emit finished(QVector<FailoverData>() << FailoverData(domain_, sniDomain_));
    }

    QString name() const override
    {
        // the domain name has been reduced to 3 characters for log security
        return "cdn: " + domain_.left(3);
    }

private:
    QString domain_;
    QString sniDomain_;
};

} // namespace failover
