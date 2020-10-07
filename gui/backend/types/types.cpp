#include "types.h"
#include <QMetaType>

const int typeIdOpenVpnError = qRegisterMetaType<CONNECTION_ERROR>("CONNECTION_ERROR");
const int typeIdProxyOption = qRegisterMetaType<PROXY_OPTION>("PROXY_OPTION");
const int typeIdLoginRet = qRegisterMetaType<LOGIN_RET>("LOGIN_RET");
const int typeIdServerApiRetCode = qRegisterMetaType<SERVER_API_RET_CODE>("SERVER_API_RET_CODE");
const int typeIdEngineInitRetCode = qRegisterMetaType<ENGINE_INIT_RET_CODE>("ENGINE_INIT_RET_CODE");
const int typeIdDisconnectReason = qRegisterMetaType<DISCONNECT_REASON>("DISCONNECT_REASON");

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
    else if (ret == LOGIN_BAD_CODE2FA)
    {
        return "BAD_CODE2FA";
    }
    else if (ret == LOGIN_MISSING_CODE2FA)
    {
        return "MISSING_CODE2FA";
    }
    else
    {
        Q_ASSERT(false);
        return "UNKNOWN";
    }
}

