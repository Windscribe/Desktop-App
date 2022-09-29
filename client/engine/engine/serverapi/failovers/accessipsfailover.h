#pragma once

#include "basefailover.h"

namespace server_api {

class AccessIpsFailover : public BaseFailover
{
    Q_OBJECT
public:
    explicit AccessIpsFailover(QObject *parent, NetworkAccessManager *networkAccessManager, const QString &ip) :
        BaseFailover(parent, networkAccessManager),
        ip_(ip)
    {}
    void getHostnames(bool bIgnoreSslErrors) override;
    QString name() const override;

private slots:
    void onNetworkRequestFinished();

private:
    QString ip_;
};

} // namespace server_api

