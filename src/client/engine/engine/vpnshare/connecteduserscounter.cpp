#include "connecteduserscounter.h"
#include "utils/ws_assert.h"

ConnectedUsersCounter::ConnectedUsersCounter(QObject *parent) : QObject(parent)
{
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

void ConnectedUsersCounter::userDisconnected(const QString &hostname)
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
        WS_ASSERT(false);
    }
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
