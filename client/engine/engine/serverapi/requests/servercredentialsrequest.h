#pragma once

#include "baserequest.h"

namespace server_api {

class ServerCredentialsRequest : public BaseRequest
{
    Q_OBJECT
public:
    explicit ServerCredentialsRequest(QObject *parent, const QString &authHash,  PROTOCOL protocol);

    QUrl url(const QString &domain) const override;
    QString name() const override;
    void handle(const QByteArray &arr) override;

    // output values
    QString radiusUsername() const { return radiusUsername_; }
    QString radiusPassword() const { return radiusPassword_; }
    PROTOCOL protocol() const { return protocol_; }

private:
    QString authHash_;
    PROTOCOL protocol_;

    // output values
    QString radiusUsername_;
    QString radiusPassword_;
};

} // namespace server_api {

