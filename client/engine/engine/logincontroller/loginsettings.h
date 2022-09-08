#pragma once

#include <QString>
#include "engine/apiinfo/servercredentials.h"

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

    void setServerCredentials(const apiinfo::ServerCredentials &serverCredentials);
    const apiinfo::ServerCredentials &getServerCredentials() { return serverCredentials_; }

private:
    bool isAuthHashLogin_;
    QString username_;
    QString password_;
    QString code2fa_;
    QString authHash_;
    apiinfo::ServerCredentials serverCredentials_;
};

