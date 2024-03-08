#include "myip.h"
#include <QJsonDocument>
#include <QJsonObject>

namespace api_responses {

MyIp::MyIp(const std::string &json)
{
    QJsonParseError errCode;
    auto doc = QJsonDocument::fromJson(QByteArray(json.c_str()), &errCode);
    auto jsonObject = doc.object();
    auto jsonData =  jsonObject["data"].toObject();
    ip_ = jsonData["user_ip"].toString();
}


} // namespace api_responses
