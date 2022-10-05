#include "confirmemailrequest.h"

#include <QJsonDocument>
#include <QJsonObject>

#include "utils/logger.h"
#include "engine/utils/urlquery_utils.h"

namespace server_api {

ConfirmEmailRequest::ConfirmEmailRequest(QObject *parent, const QString &authHash) :
    BaseRequest(parent, RequestType::kPost),
    authHash_(authHash)
{
}

QByteArray ConfirmEmailRequest::postData() const
{
    QUrlQuery postData;
    postData.addQueryItem("resend_confirmation", "1");
    urlquery_utils::addAuthQueryItems(postData, authHash_);
    urlquery_utils::addPlatformQueryItems(postData);
    return postData.toString(QUrl::FullyEncoded).toUtf8();
}

QUrl ConfirmEmailRequest::url(const QString &domain) const
{
    QUrl url("https://" + hostname(domain, SudomainType::kApi) + "/Users");
    return url;
}

QString ConfirmEmailRequest::name() const
{
    return "Users";
}

void ConfirmEmailRequest::handle(const QByteArray &arr)
{
    QJsonParseError errCode;
    QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
    if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
        qCDebug(LOG_SERVER_API) << "Failed parse JSON for Users";
        setNetworkRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }

    QJsonObject jsonObject = doc.object();
    if (!jsonObject.contains("data")) {
        qCDebug(LOG_SERVER_API) << "Failed parse JSON for Users";
        setNetworkRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }
    QJsonObject jsonData =  jsonObject["data"].toObject();
    if (!jsonData.contains("email_sent")) {
        qCDebug(LOG_SERVER_API) << "Failed parse JSON for Users";
        setNetworkRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }
    if (jsonData["email_sent"].toInt() != 1)
        setNetworkRetCode(SERVER_RETURN_INCORRECT_JSON);
}

} // namespace server_api {
