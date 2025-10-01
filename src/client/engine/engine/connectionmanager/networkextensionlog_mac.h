#pragma once

#include <QObject>
#include <QDateTime>
#include <QMap>

// collect logs from system log from neagent process
class NetworkExtensionLog_mac : public QObject
{
    Q_OBJECT
public:
    explicit NetworkExtensionLog_mac(QObject *parent = nullptr);
    QMap<time_t, QString> collectLogs(const QDateTime &start);

private:
};
