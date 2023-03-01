#pragma once

#include <QHash>

#include "types/pingtime.h"

namespace locationsmodel {

class PingData;

class PingStorage
{
public:
    explicit PingStorage(const QString& settingsKey);
    virtual ~PingStorage();

    quint32 getCurrentIteration() const;
    void incIteration();

protected:
    const QString& settingsKey() const;
    void setCurrentIteration(quint32 iteration);

private:
    const QString settingsKey_;
    quint32 curIteration_ = 0;
};

class ApiPingStorage : public PingStorage
{
public:
    explicit ApiPingStorage();
    virtual ~ApiPingStorage();

    void setPing(int id, PingTime timeMs);
    PingTime getPing(int id) const;

    void getState(bool &isAllNodesHaveCurIteration);

private:
    // Maps the immutable location (data center) identifier, or static IP identifier, to its ping data.
    QHash<int, PingData> pingDataDB_;

    static constexpr quint32 magic_ = 0x734AB2AE;
    static constexpr int versionForSerialization_ = 2;  // should increment the version if the data format is changed

    void saveToSettings();
    void loadFromSettings();
};

class CustomConfigPingStorage : public PingStorage
{
public:
    explicit CustomConfigPingStorage();
    virtual ~CustomConfigPingStorage();

    void setPing(const QString &ip, PingTime timeMs);
    PingTime getPing(const QString &ip) const;

private:
    // Maps the IP/hostname for a custom config to its ping data.
    QHash<QString, PingData> pingDataDB_;

    static constexpr quint32 magic_ = 0x734AB2AE;
    static constexpr int versionForSerialization_ = 2;  // should increment the version if the data format is changed

    void saveToSettings();
    void loadFromSettings();
};

} //namespace locationsmodel
