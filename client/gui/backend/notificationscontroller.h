#pragma once

#include <QObject>
#include <QSet>
#include <QVector>
#include "api_responses/notification.h"

class NotificationsController : public QObject
{
    Q_OBJECT
public:
    explicit NotificationsController(QObject *parent = nullptr);
    virtual ~NotificationsController() {}
    void shutdown();

    int totalMessages() const;
    int unreadMessages() const;

    QVector<api_responses::Notification> messages() const;
    const QSet<qint64> &shownIds() const;

public slots:
    void updateNotifications(const QVector<api_responses::Notification> &arr);
    void setNotificationRead(qint64 notificationId);

signals:
    void stateChanged(int totalMessages, int unread);
    void newPopupMessage(int messageId);

private:
    QVector<api_responses::Notification> notifications_;
    QSet<qint64> idOfShownNotifications_;
    QVector<int> unreadPopupNotificationIds_;

    int latestTotal_;
    int latestUnreadCnt_;

    // for serialization
    static constexpr quint32 magic_ = 0x7745C2AE;
    static constexpr int versionForSerialization_ = 1;  // should increment the version if the data format is changed

    void saveToSettings();
    void readFromSettings();
    void updateState();
    void checkForUnreadPopup();
};
