#ifndef TYPES_LOGINSETTINGS_H
#define TYPES_LOGINSETTINGS_H

#include <QString>
#include "servercredentials.h"

namespace types {

class LoginSettings
{
public:
    LoginSettings();
    explicit LoginSettings(const QString &authHash);
    LoginSettings(const QString &username, const QString &password, const QString &code2fa);

    bool isAuthHashLogin() const { return isAuthHashLogin_; }
    QString authHash() const { Q_ASSERT(isAuthHashLogin_); return authHash_; }
    QString username() const { Q_ASSERT(!isAuthHashLogin_); return username_; }
    QString password() const { Q_ASSERT(!isAuthHashLogin_); return password_; }
    QString code2fa() const { Q_ASSERT(!isAuthHashLogin_); return code2fa_; }

    void setServerCredentials(const ServerCredentials &serverCredentials);
    const ServerCredentials &getServerCredentials() { return serverCredentials_; }

private:
    bool isAuthHashLogin_;
    QString username_;
    QString password_;
    QString code2fa_;
    QString authHash_;
    ServerCredentials serverCredentials_;
};

} //namespace types

#endif // TYPES_LOGINSETTINGS_H
