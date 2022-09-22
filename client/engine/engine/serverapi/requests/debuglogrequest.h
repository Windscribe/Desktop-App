#pragma once

#include "baserequest.h"

namespace server_api {

class DebugLogRequest : public BaseRequest
{
    Q_OBJECT
public:
    explicit DebugLogRequest(QObject *parent, const QString &hostname, const QString &username, const QString &strLog);

    QString contentTypeHeader() const override;
    QByteArray postData() const override;
    QUrl url() const override;
    QString name() const override;
    void handle(const QByteArray &arr) override;

private:
    QString username_;
    QString strLog_;
};

} // namespace server_api {

