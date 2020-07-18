#include "loginsettings.h"

LoginSettings::LoginSettings()
{
    isAuthHashLogin_ = false;
}

LoginSettings::LoginSettings(const QString &authHash)
{
    authHash_ = authHash;
    isAuthHashLogin_ = true;
}

LoginSettings::LoginSettings(const QString &username, const QString &password)
{
    username_ = username;
    password_ = password;
    isAuthHashLogin_ = false;
}

void LoginSettings::setServerCredentials(const ServerCredentials &serverCredentials)
{
    serverCredentials_ = serverCredentials;
}
