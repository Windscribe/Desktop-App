#pragma once

#include "baserequest.h"

namespace server_api {

class PingTestRequest : public BaseRequest
{
    Q_OBJECT
public:
    explicit PingTestRequest(QObject *parent, int timeout);

    QUrl url(const QString &domain) const override;
    QString name() const override;
    void handle(const QByteArray &arr) override;

    // output values
    QString data() const { return data_; }

private:
    // output values
    QString data_;
};

} // namespace server_api {

