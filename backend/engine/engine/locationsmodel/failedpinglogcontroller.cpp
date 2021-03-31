#include "failedpinglogcontroller.h"
#include "utils/logger.h"

bool FailedPingLogController::logFailedIPs(const QString &ip)
{
    QMutexLocker locker(&mutex_);

    if (!failedPingIps_.contains(ip))
    {
        qCDebug(LOG_PING) << "Ping failed for node: " << ip;
        failedPingIps_ << ip;
        return true;
    }
    return false;
}

void FailedPingLogController::clear()
{
    QMutexLocker locker(&mutex_);
    failedPingIps_.clear();
    locations_.clear();
}
