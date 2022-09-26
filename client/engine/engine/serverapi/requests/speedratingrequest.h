#pragma once

#include "baserequest.h"

namespace server_api {

class SpeedRatingRequest : public BaseRequest
{
    Q_OBJECT
public:
    explicit SpeedRatingRequest(QObject *parent, const QString &hostname, const QString &authHash, const QString &speedRatingHostname, const QString &ip, int rating);

    QString contentTypeHeader() const override;
    QByteArray postData() const override;
    QUrl url() const override;
    QString name() const override;
    void handle(const QByteArray &arr) override;

private:
    QString authHash_;
    QString speedRatingHostname_;
    QString ip_;
    int rating_;
};

} // namespace server_api {

