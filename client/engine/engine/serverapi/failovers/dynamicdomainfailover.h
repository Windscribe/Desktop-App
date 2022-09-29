#pragma once

#include "basefailover.h"

namespace server_api {

class DynamicDomainFailover : public BaseFailover
{
    Q_OBJECT
public:
    explicit DynamicDomainFailover(QObject *parent, NetworkAccessManager *networkAccessManager, const QString &urlString, const QString &domainName) :
        BaseFailover(parent, networkAccessManager),
        urlString_(urlString),
        domainName_(domainName)
    {}

    void getHostnames(bool bIgnoreSslErrors) override;
    QString name() const override;

private:
    QString urlString_;
    QString domainName_;

    QString parseHostnameFromJson(const QByteArray &arr);
};

} // namespace server_api

