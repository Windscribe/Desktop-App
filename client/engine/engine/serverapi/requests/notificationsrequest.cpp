#include "notificationsrequest.h"

#include <QJsonArray>
#include <QJsonDocument>

#include "utils/logger.h"

namespace server_api {

NotificationsRequest::NotificationsRequest(QObject *parent, const QString &hostname, const QString &authHash) : BaseRequest(parent, RequestType::kGet, hostname),
    authHash_(authHash)
{
}

QUrl NotificationsRequest::url() const
{
    QUrl url("https://" + hostname(SudomainType::kApi) + "/Notifications");
    QUrlQuery query;
    addAuthQueryItems(query, authHash_);
    addPlatformQueryItems(query);
    url.setQuery(query);
    return url;
}

QString NotificationsRequest::name() const
{
    return "Notifications";
}

void NotificationsRequest::handle(const QByteArray &arr)
{
    QJsonParseError errCode;
    QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
    if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "Failed parse JSON for Notifications";
        setRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }

    QJsonObject jsonObject = doc.object();
    if (!jsonObject.contains("data")) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "Failed parse JSON for Notifications";
        setRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }

    QJsonObject jsonData =  jsonObject["data"].toObject();
    const QJsonArray jsonNotifications = jsonData["notifications"].toArray();

    for (const QJsonValue &value : jsonNotifications) {
        QJsonObject obj = value.toObject();
        types::Notification n;
        if (!n.initFromJson(obj)) {
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for Notifications (not all required fields)";
            setRetCode(SERVER_RETURN_INCORRECT_JSON);
            return;
        }
        notifications_.push_back(n);
    }
    qCDebug(LOG_SERVER_API) << "Notifications request successfully executed";
    setRetCode(SERVER_RETURN_SUCCESS);
}

} // namespace server_api {
