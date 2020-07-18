#ifndef INETWORKSTATEMANAGER_H
#define INETWORKSTATEMANAGER_H

#include <QObject>

class INetworkStateManager : public QObject
{
    Q_OBJECT
public:
    explicit INetworkStateManager(QObject *parent): QObject(parent) {}
    virtual ~INetworkStateManager() {}
    virtual bool isOnline() = 0;

signals:
    void stateChanged(bool isAlive, const QString &networkInterface);

};

#endif // INETWORKSTATEMANAGER_H
