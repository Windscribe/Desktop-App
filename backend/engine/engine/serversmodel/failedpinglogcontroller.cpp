#include "failedpinglogcontroller.h"
#include "utils/logger.h"

void FailedPingLogController::logFailedIPs(const QStringList &ips)
{
    QMutexLocker locker(&mutex_);
    Q_FOREACH(const QString &ip, ips)
    {
        if (!failedPingIps_.contains(ip))
        {
            qCDebug(LOG_PING) << "Ping failed for node: " << ip;
            failedPingIps_ << ip;
        }
    }
}

void FailedPingLogController::addLoggedLocationWeights(const QString &locationName)
{
    QMutexLocker locker(&mutex_);
    if (!locations_.contains(locationName))
    {
        locations_ << locationName;
    }
}

bool FailedPingLogController::isLoggedLocation(const QString &locationName)
{
    QMutexLocker locker(&mutex_);
    return locations_.contains(locationName);
}

void FailedPingLogController::clear()
{
    QMutexLocker locker(&mutex_);
    failedPingIps_.clear();
    locations_.clear();
}

FailedPingLogController::FailedPingLogController()
{

}
