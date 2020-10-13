#include "loginsettings.h"

LoginSettings::LoginSettings() : isAuthHashLogin_(false)
{
}

LoginSettings::LoginSettings(const QString &authHash) : isAuthHashLogin_(true), authHash_(authHash)
{
}

LoginSettings::LoginSettings(const QString &username, const QString &password, const QString &code2fa)
    : isAuthHashLogin_(false), username_(username), password_(password), code2fa_(code2fa)
{
}

void LoginSettings::setServerCredentials(const apiinfo::ServerCredentials &serverCredentials)
{
    serverCredentials_ = serverCredentials;
}
