#pragma once

#include "baserequest.h"

namespace server_api {

//FIXME: check that works
class WebSessionRequest : public BaseRequest
{
    Q_OBJECT
public:
    explicit WebSessionRequest(QObject *parent, const QString &authHash, WEB_SESSION_PURPOSE purpose);

    QByteArray postData() const override;
    QUrl url(const QString &domain) const override;
    QString name() const override;
    void handle(const QByteArray &arr) override;

    // output values
    QString token() const { return token_; }
    WEB_SESSION_PURPOSE purpose() const { return purpose_; }

private:
    QString authHash_;
    // output values
    WEB_SESSION_PURPOSE purpose_;
    QString token_;
};

} // namespace server_api {

