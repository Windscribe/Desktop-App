#include "wgconfigsinitrequest.h"

#include <QJsonDocument>
#include <QJsonObject>

#include "utils/logger.h"

namespace server_api {

WgConfigsInitRequest::WgConfigsInitRequest(QObject *parent, const QString &authHash, const QString &clientPublicKey, bool deleteOldestKey) :
    BaseRequest(parent, RequestType::kPost),
    authHash_(authHash),
    clientPublicKey_(clientPublicKey),
    deleteOldestKey_(deleteOldestKey)
{
}

QString WgConfigsInitRequest::contentTypeHeader() const
{
    return "Content-type: text/html; charset=utf-8";
}

QByteArray WgConfigsInitRequest::postData() const
{
    QUrlQuery postData;
    postData.addQueryItem("wg_pubkey", QUrl::toPercentEncoding(clientPublicKey_));
    if (deleteOldestKey_)
        postData.addQueryItem("force_init", "1");
    addAuthQueryItems(postData, authHash_);
    addPlatformQueryItems(postData);
    return postData.toString(QUrl::FullyEncoded).toUtf8();
}

QUrl WgConfigsInitRequest::url(const QString &domain) const
{
    return QUrl("https://" + hostname(domain, SudomainType::kApi) + "/WgConfigs/init");
}

QString WgConfigsInitRequest::name() const
{
    return "WgConfigsInit";
}

void WgConfigsInitRequest::handle(const QByteArray &arr)
{
    QJsonParseError errCode;
    QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
    if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "Failed to parse JSON for WgConfigs init response";
        setRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }

    QJsonObject jsonObject = doc.object();
    if (jsonObject.contains("errorCode")) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        errorCode_ = jsonObject["errorCode"].toInt();
        isErrorCode_ = true;
        qCDebug(LOG_SERVER_API) << "WgConfigs init failed:" << jsonObject["errorMessage"].toString() << "(" << errorCode_ << ")";
        setRetCode(SERVER_RETURN_SUCCESS);
        return;
    }

    if (!jsonObject.contains("data")) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "WgConfigs init JSON is missing the 'data' element";
        setRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }

    QJsonObject jsonData = jsonObject["data"].toObject();
    if (!jsonData.contains("success") || jsonData["success"].toInt(0) == 0) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "WgConfigs init JSON contains a missing or invalid 'success' field";
        setRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }

    QJsonObject jsonConfig = jsonData["config"].toObject();
    if (jsonConfig.contains("PresharedKey") && jsonConfig.contains("AllowedIPs")) {
        presharedKey_ = jsonConfig["PresharedKey"].toString();
        allowedIps_   = jsonConfig["AllowedIPs"].toString();
        qCDebug(LOG_SERVER_API) << "WgConfigs/init json:" << doc.toJson(QJsonDocument::Compact);
        qCDebug(LOG_SERVER_API) << "WgConfigs init request successfully executed";
        setRetCode(SERVER_RETURN_SUCCESS);
    } else {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "WgConfigs init 'config' entry is missing required elements";
        setRetCode(SERVER_RETURN_INCORRECT_JSON);
    }
}


} // namespace server_api {
