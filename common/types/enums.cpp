#include "enums.h"
#include <QMetaType>

const int typeIdOpenVpnError = qRegisterMetaType<CONNECT_ERROR>("CONNECT_ERROR");
const int typeIdProxyOption = qRegisterMetaType<PROXY_OPTION>("PROXY_OPTION");
const int typeIdLoginRet = qRegisterMetaType<LOGIN_RET>("LOGIN_RET");
const int typeIdLoginMessage = qRegisterMetaType<LOGIN_MESSAGE>("LOGIN_MESSAGE");
const int typeIdServerApiRetCode = qRegisterMetaType<SERVER_API_RET_CODE>("SERVER_API_RET_CODE");
const int typeIdEngineInitRetCode = qRegisterMetaType<ENGINE_INIT_RET_CODE>("ENGINE_INIT_RET_CODE");
const int typeIdConnectState = qRegisterMetaType<CONNECT_STATE>("CONNECT_STATE");
const int typeIdDisconnectReason = qRegisterMetaType<DISCONNECT_REASON>("DISCONNECT_REASON");
const int typeIdProxySharingType = qRegisterMetaType<PROXY_SHARING_TYPE>("PROXY_SHARING_TYPE");
const int typeIdUpdateVersionState = qRegisterMetaType<ProtoTypes::Protocol>("ProtoTypes::UpdateVersionState");
const int typeIdUpdateVersionError = qRegisterMetaType<ProtoTypes::Protocol>("ProtoTypes::UpdateVersionError");
const int typeIdUpdateChannel = qRegisterMetaType<ProtoTypes::UpdateChannel>("ProtoTypes::UpdateChannel");
const int typeIdWebSessionPurpose = qRegisterMetaType<WEB_SESSION_PURPOSE>("WEB_SESSION_PURPOSE");

QString LOGIN_RET_toString(LOGIN_RET ret)
{
    if (ret == LOGIN_RET_SUCCESS)
    {
        return "SUCCESS";
    }
    else if (ret == LOGIN_RET_NO_API_CONNECTIVITY)
    {
        return "NO_API_CONNECTIVITY";
    }
    else if (ret == LOGIN_RET_NO_CONNECTIVITY)
    {
        return "NO_CONNECTIVITY";
    }
    else if (ret == LOGIN_RET_INCORRECT_JSON)
    {
        return "INCORRECT_JSON";
    }
    else if (ret == LOGIN_RET_BAD_USERNAME)
    {
        return "BAD_USERNAME";
    }
    else if (ret == LOGIN_RET_PROXY_AUTH_NEED)
    {
        return "PROXY_AUTH_NEED";
    }
    else if (ret == LOGIN_RET_SSL_ERROR)
    {
        return "SSL_ERROR";
    }
    else if (ret == LOGIN_RET_BAD_CODE2FA)
    {
        return "BAD_CODE2FA";
    }
    else if (ret == LOGIN_RET_MISSING_CODE2FA)
    {
        return "MISSING_CODE2FA";
    }
    else if (ret == LOGIN_RET_ACCOUNT_DISABLED)
    {
        return "ACCOUNT_DISABLED";
    }
    else if (ret == LOGIN_RET_SESSION_INVALID)
    {
        return "SESSION_INVALID";
    }
    else
    {
        Q_ASSERT(false);
        return "UNKNOWN";
    }
}

QString dnsPolicyTypeToString(DNS_POLICY_TYPE d)
{
    if (d == DNS_TYPE_OS_DEFAULT)
    {
        return "OS Default";
    }
    else if (d == DNS_TYPE_OPEN_DNS)
    {
        return "OpenDNS";
    }
    else if (d == DNS_TYPE_CLOUDFLARE)
    {
        return "Cloudflare";
    }
    else if (d == DNS_TYPE_GOOGLE)
    {
        return "Google";
    }
    else if (d == DNS_TYPE_CONTROLD)
    {
        return "ControlD";
    }
    else
    {
        Q_ASSERT(false);
        return "UNKNOWN";
    }
}

QString PROXY_SHARING_TYPE_toString(PROXY_SHARING_TYPE t)
{
    if (t == PROXY_SHARING_HTTP) return "HTTP";
    else if (t == PROXY_SHARING_SOCKS) return "SOCKS";
    else {
        Q_ASSERT(false);
        return "UNKNOWN";
    }
}
