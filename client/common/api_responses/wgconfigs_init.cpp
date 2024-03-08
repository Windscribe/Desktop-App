#include "wgconfigs_init.h"
#include <QJsonDocument>
#include <QJsonObject>

namespace api_responses {

WgConfigsInit::WgConfigsInit(const std::string &json)
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
    presharedKey_ = jsonConfig["PresharedKey"].toString();
    allowedIps_   = jsonConfig["AllowedIPs"].toString();
}


} // namespace api_responses
