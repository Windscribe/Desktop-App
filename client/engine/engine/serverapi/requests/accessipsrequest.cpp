#include "accessipsrequest.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "utils/logger.h"

namespace server_api {

AcessIpsRequest::AcessIpsRequest(QObject *parent, const QString &hostname) : BaseRequest(parent, RequestType::kGet, hostname)
{
}

QUrl AcessIpsRequest::url() const
{
    QUrl url("https://" + hostname(SudomainType::kApi) + "/ApiAccessIps");
    QUrlQuery query;
    addAuthQueryItems(query);
    addPlatformQueryItems(query);
    url.setQuery(query);
    return url;
}

QString AcessIpsRequest::name() const
{
    return "ApiAccessIps";
}

void AcessIpsRequest::handle(const QByteArray &arr)
{
    hosts_.clear();

    QJsonParseError errCode;
    QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
    if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
        qCDebug(LOG_SERVER_API) << "API request ApiAccessIps incorrect json";
        setRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }
    QJsonObject jsonObject = doc.object();
    if (!jsonObject.contains("data")) {
        qCDebug(LOG_SERVER_API) << "API request ApiAccessIps incorrect json (data field not found)";
        setRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }
    QJsonObject jsonData =  jsonObject["data"].toObject();
    if (!jsonData.contains("hosts")) {
        qCDebug(LOG_SERVER_API) << "API request ApiAccessIps incorrect json (hosts field not found)";
        setRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }

    const QJsonArray jsonArray = jsonData["hosts"].toArray();
    for (const QJsonValue &value : jsonArray)
        hosts_ << value.toString();

    qCDebug(LOG_SERVER_API) << "API request ApiAccessIps successfully executed";
    setRetCode(SERVER_RETURN_SUCCESS);
}

QStringList AcessIpsRequest::hosts() const
{
    return hosts_;
}

} // namespace server_api {
