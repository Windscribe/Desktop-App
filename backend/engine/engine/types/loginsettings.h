#ifndef LOGINSETTINGS_H
#define LOGINSETTINGS_H

#include <QString>
#include "servercredentials.h"

class LoginSettings
{
public:
    LoginSettings();
    LoginSettings(const QString &authHash);
    LoginSettings(const QString &username, const QString &password);

    bool isAuthHashLogin() const { return isAuthHashLogin_; }
    QString authHash() const { Q_ASSERT(isAuthHashLogin_); return authHash_; }
    QString username() const { Q_ASSERT(!isAuthHashLogin_); return username_; }
    QString password() const { Q_ASSERT(!isAuthHashLogin_); return password_; }

    void setServerCredentials(const ServerCredentials &serverCredentials);
    const ServerCredentials &getServerCredentials() { return serverCredentials_; }

private:
    bool isAuthHashLogin_;
    QString username_;
    QString password_;
    QString authHash_;
    ServerCredentials serverCredentials_;
};

#endif // LOGINSETTINGS_H
