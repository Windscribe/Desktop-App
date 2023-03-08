#pragma once

#include "../basefailover.h"
#include "utils/hardcodedsettings.h"

namespace failover {

// Procedurally Generated Domain (deprecated)
class RandomDomainFailover : public BaseFailover
{
    Q_OBJECT
public:
    explicit RandomDomainFailover(QObject *parent, const QString &uniqueId) : BaseFailover(parent, uniqueId)
    {
        domain_ = HardcodedSettings::instance().generateDomain();
    }
    void getData(bool /*bIgnoreSslErrors*/) override
    {
        emit finished(QVector<FailoverData>() << FailoverData(domain_));
    }

    QString name() const override
    {
        // the domain name has been reduced to 3 characters for log security
        return "rnd: " + domain_.left(3);
    }

private:
    QString domain_;
};

} // namespace failover
