#include "nodesspeedstore.h"
#include <QDataStream>
#include <QDebug>
#include <QSettings>

NodesSpeedStore::NodesSpeedStore(QObject *parent) : QObject(parent),
    curIteration_(0)
{
    loadFromSettings();
}

NodesSpeedStore::~NodesSpeedStore()
{
    saveToSettings();
}

void NodesSpeedStore::updateNodes(const QStringList &nodes)
{
     QMutexLocker locker(&mutex_);
    // find and remove unused nodes
    for (auto it = hash_.begin(); it != hash_.end(); ++it)
    {
        it.value().isUsed_ = false;
    }
    Q_FOREACH(const QString &node, nodes)
    {
        auto it = hash_.find(node);
        if (it != hash_.end())
        {
            it.value().isUsed_ = true;
        }
    }

    auto it = hash_.begin();
    while (it != hash_.end())
    {
        if (!it.value().isUsed_)
        {
            it = hash_.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void NodesSpeedStore::setNodeSpeed(const QString &nodeIp, PingTime timeMs, quint32 iteration)
{
    QMutexLocker locker(&mutex_);
    PingData pd;
    pd.isUsed_ = false;
    pd.timeMs_ = timeMs;
    pd.iteration_ = iteration;
    hash_[nodeIp] = pd;

    if (iteration != curIteration_)
    {
        quint32 newIteration;
        if (isAllPingsHaveSameIteration(newIteration))
        {
            curIteration_ = newIteration;
            emit pingIterationChanged();
        }
    }
}

PingTime NodesSpeedStore::getNodeSpeed(const QString &nodeIp)
{
    QMutexLocker locker(&mutex_);
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

QSet<QString> NodesSpeedStore::getNodesWithoutPingData(const QStringList &ips)
{
    QMutexLocker locker(&mutex_);
    QSet<QString> nodesWithoutPing;
    Q_FOREACH(const QString &ip, ips)
    {
        auto it = hash_.find(ip);
        if (it != hash_.end())
        {
            if (it.value().timeMs_ == PingTime::NO_PING_INFO)
            {
                nodesWithoutPing.insert(ip);
            }
        }
        else
        {
            nodesWithoutPing.insert(ip);
        }
    }
    return nodesWithoutPing;
}

void NodesSpeedStore::saveToSettings()
{
    QMutexLocker locker(&mutex_);

    QHash<QString, int> hashForSave;
    for (auto it = hash_.begin(); it != hash_.end(); ++it)
    {
        hashForSave[it.key()] = it.value().timeMs_.toInt();
    }

    QByteArray buf;
    {
        QDataStream stream(&buf, QIODevice::WriteOnly);
        stream << hashForSave;
    }
    QSettings settings;
    settings.setValue("pingStorage", buf);
}

void NodesSpeedStore::loadFromSettings()
{
    QMutexLocker locker(&mutex_);

    QHash<QString, int> hashForLoad;
    QSettings settings;
    if (settings.contains("pingStorage"))
    {
        QByteArray buf = settings.value("pingStorage").toByteArray();
        QDataStream stream(&buf, QIODevice::ReadOnly);
        stream >> hashForLoad;
    }
    curIteration_ = 0;
    hash_.clear();
    for (auto it = hashForLoad.begin(); it != hashForLoad.end(); ++it)
    {
        PingData pd;
        pd.isUsed_ = false;
        pd.timeMs_ = it.value();
        pd.iteration_ = 0;
        hash_[it.key()] = pd;
    }
}

bool NodesSpeedStore::isAllPingsHaveSameIteration(quint32 &outIteration)
{
    if (hash_.isEmpty())
    {
        return false;
    }
    quint32 iteration = hash_.begin().value().iteration_;
    for (auto it = hash_.begin() + 1; it != hash_.end(); ++it)
    {
        if (it.value().iteration_ != iteration)
        {
            return false;
        }
    }

    outIteration = iteration;
    return true;
}
