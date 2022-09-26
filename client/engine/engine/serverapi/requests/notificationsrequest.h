#pragma once

#include "baserequest.h"
#include "types/notification.h"

namespace server_api {

class NotificationsRequest : public BaseRequest
{
    Q_OBJECT
public:
    explicit NotificationsRequest(QObject *parent, const QString &hostname, const QString &authHash);

    QUrl url() const override;
    QString name() const override;
    void handle(const QByteArray &arr) override;

    // output values
    QVector<types::Notification> notifications() const { return notifications_; }

private:
    QString authHash_;
    // output values
    QVector<types::Notification> notifications_;
};

} // namespace server_api {

