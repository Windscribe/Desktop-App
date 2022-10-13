#include "urlquery_utils.h"

#include <QCryptographicHash>

#include "version/appversion.h"
#include "utils/utils.h"
#include "utils/hardcodedsettings.h"

void urlquery_utils::addPlatformQueryItems(QUrlQuery &urlQuery)
{
    urlQuery.addQueryItem("platform", Utils::getPlatformNameSafe());
    urlQuery.addQueryItem("app_version", AppVersion::instance().semanticVersionString());
}

void urlquery_utils::addAuthQueryItems(QUrlQuery &urlQuery, const QString &authHash)
{
    time_t timestamp;
    time(&timestamp);
    QString strTimestamp = QString::number(timestamp);
    QString strHash = HardcodedSettings::instance().serverSharedKey() + strTimestamp;
    QString md5Hash = QCryptographicHash::hash(strHash.toStdString().c_str(), QCryptographicHash::Md5).toHex();
    if (!authHash.isEmpty())
        urlQuery.addQueryItem("session_auth_hash", authHash);
    urlQuery.addQueryItem("time", strTimestamp);
    urlQuery.addQueryItem("client_auth_hash", md5Hash);
}
