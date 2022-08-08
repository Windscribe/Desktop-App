#include "notification.h"
#include <QJsonArray>
#include <QMetaType>

const int typeIdNotification = qRegisterMetaType<types::Notification>("types::Notification");

namespace types {

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

    id = json["id"].toDouble();
    title = json["title"].toString();
    message = json["message"].toString();
    date = json["date"].toDouble();
    permFree = json["perm_free"].toInt();
    permPro = json["perm_pro"].toInt();
    popup = json["popup"].toInt();

    return true;
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



} // namespace types
