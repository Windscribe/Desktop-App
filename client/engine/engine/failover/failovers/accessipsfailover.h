#pragma once

#include "../basefailover.h"

namespace failover {

class AccessIpsFailover : public BaseFailover
{
    Q_OBJECT
public:
    explicit AccessIpsFailover(QObject *parent, const QString &uniqueId, NetworkAccessManager *networkAccessManager, const QString &ip) :
        BaseFailover(parent, uniqueId, networkAccessManager),
        ip_(ip)
    {}
    void getData(bool bIgnoreSslErrors) override;
    QString name() const override;

private slots:
    void onNetworkRequestFinished();

private:
    static constexpr int kTimeout = 5000;          // timeout 5 sec by default
    QString ip_;
    QStringList handleRequest(const QByteArray &arr);
};

} // namespace failover

