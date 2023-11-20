#pragma once

#include <QSet>
#include <QString>
#include <QMutex>

class FailedPingLogController
{
public:
    bool logFailedIPs(const QString &ip);
    void clear();


private:
    QSet<QString> failedPingIps_;
    QSet<QString> locations_;
    QMutex mutex_;
};

