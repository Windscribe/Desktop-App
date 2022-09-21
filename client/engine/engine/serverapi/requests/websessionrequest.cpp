#include "websessionrequest.h"

#include <QJsonDocument>
#include <QJsonObject>

#include "utils/logger.h"

namespace server_api {

WebSessionRequest::WebSessionRequest(QObject *parent, const QString &hostname, const QString &authHash, WEB_SESSION_PURPOSE purpose) :
    BaseRequest(parent, RequestType::kPost, hostname),
    authHash_(authHash),
    purpose_(purpose)
{
}

QByteArray WebSessionRequest::postData() const
{
    QUrlQuery postData;
    postData.addQueryItem("temp_session", "1");
    postData.addQueryItem("session_type_id", "1");
    addAuthQueryItems(postData, authHash_);
    addPlatformQueryItems(postData);
    return postData.toString(QUrl::FullyEncoded).toUtf8();
}

QUrl WebSessionRequest::url() const
{
    return QUrl("https://" + hostname_ + "/WebSession");
}

QString WebSessionRequest::name() const
{
    return "WebSession";
}

void WebSessionRequest::handle(const QByteArray &arr)
{
    qCDebugMultiline(LOG_SERVER_API) << arr;

    QJsonParseError errCode;
    QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
    if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
        qCDebug(LOG_SERVER_API) << "API request WebSession incorrect json";
        setRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }
    QJsonObject jsonObject = doc.object();
    if (!jsonObject.contains("data")) {
        qCDebug(LOG_SERVER_API) << "API request WebSession incorrect json (data field not found)";
        setRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }
    QJsonObject jsonData =  jsonObject["data"].toObject();
    if (!jsonData.contains("temp_session")) {
        qCDebug(LOG_SERVER_API) << "API request WebSession incorrect json (temp_session field not found)";
        setRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }

    token_ = jsonData["temp_session"].toString();
    qCDebug(LOG_SERVER_API) << "API request WebSession successfully executed";
    setRetCode(SERVER_RETURN_SUCCESS);
}

} // namespace server_api {
