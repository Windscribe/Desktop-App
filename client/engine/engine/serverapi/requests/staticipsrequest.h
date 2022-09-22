#pragma once

#include "baserequest.h"
#include "engine/apiinfo/staticips.h"

namespace server_api {

class StaticIpsRequest : public BaseRequest
{
    Q_OBJECT
public:
    explicit StaticIpsRequest(QObject *parent, const QString &hostname, const QString &authHash, const QString &deviceId);

    QUrl url() const override;
    QString name() const override;
    void handle(const QByteArray &arr) override;

    // output values
    apiinfo::StaticIps staticIps() const { return staticIps_; }

private:
    QString authHash_;
    QString deviceId_;

    // output values
    apiinfo::StaticIps staticIps_;
};

} // namespace server_api {

