#include "pingstorage.h"
#include <QDataStream>
#include <QDebug>
#include <QSettings>
#include "utils/protobuf_includes.h"

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
    ProtoApiInfo::PingStorage storage;

    storage.set_cur_iteration(curIteration_);
    for (auto it = hash_.begin(); it != hash_.end(); ++it)
    {
        ProtoApiInfo::PingData *pingData = storage.add_pings();
        pingData->set_ip(it.key().toStdString());
        pingData->set_pingtime(it.value().timeMs_.toInt());
        pingData->set_iteration(it.value().iteration_);
        pingData->set_from_disconnected_state(it.value().fromDisconnectedState_);
    }
    size_t size = storage.ByteSizeLong();
    QByteArray arr(size, Qt::Uninitialized);
    storage.SerializeToArray(arr.data(), size);

    QSettings settings;
    settings.setValue(settingsKeyName_, arr);
}

void PingStorage::loadFromSettings()
{
    QSettings settings;
    if (settings.contains(settingsKeyName_))
    {
        QByteArray arr = settings.value(settingsKeyName_).toByteArray();
        ProtoApiInfo::PingStorage storage;
        if (storage.ParseFromArray(arr.data(), arr.size()))
        {
            hash_.clear();
            curIteration_ = storage.cur_iteration();
            for (int i = 0; i < storage.pings_size(); ++i)
            {
                PingData pd;
                pd.timeMs_ = storage.pings(i).pingtime();
                pd.iteration_ = storage.pings(i).iteration();
                pd.fromDisconnectedState_ = storage.pings(i).from_disconnected_state();
                hash_[QString::fromStdString(storage.pings(i).ip())] = pd;
            }
        }
    }
}

} //namespace locationsmodel
