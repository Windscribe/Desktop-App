#include "staticipsrequest.h"

#include <QJsonDocument>
#include <QJsonObject>

#include "utils/logger.h"
#include "engine/utils/urlquery_utils.h"

namespace server_api {

StaticIpsRequest::StaticIpsRequest(QObject *parent, const QString &authHash, const QString &deviceId) : BaseRequest(parent, RequestType::kGet),
    authHash_(authHash),
    deviceId_(deviceId)
{
}

QUrl StaticIpsRequest::url(const QString &domain) const
{
    QUrl url("https://" + hostname(domain, SudomainType::kApi) + "/StaticIps");
    QUrlQuery query;

#ifdef Q_OS_WIN
    QString strOs = "win";
#elif defined Q_OS_MAC
    QString strOs = "mac";
#elif defined Q_OS_LINUX
    QString strOs = "linux";
#endif
    query.addQueryItem("os", strOs);
    query.addQueryItem("device_id", deviceId_);
    urlquery_utils::addAuthQueryItems(query, authHash_);
    urlquery_utils::addPlatformQueryItems(query);
    url.setQuery(query);
    return url;
}

QString StaticIpsRequest::name() const
{
    return "StaticIps";
}

void StaticIpsRequest::handle(const QByteArray &arr)
{
    QJsonParseError errCode;
    QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
    if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "Failed parse JSON for StaticIps";
        setNetworkRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }

    QJsonObject jsonObject = doc.object();
    if (!jsonObject.contains("data")) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "Failed parse JSON for StaticIps";
        setNetworkRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }

    QJsonObject jsonData =  jsonObject["data"].toObject();
    if (!staticIps_.initFromJson(jsonData)) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "Failed parse JSON for StaticIps";
        setNetworkRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }

    qCDebug(LOG_SERVER_API) << "StaticIps request successfully executed";
}

} // namespace server_api {
