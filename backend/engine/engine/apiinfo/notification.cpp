#include "notification.h"
#include <QJsonArray>

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




} // namespace ApiInfo
