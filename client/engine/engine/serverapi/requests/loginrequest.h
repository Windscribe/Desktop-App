#include "baserequest.h"
#include "types/sessionstatus.h"

#pragma once

namespace server_api {

// Implements the Session request which is used in two ways from the server API (login() and session() functions)
// It can be either a get request(for the session call) or a post request(for login call) depending on the constructor called
class LoginRequest : public BaseRequest
{
    Q_OBJECT
public:
    explicit LoginRequest(QObject *parent, const QString &username, const QString &password, const QString &code2fa);
    explicit LoginRequest(QObject *parent, const QString &authHash);

    QString contentTypeHeader() const override;
    QByteArray postData() const override;
    QString urlPath() const override;
    QString name() const override;
    void handle(const QByteArray &arr) override;

private:
    QString authHash_;
    QString username_;
    QString password_;
    QString code2fa_;
    bool isLoginRequest_;

    // output values
    types::SessionStatus sessionStatus_;
    QString errorMessage_;
    QString authHashAfterLogin_;

};

} // namespace server_api {

