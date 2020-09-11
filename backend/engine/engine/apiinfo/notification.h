#ifndef APIINFO_NOTIFICATION_H
#define APIINFO_NOTIFICATION_H

#include <QString>
#include <QJsonObject>
#include <QVector>
#include "node.h"

namespace ApiInfo {

struct ApiNotification
{
    qint64 id;
    QString title;
    QString message;
    qint64 date;
    int perm_free;
    int perm_pro;
    int popup;
};

struct ApiNotifications
{
    QVector<ApiNotification> notifications;
};

} //namespace ApiInfo

#endif // APIINFO_NOTIFICATION_H
