#include "pingstorage.h"
#include <QDataStream>
#include <QIODevice>
#include <QSettings>
#include <QCborMap>
#include <QCborArray>

namespace  {
    enum CBOR_FIELDS {
        CBOR_VERSION = 0,
        CBOR_CUR_ITERATION = 1,
        CBOR_PINGS = 2,
        CBOR_IP = 3,
        CBOR_PING_TIME = 4,
        CBOR_ITERATION = 5,
        CBOR_FROM_DISCONNECTED_STATE = 6
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
    QCborMap map;
    map[CBOR_VERSION] = versionForSerialization_;
    map[CBOR_CUR_ITERATION] = curIteration_;

    QCborArray cborArr;
    for (auto it = hash_.begin(); it != hash_.end(); ++it)
    {
        QCborMap item;
        item[CBOR_IP] = it.key();
        item[CBOR_PING_TIME] = it.value().timeMs_.toInt();
        item[CBOR_ITERATION] = it.value().iteration_;
        item[CBOR_FROM_DISCONNECTED_STATE] = it.value().fromDisconnectedState_;
        cborArr << item;
    }
    map[CBOR_PINGS] = cborArr;

    QByteArray arr = map.toCborValue().toCbor();
    QSettings settings;
    settings.setValue(settingsKeyName_, arr);
}

void PingStorage::loadFromSettings()
{
    QSettings settings;
    if (settings.contains(settingsKeyName_))
    {
        QByteArray arr = settings.value(settingsKeyName_).toByteArray();
        QCborValue val = QCborValue::fromCbor(arr);
        if (!val.isInvalid() && val.isMap())
        {
            QCborMap map = val.toMap();
            if (map.contains(CBOR_VERSION) && map[CBOR_VERSION].toInteger(INT_MAX) <= versionForSerialization_)
            {
                curIteration_ = map[CBOR_CUR_ITERATION].toInteger();
                QCborArray cborArr = map[CBOR_PINGS].toArray();
                for (const auto &it : cborArr)
                {
                    PingData pd;
                    pd.timeMs_ = it[CBOR_PING_TIME].toInteger(PingTime::NO_PING_INFO);
                    pd.iteration_ = it[CBOR_ITERATION].toInteger();
                    pd.fromDisconnectedState_ = it[CBOR_FROM_DISCONNECTED_STATE].toBool();
                    hash_[it[CBOR_IP].toString()] = pd;
                }
            }
        }
    }
}

} //namespace locationsmodel
