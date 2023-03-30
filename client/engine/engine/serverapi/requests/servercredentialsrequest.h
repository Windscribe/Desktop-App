#pragma once

#include "baserequest.h"
#include "types/protocol.h"

namespace server_api {

class ServerCredentialsRequest : public BaseRequest
{
    Q_OBJECT
public:
    explicit ServerCredentialsRequest(QObject *parent, const QString &authHash, types::Protocol protocol);

    QUrl url(const QString &domain) const override;
    QString name() const override;
    void handle(const QByteArray &arr) override;

    // output values
    QString radiusUsername() const { return radiusUsername_; }
    QString radiusPassword() const { return radiusPassword_; }
    types::Protocol protocol() const { return protocol_; }

private:
    QString authHash_;
    types::Protocol protocol_;

    // output values
    QString radiusUsername_;
    QString radiusPassword_;
};

} // namespace server_api {

