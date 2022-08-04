#include "checkupdate.h"
#include <QMetaType>

const int typeIdCheckUpdate = qRegisterMetaType<types::CheckUpdate>("types::CheckUpdate");

namespace types {

bool CheckUpdate::initFromJson(QJsonObject &json, QString &outErrorMessage)
{
    // check for required fields in json
    QStringList required_fields({"current_version", "supported", "latest_version", "latest_build", "is_beta",
                                 "min_version", "update_needed_flag", "update_url"});
    for (const QString &str : required_fields)
    {
        if (!json.contains(str))
        {
            outErrorMessage = str + " field not found";
            return false;
        }
    }

    isAvailable = true; // is_available used as success indicator in gui
    version = json["latest_version"].toString();
    latestBuild = json["latest_build"].toInt();
    updateChannel = static_cast<UPDATE_CHANNEL>(json["is_beta"].toInt());
    url = json["update_url"].toString();
    isSupported = (json["supported"].toInt() == 1);

    if (json.contains("sha256"))
    {
        sha256 = json["sha256"].toString();
    }
    return true;
}

} // types namespace
