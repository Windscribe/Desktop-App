#include "connecteduserscounter.h"

ConnectedUsersCounter::ConnectedUsersCounter(QObject *parent) : QObject(parent)
{
    lastCnt_ = 0;
}

void ConnectedUsersCounter::newUserConnected(const QString &hostname)
{
    auto it = connections_.find(hostname);
    if (it != connections_.end())
    {
        it.value()++;
    }
    else
    {
        connections_[hostname] = 1;
    }
    checkUsersCount();
}

void ConnectedUsersCounter::userDiconnected(const QString &hostname)
{
    auto it = connections_.find(hostname);
    if (it != connections_.end())
    {
        it.value()--;
        if (it.value() <= 0)
        {
            connections_.erase(it);
        }
    }
    else
    {
        Q_ASSERT(false);
    }
    checkUsersCount();
}

void ConnectedUsersCounter::reset()
{
    connections_.clear();
    checkUsersCount();
}

int ConnectedUsersCounter::getConnectedUsersCount()
{
    return connections_.count();
}

void ConnectedUsersCounter::checkUsersCount()
{
    if (connections_.count() != lastCnt_)
    {
        lastCnt_ = connections_.count();
        emit usersCountChanged();
    }
}
