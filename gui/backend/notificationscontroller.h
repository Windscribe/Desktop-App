#ifndef NOTIFICATIONSCONTROLLER_H
#define NOTIFICATIONSCONTROLLER_H

#include <QObject>
#include <QSet>

#include "ipc/generated_proto/types.pb.h"

class NotificationsController : public QObject
{
    Q_OBJECT
public:
    explicit NotificationsController(QObject *parent = nullptr);
    virtual ~NotificationsController() {}
    void shutdown();

    int totalMessages() const;
    int unreadMessages() const;

    ProtoTypes::ArrayApiNotification messages() const;

public slots:
    void updateNotifications(const ProtoTypes::ArrayApiNotification &arr);
    void setNotificationReaded(qint64 notificationId);

signals:
    void stateChanged(int totalMessages, int unread);

private:
    ProtoTypes::ArrayApiNotification notifications_;
    QSet<qint64> idOfShownNotifications_;

    int latestTotal_;
    int latestUnreadCnt_;

    void saveToSettings();
    void readFromSettings();
    void updateState();
};

#endif // NOTIFICATIONSCONTROLLER_H
