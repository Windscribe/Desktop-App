#include "debuglogrequest.h"

#include <QJsonDocument>
#include <QJsonObject>

#include "utils/logger.h"

namespace server_api {

DebugLogRequest::DebugLogRequest(QObject *parent, const QString &hostname, const QString &username, const QString &strLog) :
    BaseRequest(parent, RequestType::kPost, hostname),
    username_(username),
    strLog_(strLog)
{
}

QString DebugLogRequest::contentTypeHeader() const
{
    return "Content-type: application/x-www-form-urlencoded";
}

QByteArray DebugLogRequest::postData() const
{
    QUrlQuery postData;
    QByteArray ba = strLog_.toUtf8();
    postData.addQueryItem("logfile", ba.toBase64());
    if (!username_.isEmpty())
        postData.addQueryItem("username", username_);
    addAuthQueryItems(postData);
    addPlatformQueryItems(postData);
    return postData.toString(QUrl::FullyEncoded).toUtf8();
}

QUrl DebugLogRequest::url() const
{
    return  QUrl("https://" + hostname_ + "/Report/applog");
}

QString DebugLogRequest::name() const
{
    return "DebugLog";
}

void DebugLogRequest::handle(const QByteArray &arr)
{
    QJsonParseError errCode;
    QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
    if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "Failed parse JSON for Report";
        setRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }

    QJsonObject jsonObject = doc.object();
    if (!jsonObject.contains("data")) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "Failed parse JSON for Report";
        setRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }
    QJsonObject jsonData =  jsonObject["data"].toObject();
    if (!jsonData.contains("success")) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "Failed parse JSON for Report";
        setRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }

    int is_success = jsonData["success"].toInt();
    setRetCode(is_success == 1 ? SERVER_RETURN_SUCCESS : SERVER_RETURN_INCORRECT_JSON);
}

} // namespace server_api {
