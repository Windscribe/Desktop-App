#include "myip.h"
#include <QJsonDocument>
#include <QJsonObject>
#include "utils/networkingvalidation.h"

namespace api_responses {

MyIp::MyIp(const std::string &json)
{
    QJsonParseError errCode;
    auto doc = QJsonDocument::fromJson(QByteArray(json.c_str()), &errCode);
    auto jsonObject = doc.object();
    auto jsonData =  jsonObject["data"].toObject();
    const QString ip = jsonData["user_ip"].toString();
    // user_ip must be a valid IP or nothing; drop a malformed value rather than propagate it to display/persistence/IPC.
    if (NetworkingValidation::isIp(ip)) {
        ip_ = ip;
    }
}


} // namespace api_responses
