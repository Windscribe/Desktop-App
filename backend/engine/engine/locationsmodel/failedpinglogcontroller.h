#ifndef FAILEDPINGLOGCONTROLLER_H
#define FAILEDPINGLOGCONTROLLER_H

#include <QSet>
#include <QString>
#include <QMutex>

class FailedPingLogController
{
public:
    void logFailedIPs(const QStringList &ips);
    void clear();


private:
    QSet<QString> failedPingIps_;
    QSet<QString> locations_;
    QMutex mutex_;
};

#endif // FAILEDPINGLOGCONTROLLER_H
