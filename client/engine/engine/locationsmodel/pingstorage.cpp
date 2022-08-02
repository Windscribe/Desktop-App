#include "pingstorage.h"
#include <QDataStream>
#include <QIODevice>
#include <QDebug>
#include <QSettings>
#include "utils/protobuf_includes.h"

namespace  {
    struct PingDataSerializable
    {
        QString ip;
        PingTime pingTime;
        quint32 iteration;
        bool isFromDisconnectedState;

        friend QDataStream& operator <<(QDataStream& stream, const PingDataSerializable& p)
        {
            stream << versionForSerialization_;
            stream << p.ip << p.pingTime << p.iteration << p.isFromDisconnectedState;
            return stream;
        }
        friend QDataStream& operator >>(QDataStream& stream, PingDataSerializable& p)
        {
            quint32 version;
            stream >> version;
            Q_ASSERT(version == versionForSerialization_);
            stream >> p.ip >> p.pingTime >> p.iteration >> p.isFromDisconnectedState;
            return stream;
        }

    private:
        static constexpr quint32 versionForSerialization_ = 1;
    };

    struct PingStorageSerializable
    {
        PingStorageSerializable() : isValid_(false) {}

        bool isValid() const { return isValid_; }
        void setData(quint32 curIteration, const QVector<PingDataSerializable> &pings)
        {
            curIteration_ = curIteration;
            pings_ = pings;
            isValid_ = true;
        }
        quint32 getCurIteration() const { return curIteration_; }
        const QVector<PingDataSerializable> & getPings() const { return pings_; }

        friend QDataStream& operator <<(QDataStream& stream, const PingStorageSerializable& p)
        {
            Q_ASSERT(p.isValid_);
            stream << magic_;
            stream << versionForSerialization_;
            stream << p.curIteration_ << p.pings_;
            return stream;
        }
        friend QDataStream& operator >>(QDataStream& stream, PingStorageSerializable& p)
        {
            quint32 magic, version;
            stream >> magic;
            if (magic != magic_)
            {
                p.isValid_ = false;
                return stream;
            }
            stream >> version;
            if (version > versionForSerialization_)
            {
                p.isValid_ = false;
                return stream;
            }

            stream >> p.curIteration_ >> p.pings_;
            p.isValid_ = true;
            return stream;
        }

    private:
        bool isValid_;
        quint32 curIteration_;
        QVector<PingDataSerializable> pings_;

        // for serialization
        static constexpr quint32 magic_ = 0xA4B0C537;
        static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed
    };
}

namespace locationsmodel {

PingStorage::PingStorage(const QString &settingsKeyName) : curIteration_(0),
    settingsKeyName_(settingsKeyName)
{
    loadFromSettings();
}

PingStorage::~PingStorage()
{
    saveToSettings();
}

void PingStorage::updateNodes(const QStringList &ips)
{
    QSet<QString> setIps;
    for (const QString &ip : ips)
    {
        setIps.insert(ip);
    }

    // find and remove unused nodes
    auto it = hash_.begin();
    while (it != hash_.end())
    {
        if (!setIps.contains(it.key()))
        {
            it = hash_.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // and new IPs
    for (const QString &ip : setIps)
    {
        if (!hash_.contains(ip))
        {
            hash_[ip] = PingData();
        }
    }

}

void PingStorage::setNodePing(const QString &nodeIp, PingTime timeMs, bool fromDisconnectedState)
{
    PingData pd;
    pd.timeMs_ = timeMs;
    pd.iteration_ = curIteration_;
    pd.fromDisconnectedState_ = fromDisconnectedState;
    hash_[nodeIp] = pd;
}

PingTime PingStorage::getNodeSpeed(const QString &nodeIp) const
{
    auto it = hash_.find(nodeIp);
    if (it != hash_.end())
    {
        return it.value().timeMs_;
    }
    else
    {
        return PingTime::NO_PING_INFO;
    }
}

quint32 PingStorage::getCurrentIteration() const
{
    return curIteration_;
}

void PingStorage::incIteration()
{
    curIteration_++;
}

void PingStorage::getState(bool &isAllNodesHaveCurIteration, bool &isAllNodesInDisconnectedState)
{
    isAllNodesHaveCurIteration = true;
    isAllNodesInDisconnectedState = true;

    for (auto it = hash_.begin(); it != hash_.end(); ++it)
    {
        if (it.value().iteration_ != curIteration_)
        {
            isAllNodesHaveCurIteration = false;
        }
        if (!it.value().fromDisconnectedState_)
        {
            isAllNodesInDisconnectedState = false;
        }
    }
}

void PingStorage::saveToSettings()
{
    PingStorageSerializable storage;
    QVector<PingDataSerializable> pings;
    for (auto it = hash_.begin(); it != hash_.end(); ++it)
    {
        PingDataSerializable pingData;
        pingData.ip = it.key();
        pingData.pingTime = it.value().timeMs_;
        pingData.iteration = it.value().iteration_;
        pingData.isFromDisconnectedState = it.value().fromDisconnectedState_;
        pings << pingData;
    }
    storage.setData(curIteration_, pings);

    QByteArray arr;
    {
        QDataStream ds(&arr, QIODevice::WriteOnly);
        ds << storage;
    }

    QSettings settings;
    settings.setValue(settingsKeyName_, arr);
}

void PingStorage::loadFromSettings()
{
    QSettings settings;
    if (settings.contains(settingsKeyName_))
    {
        QByteArray arr = settings.value(settingsKeyName_).toByteArray();
        QDataStream ds(&arr, QIODevice::ReadOnly);
        PingStorageSerializable storage;
        ds >> storage;
        if (storage.isValid())
        {
            hash_.clear();
            curIteration_ = storage.getCurIteration();
            const QVector<PingDataSerializable> &pings = storage.getPings();
            for (int i = 0; i < pings.size(); ++i)
            {
                PingData pd;
                pd.timeMs_ = pings[i].pingTime;
                pd.iteration_ = pings[i].iteration;
                pd.fromDisconnectedState_ = pings[i].isFromDisconnectedState;
                hash_[pings[i].ip] = pd;
            }
        }
    }
}

} //namespace locationsmodel
