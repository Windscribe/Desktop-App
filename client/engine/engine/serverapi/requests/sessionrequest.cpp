#include "sessionrequest.h"

#include <QJsonDocument>

#include "utils/logger.h"
#include "utils/ws_assert.h"
#include "version/appversion.h"

namespace server_api {

SessionRequest::SessionRequest(QObject *parent, const QString &hostname, const QString &authHash) : BaseRequest(parent, RequestType::kGet, hostname),
    authHash_(authHash)
{
}

QUrl SessionRequest::url() const
{
    QUrl url("https://" + hostname(SudomainType::kApi) + "/Session");

    QUrlQuery query;
    query.addQueryItem("session_type_id", "3");
    addAuthQueryItems(query, authHash_);
    addPlatformQueryItems(query);
    url.setQuery(query);
    return url;
}

QString SessionRequest::name() const
{
    return "Session";
}

void SessionRequest::handle(const QByteArray &arr)
{
    QJsonParseError errCode;
    QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
    if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
        qCDebug(LOG_SERVER_API) << "API request " + name() + " incorrect json";
        setRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }

    QJsonObject jsonObject = doc.object();
    if (jsonObject.contains("errorCode")) {
        int errorCode = jsonObject["errorCode"].toInt();

        // 701 - will be returned if the supplied session_auth_hash is invalid. Any authenticated endpoint can
        //       throw this error.  This can happen if the account gets disabled, or they rotate their session
        //       secret (pressed Delete Sessions button in the My Account section).  We should terminate the
        //       tunnel and go to the login screen.
        // 702 - will be returned ONLY in the login flow, and means the supplied credentials were not valid.
        //       Currently we disregard the API errorMessage and display the hardcoded ones (this is for
        //       multi-language support).
        // 703 - deprecated / never returned anymore, however we should still keep this for future purposes.
        //       If 703 is thrown on login (and only on login), display the exact errorMessage to the user,
        //       instead of what we do for 702 errors.
        // 706 - this is thrown only on login flow, and means the target account is disabled or banned.
        //       Do exactly the same thing as for 703 - show the errorMessage.

        if (errorCode == 701) {
            qCDebug(LOG_SERVER_API) << "API request " + name() + " return session auth hash invalid";
            setRetCode(SERVER_RETURN_SESSION_INVALID);
        } else if (errorCode == 702) {
            // According to the server API docs, we should not get here for the session call.
            WS_ASSERT(false);

            qCDebug(LOG_SERVER_API) << "API request " + name() + " return bad username";
            setRetCode(SERVER_RETURN_BAD_USERNAME);
        } else if (errorCode == 703 || errorCode == 706) {
            // According to the server API docs, we should not get here for the session call.
            WS_ASSERT(false);
            qCDebug(LOG_SERVER_API) << "API request " + name() + " return account disabled or banned";
            setRetCode(SERVER_RETURN_ACCOUNT_DISABLED);
        } else {
            qCDebug(LOG_SERVER_API) << "API request " + name() + " return error";
            setRetCode(SERVER_RETURN_NETWORK_ERROR);
        }
        return;
    }

    if (!jsonObject.contains("data"))  {
        qCDebug(LOG_SERVER_API) << "API request " + name() + " incorrect json (data field not found)";
        setRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }
    QJsonObject jsonData =  jsonObject["data"].toObject();
    QString outErrorMsg;
    bool success = sessionStatus_.initFromJson(jsonData, outErrorMsg);
    if (!success) {
        qCDebug(LOG_SERVER_API) << "API request " + name() + " incorrect json:"  << outErrorMsg;
        setRetCode(SERVER_RETURN_INCORRECT_JSON);
    }

    // Commented debug entry out as this request may occur every minute and we don't
    // need to flood the log with this info.  Enabled it for staging builds to aid
    // QA in verifying session requests are being made when they're supposed to be.
    if (AppVersion::instance().isStaging())
        qCDebug(LOG_SERVER_API) << "API request " + name() + " successfully executed";

    setRetCode(SERVER_RETURN_SUCCESS);
}

} // namespace server_api {
