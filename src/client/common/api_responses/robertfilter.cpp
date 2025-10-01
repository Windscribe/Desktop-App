#include "robertfilter.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QMetaType>

const int typeIdRobertFilter = qRegisterMetaType<api_responses::RobertFilter>("api_responses::RobertFilter");

namespace api_responses {

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

RobertFilters::RobertFilters(const std::string &json)
{
    QJsonParseError errCode;
    auto doc = QJsonDocument::fromJson(QByteArray(json.c_str()), &errCode);
    auto jsonObject = doc.object();
    auto jsonData =  jsonObject["data"].toObject();
    auto jsonFilters = jsonData["filters"].toArray();

    for (const QJsonValue &value : jsonFilters) {
        QJsonObject obj = value.toObject();
        RobertFilter f;
        if (f.initFromJson(obj)) {
            filters_.push_back(f);
        }
    }
}

} // namespace api_responses
