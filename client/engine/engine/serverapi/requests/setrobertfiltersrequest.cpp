#include "setrobertfiltersrequest.h"

#include <QJsonDocument>

#include "utils/logger.h"
#include "engine/utils/urlquery_utils.h"

namespace server_api {

SetRobertFiltersRequest::SetRobertFiltersRequest(QObject *parent, const QString &authHash, const types::RobertFilter &filter) :
    BaseRequest(parent, RequestType::kPut),
    authHash_(authHash),
    filter_(filter)
{
}

QString SetRobertFiltersRequest::contentTypeHeader() const
{
    return "Content-type: text/html; charset=utf-8";
}

QByteArray SetRobertFiltersRequest::postData() const
{
    QString json = QString("{\"filter\":\"%1\", \"status\":%2}").arg(filter_.id).arg(filter_.status);
    return json.toUtf8();
}

QUrl SetRobertFiltersRequest::url(const QString &domain) const
{
    QUrl url("https://" + hostname(domain, SudomainType::kApi) + "/Robert/filter");
    QUrlQuery query;
    urlquery_utils::addAuthQueryItems(query, authHash_);
    urlquery_utils::addPlatformQueryItems(query);
    url.setQuery(query);
    return url;
}

QString SetRobertFiltersRequest::name() const
{
    return "SetRobertFilters";
}

void SetRobertFiltersRequest::handle(const QByteArray &arr)
{
    QJsonParseError errCode;
    QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
    if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "Failed parse JSON for set ROBERT filters";
        setNetworkRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }

    QJsonObject jsonObject = doc.object();
    if (!jsonObject.contains("data")) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "Failed parse JSON for set ROBERT filters";
        setNetworkRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }

    QJsonObject jsonData =  jsonObject["data"].toObject();
    qCDebug(LOG_SERVER_API) << "Set ROBERT request successfully executed ( result =" << jsonData["success"] << ")";
    if (jsonData["success"] != 1)
        setNetworkRetCode(SERVER_RETURN_INCORRECT_JSON);
}

} // namespace server_api {
