#include "robertfilter.h"
#include <QJsonArray>
#include <QMetaType>

const int typeIdRobertFilter = qRegisterMetaType<types::RobertFilter>("types::RobertFilter");

namespace types {

bool RobertFilter::initFromJson(const QJsonObject &json)
{
    // check for required fields in json
    QStringList required_fields({"id", "title", "description", "status"});
    for (const QString &str : required_fields)
    {
        if (!json.contains(str))
        {
            return false;
        }
    }

    id = json["id"].toString();
    title = json["title"].toString();
    description = json["description"].toString();
    status = json["status"].toInt();

    return true;
}

bool RobertFilter::operator==(const RobertFilter &other) const
{
    return other.id == id &&
           other.title == title &&
           other.description == description &&
           other.status == status;
}

bool RobertFilter::operator!=(const RobertFilter &other) const
{
    return !(*this == other);
}

QDataStream& operator <<(QDataStream &stream, const RobertFilter &o)
{
    stream << o.versionForSerialization_;
    stream << o.id << o.title << o.description << o.status;
    return stream;
}

QDataStream& operator >>(QDataStream &stream, RobertFilter &o)
{
    quint32 version;
    stream >> version;
    if (version > o.versionForSerialization_)
    {
        stream.setStatus(QDataStream::ReadCorruptData);
        return stream;
    }
    stream >> o.id >> o.title >> o.description >> o.status;
    return stream;
}

} // namespace types