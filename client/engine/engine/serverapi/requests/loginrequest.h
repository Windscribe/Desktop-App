#pragma once

#include "baserequest.h"
#include "types/sessionstatus.h"
#include "sessionerrorcode.h"

namespace server_api {

class LoginRequest : public BaseRequest
{
    Q_OBJECT
public:
    explicit LoginRequest(QObject *parent, const QString &username, const QString &password, const QString &code2fa);

    QString contentTypeHeader() const override;
    QByteArray postData() const override;
    QUrl url(const QString &domain) const override;
    QString name() const override;
    void handle(const QByteArray &arr) override;

    // output values
    SessionErrorCode sessionErrorCode() const { return sessionErrorCode_; }
    types::SessionStatus sessionStatus() const { return sessionStatus_; }
    QString authHash() const { return authHash_; }
    QString errorMessage() const { return errorMessage_;}

private:
    QString username_;
    QString password_;
    QString code2fa_;

    // output values
    SessionErrorCode sessionErrorCode_ = SessionErrorCode::kSuccess;
    types::SessionStatus sessionStatus_;
    QString errorMessage_;
    QString authHash_;
};

} // namespace server_api {

