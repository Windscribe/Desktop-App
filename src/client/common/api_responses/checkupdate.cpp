#include "checkupdate.h"
#include <QJsonDocument>

const int typeIdCheckUpdate = qRegisterMetaType<api_responses::CheckUpdate>("api_responses::CheckUpdate");

namespace api_responses {

CheckUpdate::CheckUpdate(const std::string &json)
{
    QJsonParseError errCode;
    auto doc = QJsonDocument::fromJson(QByteArray(json.c_str()), &errCode);
    auto jsonObject = doc.object();
    auto jsonData =  jsonObject["data"].toObject();

    int updateNeeded = jsonData["update_needed_flag"].toInt();
    if (updateNeeded != 1) {
        isAvailable_ = false;
        return;
    } else {
        isAvailable_ = true; // is_available used as success indicator in gui
    }

    version_ = jsonData["latest_version"].toString();
    updateChannel_ = static_cast<UPDATE_CHANNEL>(jsonData["is_beta"].toInt());
    latestBuild_ = jsonData["latest_build"].toInt();
    url_ = jsonData["update_url"].toString();
    isSupported_ = (jsonData["supported"].toInt() == 1);

    if (jsonData.contains("sha256")) {
        sha256_ = jsonData["sha256"].toString();
    }
}

bool CheckUpdate::operator==(const CheckUpdate &other) const
{
    return other.version_ == version_ &&
           other.updateChannel_ == updateChannel_ &&
           other.latestBuild_ == latestBuild_ &&
           other.url_ == url_ &&
           other.isSupported_ == isSupported_ &&
           other.sha256_ == sha256_;
}

bool CheckUpdate::operator!=(const CheckUpdate &other) const
{
    return !(*this == other);
}

} // api_responses namespace
