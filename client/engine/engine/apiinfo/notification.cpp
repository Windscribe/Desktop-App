#include "notification.h"
#include <QJsonArray>
#include <QMetaType>

const int typeIdNotification = qRegisterMetaType<apiinfo::Notification>("apiinfo::Notification");
const int typeIdNotificationArray = qRegisterMetaType<QVector<apiinfo::Notification> >("QVector<apiinfo::Notification>");

namespace apiinfo {




} // namespace apiinfo
