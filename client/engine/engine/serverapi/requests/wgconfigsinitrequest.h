#pragma once

#include "baserequest.h"

namespace server_api {

class WgConfigsInitRequest : public BaseRequest
{
    Q_OBJECT
public:
    explicit WgConfigsInitRequest(QObject *parent,  const QString &hostname, const QString &authHash, const QString &clientPublicKey, bool deleteOldestKey);

    QString contentTypeHeader() const override;
    QByteArray postData() const override;
    QUrl url() const override;
    QString name() const override;
    void handle(const QByteArray &arr) override;

    // output values
    bool isErrorCode() const { return isErrorCode_; }
    int errorCode() const { return errorCode_; }
    QString presharedKey() const { return presharedKey_; }
    QString allowedIps() const { return allowedIps_; }

private:
    QString authHash_;
    QString clientPublicKey_;
    bool deleteOldestKey_;

    // output values
    bool isErrorCode_ = false;
    int errorCode_ = 0;
    QString presharedKey_;
    QString allowedIps_;
};

} // namespace server_api {

