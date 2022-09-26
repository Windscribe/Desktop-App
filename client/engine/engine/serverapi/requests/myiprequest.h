#pragma once

#include "baserequest.h"

namespace server_api {

class MyIpRequest : public BaseRequest
{
    Q_OBJECT
public:
    explicit MyIpRequest(QObject *parent, const QString &hostname, int timeout);

    QUrl url() const override;
    QString name() const override;
    void handle(const QByteArray &arr) override;

    // output values
    QString ip() const { return ip_; }

private:
    // output values
    QString ip_;
};

} // namespace server_api {

