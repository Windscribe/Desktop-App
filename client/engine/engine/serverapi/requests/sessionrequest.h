#pragma once

#include "baserequest.h"
#include "types/sessionstatus.h"
#include "sessionerrorcode.h"

namespace server_api {

class SessionRequest : public BaseRequest
{
    Q_OBJECT
public:
    explicit SessionRequest(QObject *parent, const QString &authHash);

    QUrl url(const QString &domain) const override;
    QString name() const override;
    void handle(const QByteArray &arr) override;

    // output values
    SessionErrorCode sessionErrorCode() const { return sessionErrorCode_; }
    types::SessionStatus sessionStatus() const { return sessionStatus_; }
    QString authHash() const { return authHash_; }

private:
    QString authHash_;
    // output values
    SessionErrorCode sessionErrorCode_ = SessionErrorCode::kSuccess;
    types::SessionStatus sessionStatus_;
};

} // namespace server_api {

