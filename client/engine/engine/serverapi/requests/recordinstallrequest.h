#pragma once

#include "baserequest.h"

namespace server_api {

class RecordInstallRequest : public BaseRequest
{
    Q_OBJECT
public:
    explicit RecordInstallRequest(QObject *parent);

    QString contentTypeHeader() const override;
    QByteArray postData() const override;
    QUrl url(const QString &domain) const override;
    QString name() const override;
    void handle(const QByteArray &arr) override;
};

} // namespace server_api {

