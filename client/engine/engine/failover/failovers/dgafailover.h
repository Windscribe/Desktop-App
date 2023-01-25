#pragma once

#include "basefailover.h"

namespace failover {

// Procedurally Generated Domain
class DgaFailover : public BaseFailover
{
    Q_OBJECT
public:
    explicit DgaFailover(QObject *parent) : BaseFailover(parent) {}
    void getHostnames(bool /*bIgnoreSslErrors*/) override;

    QString name() const override
    {
        // the domain name has been reduced to 3 characters for log security
        return "dga";
    }

};

} // namespace failover
