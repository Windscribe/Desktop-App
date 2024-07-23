#include "servercredentials.h"
#include <QJsonDocument>
#include <QJsonObject>

namespace api_responses {

ServerCredentials::ServerCredentials(const std::string &json)
{
    if (json.empty())
        return;
    QJsonParseError errCode;
    auto doc = QJsonDocument::fromJson(QByteArray(json.c_str()), &errCode);
    auto jsonObject = doc.object();
    auto jsonData =  jsonObject["data"].toObject();
    username_ = QByteArray::fromBase64(jsonData["username"].toString().toUtf8());
    password_ = QByteArray::fromBase64(jsonData["password"].toString().toUtf8());
}


} // namespace api_responses
