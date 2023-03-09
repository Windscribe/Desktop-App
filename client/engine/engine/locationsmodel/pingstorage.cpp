#include "pingstorage.h"

#include <QDataStream>
#include <QIODevice>
#include <QSettings>

#include "utils/simplecrypt.h"
#include "types/global_consts.h"

namespace locationsmodel {

class PingData
{
public:
    explicit PingData() : timeMs_(PingTime::NO_PING_INFO), iteration_(0) {};
    explicit PingData(PingTime timeMs, quint32 iteration) : timeMs_(timeMs), iteration_(iteration)
    {
    }

    PingTime pingTime() const { return timeMs_; }
    quint32 iteration() const { return iteration_; }

private:
    PingTime timeMs_;
    quint32 iteration_;
};

PingStorage::PingStorage(const QString &settingsKey) : settingsKey_(settingsKey)
{
}

PingStorage::~PingStorage()
{
}

const QString& PingStorage::settingsKey() const
{
    return settingsKey_;
}

quint32 PingStorage::getCurrentIteration() const
{
    return curIteration_;
}

void PingStorage::incIteration()
{
    curIteration_++;
}

void PingStorage::setCurrentIteration(quint32 iteration)
{
    curIteration_ = iteration;
}


ApiPingStorage::ApiPingStorage() : PingStorage("pingStorage")
{
    loadFromSettings();
}

ApiPingStorage::~ApiPingStorage()
{
    saveToSettings();
}

void ApiPingStorage::setPing(int id, PingTime timeMs)
{
    pingDataDB_[id] = PingData(timeMs, getCurrentIteration());
}

PingTime ApiPingStorage::getPing(int id) const
{
    auto it = pingDataDB_.constFind(id);
    if (it != pingDataDB_.constEnd()) {
        return it.value().pingTime();
    }

    return PingTime::NO_PING_INFO;
}

void ApiPingStorage::getState(bool &isAllNodesHaveCurIteration)
{
    isAllNodesHaveCurIteration = true;

    for (auto it = pingDataDB_.begin(); it != pingDataDB_.end(); ++it) {
        if (it.value().iteration() != getCurrentIteration()) {
            isAllNodesHaveCurIteration = false;
            break;
        }
    }
}

void ApiPingStorage::saveToSettings()
{
    QByteArray arr;
    {
        QDataStream ds(&arr, QIODevice::WriteOnly);
        ds << magic_;
        ds << versionForSerialization_;
        ds << getCurrentIteration();
        ds << pingDataDB_.size();
        for (auto it = pingDataDB_.begin(); it != pingDataDB_.end(); ++it) {
            ds << it.key() << it.value().pingTime().toInt() << it.value().iteration();
        }
    }

    QSettings settings;
    SimpleCrypt simpleCrypt(SIMPLE_CRYPT_KEY);
    settings.setValue(settingsKey(), simpleCrypt.encryptToString(arr));
}

void ApiPingStorage::loadFromSettings()
{
    pingDataDB_.clear();

    QSettings settings;
    if (!settings.contains(settingsKey())) {
        return;
    }

    SimpleCrypt simpleCrypt(SIMPLE_CRYPT_KEY);
    QString str = settings.value(settingsKey()).toString();
    QByteArray arr = simpleCrypt.decryptToByteArray(str);

    QDataStream ds(&arr, QIODevice::ReadOnly);
    quint32 magic;
    ds >> magic;

    if (magic == magic_) {
        quint32 version;
        ds >> version;
        if (version <= 1) {
            // We can't use the old stored data that mapped pingIP to ping data.
            return;
        }
        if (version <= versionForSerialization_) {
            quint32 iteration;
            ds >> iteration;
            qsizetype numDBElements;
            ds >> numDBElements;

            for (qsizetype i = 0; i < numDBElements; ++i) {
                int locationID;
                int timeMs;
                quint32 iteration;
                ds >> locationID >> timeMs >> iteration;
                pingDataDB_[locationID] = PingData(timeMs, iteration);
            }

            if (ds.status() != QDataStream::Ok) {
                pingDataDB_.clear();
                iteration = 0;
            }

            setCurrentIteration(iteration);
        }
    }
}


CustomConfigPingStorage::CustomConfigPingStorage() : PingStorage("pingStorageCustomConfigs")
{
    loadFromSettings();
}

CustomConfigPingStorage::~CustomConfigPingStorage()
{
    saveToSettings();
}

void CustomConfigPingStorage::setPing(const QString &ip, PingTime timeMs)
{
    pingDataDB_[ip] = PingData(timeMs, getCurrentIteration());
}

PingTime CustomConfigPingStorage::getPing(const QString &ip) const
{
    const auto it = pingDataDB_.constFind(ip);
    if (it != pingDataDB_.constEnd()) {
        return it.value().pingTime();
    }

    return PingTime::NO_PING_INFO;
}

void CustomConfigPingStorage::saveToSettings()
{
    QByteArray arr;
    {
        QDataStream ds(&arr, QIODevice::WriteOnly);
        ds << magic_;
        ds << versionForSerialization_;
        ds << getCurrentIteration();
        ds << pingDataDB_.size();
        for (auto it = pingDataDB_.begin(); it != pingDataDB_.end(); ++it) {
            ds << it.key() << it.value().pingTime().toInt() << it.value().iteration();
        }
    }

    QSettings settings;
    SimpleCrypt simpleCrypt(SIMPLE_CRYPT_KEY);
    settings.setValue(settingsKey(), simpleCrypt.encryptToString(arr));
}

void CustomConfigPingStorage::loadFromSettings()
{
    pingDataDB_.clear();

    QSettings settings;
    if (!settings.contains(settingsKey())) {
        return;
    }

    SimpleCrypt simpleCrypt(SIMPLE_CRYPT_KEY);
    QString str = settings.value(settingsKey()).toString();
    QByteArray arr = simpleCrypt.decryptToByteArray(str);

    QDataStream ds(&arr, QIODevice::ReadOnly);
    quint32 magic;
    ds >> magic;

    if (magic == magic_) {
        quint32 version;
        ds >> version;
        if (version <= versionForSerialization_) {
            quint32 iteration;
            ds >> iteration;
            qsizetype numDBElements;
            ds >> numDBElements;

            for (qsizetype i = 0; i < numDBElements; ++i) {
                QString ip;
                int timeMs;
                quint32 iteration;
                ds >> ip >> timeMs >> iteration;

                if (version <= 1) {
                    // Skip over the old 'pinged while disconnected' flag we no longer use.
                    bool fromDisconnectedState;
                    ds >> fromDisconnectedState;
                }

                pingDataDB_[ip] = PingData(timeMs, iteration);
            }

            if (ds.status() != QDataStream::Ok) {
                pingDataDB_.clear();
                iteration = 0;
            }

            setCurrentIteration(iteration);
        }
    }
}

} //namespace locationsmodel
