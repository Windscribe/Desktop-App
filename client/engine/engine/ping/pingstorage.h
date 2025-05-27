#pragma once

#include <QHash>

#include "types/pingtime.h"

// IP ping storage that saves state between program launches
class PingStorage
{
public:
    explicit PingStorage(const QString& settingsKey);
    virtual ~PingStorage();

    qint64 currentIterationTime() const { return curIterationTime_; }
    QString currentIterationNetworkOrSsid() const { return curIterationNetworkOrSsid_; }

    void setCurrentIterationData(qint64 msecsSinceEpoch, const QString &networkOrSsid);

    void setPing(const QString &ip, PingTime timeMs);
    PingTime getPing(const QString &ip) const;
    void getPingData(const QString &ip, PingTime &outPingTime, qint64 &outIterationTime) const;
    void initPingDataIfNotExists(const QString &ip);

    void removeUnusedNodes(const QSet<QString> &ips);
    bool isAllNodesHaveCurIteration() const;
    void clearAllPingData();

private:
    struct PingData
    {
        PingTime timeMs_;
        qint64 iterationTime_ = 0;
    };

    const QString settingsKey_;
    qint64 curIterationTime_ = 0;    // last iteration date and time in UTC time in ms
    QString curIterationNetworkOrSsid_;     // the name of the network to which the pings were made

    // Maps the ip to its ping data.
    QHash<QString, PingData> pingDataDB_;

    static constexpr quint32 magic_ = 0x734AB2AE;
    static constexpr int versionForSerialization_ = 3;  // should increment the version if the data format is changed

    void saveToSettings();
    void loadFromSettings();
};
