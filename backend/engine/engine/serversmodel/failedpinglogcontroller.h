#ifndef FAILEDPINGLOGCONTROLLER_H
#define FAILEDPINGLOGCONTROLLER_H

#include <QSet>
#include <QString>
#include <QMutex>

class FailedPingLogController
{
public:
    static FailedPingLogController &instance()
    {
        static FailedPingLogController s;
        return s;
    }

    void logFailedIPs(const QStringList &ips);
    void addLoggedLocationWeights(const QString &locationName);
    bool isLoggedLocation(const QString &locationName);
    void clear();


private:
    FailedPingLogController();

    QSet<QString> failedPingIps_;
    QSet<QString> locations_;
    QMutex mutex_;
};

#endif // FAILEDPINGLOGCONTROLLER_H
