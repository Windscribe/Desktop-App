#pragma once

#include <QObject>
#include <QHash>

class ConnectedUsersCounter : public QObject
{
    Q_OBJECT
public:
    explicit ConnectedUsersCounter(QObject *parent);
    void newUserConnected(const QString &hostname);
    void userDisconnected(const QString &hostname);

    int getConnectedUsersCount();

signals:
    void usersCountChanged();

private:
    QHash<QString, int> connections_;
    int lastCnt_ = 0;

    void checkUsersCount();
};
