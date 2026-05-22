#include "wgconfigs_init.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

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

    if (jsonConfig.contains("AllowedIPsV6")) {
        allowedIpsV6_ = jsonConfig["AllowedIPsV6"].toString();
    }

    if (jsonConfig.contains("HashedCIDR")) {
        auto hashedCIDRArray = jsonConfig["HashedCIDR"].toArray();
        for (const auto &cidr : hashedCIDRArray) {
            hashedCIDR_.append(cidr.toString());
        }
    }

    if (jsonConfig.contains("HashedCIDRv6")) {
        auto hashedCIDRv6Array = jsonConfig["HashedCIDRv6"].toArray();
        for (const auto &cidr : hashedCIDRv6Array) {
            hashedCIDRv6_.append(cidr.toString());
        }
    }
}


} // namespace api_responses
