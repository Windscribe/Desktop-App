#include "wgconfigsconnectrequest.h"

#include <QJsonDocument>
#include <QJsonObject>

#include "utils/logger.h"
#include "engine/utils/urlquery_utils.h"

namespace server_api {

WgConfigsConnectRequest::WgConfigsConnectRequest(QObject *parent, const QString &authHash, const QString &clientPublicKey,
                                                 const QString &serverName, const QString &deviceId) :
    BaseRequest(parent, RequestType::kPost),
    authHash_(authHash),
    clientPublicKey_(clientPublicKey),
    serverName_(serverName),
    deviceId_(deviceId)
{
}

QString WgConfigsConnectRequest::contentTypeHeader() const
{
    return "Content-type: text/html; charset=utf-8";
}

QByteArray WgConfigsConnectRequest::postData() const
{
    QUrlQuery postData;
    postData.addQueryItem("wg_pubkey", QUrl::toPercentEncoding(clientPublicKey_));
    postData.addQueryItem("hostname", serverName_);
    if (!deviceId_.isEmpty()) {
        qDebug(LOG_SERVER_API) << "Setting device_id for WgConfigs connect request:" << deviceId_;
        postData.addQueryItem("device_id", deviceId_);
    }
    urlquery_utils::addAuthQueryItems(postData, authHash_);
    urlquery_utils::addPlatformQueryItems(postData);
    return postData.toString(QUrl::FullyEncoded).toUtf8();
}

QUrl WgConfigsConnectRequest::url(const QString &domain) const
{
    return QUrl("https://" + hostname(domain, SudomainType::kApi) + "/WgConfigs/connect");
}

QString WgConfigsConnectRequest::name() const
{
    return "WgConfigsConnect";
}

void WgConfigsConnectRequest::handle(const QByteArray &arr)
{
    QJsonParseError errCode;
    QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
    if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "Failed to parse JSON for WgConfigs connect response";
        setNetworkRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }

    QJsonObject jsonObject = doc.object();

    if (jsonObject.contains("errorCode")) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        errorCode_ = jsonObject["errorCode"].toInt();
        isErrorCode_ = true;
        qCDebug(LOG_SERVER_API) << "WgConfigs connect failed:" << jsonObject["errorMessage"].toString() << "(" << errorCode_ << ")";
        return;
    }

    if (!jsonObject.contains("data")) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "WgConfigs connect JSON is missing the 'data' element";
        setNetworkRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }

    QJsonObject jsonData = jsonObject["data"].toObject();
    if (!jsonData.contains("success") || jsonData["success"].toInt(0) == 0) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "WgConfigs connect JSON contains a missing or invalid 'success' field";
        setNetworkRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }

    QJsonObject jsonConfig = jsonData["config"].toObject();
    if (jsonConfig.contains("Address") && jsonConfig.contains("DNS")) {
        ipAddress_  = jsonConfig["Address"].toString();
        dnsAddress_ = jsonConfig["DNS"].toString();
        qCDebug(LOG_SERVER_API) << "WgConfigs connect request successfully executed";

    } else {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "WgConfigs connect 'config' entry is missing required elements";
        setNetworkRetCode(SERVER_RETURN_INCORRECT_JSON);
    }
}


} // namespace server_api {
