#include "customsniparams.h"

#include <QJsonDocument>
#include <QJsonObject>

namespace api_responses {

CustomSniParams::CustomSniParams(const std::string &json)
{
    if (json.empty()) {
        return;
    }

    QJsonParseError errCode;
    auto doc = QJsonDocument::fromJson(QByteArray(json.c_str()), &errCode);
    if (errCode.error != QJsonParseError::ParseError::NoError) {
        return;
    }

    auto jsonObject = doc.object();
    if (!jsonObject.contains("data")) {
        return;
    }

    auto jsonData = jsonObject["data"].toObject();

    // Parse custom_sni field
    if (jsonData.contains("custom_sni") && jsonData["custom_sni"].isString()) {
        domain_ = jsonData["custom_sni"].toString();
    }
}

} // namespace api_responses
