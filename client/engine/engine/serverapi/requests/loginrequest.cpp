#include "loginrequest.h"

#include <QJsonDocument>

#include "utils/logger.h"
#include "utils/ws_assert.h"

namespace server_api {

LoginRequest::LoginRequest(QObject *parent, const QString &username, const QString &password, const QString &code2fa) :
    BaseRequest(parent, RequestType::kPost),
    username_(username),
    password_(password),
    code2fa_(code2fa),
    isLoginRequest_(true)
{
}

LoginRequest::LoginRequest(QObject *parent, const QString &authHash) : BaseRequest(parent, RequestType::kGet),
    authHash_(authHash),
    isLoginRequest_(false)
{
}

QString LoginRequest::contentTypeHeader() const
{
    return "Content-type: text/html; charset=utf-8";
}

QByteArray LoginRequest::postData() const
{
    QUrlQuery postData;
    postData.addQueryItem("username", QUrl::toPercentEncoding(username_));
    postData.addQueryItem("password", QUrl::toPercentEncoding(password_));
    if (!code2fa_.isEmpty())
        postData.addQueryItem("2fa_code", QUrl::toPercentEncoding(code2fa_));
    postData.addQueryItem("session_type_id", "3");
    addAuthQueryItems(postData);
    addPlatformQueryItems(postData);
    return postData.toString(QUrl::FullyEncoded).toUtf8();
}

QString LoginRequest::urlPath() const
{
    return "/Session";
}

QString LoginRequest::name() const
{
    return isLoginRequest_ ?  "Login" : "Session";
}

void LoginRequest::handle(const QByteArray &arr)
{
    QJsonParseError errCode;
    QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
    if (errCode.error != QJsonParseError::NoError || !doc.isObject())
    {
        qCDebug(LOG_SERVER_API) << "API request " + name() + " incorrect json";
        setRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }

    QJsonObject jsonObject = doc.object();
    if (jsonObject.contains("errorCode"))
    {
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

        if (errorCode == 701)
        {
            qCDebug(LOG_SERVER_API) << "API request " + name() + " return session auth hash invalid";
            setRetCode(SERVER_RETURN_SESSION_INVALID);
        }
        else if (errorCode == 702)
        {
            // According to the server API docs, we should not get here for the session call.
            if (!isLoginRequest_) {
                WS_ASSERT(false);
            }

            qCDebug(LOG_SERVER_API) << "API request " + name() + " return bad username";
            setRetCode(SERVER_RETURN_BAD_USERNAME);
        }
        else if (errorCode == 703 || errorCode == 706)
        {
            // According to the server API docs, we should not get here for the session call.
            if (!isLoginRequest_) {
                WS_ASSERT(false);
            }

            errorMessage_ = jsonObject["errorMessage"].toString();
            qCDebug(LOG_SERVER_API) << "API request " + name() + " return account disabled or banned";
            setRetCode(SERVER_RETURN_ACCOUNT_DISABLED);
        }
        else
        {
            if (isLoginRequest_)
            {
                if (errorCode == 1340)
                {
                    qCDebug(LOG_SERVER_API) << "API request " + name() + " return missing 2FA code";
                    setRetCode(SERVER_RETURN_MISSING_CODE2FA);
                }
                else if (errorCode == 1341)
                {
                    qCDebug(LOG_SERVER_API) << "API request " + name() + " return invalid 2FA code";
                    setRetCode(SERVER_RETURN_BAD_CODE2FA);
                }
                else
                {
                    qCDebug(LOG_SERVER_API) << "API request " + name() + " return error";
                    setRetCode(SERVER_RETURN_NETWORK_ERROR);
                }
            }
            else
            {
                qCDebug(LOG_SERVER_API) << "API request " + name() + " return error";
                setRetCode(SERVER_RETURN_NETWORK_ERROR);
            }
        }
        return;
    }

    if (!jsonObject.contains("data"))
    {
        qCDebug(LOG_SERVER_API) << "API request " + name() + " incorrect json (data field not found)";
        setRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }
    QJsonObject jsonData =  jsonObject["data"].toObject();
    if (jsonData.contains("session_auth_hash"))
    {
        authHashAfterLogin_ = jsonData["session_auth_hash"].toString();
    }

    QString outErrorMsg;
    bool success = sessionStatus_.initFromJson(jsonData, outErrorMsg);
    if (!success)
    {
        qCDebug(LOG_SERVER_API) << "API request " + name() + " incorrect json:"  << outErrorMsg;
        setRetCode(SERVER_RETURN_INCORRECT_JSON);
    }

    /*if (isLoginRequest)
    {
        qCDebug(LOG_SERVER_API) << "API request Login successfully executed";
        emit loginAnswer(SERVER_RETURN_SUCCESS, sessionStatus, authHash, userRole, "");
    }
    else
    {
        // Commented debug entry out as this request may occur every minute and we don't
        // need to flood the log with this info.  Enabled it for staging builds to aid
        // QA in verifying session requests are being made when they're supposed to be.
        if (AppVersion::instance().isStaging())
        {
            qCDebug(LOG_SERVER_API) << "API request Session successfully executed";
        }
        emit sessionAnswer(SERVER_RETURN_SUCCESS, sessionStatus, userRole);
    }*/
}




} // namespace server_api {
