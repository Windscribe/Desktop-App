#include "notification.h"
#include <QJsonArray>
#include <QMetaType>

const int typeIdNotification = qRegisterMetaType<ApiInfo::Notification>("ApiInfo::Notification");
const int typeIdNotificationArray = qRegisterMetaType<QVector<ApiInfo::Notification> >("QVector<ApiInfo::Notification>");

namespace ApiInfo {

bool Notification::initFromJson(const QJsonObject &json)
{
    // check for required fields in json
    QStringList required_fields({"id", "title", "message", "date", "perm_free", "perm_pro", "popup"});
    for (const QString &str : required_fields)
    {
        if (!json.contains(str))
        {
            return false;
        }
    }

    d->id_ = json["id"].toDouble();
    d->title_ = json["title"].toString();
    d->message_ = json["message"].toString();
    d->date_ = json["date"].toDouble();
    d->perm_free_ = json["perm_free"].toInt();
    d->perm_pro_ = json["perm_pro"].toInt();
    d->popup_ = json["popup"].toInt();
    d->isInitialized_ = true;

    return true;
}

ProtoTypes::ApiNotification Notification::getProtoBuf() const
{
    ProtoTypes::ApiNotification ian;
    ian.set_id(d->id_);
    ian.set_title(d->title_.toStdString());
    ian.set_message(d->message_.toStdString());
    ian.set_date(d->date_);
    ian.set_perm_free(d->perm_free_);
    ian.set_perm_pro(d->perm_pro_);
    ian.set_popup(d->popup_);
    return ian;
}




} // namespace ApiInfo
