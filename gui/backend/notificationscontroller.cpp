#include "notificationscontroller.h"

#include <QDataStream>
#include <QSettings>

NotificationsController::NotificationsController(QObject *parent) : QObject(parent),
    latestTotal_(0), latestUnreadCnt_(0)
{
    readFromSettings();
}

void NotificationsController::shutdown()
{
    saveToSettings();
}

int NotificationsController::totalMessages() const
{
    return notifications_.api_notifications_size();
}

int NotificationsController::unreadMessages() const
{
    return latestUnreadCnt_;
}

ProtoTypes::ArrayApiNotification NotificationsController::messages() const
{
    if (notifications_.api_notifications_size() > 0)
    {
        return notifications_;
    }
    else
    {
        // generate message for empty list
        ProtoTypes::ArrayApiNotification arr;

        ProtoTypes::ApiNotification* notification = arr.add_api_notifications();

        notification->set_id(0);
        notification->set_title(QT_TR_NOOP("WELCOME TO WINDSCRIBE"));
        notification->set_message(QT_TR_NOOP("You will find announcements and general Windscribe related news here. "
                                             "Perhaps even delicious cake, Everyone loves cake!"));

        return arr;
    }
}

void NotificationsController::updateNotifications(const ProtoTypes::ArrayApiNotification &arr)
{
    notifications_ = arr;
    updateState();
}

void NotificationsController::setNotificationReaded(qint64 notificationId)
{
    idOfShownNotifications_.insert(notificationId);
    updateState();
}

void NotificationsController::updateState()
{
    int unreaded = 0;
    for(int i = 0; i < notifications_.api_notifications_size(); ++i)
    {
        if (idOfShownNotifications_.find(notifications_.api_notifications(i).id()) == idOfShownNotifications_.end())
        {
            unreaded++;
        }
    }
    if (latestTotal_ != notifications_.api_notifications_size() || latestUnreadCnt_ != unreaded)
    {
        latestTotal_ = notifications_.api_notifications_size();
        latestUnreadCnt_ = unreaded;
        emit stateChanged(latestTotal_, latestUnreadCnt_);
    }
}

void NotificationsController::saveToSettings()
{
    QSettings settings("Windscribe", "Windscribe");

    size_t size = notifications_.ByteSizeLong();
    QByteArray arr(size, Qt::Uninitialized);
    notifications_.SerializeToArray(arr.data(), size);

    settings.setValue("notifications_v3", arr);

    QByteArray arrShownPopups;
    {
        QDataStream stream(&arrShownPopups, QIODevice::WriteOnly);
        stream << idOfShownNotifications_;
    }
    settings.setValue("idForShownPopups", arrShownPopups);
    settings.sync();
}

void NotificationsController::readFromSettings()
{
    QSettings settings("Windscribe", "Windscribe");

    // remove setting from old versions
    if (settings.contains("notifications_v2"))
    {
        settings.remove("notifications_v2");
    }

    if (settings.contains("notifications_v3"))
    {
        QByteArray arr = settings.value("notifications_v3").toByteArray();
        notifications_.ParseFromArray(arr.data(), arr.size());
    }

    if (settings.contains("idForShownPopups"))
    {
        QByteArray arr = settings.value("idForShownPopups").toByteArray();
        QDataStream stream(&arr, QIODevice::ReadOnly);
        stream >> idOfShownNotifications_;
    }
    updateState();
}
