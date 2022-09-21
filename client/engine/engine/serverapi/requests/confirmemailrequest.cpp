#include "confirmemailrequest.h"

#include <QJsonDocument>
#include <QJsonObject>

#include "utils/logger.h"

namespace server_api {

ConfirmEmailRequest::ConfirmEmailRequest(QObject *parent, const QString &hostname, const QString &authHash) :
    BaseRequest(parent, RequestType::kPost, hostname),
    authHash_(authHash)
{
}

QByteArray ConfirmEmailRequest::postData() const
{
    QUrlQuery postData;
    postData.addQueryItem("resend_confirmation", "1");
    addAuthQueryItems(postData, authHash_);
    addPlatformQueryItems(postData);
    return postData.toString(QUrl::FullyEncoded).toUtf8();
}

QUrl ConfirmEmailRequest::url() const
{
    QUrl url("https://" + hostname_ + "/Users");
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
        setRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }

    QJsonObject jsonObject = doc.object();
    if (!jsonObject.contains("data")) {
        qCDebug(LOG_SERVER_API) << "Failed parse JSON for Users";
        setRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }
    QJsonObject jsonData =  jsonObject["data"].toObject();
    if (!jsonData.contains("email_sent")) {
        qCDebug(LOG_SERVER_API) << "Failed parse JSON for Users";
        setRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }
    int emailSent = jsonData["email_sent"].toInt();
    setRetCode(emailSent == 1 ? SERVER_RETURN_SUCCESS : SERVER_RETURN_INCORRECT_JSON);
}

} // namespace server_api {
