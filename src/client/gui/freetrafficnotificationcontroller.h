#pragma once

#include <QObject>

class FreeTrafficNotificationController : public QObject
{
    Q_OBJECT
public:
    explicit FreeTrafficNotificationController(QObject *parent = 0);
    void updateTrafficInfo(qint64 trafficUsed, qint64 trafficMax);

signals:
    void freeTrafficNotification(const QString &message);

private:
    bool b90emitted_;
    bool b95emitted_;

    bool isReached(qint64 trafficUsed, qint64 trafficMax, int procent);
};
