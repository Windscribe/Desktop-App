#pragma once

#include "../basefailover.h"
#include "utils/hardcodedsettings.h"

namespace failover {

// Procedurally Generated Domain (deprecated)
class RandomDomainFailover : public BaseFailover
{
    Q_OBJECT
public:
    explicit RandomDomainFailover(QObject *parent, const QString &uniqueId) : BaseFailover(parent, uniqueId) {}
    void getData(bool /*bIgnoreSslErrors*/) override {
        emit finished(QVector<FailoverData>() << FailoverData(HardcodedSettings::instance().generateDomain()));
    }

    QString name() const override
    {
        // the domain name has been reduced to 3 characters for log security
        return "rnd";
    }
};

} // namespace failover
