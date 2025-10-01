#include "wgconfigs_connect.h"
#include <QJsonDocument>
#include <QJsonObject>

namespace api_responses {

WgConfigsConnect::WgConfigsConnect(const std::string &json)
{
    QJsonParseError errCode;
    auto doc = QJsonDocument::fromJson(QByteArray(json.c_str()), &errCode);
    auto jsonObject = doc.object();

    if (jsonObject.contains("errorCode")) {
        errorCode_ = jsonObject["errorCode"].toInt();
        isErrorCode_ = true;
        return;
    }

    auto jsonData =  jsonObject["data"].toObject();
    auto jsonConfig = jsonData["config"].toObject();
    ipAddress_  = jsonConfig["Address"].toString();
    dnsAddress_ = jsonConfig["DNS"].toString();
}

} // namespace api_responses
