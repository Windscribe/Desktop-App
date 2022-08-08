#include "pingstorage.h"

#include <QDataStream>
#include <QIODevice>
#include <QSettings>

#include "utils/simplecrypt.h"
#include "types/global_consts.h"

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
    QByteArray arr;
    {
        QDataStream ds(&arr, QIODevice::WriteOnly);
        ds << magic_;
        ds << versionForSerialization_;
        ds << curIteration_;
        ds << hash_.size();
        for (auto it = hash_.begin(); it != hash_.end(); ++it)
        {
            ds << it.key() << it.value().timeMs_.toInt() << it.value().iteration_ << it.value().fromDisconnectedState_;
        }
    }
    QSettings settings;
    SimpleCrypt simpleCrypt(SIMPLE_CRYPT_KEY);
    settings.setValue(settingsKeyName_, simpleCrypt.encryptToString(arr));
}

void PingStorage::loadFromSettings()
{
    QSettings settings;
    if (settings.contains(settingsKeyName_))
    {
        SimpleCrypt simpleCrypt(SIMPLE_CRYPT_KEY);
        QString str = settings.value(settingsKeyName_).toString();
        QByteArray arr = simpleCrypt.decryptToByteArray(str);

        QDataStream ds(&arr, QIODevice::ReadOnly);
        quint32 magic, version;
        ds >> magic;
        if (magic == magic_)
        {
            ds >> version;
            if (version <= versionForSerialization_)
            {
                ds >> curIteration_;
                qsizetype hashSize;
                ds >> hashSize;

                for (qsizetype i = 0; i < hashSize; ++i)
                {
                    QString key;
                    int timeMs;
                    quint32 iteration;
                    bool fromDisconnectedState;

                    ds >> key >> timeMs >> iteration >> fromDisconnectedState;
                    PingData pd;
                    pd.timeMs_ = timeMs;
                    pd.iteration_ = iteration;
                    pd.fromDisconnectedState_ = fromDisconnectedState;

                    hash_[key] = pd;
                }

                if (ds.status() != QDataStream::Ok)
                {
                    hash_.clear();
                }
            }
        }
    }
}

} //namespace locationsmodel
