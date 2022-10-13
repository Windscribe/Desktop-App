#pragma once

#include <QObject>
#include "../failoverretcode.h"

class NetworkAccessManager;

namespace failover {

class BaseFailover : public QObject
{
    Q_OBJECT
public:
    explicit BaseFailover(QObject *parent, NetworkAccessManager *networkAccessManager = nullptr) : QObject(parent), networkAccessManager_(networkAccessManager) {}
    virtual void getHostnames(bool bIgnoreSslErrors) = 0;
    virtual QString name() const = 0;

signals:
    void finished(failover::FailoverRetCode retCode, const QStringList &hostnames);

protected:
     NetworkAccessManager *networkAccessManager_;
};


} // namespace failover

