#include "syncrobertrequest.h"

#include <QJsonDocument>
#include <QJsonObject>

#include "utils/logger.h"

namespace server_api {

SyncRobertRequest::SyncRobertRequest(QObject *parent, const QString &authHash) :
    BaseRequest(parent, RequestType::kPost),
    authHash_(authHash)
{
}

QUrl SyncRobertRequest::url(const QString &domain) const
{
    QUrl url("https://" + hostname(domain, SudomainType::kApi) + "/Robert/syncrobert");
    QUrlQuery query;
    addAuthQueryItems(query, authHash_);
    addPlatformQueryItems(query);
    url.setQuery(query);
    return url;
}

QString SyncRobertRequest::name() const
{
    return "SyncRobert";
}

void SyncRobertRequest::handle(const QByteArray &arr)
{
    QJsonParseError errCode;
    QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
    if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "Failed parse JSON for SyncRobert";
        setRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }

    QJsonObject jsonObject = doc.object();
    if (!jsonObject.contains("data")) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "Failed parse JSON for SyncRobert";
        setRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }

    QJsonObject jsonData =  jsonObject["data"].toObject();
    qCDebug(LOG_SERVER_API) << "SyncRobert request successfully executed ( result =" << jsonData["success"] << ")";
    setRetCode((jsonData["success"] == 1) ? SERVER_RETURN_SUCCESS : SERVER_RETURN_INCORRECT_JSON);
}

} // namespace server_api {
