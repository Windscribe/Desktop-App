#include "checkupdate.h"

const int typeIdCheckUpdate = qRegisterMetaType<types::CheckUpdate>("types::CheckUpdate");

namespace types {

bool CheckUpdate::operator==(const CheckUpdate &other) const
{
    return other.isAvailable == isAvailable &&
           other.version == version &&
           other.updateChannel == updateChannel &&
           other.latestBuild == latestBuild &&
           other.url == url &&
           other.isSupported == isSupported &&
            other.sha256 == sha256;
}

bool CheckUpdate::operator!=(const CheckUpdate &other) const
{
    return !(*this == other);
}

CheckUpdate CheckUpdate::createFromApiJson(QJsonObject &json, bool &outSuccess, QString &outErrorMessage)
{
    CheckUpdate c;
    // check for required fields in json
    QStringList required_fields({"current_version", "supported", "latest_version", "latest_build", "is_beta",
                                 "min_version", "update_needed_flag", "update_url"});
    for (const QString &str : required_fields)
    {
        if (!json.contains(str))
        {
            outErrorMessage = str + " field not found";
            outSuccess = false;
            return c;
        }
    }

    c.isAvailable = true; // is_available used as success indicator in gui
    c.version = json["latest_version"].toString();
    c.latestBuild = json["latest_build"].toInt();
    c.updateChannel = static_cast<UPDATE_CHANNEL>(json["is_beta"].toInt());
    c.url = json["update_url"].toString();
    c.isSupported = (json["supported"].toInt() == 1);

    if (json.contains("sha256"))
    {
        c.sha256 = json["sha256"].toString();
    }
    return c;
}

} // types namespace
