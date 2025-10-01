#include "notification.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QMetaType>

const int typeIdNotification = qRegisterMetaType<api_responses::Notification>("api_responses::Notification");

namespace api_responses {

Notification::Notification(const QJsonObject &json)
{
    // check for required fields in json
    QStringList required_fields({"id", "title", "message", "date", "perm_free", "perm_pro", "popup"});
    for (const QString &str : required_fields)
        if (!json.contains(str))
            return;

    id = json["id"].toDouble();
    title = json["title"].toString();
    message = json["message"].toString();
    date = json["date"].toDouble();
    permFree = json["perm_free"].toInt();
    permPro = json["perm_pro"].toInt();
    popup = json["popup"].toInt();
}

bool Notification::operator==(const Notification &other) const
{
    return other.id == id &&
           other.title == title &&
           other.message == message &&
           other.date == date &&
           other.permFree == permFree &&
           other.permPro == permPro &&
           other.popup == popup;
}

bool Notification::operator!=(const Notification &other) const
{
    return !(*this == other);
}

QDataStream& operator <<(QDataStream &stream, const Notification &o)
{
    stream << o.versionForSerialization_;
    stream << o.id << o.title << o.message << o.date << o.permFree << o.permPro << o.popup;
    return stream;
}

QDataStream& operator >>(QDataStream &stream, Notification &o)
{
    quint32 version;
    stream >> version;
    if (version > o.versionForSerialization_)
    {
        stream.setStatus(QDataStream::ReadCorruptData);
        return stream;
    }
    stream >> o.id >> o.title >> o.message >> o.date >> o.permFree >> o.permPro >> o.popup;
    return stream;
}

Notifications::Notifications(const std::string &json)
{
    QJsonParseError errCode;
    auto doc = QJsonDocument::fromJson(QByteArray(json.c_str()), &errCode);
    auto jsonObject = doc.object();
    auto jsonData =  jsonObject["data"].toObject();
    auto jsonNotifications = jsonData["notifications"].toArray();

    for (const QJsonValue &value : jsonNotifications) {
        QJsonObject obj = value.toObject();
        Notification n(obj);
        notifications_.push_back(n);
    }
}

} // namespace api_responses
