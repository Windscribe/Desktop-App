#pragma once

#include "baserequest.h"
#include "types/checkupdate.h"

namespace server_api {

class CheckUpdateRequest : public BaseRequest
{
    Q_OBJECT
public:
    explicit CheckUpdateRequest(QObject *parent, const QString &hostname, UPDATE_CHANNEL updateChannel);

    QUrl url() const override;
    QString name() const override;
    void handle(const QByteArray &arr) override;

    // output values
    types::CheckUpdate checkUpdate() const { return checkUpdate_; }

private:
    UPDATE_CHANNEL updateChannel_;

    // output values
    types::CheckUpdate checkUpdate_;
};

} // namespace server_api {

