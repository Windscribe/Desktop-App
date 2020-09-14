#include "loginsettings.h"

LoginSettings::LoginSettings() : isAuthHashLogin_(false)
{
}

LoginSettings::LoginSettings(const QString &authHash) : isAuthHashLogin_(true), authHash_(authHash)
{
}

LoginSettings::LoginSettings(const QString &username, const QString &password)
    : isAuthHashLogin_(false), username_(username), password_(password)
{
}

void LoginSettings::setServerCredentials(const apiinfo::ServerCredentials &serverCredentials)
{
    serverCredentials_ = serverCredentials;
}
