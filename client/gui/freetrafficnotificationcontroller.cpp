#include "freetrafficnotificationcontroller.h"

FreeTrafficNotificationController::FreeTrafficNotificationController(QObject *parent) : QObject(parent),
    b90emitted_(false),
    b95emitted_(false)
{

}

void FreeTrafficNotificationController::updateTrafficInfo(qint64 trafficUsed, qint64 trafficMax)
{
    if (!b95emitted_ && isReached(trafficUsed, trafficMax, 95))
    {
        emit freeTrafficNotification(tr("You reached %1% of your free bandwidth allowance").arg(95));
        b95emitted_ = true;
        b90emitted_ = true;
    }
    else if (!b90emitted_ && isReached(trafficUsed, trafficMax, 90))
    {
        emit freeTrafficNotification(tr("You reached %1% of your free bandwidth allowance").arg(90));
        b90emitted_ = true;
    }
}

bool FreeTrafficNotificationController::isReached(qint64 trafficUsed, qint64 trafficMax, int procent)
{
    double d = (double)trafficUsed / (double)trafficMax * 100.0;
    return (int)d >= procent;
}
