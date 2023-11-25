#pragma once

#include <QObject>
#include <QMap>
#include <QDateTime>

// collect logs from system log from neagent process
class NetworkExtensionLog_mac : public QObject
{
    Q_OBJECT
public:
    explicit NetworkExtensionLog_mac(QObject *parent = nullptr);
    QMap<time_t, QString> collectNext();

private:
    bool isInitialTimestampInitialized_;
    quint64 initialTimestamp_;

    QMap<quint64, QString> logs_;
    QDateTime lastTimeCollected_;
};
