#pragma once

#include "baserequest.h"

namespace server_api {

class AcessIpsRequest : public BaseRequest
{
    Q_OBJECT
public:
    explicit AcessIpsRequest(QObject *parent, const QString &hostname);

    QUrl url() const override;
    QString name() const override;
    void handle(const QByteArray &arr) override;

    // output values
    QStringList hosts() const;

private:
    // output values
    QStringList hosts_;
};

} // namespace server_api {

