#include "servercredentialsrequest.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>

#include "utils/logger.h"
#include "utils/ws_assert.h"
#include "engine/utils/urlquery_utils.h"

namespace server_api {

ServerCredentialsRequest::ServerCredentialsRequest(QObject *parent, const QString &authHash,  PROTOCOL protocol) :
    BaseRequest(parent, RequestType::kGet),
    authHash_(authHash),
    protocol_(protocol)
{
}

QUrl ServerCredentialsRequest::url(const QString &domain) const
{
    QUrl url("https://" + hostname(domain, SudomainType::kApi) + "/ServerCredentials");
    QUrlQuery query;
    if (protocol_.isOpenVpnProtocol())
        query.addQueryItem("type", "openvpn");
    else if (protocol_.isIkev2Protocol())
        query.addQueryItem("type", "ikev2");
    else
        WS_ASSERT(false);

    urlquery_utils::addAuthQueryItems(query, authHash_);
    urlquery_utils::addPlatformQueryItems(query);
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
        setNetworkRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }
    QJsonObject jsonObject = doc.object();

    if (jsonObject.contains("errorCode")) {
        qCDebug(LOG_SERVER_API) << "API request ServerCredentials" << protocol_.toShortString() << "return error code:" << arr;
        // FIXME: return SERVER_RETURN_INCORRECT_JSON?
        WS_ASSERT(false);
        setNetworkRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }

    if (!jsonObject.contains("data")) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "API request ServerCredentials" << protocol_.toShortString() << "incorrect json (data field not found)";
        setNetworkRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }
    QJsonObject jsonData =  jsonObject["data"].toObject();
    if (!jsonData.contains("username") || !jsonData.contains("password")) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "API request ServerCredentials" << protocol_.toShortString() << "incorrect json (username/password fields not found)";
        setNetworkRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }
    qCDebug(LOG_SERVER_API) << "API request ServerCredentials" << protocol_.toShortString() << "successfully executed";

    radiusUsername_ = QByteArray::fromBase64(jsonData["username"].toString().toUtf8());
    radiusPassword_ = QByteArray::fromBase64(jsonData["password"].toString().toUtf8());
}


} // namespace server_api {
