#pragma once

#include <QObject>
#include "failoverretcode.h"
#include "types/apiresolutionsettings.h"

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

    virtual void reset() = 0;
    virtual void getNextHostname(bool bIgnoreSslErrors) = 0;
    virtual void setApiResolutionSettings(const types::ApiResolutionSettings &apiResolutionSettings) = 0;

signals:
    void nextHostnameAnswer(failover::FailoverRetCode retCode, const QString &hostname);
    void tryingBackupEndpoint(int num, int cnt);
};

} // namespace failover
