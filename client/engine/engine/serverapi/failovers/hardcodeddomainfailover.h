#pragma once

#include "basefailover.h"

namespace server_api {

class HardcodedDomainFailover : public BaseFailover
{
    Q_OBJECT
public:
    explicit HardcodedDomainFailover(QObject *parent, const QString &domain) : BaseFailover(parent), domain_(domain) {}
    void getHostnames(bool /*bIgnoreSslErrors*/) override
    {
        emit finished(FailoverRetCode::kSuccess, QStringList() << domain_);
    }

    QString name() const override
    {
        // the domain name has been reduced to 3 characters for log security
        return "hrd";
    }

private:
    QString domain_;
};

} // namespace server_api

