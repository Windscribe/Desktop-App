#pragma once

#include <QObject>
#include "failoverretcode.h"

class NetworkAccessManager;
class IConnectStateController;

namespace failover {

// Interface for a failover algorithm. Allows us to switch sequentially to the next domain and hides implementation details.
class IFailover : public QObject
{
    Q_OBJECT
public:
    explicit IFailover(QObject *parent) : QObject(parent) {}
    virtual ~IFailover() {}

    // can return an empty string if the Failover in the FailoverRetCode::kFailed state
    virtual QString currentHostname() const = 0;
    virtual void reset() = 0;
    virtual void getNextHostname(bool bIgnoreSslErrors) = 0;

signals:
    void nextHostnameAnswer(failover::FailoverRetCode retCode, const QString &hostname);
};

} // namespace failover
