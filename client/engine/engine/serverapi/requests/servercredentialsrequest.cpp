#include "servercredentialsrequest.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>

#include "utils/logger.h"
#include "utils/ws_assert.h"

namespace server_api {

ServerCredentialsRequest::ServerCredentialsRequest(QObject *parent, const QString &hostname, const QString &authHash,  PROTOCOL protocol) :
    BaseRequest(parent, RequestType::kGet, hostname),
    authHash_(authHash),
    protocol_(protocol)
{
}

QUrl ServerCredentialsRequest::url() const
{
    QUrl url("https://" + hostname(SudomainType::kApi) + "/ServerCredentials");
    QUrlQuery query;
    if (protocol_.isOpenVpnProtocol())
        query.addQueryItem("type", "openvpn");
    else if (protocol_.isIkev2Protocol())
        query.addQueryItem("type", "ikev2");
    else
        WS_ASSERT(false);

    addAuthQueryItems(query, authHash_);
    addPlatformQueryItems(query);
    url.setQuery(query);
    return url;
}

QString ServerCredentialsRequest::name() const
{
    return "ServerCredentials";
}

void ServerCredentialsRequest::handle(const QByteArray &arr)
{
    QJsonParseError errCode;
    QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
    if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "API request ServerCredentials" << protocol_.toShortString() << "incorrect json";
        setRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }
    QJsonObject jsonObject = doc.object();

    if (jsonObject.contains("errorCode")) {
        qCDebug(LOG_SERVER_API) << "API request ServerCredentials" << protocol_.toShortString() << "return error code:" << arr;
        // FIXME: return SERVER_RETURN_SUCCESS?
        setRetCode(SERVER_RETURN_SUCCESS);
        return;
    }

    if (!jsonObject.contains("data")) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "API request ServerCredentials" << protocol_.toShortString() << "incorrect json (data field not found)";
        setRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }
    QJsonObject jsonData =  jsonObject["data"].toObject();
    if (!jsonData.contains("username") || !jsonData.contains("password")) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "API request ServerCredentials" << protocol_.toShortString() << "incorrect json (username/password fields not found)";
        setRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }
    qCDebug(LOG_SERVER_API) << "API request ServerCredentials" << protocol_.toShortString() << "successfully executed";

    radiusUsername_ = QByteArray::fromBase64(jsonData["username"].toString().toUtf8());
    radiusPassword_ = QByteArray::fromBase64(jsonData["password"].toString().toUtf8());
    setRetCode(SERVER_RETURN_SUCCESS);
}


} // namespace server_api {
