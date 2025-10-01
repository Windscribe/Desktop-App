#include "notificationscontroller.h"

#include <QDataStream>
#include <QSettings>
#include <algorithm>
#include <QIODevice>

#include "utils/simplecrypt.h"
#include "types/global_consts.h"

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
    return notifications_.size();
}

int NotificationsController::unreadMessages() const
{
    return latestUnreadCnt_;
}

QVector<api_responses::Notification> NotificationsController::messages() const
{
    if (notifications_.size() > 0)
    {
        return notifications_;
    }
    else
    {
        // generate message for empty list
        QVector<api_responses::Notification> arr;

        api_responses::Notification notification;
        notification.id = 0;
        notification.title = tr("WELCOME TO WINDSCRIBE");
        notification.message = tr("<p>You will find announcements and general Windscribe related news here. "
                                  "Perhaps even delicious cake, everyone loves cake!</p>");
        arr << notification;
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

void NotificationsController::updateNotifications(const QVector<api_responses::Notification> &arr)
{
    notifications_ = arr;
    updateState();
    checkForUnreadPopup();
}

void NotificationsController::setNotificationRead(qint64 notificationId)
{
    idOfShownNotifications_.insert(notificationId);
    updateState();
    saveToSettings();
}

void NotificationsController::updateState()
{
    int unreaded = 0;
    unreadPopupNotificationIds_.clear();

    for(int i = 0; i < notifications_.size(); ++i)
    {
        if (idOfShownNotifications_.find(notifications_[i].id) == idOfShownNotifications_.end())
        {
            if (notifications_[i].popup == 1)
            {
                unreadPopupNotificationIds_.push_back(notifications_[i].id);
            }
            unreaded++;
        }
    }

    // updates connect window logo
    if (latestTotal_ != notifications_.size() || latestUnreadCnt_ != unreaded)
    {
        latestTotal_ = notifications_.size();
        latestUnreadCnt_ = unreaded;
        emit stateChanged(latestTotal_, latestUnreadCnt_);
    }
}

void NotificationsController::saveToSettings()
{
    QByteArray arr;
    {
        QDataStream ds(&arr, QIODevice::WriteOnly);
        ds << magic_;
        ds << versionForSerialization_;
        ds << notifications_ << idOfShownNotifications_;
    }
    QSettings settings;
    SimpleCrypt simpleCrypt(SIMPLE_CRYPT_KEY);
    settings.setValue("notifications", simpleCrypt.encryptToString(arr));
}

void NotificationsController::readFromSettings()
{
    QSettings settings;
    SimpleCrypt simpleCrypt(SIMPLE_CRYPT_KEY);

    if (settings.contains("notifications")) {
        QString str = settings.value("notifications", "").toString();
        QByteArray arr = simpleCrypt.decryptToByteArray(str);

        QDataStream ds(&arr, QIODevice::ReadOnly);

        quint32 magic, version;
        ds >> magic;
        if (magic == magic_) {
            ds >> version;
            if (version <= versionForSerialization_) {
                ds >> notifications_ >> idOfShownNotifications_;
            }
        }
    }

    updateState();
    checkForUnreadPopup();
}
