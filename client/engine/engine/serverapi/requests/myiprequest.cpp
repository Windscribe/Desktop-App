#include "myiprequest.h"

#include <QJsonDocument>
#include <QJsonObject>

#include "utils/logger.h"

namespace server_api {

MyIpRequest::MyIpRequest(QObject *parent, const QString &hostname, int timeout) : BaseRequest(parent, RequestType::kGet, hostname, true, timeout)
{
}

QUrl MyIpRequest::url() const
{
    QUrl url("https://" + hostname_ + "/MyIp");
    QUrlQuery query;
    addAuthQueryItems(query);
    addPlatformQueryItems(query);
    url.setQuery(query);
    return url;
}

QString MyIpRequest::name() const
{
    return "MyIp";
}

void MyIpRequest::handle(const QByteArray &arr)
{
    QJsonParseError errCode;
    QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
    if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "API request MyIP incorrect json";
        setRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }

    QJsonObject jsonObject = doc.object();
    if (!jsonObject.contains("data")) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "API request MyIP incorrect json(data field not found)";
        setRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }
    QJsonObject jsonData =  jsonObject["data"].toObject();
    if (!jsonData.contains("user_ip")) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "API request MyIP incorrect json(user_ip field not found)";
        setRetCode(SERVER_RETURN_INCORRECT_JSON);
    }
    ip_ = jsonData["user_ip"].toString();
    setRetCode(SERVER_RETURN_SUCCESS);
}


} // namespace server_api {
