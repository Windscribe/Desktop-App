#include "pingstorage.h"

#include <QDataStream>
#include <QIODevice>
#include <QSettings>

#include "utils/simplecrypt.h"
#include "types/global_consts.h"

PingStorage::PingStorage(const QString &settingsKey) : settingsKey_(settingsKey)
{
    loadFromSettings();
}

PingStorage::~PingStorage()
{
    saveToSettings();
}

void PingStorage::setCurrentIterationData(qint64 msecsSinceEpoch, const QString &networkOrSsid)
{
    curIterationTime_ = msecsSinceEpoch;
    curIterationNetworkOrSsid_ = networkOrSsid;
}

void PingStorage::setPing(const QString &ip, PingTime timeMs)
{
    pingDataDB_[ip] = PingData{timeMs, curIterationTime_};
}

PingTime PingStorage::getPing(const QString &ip) const
{
    auto it = pingDataDB_.constFind(ip);
    if (it != pingDataDB_.constEnd()) {
        return it.value().timeMs_;
    }

    return PingTime::NO_PING_INFO;
}

void PingStorage::getPingData(const QString &ip, PingTime &outPingTime, qint64 &outIterationTime) const
{
    auto it = pingDataDB_.constFind(ip);
    if (it != pingDataDB_.constEnd()) {
        outPingTime = it.value().timeMs_;
        outIterationTime = it.value().iterationTime_;
        return;
    }

    outPingTime = PingTime::NO_PING_INFO;
    outIterationTime = 0;
}

void PingStorage::initPingDataIfNotExists(const QString &ip)
{
    if (!pingDataDB_.contains(ip))
        pingDataDB_[ip] = PingData();
}

void PingStorage::removeUnusedNodes(const QSet<QString> &ips)
{
    for (auto it = pingDataDB_.begin(); it != pingDataDB_.end();) {
        if (!ips.contains(it.key())) {
            it = pingDataDB_.erase(it);
        } else {
            ++it;
        }
    }
}

bool PingStorage::isAllNodesHaveCurIteration() const
{
    for (const auto &it : pingDataDB_.values())
        if (it.iterationTime_ != curIterationTime_)
            return false;

    return true;
}

void PingStorage::saveToSettings()
{
    QByteArray arr;
    {
        QDataStream ds(&arr, QIODevice::WriteOnly);
        ds << magic_;
        ds << versionForSerialization_;
        ds << curIterationTime_;
        ds << curIterationNetworkOrSsid_;
        ds << pingDataDB_.size();
        for (auto it = pingDataDB_.begin(); it != pingDataDB_.end(); ++it) {
            ds << it.key() << it.value().timeMs_.toInt() << it.value().iterationTime_;
        }
    }

    QSettings settings;
    SimpleCrypt simpleCrypt(SIMPLE_CRYPT_KEY);
    settings.setValue(settingsKey_, simpleCrypt.encryptToString(arr));
}

void PingStorage::loadFromSettings()
{
    curIterationNetworkOrSsid_.clear();
    pingDataDB_.clear();

    QSettings settings;
    if (!settings.contains(settingsKey_)) {
        return;
    }

    SimpleCrypt simpleCrypt(SIMPLE_CRYPT_KEY);
    QString str = settings.value(settingsKey_).toString();
    QByteArray arr = simpleCrypt.decryptToByteArray(str);

    QDataStream ds(&arr, QIODevice::ReadOnly);
    quint32 magic;
    ds >> magic;

    if (magic == magic_) {
        quint32 version;
        ds >> version;

        if (version != versionForSerialization_)
            return;

        ds >> curIterationTime_;
        ds >> curIterationNetworkOrSsid_;

        qsizetype numDBElements;
        ds >> numDBElements;
        for (qsizetype i = 0; i < numDBElements; ++i) {
            QString ip;
            int timeMs;
            qint64 iterationTime;
            ds >> ip >> timeMs >> iterationTime;

            PingData pingData;
            pingData.timeMs_ = timeMs;
            pingData.iterationTime_ = iterationTime;

            pingDataDB_[ip] = pingData;
        }

        if (ds.status() != QDataStream::Ok) {
            pingDataDB_.clear();
            curIterationTime_ = 0;
            curIterationNetworkOrSsid_.clear();
        }
    }
}
