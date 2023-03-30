#include "notificationscontroller.h"

#include <QDataStream>
#include <QSettings>
#include <algorithm>
#include <QIODevice>

#include "utils/simplecrypt.h"
#include "types/global_consts.h"

extern "C" {
    #include "legacy_protobuf_support/types.pb-c.h"
}

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

QVector<types::Notification> NotificationsController::messages() const
{
    if (notifications_.size() > 0)
    {
        return notifications_;
    }
    else
    {
        // generate message for empty list
        QVector<types::Notification> arr;

        types::Notification notification;
        notification.id = 0;
        notification.title = QT_TR_NOOP("WELCOME TO WINDSCRIBE");
        notification.message = QT_TR_NOOP("<p>You will find announcements and general Windscribe related news here. "
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

void NotificationsController::updateNotifications(const QVector<types::Notification> &arr)
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
    bool bLoaded = false;

    if (settings.contains("notifications"))
    {
        QString str = settings.value("notifications", "").toString();
        QByteArray arr = simpleCrypt.decryptToByteArray(str);

        QDataStream ds(&arr, QIODevice::ReadOnly);

        quint32 magic, version;
        ds >> magic;
        if (magic == magic_)
        {
            ds >> version;
            if (version <= versionForSerialization_)
            {
                ds >> notifications_ >> idOfShownNotifications_;
                if (ds.status() == QDataStream::Ok)
                {
                    bLoaded = true;
                }
            }
        }

        if (!bLoaded)
        {
            // try load from legacy protobuf
            // todo remove this code at some point later
            QByteArray arr = settings.value("notifications").toByteArray();
            ProtoTypes__ArrayApiNotification *a = proto_types__array_api_notification__unpack(NULL, arr.size(), (const uint8_t *)arr.data());
            if (a)
            {
                for (size_t i = 0; i < a->n_api_notifications; ++i)
                {
                    ProtoTypes__ApiNotification *n = a->api_notifications[i];
                    types::Notification notification;
                    if (n->has_id) notification.id = n->id;
                    notification.title = n->title;
                    notification.message = n->message;
                    if (n->has_date) notification.date = n->date;
                    if (n->has_perm_free) notification.permFree = n->perm_free;
                    if (n->has_perm_pro) notification.permPro = n->perm_pro;
                    if (n->has_popup) notification.popup = n->popup;

                    notifications_ << notification;
                }
                proto_types__array_api_notification__free_unpacked(a, NULL);
            }

            if (settings.contains("idForShownPopups"))
            {
                QByteArray arr = settings.value("idForShownPopups").toByteArray();
                QDataStream stream(&arr, QIODevice::ReadOnly);
                stream >> idOfShownNotifications_;
                settings.remove("idForShownPopups");
            }
        }
    }

    updateState();
    checkForUnreadPopup();
}
