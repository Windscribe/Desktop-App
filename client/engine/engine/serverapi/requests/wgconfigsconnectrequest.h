#pragma once

#include "baserequest.h"

namespace server_api {

class WgConfigsConnectRequest : public BaseRequest
{
    Q_OBJECT
public:
    explicit WgConfigsConnectRequest(QObject *parent, const QString &authHash, const QString &clientPublicKey,
                                     const QString &serverName, const QString &deviceId);

    QString contentTypeHeader() const override;
    QByteArray postData() const override;
    QUrl url(const QString &domain) const override;
    QString name() const override;
    void handle(const QByteArray &arr) override;

    // output values
    bool isErrorCode() const { return isErrorCode_; }
    int errorCode() const { return errorCode_; }
    QString ipAddress() const { return ipAddress_; }
    QString dnsAddress() const { return dnsAddress_; }

private:
    QString authHash_;
    QString clientPublicKey_;
    QString serverName_;
    QString deviceId_;

    // output values
    bool isErrorCode_ = false;
    int errorCode_ = 0;
    QString ipAddress_;
    QString dnsAddress_;
};

} // namespace server_api {

