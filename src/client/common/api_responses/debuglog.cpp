#include "debuglog.h"
#include <QJsonDocument>
#include <QJsonObject>

namespace api_responses {

DebugLog::DebugLog(const std::string &json)
{
    QJsonParseError errCode;
    auto doc = QJsonDocument::fromJson(QByteArray(json.c_str()), &errCode);
    auto jsonObject = doc.object();

    if (!jsonObject.contains("data")) {
        bSuccess_ = false;
        return;
    }

    auto jsonData =  jsonObject["data"].toObject();
    bSuccess_ = jsonData["success"].toInt() == 1;
}


} // namespace api_responses
