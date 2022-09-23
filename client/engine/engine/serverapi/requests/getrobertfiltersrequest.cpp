#include "getrobertfiltersrequest.h"

#include <QJsonArray>
#include <QJsonDocument>

#include "utils/logger.h"

namespace server_api {

GetRobertFiltersRequest::GetRobertFiltersRequest(QObject *parent, const QString &hostname, const QString &authHash) : BaseRequest(parent, RequestType::kGet, hostname),
    authHash_(authHash)
{
}

QUrl GetRobertFiltersRequest::url() const
{
    QUrl url("https://" + hostname_ + "/Robert/filters");
    QUrlQuery query;
    addAuthQueryItems(query, authHash_);
    addPlatformQueryItems(query);
    url.setQuery(query);
    return url;
}

QString GetRobertFiltersRequest::name() const
{
    return "GetRobertFilters";
}

void GetRobertFiltersRequest::handle(const QByteArray &arr)
{
    QJsonParseError errCode;
    QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
    if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "Failed parse JSON for get ROBERT filters";
        setRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }

    QJsonObject jsonObject = doc.object();
    if (!jsonObject.contains("data")) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "Failed parse JSON for get ROBERT filters";
        setRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }

    QJsonObject jsonData =  jsonObject["data"].toObject();
    const QJsonArray jsonFilters = jsonData["filters"].toArray();

    for (const QJsonValue &value : jsonFilters) {
        QJsonObject obj = value.toObject();

        types::RobertFilter f;
        if (!f.initFromJson(obj)) {
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for get ROBERT filters (not all required fields)";
            setRetCode(SERVER_RETURN_INCORRECT_JSON);
            return;
        }
        filters_.push_back(f);
    }
    qCDebug(LOG_SERVER_API) << "Get ROBERT request successfully executed";
    setRetCode(SERVER_RETURN_SUCCESS);
}

} // namespace server_api {
