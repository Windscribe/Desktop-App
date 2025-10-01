#include "serverlist.h"
#include <QJsonDocument>
#include <QJsonArray>

namespace api_responses {

ServerList::ServerList(const std::string &json)
{
    if (json.empty())
        return;

    QJsonParseError errCode;
    auto doc = QJsonDocument::fromJson(QByteArray(json.c_str()), &errCode);
    auto jsonObject = doc.object();

    // get country_override parameter
    if (jsonObject.contains("info")) {
        auto jsonInfo = jsonObject["info"].toObject();
        if (jsonInfo.contains("country_override")) {
            countryOverride_ = jsonInfo["country_override"].toString();
        }
    }

    // parse locations array
    const QJsonArray jsonData = jsonObject["data"].toArray();
    for (int i = 0; i < jsonData.size(); ++i) {
        if (jsonData.at(i).isObject()) {
            Location sl;
            QJsonObject dataElement = jsonData.at(i).toObject();
            if (sl.initFromJson(dataElement, forceDisconnectNodes_)) {
                locations_ << sl;
            } else {
                QJsonDocument invalidData(dataElement);
            }
        }
    }
}


} // namespace api_responses
