#ifndef LOGINSETTINGS_H
#define LOGINSETTINGS_H

#include <QString>
#include "../apiinfo/servercredentials.h"

class LoginSettings
{
public:
    LoginSettings();
    explicit LoginSettings(const QString &authHash);
    LoginSettings(const QString &username, const QString &password);

    bool isAuthHashLogin() const { return isAuthHashLogin_; }
    QString authHash() const { Q_ASSERT(isAuthHashLogin_); return authHash_; }
    QString username() const { Q_ASSERT(!isAuthHashLogin_); return username_; }
    QString password() const { Q_ASSERT(!isAuthHashLogin_); return password_; }

    void setServerCredentials(const ApiInfo::ServerCredentials &serverCredentials);
    const ApiInfo::ServerCredentials &getServerCredentials() { return serverCredentials_; }

private:
    bool isAuthHashLogin_;
    QString username_;
    QString password_;
    QString authHash_;
    ApiInfo::ServerCredentials serverCredentials_;
};

#endif // LOGINSETTINGS_H
