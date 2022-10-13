#include "getrobertfiltersrequest.h"

#include <QJsonArray>
#include <QJsonDocument>

#include "utils/logger.h"
#include "engine/utils/urlquery_utils.h"

namespace server_api {

GetRobertFiltersRequest::GetRobertFiltersRequest(QObject *parent, const QString &authHash) : BaseRequest(parent, RequestType::kGet),
    authHash_(authHash)
{
}

QUrl GetRobertFiltersRequest::url(const QString &domain) const
{
    QUrl url("https://" + hostname(domain, SudomainType::kApi) + "/Robert/filters");
    QUrlQuery query;
    urlquery_utils::addAuthQueryItems(query, authHash_);
    urlquery_utils::addPlatformQueryItems(query);
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
        setNetworkRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }

    QJsonObject jsonObject = doc.object();
    if (!jsonObject.contains("data")) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "Failed parse JSON for get ROBERT filters";
        setNetworkRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }

    QJsonObject jsonData =  jsonObject["data"].toObject();
    const QJsonArray jsonFilters = jsonData["filters"].toArray();

    for (const QJsonValue &value : jsonFilters) {
        QJsonObject obj = value.toObject();

        types::RobertFilter f;
        if (!f.initFromJson(obj)) {
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for get ROBERT filters (not all required fields)";
            setNetworkRetCode(SERVER_RETURN_INCORRECT_JSON);
            return;
        }
        filters_.push_back(f);
    }
    qCDebug(LOG_SERVER_API) << "Get ROBERT request successfully executed";
}

} // namespace server_api {
