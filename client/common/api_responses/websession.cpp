#include "websession.h"
#include <QJsonDocument>
#include <QJsonObject>

namespace api_responses {

WebSession::WebSession(const std::string &json)
{
    QJsonParseError errCode;
    auto doc = QJsonDocument::fromJson(QByteArray(json.c_str()), &errCode);
    auto jsonObject = doc.object();
    auto jsonData =  jsonObject["data"].toObject();
    token_ = jsonData["temp_session"].toString();
}


} // namespace api_responses
