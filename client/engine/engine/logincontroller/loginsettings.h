#pragma once

#include <QString>
#include "engine/apiinfo/servercredentials.h"
#include "utils/ws_assert.h"

class LoginSettings
{
public:
    LoginSettings();
    explicit LoginSettings(const QString &authHash);
    LoginSettings(const QString &username, const QString &password, const QString &code2fa);

    bool isAuthHashLogin() const { return isAuthHashLogin_; }
    QString authHash() const { WS_ASSERT(isAuthHashLogin_); return authHash_; }
    QString username() const { WS_ASSERT(!isAuthHashLogin_); return username_; }
    QString password() const { WS_ASSERT(!isAuthHashLogin_); return password_; }
    QString code2fa() const { WS_ASSERT(!isAuthHashLogin_); return code2fa_; }

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

