#include "notificationscontroller.h"

#include <QDataStream>
#include <QSettings>
#include <algorithm>
// #include <QDebug>

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

const QSet<qint64> &NotificationsController::shownIds() const
{
    return idOfShownNotifications_;
}

void NotificationsController::checkForUnreadPopup()
{
    // opens news feed programmatically to specified id
    if (!unreadPopupNotificationIds_.empty())
    {
        int maxId = *std::max_element(unreadPopupNotificationIds_.begin(),
                                      unreadPopupNotificationIds_.end());
        emit newPopupMessage(maxId);
    }
}

void NotificationsController::updateNotifications(const ProtoTypes::ArrayApiNotification &arr)
{
    notifications_ = arr;
    updateState();
    checkForUnreadPopup();
}

void NotificationsController::setNotificationReaded(qint64 notificationId)
{
    idOfShownNotifications_.insert(notificationId);
    updateState();
}

void NotificationsController::updateState()
{
    int unreaded = 0;
    unreadPopupNotificationIds_.clear();

    for(int i = 0; i < notifications_.api_notifications_size(); ++i)
    {
        if (idOfShownNotifications_.find(notifications_.api_notifications(i).id()) == idOfShownNotifications_.end())
        {
            if (notifications_.api_notifications(i).popup() == 1)
            {
                unreadPopupNotificationIds_.push_back(notifications_.api_notifications(i).id());
            }
            unreaded++;
        }
    }

    // updates connect window logo
    if (latestTotal_ != notifications_.api_notifications_size() || latestUnreadCnt_ != unreaded)
    {
        latestTotal_ = notifications_.api_notifications_size();
        latestUnreadCnt_ = unreaded;
        emit stateChanged(latestTotal_, latestUnreadCnt_);
    }
}

void NotificationsController::saveToSettings()
{
    QSettings settings;

    size_t size = notifications_.ByteSizeLong();
    QByteArray arr(size, Qt::Uninitialized);
    notifications_.SerializeToArray(arr.data(), size);

    settings.setValue("notifications", arr);

    QByteArray arrShownPopups;
    {
        QDataStream stream(&arrShownPopups, QIODeviceBase::WriteOnly);
        stream << idOfShownNotifications_;
    }
    settings.setValue("idForShownPopups", arrShownPopups);
    settings.sync();
}

void NotificationsController::readFromSettings()
{
    QSettings settings;

    if (settings.contains("notifications"))
    {
        QByteArray arr = settings.value("notifications").toByteArray();
        notifications_.ParseFromArray(arr.data(), arr.size());
    }

    if (settings.contains("idForShownPopups"))
    {
        QByteArray arr = settings.value("idForShownPopups").toByteArray();
        QDataStream stream(&arr, QIODeviceBase::ReadOnly);
        stream >> idOfShownNotifications_;
    }
    updateState();
    checkForUnreadPopup();
}
