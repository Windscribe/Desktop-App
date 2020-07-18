#ifndef CONNECTEDUSERSCOUNTER_H
#define CONNECTEDUSERSCOUNTER_H

#include <QObject>
#include <QMap>

class ConnectedUsersCounter : public QObject
{
    Q_OBJECT
public:
    explicit ConnectedUsersCounter(QObject *parent);
    void newUserConnected(const QString &hostname);
    void userDiconnected(const QString &hostname);
    void reset();

    int getConnectedUsersCount();

signals:
    void usersCountChanged();

private:
    enum { MAX_NOT_ACTIVITY_TIME = 10000 };
    QMap<QString, int> connections_;
    int lastCnt_;

    void checkUsersCount();
};

#endif // CONNECTEDUSERSCOUNTER_H
