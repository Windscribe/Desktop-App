#include "types.h"
#include <QMetaType>

const int typeIdOpenVpnError = qRegisterMetaType<CONNECTION_ERROR>("CONNECTION_ERROR");
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

QString loginRetToString(LOGIN_RET ret)
{
    if (ret == LOGIN_SUCCESS)
    {
        return "SUCCESS";
    }
    else if (ret == LOGIN_NO_API_CONNECTIVITY)
    {
        return "NO_API_CONNECTIVITY";
    }
    else if (ret == LOGIN_NO_CONNECTIVITY)
    {
        return "NO_CONNECTIVITY";
    }
    else if (ret == LOGIN_INCORRECT_JSON)
    {
        return "INCORRECT_JSON";
    }
    else if (ret == LOGIN_BAD_USERNAME)
    {
        return "BAD_USERNAME";
    }
    else if (ret == LOGIN_PROXY_AUTH_NEED)
    {
        return "PROXY_AUTH_NEED";
    }
    else if (ret == LOGIN_SSL_ERROR)
    {
        return "SSL_ERROR";
    }
    else
    {
        Q_ASSERT(false);
        return "UNKNOWN";
    }
}

ProtoTypes::LoginError loginRetToProtobuf(LOGIN_RET ret)
{
    if (ret == LOGIN_SUCCESS)
    {
        return ProtoTypes::LOGIN_ERROR_SUCCESS;
    }
    else if (ret == LOGIN_NO_API_CONNECTIVITY)
    {
        return ProtoTypes::LOGIN_ERROR_NO_API_CONNECTIVITY;
    }
    else if (ret == LOGIN_NO_CONNECTIVITY)
    {
        return ProtoTypes::LOGIN_ERROR_NO_CONNECTIVITY;
    }
    else if (ret == LOGIN_INCORRECT_JSON)
    {
        return ProtoTypes::LOGIN_ERROR_INCORRECT_JSON;
    }
    else if (ret == LOGIN_BAD_USERNAME)
    {
        return ProtoTypes::LOGIN_ERROR_BAD_USERNAME;
    }
    else if (ret == LOGIN_PROXY_AUTH_NEED)
    {
        return ProtoTypes::LOGIN_ERROR_PROXY_AUTH_NEED;
    }
    else if (ret == LOGIN_SSL_ERROR)
    {
        return ProtoTypes::LOGIN_ERROR_SSL_ERROR;
    }
    else
    {
        Q_ASSERT(false);
        return ProtoTypes::LOGIN_ERROR_SUCCESS;
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
    else
    {
        Q_ASSERT(false);
        return "UNKNOWN";
    }
}


