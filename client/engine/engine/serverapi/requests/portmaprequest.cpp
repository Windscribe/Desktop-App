#include "portmaprequest.h"

#include <QJsonDocument>
#include <QJsonObject>

#include "utils/logger.h"

namespace server_api {

PortMapRequest::PortMapRequest(QObject *parent, const QString &hostname, const QString &authHash) : BaseRequest(parent, RequestType::kGet, hostname),
    authHash_(authHash)
{
}

QUrl PortMapRequest::url() const
{
    QUrl url("https://" + hostname(SudomainType::kApi) + "/PortMap");
    QUrlQuery query;
    query.addQueryItem("version", "5");
    addAuthQueryItems(query, authHash_);
    addPlatformQueryItems(query);
    url.setQuery(query);
    return url;
}

QString PortMapRequest::name() const
{
    return "PortMap";
}

void PortMapRequest::handle(const QByteArray &arr)
{
    QJsonParseError errCode;
    QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
    if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "API request PortMap incorrect json";
        setRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }

    QJsonObject jsonObject = doc.object();
    if (!jsonObject.contains("data")) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "API request PortMap incorrect json (data field not found)";
        setRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }
    QJsonObject jsonData =  jsonObject["data"].toObject();
    if (!jsonData.contains("portmap")) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "API request PortMap incorrect json (portmap field not found)";
        setRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }

    QJsonArray jsonArr = jsonData["portmap"].toArray();
    if (!portMap_.initFromJson(jsonArr)) {
        qCDebug(LOG_SERVER_API) << "API request PortMap incorrect json (portmap required fields not found)";
        setRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }

    qCDebug(LOG_SERVER_API) << "API request PortMap successfully executed";
    setRetCode(SERVER_RETURN_SUCCESS);
}


} // namespace server_api {
