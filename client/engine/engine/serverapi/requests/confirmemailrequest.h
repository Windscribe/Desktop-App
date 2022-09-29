#pragma once

#include "baserequest.h"

namespace server_api {

//FIXME: check that works
class ConfirmEmailRequest : public BaseRequest
{
    Q_OBJECT
public:
    explicit ConfirmEmailRequest(QObject *parent, const QString &authHash);

    QByteArray postData() const override;
    QUrl url(const QString &domain) const override;
    QString name() const override;
    void handle(const QByteArray &arr) override;

private:
    QString authHash_;
};

} // namespace server_api {

